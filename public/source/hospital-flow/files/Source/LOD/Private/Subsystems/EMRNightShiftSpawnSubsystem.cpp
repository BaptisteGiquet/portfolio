#include "Subsystems/EMRNightShiftSpawnSubsystem.h"

#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRPatientData.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRBaseMachine.h"
#include "Curves/CurveFloat.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GameFramework/PlayerController.h"
#include "Environment/EMRPatientEntryPoint.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "GAS/Abilities/EMRGameplayAbility_MoveToWaitingSeat.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "Subsystems/EMRPatientPoolSubsystem.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Framework/EMRAssetManager.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogNightShiftSpawn, Log, All);

void UEMRNightShiftSpawnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}


void UEMRNightShiftSpawnSubsystem::Deinitialize()
{
    StopNightShiftSpawning();
    Super::Deinitialize();
}


void UEMRNightShiftSpawnSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
}


TStatId UEMRNightShiftSpawnSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UEMRNightShiftSpawnSubsystem, STATGROUP_Tickables);
}


FEMRNightShiftSpawnDebugInfo UEMRNightShiftSpawnSubsystem::GetDebugInfo() const
{
    FEMRNightShiftSpawnDebugInfo Info;
    Info.bSpawningEnabled = bSpawningEnabled;
    Info.ElapsedSeconds = ElapsedSeconds;
    Info.BaseAccumulator = BaseAccumulator;
    Info.ActivePatientCount = ActivePatientCount;
    Info.PendingRequestCount = PendingSpawnRequests.Num();
    Info.SpawnSequenceCounter = SpawnSequenceCounter;
    Info.bImmediateRequestPending = bImmediateRequestPending;
    Info.BaseSpawnRatePerSecond = BaseSpawnRatePerSecond;
    Info.DifficultySpawnRateMultiplier = DifficultySpawnRateMultiplier;
    Info.PlayerCountMultiplier = PlayerCountMultiplier;
    Info.SpecialEventSpawnRateMultiplier = SpecialEventSpawnRateMultiplier;
    Info.DeferredAutomaticSpawnCount = AutomaticSpawnWorkItems.Num();
    Info.OldestDeferredAutomaticSpawnSeconds = GetOldestAutomaticSpawnWorkItemAgeSeconds();

    if (const UWorld* World = GetWorld())
    {
        if (const UEMRPatientPoolSubsystem* PoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
        {
            Info.WarmReadyPatientCount = PoolSubsystem->GetDebugInfo().InactivePatients;
        }
    }

    Info.ActiveProfile = ActiveProfile;

    Info.Modifiers.Reserve(ModifierStates.Num());
    for (const FSpawnModifierState& ModifierState : ModifierStates)
    {
        FEMRNightShiftSpawnModifierDebug& ModifierDebug = Info.Modifiers.AddDefaulted_GetRef();
        ModifierDebug.ModifierDefinition = ModifierState.ModifierDefinition;
        ModifierDebug.Accumulator = ModifierState.Accumulator;
        ModifierDebug.bEnabled = ModifierState.bEnabled;
    }

    Info.PendingRequests = PendingSpawnRequests;
    Algo::Sort(Info.PendingRequests, [](const FEMRNightShiftSpawnRequest& A, const FEMRNightShiftSpawnRequest& B)
    {
        return A.SequenceId < B.SequenceId;
    });

    return Info;
}


void UEMRNightShiftSpawnSubsystem::Tick(float DeltaTime)
{
    if (!bSpawningEnabled)
    {
        return;
    }

    if (!EnsureServerContext())
    {
        UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Tick] Lost server context, stopping spawning."));
        StopNightShiftSpawning();
        return;
    }

    ElapsedSeconds += DeltaTime;

    if (!CachedDifficultyTuning || BaseSpawnRatePerSecond <= 0.f)
    {
        TuningRefreshAccumulator += DeltaTime;
        if (TuningRefreshAccumulator >= FMath::Max(TuningRefreshIntervalSeconds, 0.1f))
        {
            TuningRefreshAccumulator = 0.0f;
            RefreshRuntimeTuningValues(true);
        }
    }

    bool bIssuedInitialRequestThisTick = false;
    if (bInitialAutomaticSpawnDelayActive)
    {
        InitialAutomaticSpawnDelayRemainingSeconds = FMath::Max(InitialAutomaticSpawnDelayRemainingSeconds - DeltaTime, 0.0f);
        if (InitialAutomaticSpawnDelayRemainingSeconds <= 0.0f)
        {
            bInitialAutomaticSpawnDelayActive = false;
            UE_LOG(LogNightShiftSpawn, Log, TEXT("[StartDelay] Automatic spawn delay elapsed; enabling automatic generation."));
        }
    }

    if (!bInitialAutomaticSpawnDelayActive && !bInitialAutomaticSpawnRequestIssued)
    {
        EnqueueSpawnRequest({}, false);
        bInitialAutomaticSpawnRequestIssued = true;
        bIssuedInitialRequestThisTick = true;
        UE_LOG(LogNightShiftSpawn, Log, TEXT("[StartDelay] Issued first automatic spawn request at elapsed %.2fs."), ElapsedSeconds);
    }

    if (!bInitialAutomaticSpawnDelayActive && !bIssuedInitialRequestThisTick)
    {
        GenerateBaseRequests(DeltaTime);
        GenerateModifierRequests(DeltaTime);
    }

    ProcessAutomaticSpawnWorkItems(DeltaTime);

    if (bImmediateRequestPending)
    {
        UE_LOG(LogNightShiftSpawn, Verbose, TEXT("[Tick] Immediate request pending, forcing pump."));
        PumpQueue();
    }

    PumpQueue();
}


bool UEMRNightShiftSpawnSubsystem::StartNightShiftSpawning(const UEMRNightShiftDefinition* Definition)
{
    const bool bServerContext = EnsureServerContext();
    if (!Definition || !bServerContext)
    {
        const UWorld* World = GetWorld();
        const int32 NetModeValue = World ? static_cast<int32>(World->GetNetMode()) : -1;
        UE_LOG(
            LogNightShiftSpawn,
            Warning,
            TEXT("[Start] Failed to start spawning (Definition=%p, ServerContext=%d, NetMode=%d)."),
            Definition,
            bServerContext ? 1 : 0,
            NetModeValue);
        return false;
    }

    ActiveDefinition = Definition;
    ActiveProfile = Definition->SpawnProfile;
    RefreshRuntimeTuningValues(true);
    SpecialEventSpawnRateMultiplier = 1.0f;

    UE_LOG(LogNightShiftSpawn, Log, TEXT("[Start] Starting NightShift \"%s\" | Difficulty=%s"),
        *Definition->GetName(), *StaticEnum<EEMRNightShiftDifficultyTier>()->GetValueAsString(Definition->DifficultyTier));

    ResetRuntimeState();
    RefreshInitialAutomaticSpawnDelay();
    CachedWaitingRoomManager.Reset();
    CachedEntryPoint.Reset();

    UE_LOG(LogNightShiftSpawn, Log, TEXT("[Start] BaseRate=%.2f/s | PlayerCountMult=%.2f | DifficultySpawnMult=%.2f"),
        BaseSpawnRatePerSecond,
        PlayerCountMultiplier,
        DifficultySpawnRateMultiplier);

    ModifierStates.Reset();
    for (const FEMRNightShiftSpawnModifier& Modifier : Definition->SpawnProfile.SpawnModifiers)
    {
        FSpawnModifierState& ModifierState = ModifierStates.AddDefaulted_GetRef();
        ModifierState.ModifierDefinition = Modifier;
        ModifierState.SpawnCurve = Modifier.SpawnRateCurve.LoadSynchronous();
        ModifierState.bEnabled = Modifier.bEnabledByDefault;
        ModifierState.Accumulator = 0.f;

        UE_LOG(LogNightShiftSpawn, Log, TEXT("[Start] Modifier %s | Enabled=%d | Curve=%s"),
            *Modifier.ModifierTag.ToString(), Modifier.bEnabledByDefault,
            ModifierState.SpawnCurve ? *ModifierState.SpawnCurve->GetPathName() : TEXT("None"));
    }

    bSpawningEnabled = true;
    if (UEMRWaitingRoomManagerComponent* Manager = GetWaitingRoomManager())
    {
        Manager->OnSeatReleased.RemoveDynamic(this, &ThisClass::HandleSeatReleased);
        Manager->OnSeatReleased.AddDynamic(this, &ThisClass::HandleSeatReleased);
    }
    PumpQueue();
    return true;
}


void UEMRNightShiftSpawnSubsystem::StopNightShiftSpawning()
{
    if (bSpawningEnabled)
    {
        UE_LOG(LogNightShiftSpawn, Log, TEXT("[Stop] Stopping NightShift spawning."));
    }

    bSpawningEnabled = false;
    bImmediateRequestPending = false;
    ActiveDefinition.Reset();
    ModifierStates.Reset();
    PendingSpawnRequests.Reset();
    BaseSpawnRatePerSecond = 0.f;
    SpecialEventSpawnRateMultiplier = 1.0f;
    InitialAutomaticSpawnDelayRemainingSeconds = 0.0f;
    bInitialAutomaticSpawnDelayActive = false;
    bInitialAutomaticSpawnRequestIssued = false;
    DelayAppliedDefinition.Reset();
    DelayAppliedCycleIndex = INDEX_NONE;
    DelayAppliedNightShiftIndex = INDEX_NONE;
    DelayAppliedDifficultyTuning.Reset();

    for (FAutomaticSpawnWorkItem& WorkItem : AutomaticSpawnWorkItems)
    {
        ReleaseAutomaticSpawnWorkItem(WorkItem, true);
    }
    AutomaticSpawnWorkItems.Reset();

    if (UEMRWaitingRoomManagerComponent* Manager = CachedWaitingRoomManager.Get())
    {
        Manager->OnSeatReleased.RemoveDynamic(this, &ThisClass::HandleSeatReleased);
    }
    CachedWaitingRoomManager.Reset();
    CachedEntryPoint.Reset();
}


void UEMRNightShiftSpawnSubsystem::SetSpawnModifierEnabled(FGameplayTag ModifierTag, bool bEnabled)
{
    for (FSpawnModifierState& ModifierState : ModifierStates)
    {
        if (ModifierState.ModifierDefinition.ModifierTag == ModifierTag)
        {
            UE_LOG(LogNightShiftSpawn, Log, TEXT("[Modifier] %s -> Enabled=%d"), *ModifierTag.ToString(), bEnabled);
            ModifierState.bEnabled = bEnabled;
            ModifierState.Accumulator = 0.f;
            break;
        }
    }
}


void UEMRNightShiftSpawnSubsystem::NotifyActivePatientReleased()
{
    ActivePatientCount = FMath::Max(0, ActivePatientCount - 1);
    UE_LOG(LogNightShiftSpawn, Verbose, TEXT("[Release] Active patients decremented -> %d"), ActivePatientCount);
}


void UEMRNightShiftSpawnSubsystem::RequestImmediateSpawn(const FGameplayTagContainer& SourceTags)
{
    if (!bSpawningEnabled)
    {
        UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Immediate] Request ignored, spawning disabled."));
        return;
    }

    UE_LOG(LogNightShiftSpawn, Log, TEXT("[Immediate] Request queued | Tags=%s"),
        *SourceTags.ToStringSimple());

    EnqueueImmediateRequest(SourceTags);
}

void UEMRNightShiftSpawnSubsystem::SetSpecialEventSpawnRateMultiplier(const float Multiplier)
{
    SpecialEventSpawnRateMultiplier = FMath::Max(Multiplier, 0.0f);
}


void UEMRNightShiftSpawnSubsystem::ResetRuntimeState()
{
    BaseAccumulator = 0.f;
    ElapsedSeconds = 0.f;
    PendingSpawnRequests.Reset();
    AutomaticSpawnWorkItems.Reset();
    SpawnSequenceCounter = 0;
    ActivePatientCount = 0;
    SpawnOffsetCounter = 0;
    SpecialEventSpawnRateMultiplier = 1.0f;
    TuningRefreshAccumulator = 0.0f;
    bHasLoggedMissingDifficultyTuning = false;
}

bool UEMRNightShiftSpawnSubsystem::EnsureServerContext() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}

float UEMRNightShiftSpawnSubsystem::ApplySpawnRateMultipliers(float BaseRate) const
{
    float OvertimeMult = 1.f;
    if (const AEMRNightShiftGameState* NightShiftGameState = GetNightShiftGameState())
    {
        OvertimeMult = NightShiftGameState->GetOvertimeSpawnRateMultiplier();
    }

    return FMath::Max(BaseRate * PlayerCountMultiplier * OvertimeMult * DifficultySpawnRateMultiplier * SpecialEventSpawnRateMultiplier, 0.f);
}

void UEMRNightShiftSpawnSubsystem::LoadDifficultyTuningData()
{
    if (CachedDifficultyTuning)
    {
        return;
    }

    TArray<const UEMRSubsystemConfig*> Configs;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(Configs) || Configs.Num() == 0)
    {
        static const TSoftObjectPtr<UEMRSubsystemConfig> FallbackSubsystemConfig(
            FSoftObjectPath(TEXT("/Game/EmergencyRoom/Framework/DA_SubsystemConfig.DA_SubsystemConfig")));
        if (UEMRSubsystemConfig* LoadedFallbackConfig = FallbackSubsystemConfig.LoadSynchronous())
        {
            LoadedFallbackConfig->PreloadRuntimeAssets();
            CachedDifficultyTuning = LoadedFallbackConfig->GetDifficultyTuning();
            UE_LOG(
                LogNightShiftSpawn,
                Log,
                TEXT("[Tuning] Fallback-loaded SubsystemConfig '%s' (DifficultyTuning=%s)."),
                *LoadedFallbackConfig->GetName(),
                *GetNameSafe(CachedDifficultyTuning));
        }
        else
        {
            UE_LOG(
                LogNightShiftSpawn,
                Warning,
                TEXT("[Tuning] Failed to fallback-load SubsystemConfig from '%s'."),
                *FallbackSubsystemConfig.ToSoftObjectPath().ToString());
        }
        return;
    }

    if (const UEMRSubsystemConfig* Config = Configs[0])
    {
        Config->PreloadRuntimeAssets();
        CachedDifficultyTuning = Config->GetDifficultyTuning();
        UE_LOG(
            LogNightShiftSpawn,
            Log,
            TEXT("[Tuning] Using loaded SubsystemConfig '%s' (DifficultyTuning=%s)."),
            *Config->GetName(),
            *GetNameSafe(CachedDifficultyTuning));
    }
}

void UEMRNightShiftSpawnSubsystem::RefreshRuntimeTuningValues(const bool bForceLog)
{
    const float PreviousBaseRate = BaseSpawnRatePerSecond;
    const float PreviousPlayerMultiplier = PlayerCountMultiplier;
    const float PreviousDifficultyMultiplier = DifficultySpawnRateMultiplier;

    LoadDifficultyTuningData();

    const UEMRNightShiftDefinition* Definition = ActiveDefinition.Get();
    DifficultySpawnRateMultiplier = (CachedDifficultyTuning && Definition)
    ? CachedDifficultyTuning->GetSpawnRateMultiplier(Definition->DifficultyTier)
    : 1.0f;

    const int32 PlayerCount = FMath::Max(GetConnectedPlayerCount(), 1);
    PlayerCountMultiplier = CachedDifficultyTuning
    ? CachedDifficultyTuning->GetPlayerCountMultiplier(PlayerCount)
    : 1.0f;

    BaseSpawnRatePerSecond = CachedDifficultyTuning
    ? CachedDifficultyTuning->GetBaseSpawnRatePerSecond()
    : 0.0f;

    if (!CachedDifficultyTuning)
    {
        if (bForceLog || !bHasLoggedMissingDifficultyTuning)
        {
            UE_LOG(
                LogNightShiftSpawn,
                Warning,
                TEXT("[Tuning] Difficulty tuning unavailable; spawn base rate is %.2f. Waiting for SubsystemConfig load."),
                BaseSpawnRatePerSecond);
        }
        bHasLoggedMissingDifficultyTuning = true;
        return;
    }

    const bool bTuningChanged =
    !FMath::IsNearlyEqual(PreviousBaseRate, BaseSpawnRatePerSecond, KINDA_SMALL_NUMBER)
    || !FMath::IsNearlyEqual(PreviousPlayerMultiplier, PlayerCountMultiplier, KINDA_SMALL_NUMBER)
    || !FMath::IsNearlyEqual(PreviousDifficultyMultiplier, DifficultySpawnRateMultiplier, KINDA_SMALL_NUMBER);

    if (bForceLog || bTuningChanged || bHasLoggedMissingDifficultyTuning)
    {
        UE_LOG(
            LogNightShiftSpawn,
            Log,
            TEXT("[Tuning] Applied BaseRate=%.2f/s | PlayerCountMult=%.2f | DifficultySpawnMult=%.2f"),
            BaseSpawnRatePerSecond,
            PlayerCountMultiplier,
            DifficultySpawnRateMultiplier);
    }

    bHasLoggedMissingDifficultyTuning = false;
}

void UEMRNightShiftSpawnSubsystem::RefreshInitialAutomaticSpawnDelay()
{
    const UEMRNightShiftDefinition* Definition = ActiveDefinition.Get();
    if (!Definition)
    {
        return;
    }

    const AEMRNightShiftGameState* NightShiftGameState = GetNightShiftGameState();
    const int32 CurrentCycleIndex = NightShiftGameState ? NightShiftGameState->GetCurrentCycleIndex() : INDEX_NONE;
    const int32 CurrentNightShiftIndex = NightShiftGameState ? NightShiftGameState->GetNightShiftIndexInCycle() : INDEX_NONE;

    const bool bAlreadyAppliedForThisShift =
    DelayAppliedDefinition.Get() == Definition
    && DelayAppliedCycleIndex == CurrentCycleIndex
    && DelayAppliedNightShiftIndex == CurrentNightShiftIndex;

    if (bAlreadyAppliedForThisShift && bInitialAutomaticSpawnRequestIssued)
    {
        UE_LOG(
            LogNightShiftSpawn,
            Verbose,
            TEXT("[StartDelay] First automatic request already issued for this shift. Remaining=%.2f Active=%s"),
            InitialAutomaticSpawnDelayRemainingSeconds,
            bInitialAutomaticSpawnDelayActive ? TEXT("true") : TEXT("false"));
        return;
    }

    const bool bHasSameTuningAsset = DelayAppliedDifficultyTuning.Get() == CachedDifficultyTuning;
    if (bAlreadyAppliedForThisShift && bHasSameTuningAsset)
    {
        UE_LOG(
            LogNightShiftSpawn,
            Verbose,
            TEXT("[StartDelay] Delay already initialized for this shift. Remaining=%.2f Active=%s"),
            InitialAutomaticSpawnDelayRemainingSeconds,
            bInitialAutomaticSpawnDelayActive ? TEXT("true") : TEXT("false"));
        return;
    }

    const float ConfiguredDelaySeconds = CachedDifficultyTuning
    ? CachedDifficultyTuning->GetInitialAutomaticSpawnDelaySeconds(Definition->DifficultyTier)
    : 0.0f;

    InitialAutomaticSpawnDelayRemainingSeconds = FMath::Max(ConfiguredDelaySeconds, 0.0f);
    bInitialAutomaticSpawnDelayActive = InitialAutomaticSpawnDelayRemainingSeconds > 0.0f;
    if (!bAlreadyAppliedForThisShift)
    {
        bInitialAutomaticSpawnRequestIssued = false;
    }

    DelayAppliedDefinition = Definition;
    DelayAppliedCycleIndex = CurrentCycleIndex;
    DelayAppliedNightShiftIndex = CurrentNightShiftIndex;
    DelayAppliedDifficultyTuning = CachedDifficultyTuning;

    UE_LOG(
        LogNightShiftSpawn,
        Log,
        TEXT("[StartDelay] Configured delay=%.2fs | Difficulty=%s | Tuning=%s | Shift=(Cycle=%d NightShift=%d)."),
        ConfiguredDelaySeconds,
        *StaticEnum<EEMRNightShiftDifficultyTier>()->GetValueAsString(Definition->DifficultyTier),
        *GetNameSafe(CachedDifficultyTuning),
        CurrentCycleIndex,
        CurrentNightShiftIndex);

    if (bInitialAutomaticSpawnDelayActive)
    {
        UE_LOG(
            LogNightShiftSpawn,
            Log,
            TEXT("[StartDelay] Automatic generation delayed by %.2fs for %s."),
            InitialAutomaticSpawnDelayRemainingSeconds,
            *StaticEnum<EEMRNightShiftDifficultyTier>()->GetValueAsString(Definition->DifficultyTier));
    }
}

int32 UEMRNightShiftSpawnSubsystem::GetConnectedPlayerCount() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const AGameStateBase* GameState = World->GetGameState())
        {
            return GameState->PlayerArray.Num();
        }

        return World->GetNumPlayerControllers();
    }

    return 0;
}

void UEMRNightShiftSpawnSubsystem::GenerateBaseRequests(float DeltaTime)
{
    if (BaseSpawnRatePerSecond <= 0.f)
    {
        return;
    }

    const float RatePerSecond = ApplySpawnRateMultipliers(BaseSpawnRatePerSecond);
    if (RatePerSecond <= 0.f)
    {
        return;
    }

    BaseAccumulator += RatePerSecond * DeltaTime;
    while (BaseAccumulator >= 1.f)
    {
        BaseAccumulator -= 1.f;
        UE_LOG(LogNightShiftSpawn, Verbose, TEXT("[Base] Enqueue request | Rate=%.2f | Pending=%d"), RatePerSecond, PendingSpawnRequests.Num());
        EnqueueSpawnRequest({}, false);
    }
}

void UEMRNightShiftSpawnSubsystem::GenerateModifierRequests(float DeltaTime)
{
    for (FSpawnModifierState& ModifierState : ModifierStates)
    {
        if (!ModifierState.bEnabled || !ModifierState.SpawnCurve)
        {
            continue;
        }

        const float CurveValue = ModifierState.SpawnCurve->GetFloatValue(ElapsedSeconds);
        const float RatePerSecond = ApplySpawnRateMultipliers(CurveValue);
        if (RatePerSecond <= 0.f)
        {
            continue;
        }

        ModifierState.Accumulator += RatePerSecond * DeltaTime;
        while (ModifierState.Accumulator >= 1.f)
        {
            ModifierState.Accumulator -= 1.f;
            UE_LOG(LogNightShiftSpawn, Verbose, TEXT("[Modifier] %s enqueued request | Rate=%.2f"), *ModifierState.ModifierDefinition.ModifierTag.ToString(), RatePerSecond);
            EnqueueSpawnRequest(FGameplayTagContainer(ModifierState.ModifierDefinition.ModifierTag), false);
        }
    }
}


void UEMRNightShiftSpawnSubsystem::PumpQueue()
{
    while (PendingSpawnRequests.Num() > 0)
    {
        FEMRNightShiftSpawnRequest Request;
        PendingSpawnRequests.HeapPop(Request);
        UE_LOG(LogNightShiftSpawn, Verbose, TEXT("[Pump] Trying spawn | Seq=%d | Pending=%d"), Request.SequenceId, PendingSpawnRequests.Num());
        if (!TrySpawnPatient(Request))
        {
            UE_LOG(
                LogNightShiftSpawn,
                Warning,
                TEXT("[Pump] Spawn failed, requeue Seq=%d | ActivePatients=%d | PendingAfterRequeue=%d"),
                Request.SequenceId,
                ActivePatientCount,
                PendingSpawnRequests.Num() + 1);
            PendingSpawnRequests.HeapPush(Request);
            break;
        }
    }

    bImmediateRequestPending = false;
}

void UEMRNightShiftSpawnSubsystem::ProcessAutomaticSpawnWorkItems(const float DeltaTime)
{
    if (AutomaticSpawnWorkItems.Num() == 0)
    {
        return;
    }

    for (FAutomaticSpawnWorkItem& WorkItem : AutomaticSpawnWorkItems)
    {
        WorkItem.TimeSinceQueuedSeconds += DeltaTime;
    }

    const double BudgetDeadlineSeconds = FPlatformTime::Seconds() + FMath::Max(AutomaticSpawnWorkBudgetMs, 0.1f) / 1000.0;
    int32 WorkIndex = 0;
    while (WorkIndex < AutomaticSpawnWorkItems.Num())
    {
        FAutomaticSpawnWorkItem& WorkItem = AutomaticSpawnWorkItems[WorkIndex];
        if (TryAdvanceAutomaticSpawnWorkItem(WorkItem))
        {
            AutomaticSpawnWorkItems.RemoveAtSwap(WorkIndex);
            continue;
        }

        ++WorkIndex;
        if (FPlatformTime::Seconds() >= BudgetDeadlineSeconds)
        {
            break;
        }
    }
}

bool UEMRNightShiftSpawnSubsystem::TryAdvanceAutomaticSpawnWorkItem(FAutomaticSpawnWorkItem& WorkItem)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        ReleaseAutomaticSpawnWorkItem(WorkItem, true);
        return true;
    }

    UEMRPatientPoolSubsystem* PoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>();
    if (!PoolSubsystem)
    {
        ReleaseAutomaticSpawnWorkItem(WorkItem, true);
        return true;
    }

    AEMRPatient* Patient = WorkItem.Patient.Get();
    if (!Patient)
    {
        ReleaseAutomaticSpawnWorkItem(WorkItem, false);
        return true;
    }

    switch (WorkItem.Stage)
    {
    case EEMRAutomaticSpawnStage::HiddenSpawn:
        Patient->SetActorTransform(WorkItem.SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
        Patient->SetActorHiddenInGame(true);
        Patient->SetActorEnableCollision(false);
        WorkItem.Stage = EEMRAutomaticSpawnStage::ApplyPatientData;
        return false;

    case EEMRAutomaticSpawnStage::ApplyPatientData:
        if (WorkItem.PatientData)
        {
            PoolSubsystem->ApplyPatientDataForAutomaticSpawn(*Patient, *WorkItem.PatientData);
        }
        WorkItem.Stage = EEMRAutomaticSpawnStage::MeshAnimInit;
        return false;

    case EEMRAutomaticSpawnStage::MeshAnimInit:
        Patient->PrimeMeshAndAnimationForSpawn();
        WorkItem.Stage = EEMRAutomaticSpawnStage::CollisionAIEnable;
        return false;

    case EEMRAutomaticSpawnStage::CollisionAIEnable:
        PoolSubsystem->EnablePatientAIForAutomaticSpawn(*Patient);
        WorkItem.Stage = EEMRAutomaticSpawnStage::Reveal;
        return false;

    case EEMRAutomaticSpawnStage::Reveal:
        if (WorkItem.TimeSinceQueuedSeconds < AutomaticSpawnMinimumRevealDelaySeconds)
        {
            return false;
        }

        PoolSubsystem->ActivatePatient(*Patient, WorkItem.SpawnTransform);
        DispatchMoveToSeatEvent(*Patient, WorkItem.ReservedSeat.Get());

        ++ActivePatientCount;
        PatientSpawnedNativeDelegate.Broadcast(Patient);
        SpawnRequestNativeDelegate.Broadcast(WorkItem.SourceRequest);
        OnSpawnRequestReady_BP.Broadcast(WorkItem.SourceRequest);
        return true;

    default:
        break;
    }

    return true;
}

void UEMRNightShiftSpawnSubsystem::ReleaseAutomaticSpawnWorkItem(FAutomaticSpawnWorkItem& WorkItem, const bool bReturnPatientToPool)
{
    if (UEMRWaitingSeatComponent* Seat = WorkItem.ReservedSeat.Get())
    {
        if (UEMRWaitingRoomManagerComponent* WaitingRoomManager = GetWaitingRoomManager())
        {
            WaitingRoomManager->ReleaseSeat(Seat, Seat->GetReservedActor());
        }
        else
        {
            Seat->ClearSeat();
        }
    }

    if (!bReturnPatientToPool)
    {
        return;
    }

    if (AEMRPatient* Patient = WorkItem.Patient.Get())
    {
        if (UWorld* World = GetWorld())
        {
            if (UEMRPatientPoolSubsystem* PoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
            {
                PoolSubsystem->ReleasePatient(Patient);
            }
        }
    }
}

void UEMRNightShiftSpawnSubsystem::DispatchMoveToSeatEvent(AEMRPatient& Patient, UEMRWaitingSeatComponent* Seat) const
{
    if (!Seat)
    {
        return;
    }

    if (UAbilitySystemComponent* ASC = Patient.GetAbilitySystemComponent())
    {
        FGameplayEventData EventData;
        EventData.EventTag = EMRTags::Abilities::MoveToSeat;
        EventData.Target = &Patient;
        EventData.OptionalObject = Seat;
        ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
    }
}


void UEMRNightShiftSpawnSubsystem::EnqueueSpawnRequest(const FGameplayTagContainer& SourceTags, const bool bIsImmediateRequest)
{
    const int32 MaxPending = GetMaxPendingRequestCount();
    if (MaxPending > 0 && PendingSpawnRequests.Num() >= MaxPending)
    {
        UE_LOG(LogNightShiftSpawn, Verbose, TEXT("[Queue] Pending limit reached (%d). Dropping oldest request."), MaxPending);
        PendingSpawnRequests.HeapPopDiscard();
    }

    FEMRNightShiftSpawnRequest NewRequest;
    NewRequest.SequenceId = ++SpawnSequenceCounter;
    NewRequest.RequestTimestamp = ElapsedSeconds;
    NewRequest.RequestedDifficulty = ActiveDefinition.IsValid() ? ActiveDefinition->DifficultyTier : EEMRNightShiftDifficultyTier::Standard;
    NewRequest.SourceTags = SourceTags;
    NewRequest.bIsImmediateRequest = bIsImmediateRequest;

    UE_LOG(LogNightShiftSpawn, Verbose, TEXT("[Queue] Enqueued Seq=%d | Difficulty=%s | Tags=%s | Pending=%d"),
        NewRequest.SequenceId,
        *StaticEnum<EEMRNightShiftDifficultyTier>()->GetValueAsString(NewRequest.RequestedDifficulty),
        *SourceTags.ToStringSimple(),
        PendingSpawnRequests.Num() + 1);

	PendingSpawnRequests.HeapPush(NewRequest);
}


bool UEMRNightShiftSpawnSubsystem::TrySpawnPatient(const FEMRNightShiftSpawnRequest& Request)
{
    if (Request.bIsImmediateRequest)
    {
        return TrySpawnPatientImmediate(Request);
    }

    return TryStartAutomaticSpawnWorkItem(Request);
}

bool UEMRNightShiftSpawnSubsystem::TrySpawnPatientImmediate(const FEMRNightShiftSpawnRequest& Request)
{
    UE_LOG(
        LogNightShiftSpawn,
        Verbose,
        TEXT("[Spawn][Immediate] Try Seq=%d | Tags=%s | Pending=%d | Active=%d"),
        Request.SequenceId,
        *Request.SourceTags.ToStringSimple(),
        PendingSpawnRequests.Num(),
        ActivePatientCount);

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Spawn][Immediate] World missing for Seq=%d"), Request.SequenceId);
        return false;
    }

    UEMRPatientPoolSubsystem* PoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>();
    if (!PoolSubsystem)
    {
        UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Spawn][Immediate] PatientPoolSubsystem missing for Seq=%d"), Request.SequenceId);
        return false;
    }

    UEMRWaitingRoomManagerComponent* WaitingRoomManager = GetWaitingRoomManager();
    UEMRWaitingSeatComponent* AssignedSeat = nullptr;
    if (WaitingRoomManager)
    {
        AssignedSeat = FindAvailableSeat(*WaitingRoomManager);
        if (!AssignedSeat)
        {
            UE_LOG(
                LogNightShiftSpawn,
                Verbose,
                TEXT("[Spawn][Immediate] No free seats available for Seq=%d (HasFreeSeat=%d)"),
                Request.SequenceId,
                WaitingRoomManager->HasFreeSeat() ? 1 : 0);
            return false;
        }
    }

    AEMRPatient* Patient = PoolSubsystem->AcquirePatient(false);
    if (!Patient)
    {
        const FEMRPatientPoolDebugInfo PoolDebug = PoolSubsystem->GetDebugInfo();
        UE_LOG(
            LogNightShiftSpawn,
            Warning,
            TEXT("[Spawn][Immediate] AcquirePatient returned null for Seq=%d | PoolTarget=%d Inactive=%d ActiveTracked=%d Classes=%s"),
            Request.SequenceId,
            PoolDebug.TargetPoolSize,
            PoolDebug.InactivePatients,
            PoolDebug.ActiveTrackedPatients,
            *PoolDebug.PatientClassName);
        return false;
    }

    FTransform SpawnTransform = BuildSpawnTransform();
    SpawnTransform.AddToTranslation(ComputeSpawnOffset());

    if (WaitingRoomManager)
    {
        if (!AssignedSeat || !WaitingRoomManager->AssignSpecificSeatToPatient(AssignedSeat, Patient))
        {
            UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Spawn][Immediate] Seat reservation failed for patient %s (Seq=%d)"), *Patient->GetName(), Request.SequenceId);
            PoolSubsystem->ReleasePatient(Patient);
            return false;
        }

        UE_LOG(LogNightShiftSpawn, Log, TEXT("[Spawn][Immediate] Seat %s reserved for patient %s"), *AssignedSeat->GetName(), *Patient->GetName());
    }

    PoolSubsystem->ActivatePatient(*Patient, SpawnTransform);
    UE_LOG(
        LogNightShiftSpawn,
        Verbose,
        TEXT("[Spawn][Immediate] Activated patient %s at Loc=(%.1f, %.1f, %.1f)"),
        *Patient->GetName(),
        SpawnTransform.GetLocation().X,
        SpawnTransform.GetLocation().Y,
        SpawnTransform.GetLocation().Z);

    if (WaitingRoomManager)
    {
        DispatchMoveToSeatEvent(*Patient, AssignedSeat);
    }
    else
    {
        UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Spawn][Immediate] No WaitingRoomManager found; patient %s will remain unmanaged."), *Patient->GetName());
    }

    ++ActivePatientCount;
    UE_LOG(
        LogNightShiftSpawn,
        Log,
        TEXT("[Spawn][Immediate] Success Seq=%d Patient=%s ActivePatients=%d Pending=%d"),
        Request.SequenceId,
        *Patient->GetName(),
        ActivePatientCount,
        PendingSpawnRequests.Num());
    PatientSpawnedNativeDelegate.Broadcast(Patient);
    SpawnRequestNativeDelegate.Broadcast(Request);
    OnSpawnRequestReady_BP.Broadcast(Request);
    return true;
}

bool UEMRNightShiftSpawnSubsystem::TryStartAutomaticSpawnWorkItem(const FEMRNightShiftSpawnRequest& Request)
{
    UE_LOG(
        LogNightShiftSpawn,
        Verbose,
        TEXT("[Spawn][Auto] Queue staged spawn Seq=%d | Tags=%s | Pending=%d | Deferred=%d"),
        Request.SequenceId,
        *Request.SourceTags.ToStringSimple(),
        PendingSpawnRequests.Num(),
        AutomaticSpawnWorkItems.Num());

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Spawn][Auto] World missing for Seq=%d"), Request.SequenceId);
        return false;
    }

    UEMRPatientPoolSubsystem* PoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>();
    if (!PoolSubsystem)
    {
        UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Spawn][Auto] PatientPoolSubsystem missing for Seq=%d"), Request.SequenceId);
        return false;
    }

    UEMRWaitingRoomManagerComponent* WaitingRoomManager = GetWaitingRoomManager();
    UEMRWaitingSeatComponent* AssignedSeat = nullptr;
    if (WaitingRoomManager)
    {
        AssignedSeat = FindAvailableSeat(*WaitingRoomManager);
        if (!AssignedSeat)
        {
            UE_LOG(
                LogNightShiftSpawn,
                Verbose,
                TEXT("[Spawn][Auto] No free seats available for Seq=%d (HasFreeSeat=%d)"),
                Request.SequenceId,
                WaitingRoomManager->HasFreeSeat() ? 1 : 0);
            return false;
        }
    }

    AEMRPatient* Patient = PoolSubsystem->AcquirePatientShell();
    if (!Patient)
    {
        UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Spawn][Auto] AcquirePatientShell failed for Seq=%d"), Request.SequenceId);
        return false;
    }

    const UEMRPatientData* PatientData = PoolSubsystem->SelectPatientDataForAutomaticSpawn();

    FTransform SpawnTransform = BuildSpawnTransform();
    SpawnTransform.AddToTranslation(ComputeSpawnOffset());

    if (WaitingRoomManager)
    {
        if (!AssignedSeat || !WaitingRoomManager->AssignSpecificSeatToPatient(AssignedSeat, Patient))
        {
            UE_LOG(LogNightShiftSpawn, Warning, TEXT("[Spawn][Auto] Seat reservation failed for patient %s (Seq=%d)"), *Patient->GetName(), Request.SequenceId);
            PoolSubsystem->ReleasePatient(Patient);
            return false;
        }
    }

    FAutomaticSpawnWorkItem& WorkItem = AutomaticSpawnWorkItems.AddDefaulted_GetRef();
    WorkItem.SourceRequest = Request;
    WorkItem.Patient = Patient;
    WorkItem.PatientData = PatientData;
    WorkItem.ReservedSeat = AssignedSeat;
    WorkItem.SpawnTransform = SpawnTransform;
    WorkItem.TimeSinceQueuedSeconds = 0.0f;
    WorkItem.Stage = EEMRAutomaticSpawnStage::HiddenSpawn;
    return true;
}


FTransform UEMRNightShiftSpawnSubsystem::BuildSpawnTransform() const
{
    FVector SpawnLocation = FVector::ZeroVector;
    FRotator SpawnRotation = FRotator::ZeroRotator;

    if (const UWorld* World = GetWorld())
    {
        if (CachedEntryPoint.IsValid())
        {
            const FTransform EntryTransform = CachedEntryPoint->GetSpawnTransform();
            SpawnLocation = EntryTransform.GetLocation();
            SpawnRotation = EntryTransform.GetRotation().Rotator();
            return FTransform(SpawnRotation, SpawnLocation);
        }

        bool bFoundEntryPoint = false;
        for (TActorIterator<AEMRPatientEntryPoint> It(World); It; ++It)
        {
            if (AEMRPatientEntryPoint* EntryPoint = *It)
            {
                CachedEntryPoint = EntryPoint;
                const FTransform EntryTransform = EntryPoint->GetSpawnTransform();
                SpawnLocation = EntryTransform.GetLocation();
                SpawnRotation = EntryTransform.GetRotation().Rotator();
                bFoundEntryPoint = true;
                break;
            }
        }

        if (!bFoundEntryPoint)
        {
            AActor* ReferenceActor = nullptr;
            if (!ReferenceActor)
            {
                if (const APlayerController* PC = World->GetFirstPlayerController())
                {
                    ReferenceActor = PC->GetPawn();
                }
            }

            if (ReferenceActor)
            {
                if (const UGameInstance* GameInstance = World->GetGameInstance())
                {
                    if (const UEMRNavigationIntentSubsystem* NavigationSubsystem = GameInstance->GetSubsystem<UEMRNavigationIntentSubsystem>())
                    {
                        if (AEMRBaseMachine* ReceptionMachine = NavigationSubsystem->FindClosestMachine(ReferenceActor, EMRTags::Machine::Reception))
                        {
                            SpawnLocation = ReceptionMachine->GetActorLocation();
                            SpawnRotation = ReceptionMachine->GetActorRotation();
                        }
                    }
                }
            }
        }
    }

    return FTransform(SpawnRotation, SpawnLocation);
}


void UEMRNightShiftSpawnSubsystem::EnqueueImmediateRequest(const FGameplayTagContainer& SourceTags)
{
    EnqueueSpawnRequest(SourceTags, true);
    bImmediateRequestPending = true;
    UE_LOG(LogNightShiftSpawn, Verbose, TEXT("[Immediate] Flag set, queue size=%d"), PendingSpawnRequests.Num());
}


AEMRNightShiftGameState* UEMRNightShiftSpawnSubsystem::GetNightShiftGameState() const
{
    if (const UWorld* World = GetWorld())
    {
        return World->GetGameState<AEMRNightShiftGameState>();
    }
    return nullptr;
}

UEMRWaitingRoomManagerComponent* UEMRNightShiftSpawnSubsystem::GetWaitingRoomManager() const
{
    if (CachedWaitingRoomManager.IsValid())
    {
        return CachedWaitingRoomManager.Get();
    }

    if (const UWorld* World = GetWorld())
    {
        for (TActorIterator<AEMRWaitingRoomArea> It(World); It; ++It)
        {
            if (AEMRWaitingRoomArea* WaitingArea = *It)
            {
                if (UEMRWaitingRoomManagerComponent* Manager = WaitingArea->GetManagerComponent())
                {
                    UE_LOG(LogNightShiftSpawn, Log, TEXT("[Spawn] Found WaitingRoomManager on %s"), *WaitingArea->GetName());
                    CachedWaitingRoomManager = Manager;
                    return Manager;
                }
            }
        }
    }

    return nullptr;
}

UEMRWaitingSeatComponent* UEMRNightShiftSpawnSubsystem::FindAvailableSeat(UEMRWaitingRoomManagerComponent& Manager) const
{
    if (!Manager.HasFreeSeat())
    {
        return nullptr;
    }

    return Manager.FindRandomFreeSeat();
}

void UEMRNightShiftSpawnSubsystem::HandleSeatReleased(UEMRWaitingSeatComponent* Seat)
{
    if (!bSpawningEnabled || !Seat)
    {
        return;
    }

    PumpQueue();
}

int32 UEMRNightShiftSpawnSubsystem::GetMaxPendingRequestCount() const
{
    return FMath::Max(MaxPendingSpawnRequests, 1);
}

FVector UEMRNightShiftSpawnSubsystem::ComputeSpawnOffset() const
{
    if (SpawnOffsetSpacing <= 0.f || SpawnOffsetColumns <= 0)
    {
        return FVector::ZeroVector;
    }

    const int32 MaxSlots = FMath::Max(SpawnOffsetColumns * SpawnOffsetColumns, 1);
    const int32 Index = SpawnOffsetCounter++ % MaxSlots;
    const int32 Column = Index % SpawnOffsetColumns;
    const int32 Row = Index / SpawnOffsetColumns;

    const float OffsetX = static_cast<float>(Row) * SpawnOffsetSpacing;
    const float OffsetY = static_cast<float>(Column) * SpawnOffsetSpacing;

    return FVector(OffsetX, OffsetY, 0.f);
}

float UEMRNightShiftSpawnSubsystem::GetOldestAutomaticSpawnWorkItemAgeSeconds() const
{
    float OldestSeconds = 0.0f;
    for (const FAutomaticSpawnWorkItem& WorkItem : AutomaticSpawnWorkItems)
    {
        OldestSeconds = FMath::Max(OldestSeconds, WorkItem.TimeSinceQueuedSeconds);
    }

    return OldestSeconds;
}
