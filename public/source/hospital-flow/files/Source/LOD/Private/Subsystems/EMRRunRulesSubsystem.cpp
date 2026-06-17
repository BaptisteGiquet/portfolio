#include "Subsystems/EMRRunRulesSubsystem.h"

#include "Data/EMRRunRulesData.h"
#include "Framework/EMRAssetManager.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Subsystems/EMRRunProgressionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"


void UEMRRunRulesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UEMRAssetManager& AssetManager = UEMRAssetManager::Get();
    AssetManager.LoadPatients(FStreamableDelegate());
    AssetManager.LoadEconomySystemGenerics(FStreamableDelegate());
    AssetManager.LoadSubsystemConfig(FStreamableDelegate());

    RequestRunRulesDataLoad();
	
    const TCHAR* NetModeString = TEXT("Unknown");
    if (UWorld* World = GetWorld())
    {
        switch (World->GetNetMode())
        {
        case NM_Client:
            NetModeString = TEXT("Client");
            break;
        case NM_DedicatedServer:
            NetModeString = TEXT("DedicatedServer");
            break;
        case NM_ListenServer:
            NetModeString = TEXT("ListenServer");
            break;
        case NM_Standalone:
            NetModeString = TEXT("Standalone");
            break;
        default:
            break;
        }
    }
	

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][RunRules] Initialize - NetMode=%s"), NetModeString);
}


void UEMRRunRulesSubsystem::InitializeNewRun(AEMRNightShiftGameState* RunState)
{
    if (!RunState || !RunState->HasAuthority())
    {
        return;
    }

    const TWeakObjectPtr<AEMRNightShiftGameState> RunStateWeak = RunState;
    EnsureRunRulesDataReady([this, RunStateWeak]()
    {
        if (!RunStateWeak.IsValid() || !RunStateWeak->HasAuthority())
        {
            return;
        }

        RunStateWeak->SetRunFailed(false);
        RunStateWeak->SetFinalMissionUnlocked(false);
        RunStateWeak->SetRunPhase(EER_RunPhase::Hub);
        RunStateWeak->SetNightShiftIndexInCycle(0);
        RunStateWeak->SetCurrentCycleInfo(0, 0.f, RunStateWeak->GetTotalRevenue());
    });
}


void UEMRRunRulesSubsystem::ApplyRunRulesData() const
{
    TArray<const UEMRRunRulesData*> LoadedRunRules;
    const bool bHadLoadedAssets = UEMRAssetManager::Get().CollectLoadedRunRulesData(LoadedRunRules);

    const UEMRRunRulesData* RunRulesData = nullptr;
    if (LoadedRunRules.Num() > 0)
    {
        RunRulesData = LoadedRunRules[0];
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[NightShiftFlow][RunRules] ApplyRunRulesData - Loaded=%d bHadLoadedAssets=%s"),
        LoadedRunRules.Num(),
        bHadLoadedAssets ? TEXT("true") : TEXT("false"));

    if (RunRulesData)
    {
        NightShiftsPerCycle = FMath::Max(1, RunRulesData->NightShiftsPerCycle);
        MaxCycles = FMath::Max(1, RunRulesData->MaxCycles);
        DefaultCycleQuotas = RunRulesData->DefaultCycleQuotas;
        FallbackCycleQuotaStep = FMath::Max(0.0f, RunRulesData->FallbackCycleQuotaStep);
        NightShiftDurationSeconds = FMath::Max(1.0f, RunRulesData->NightShiftDurationSeconds);

        UE_LOG(
            LogTemp,
            Log,
            TEXT("[NightShiftFlow][RunRules] Loaded data asset %s - NightShiftsPerCycle=%d MaxCycles=%d DefaultQuotas=%d FallbackStep=%.2f NightShiftDuration=%.2f"),
            *RunRulesData->GetName(),
            NightShiftsPerCycle,
            MaxCycles,
            DefaultCycleQuotas.Num(),
            FallbackCycleQuotaStep,
            NightShiftDurationSeconds);
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[NightShiftFlow][RunRules] No RunRulesData loaded yet - keeping defaults (Quotas=%d FallbackStep=%.2f NightShiftDuration=%.2f)"),
            DefaultCycleQuotas.Num(),
            FallbackCycleQuotaStep,
            NightShiftDurationSeconds);
    }
}


void UEMRRunRulesSubsystem::SetupCycle(AEMRNightShiftGameState* RunState, int32 CycleIndex) const
{
    if (!RunState || !RunState->HasAuthority())
    {
        return;
    }

    const TWeakObjectPtr<AEMRNightShiftGameState> RunStateWeak = RunState;
    EnsureRunRulesDataReady([this, RunStateWeak, CycleIndex]()
    {
        if (!RunStateWeak.IsValid() || !RunStateWeak->HasAuthority())
        {
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][RunRules] SetupCycle %d"), CycleIndex);

        const int32 PreviousCycleIndex = RunStateWeak->GetCurrentCycleIndex();
        const bool bCycleChanged = PreviousCycleIndex != CycleIndex;

        if (bCycleChanged)
        {
            UE_LOG(
                LogTemp,
                Log,
                TEXT("[NightShiftFlow][RunRules] Cycle index changed from %d to %d"),
                PreviousCycleIndex,
                CycleIndex);
        }

        const float CurrentTotalRevenue = RunStateWeak->GetTotalRevenue();

        float CycleQuota = CurrentTotalRevenue + FallbackCycleQuotaStep * (CycleIndex + 1);
        if (DefaultCycleQuotas.IsValidIndex(CycleIndex))
        {
            CycleQuota = DefaultCycleQuotas[CycleIndex];
        }

        UE_LOG(
            LogTemp,
            Log,
            TEXT("[NightShiftFlow][RunRules] SetupCycle %d using quota %.2f (HasDefault=%s)"),
            CycleIndex,
            CycleQuota,
            DefaultCycleQuotas.IsValidIndex(CycleIndex) ? TEXT("true") : TEXT("false"));

        UEMRRunProgressionSubsystem* ProgressionSubsystem = nullptr;
        if (UWorld* World = RunStateWeak->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
            {
                ProgressionSubsystem = GameInstance->GetSubsystem<UEMRRunProgressionSubsystem>();
            }
        }

        if (ProgressionSubsystem)
        {
            ProgressionSubsystem->CommitCycleSetup(RunStateWeak.Get(), CycleIndex, CycleQuota);
            return;
        }

        RunStateWeak->SetCurrentCycleInfo(CycleIndex, CycleQuota, CurrentTotalRevenue);
        RunStateWeak->SetNightShiftIndexInCycle(0);
    });
}


EER_RunPhase UEMRRunRulesSubsystem::HandleNightShiftCompletion(AEMRNightShiftGameState* RunState) const
{
    if (!RunState || !RunState->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunRules.HandleNightShiftCompletion aborted (RunState=%s HasAuthority=%d)"),
            *GetNameSafe(RunState),
            RunState && RunState->HasAuthority() ? 1 : 0);
        return EER_RunPhase::RunFinished;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunRules.HandleNightShiftCompletion begin Phase=%d Cycle=%d NightShiftIndex=%d Revenue=%.2f Quota=%.2f NightShiftsPerCycle=%d"),
        static_cast<int32>(RunState->GetRunPhase()),
        RunState->GetCurrentCycleIndex(),
        RunState->GetNightShiftIndexInCycle(),
        RunState->GetTotalRevenue(),
        RunState->GetCurrentCycleQuota(),
        NightShiftsPerCycle);

    RunState->SetRunPhase(EER_RunPhase::InterNightShift);
    RunState->SetRemainingTimeInNightShift(0.f);

    const int32 PrevNightShiftIndex = RunState->GetNightShiftIndexInCycle();
    const int32 NewNightShiftIndex = PrevNightShiftIndex + 1;
    RunState->SetNightShiftIndexInCycle(NewNightShiftIndex);

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][RunRules] NightShift index advanced to %d"), NewNightShiftIndex);

    const bool bReachedCycleQuota = RunState->GetTotalRevenue() >= RunState->GetCurrentCycleQuota();
    if (bReachedCycleQuota)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunRules.HandleNightShiftCompletion reached cycle quota -> EvaluateEndOfCycle"));
        RunState->SetRunPhase(EER_RunPhase::EndOfCycle);
        EvaluateEndOfCycle(RunState);
        return RunState->GetRunPhase();
    }

    if (NewNightShiftIndex < NightShiftsPerCycle)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunRules.HandleNightShiftCompletion continue run -> Hub (NewNightShiftIndex=%d)"), NewNightShiftIndex);
        RunState->SetRunPhase(EER_RunPhase::Hub);
        return EER_RunPhase::Hub;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunRules.HandleNightShiftCompletion last night shift without quota -> EvaluateEndOfCycle"));
    RunState->SetRunPhase(EER_RunPhase::EndOfCycle);
    EvaluateEndOfCycle(RunState);
    return RunState->GetRunPhase();
}


void UEMRRunRulesSubsystem::EvaluateEndOfCycle(AEMRNightShiftGameState* RunState) const
{
    if (!RunState || !RunState->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunRules.EvaluateEndOfCycle aborted (RunState=%s HasAuthority=%d)"),
            *GetNameSafe(RunState),
            RunState && RunState->HasAuthority() ? 1 : 0);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunRules.EvaluateEndOfCycle start Cycle=%d Revenue=%.2f Quota=%.2f MaxCycles=%d"),
        RunState->GetCurrentCycleIndex(),
        RunState->GetTotalRevenue(),
        RunState->GetCurrentCycleQuota(),
        MaxCycles);

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][RunRules] EvaluateEndOfCycle"));

    const int32 CycleIndex = RunState->GetCurrentCycleIndex();
    const float TotalRevenue = RunState->GetTotalRevenue();
    const float CurrentCycleQuota = RunState->GetCurrentCycleQuota();

    if (TotalRevenue < CurrentCycleQuota)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][RunRules] Cycle failed: revenue %.2f / quota %.2f"), TotalRevenue, CurrentCycleQuota);
        HandleRunFailed(RunState);
        return;
    }

    const int32 NextCycleIndex = CycleIndex + 1;

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][RunRules] Cycle success -> %d"), NextCycleIndex);

    if (NextCycleIndex >= MaxCycles)
    {
        HandleFinalMissionUnlocked(RunState);
        return;
    }

    SetupCycle(RunState, NextCycleIndex);
    RunState->SetRunPhase(EER_RunPhase::Hub);
}


void UEMRRunRulesSubsystem::HandleRunFailed(AEMRNightShiftGameState* RunState) const
{
    if (!RunState || !RunState->HasAuthority())
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunRules.HandleRunFailed setting RunFinished Cycle=%d Revenue=%.2f Quota=%.2f"),
        RunState->GetCurrentCycleIndex(),
        RunState->GetTotalRevenue(),
        RunState->GetCurrentCycleQuota());
    RunState->SetRunFailed(true);
    RunState->SetRunPhase(EER_RunPhase::RunFinished);
}


void UEMRRunRulesSubsystem::HandleFinalMissionUnlocked(AEMRNightShiftGameState* RunState) const
{
    if (!RunState || !RunState->HasAuthority())
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunRules.HandleFinalMissionUnlocked setting MissionFinal Cycle=%d Revenue=%.2f Quota=%.2f"),
        RunState->GetCurrentCycleIndex(),
        RunState->GetTotalRevenue(),
        RunState->GetCurrentCycleQuota());
    RunState->SetFinalMissionUnlocked(true);
    RunState->SetRunPhase(EER_RunPhase::MissionFinal);
}


void UEMRRunRulesSubsystem::EnsureRunRulesDataReady(TFunction<void()> Callback) const
{
    if (!Callback)
    {
        return;
    }

    if (bRunRulesDataLoaded)
    {
        Callback();
        return;
    }

    PendingReadyCallbacks.Add(MoveTemp(Callback));
    RequestRunRulesDataLoad();
}


void UEMRRunRulesSubsystem::RequestRunRulesDataLoad() const
{
    if (bRunRulesDataLoadRequested)
    {
        return;
    }

    bRunRulesDataLoadRequested = true;

    FStreamableDelegate OnLoadFinished;
    OnLoadFinished.BindUObject(this, &ThisClass::HandleRunRulesDataLoaded);

    UEMRAssetManager::Get().LoadRunRulesData(OnLoadFinished);
}


void UEMRRunRulesSubsystem::HandleRunRulesDataLoaded() const
{
    bRunRulesDataLoaded = true;
    ApplyRunRulesData();
    FlushPendingCallbacks();
}


void UEMRRunRulesSubsystem::FlushPendingCallbacks() const
{
    const TArray<TFunction<void()>> Callbacks = PendingReadyCallbacks;
    PendingReadyCallbacks.Reset();

    for (const TFunction<void()>& Callback : Callbacks)
    {
        if (Callback)
        {
            Callback();
        }
    }
}
