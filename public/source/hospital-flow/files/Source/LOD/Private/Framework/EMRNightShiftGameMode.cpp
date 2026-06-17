#include "Framework/EMRNightShiftGameMode.h"

#include "GameMapsSettings.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerController.h"
#include "Characters/Player/EMRPlayerState.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Data/EMREconomySystemGenerics.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Data/EMRSpecialEventDefinition.h"
#include "Framework/EMRAssetManager.h"
#include "Framework/EMRHubertDirector.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Lobby/EMRLobbyCharacterCatalog.h"
#include "Subsystems/EMRRunRulesSubsystem.h"
#include "Subsystems/EMRRunProgressionSubsystem.h"
#include "Subsystems/EMRPatientPoolSubsystem.h"
#include "Subsystems/EMRNightShiftSpawnSubsystem.h"
#include "Subsystems/EMRExamQueueSubsystem.h"
#include "Subsystems/EMRTreatmentSubsystem.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Subsystems/EMRLobbyInviteCodeSubsystem.h"
#include "Interaction/EMRBaseMachine.h"
#include "Interaction/EMRCoffeeMachineActor.h"
#include "Interaction/EMROxygenMachine.h"
#include "Interaction/EMRTreatmentBed.h"
#include "Environment/EMRMagicBoxActor.h"
#include "Environment/EMRSnackMachineActor.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "GAS/EMRTags.h"
#include "GAS/Attributes/EMRTeamAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Misc/DateTime.h"
#include "TimerManager.h"

namespace
{
    constexpr TCHAR NightShiftGameModeUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");
    constexpr float DefaultReputationPenalty = 5.0f;

    void AddCountForSplit(const bool bOvertimeSplit, int32& BeforeOvertimeCounter, int32& AfterOvertimeCounter)
    {
        if (bOvertimeSplit)
        {
            ++AfterOvertimeCounter;
        }
        else
        {
            ++BeforeOvertimeCounter;
        }
    }

    void AddFloatForSplit(const bool bOvertimeSplit, const float Value, float& BeforeOvertimeValue, float& AfterOvertimeValue)
    {
        if (bOvertimeSplit)
        {
            AfterOvertimeValue += Value;
        }
        else
        {
            BeforeOvertimeValue += Value;
        }
    }

    TArray<FEMRGameplayTelemetryTagCount> BuildSortedTagCounts(const TMap<FGameplayTag, int32>& CountByTag)
    {
        TArray<FGameplayTag> SortedTags;
        CountByTag.GetKeys(SortedTags);
        SortedTags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
        {
            return A.ToString() < B.ToString();
        });

        TArray<FEMRGameplayTelemetryTagCount> Result;
        Result.Reserve(SortedTags.Num());
        for (const FGameplayTag& Tag : SortedTags)
        {
            FEMRGameplayTelemetryTagCount& TagCount = Result.AddDefaulted_GetRef();
            TagCount.Tag = Tag;
            TagCount.Count = CountByTag.FindRef(Tag);
        }

        return Result;
    }
}

AEMRNightShiftGameMode::AEMRNightShiftGameMode()
{
    GameStateClass = AEMRNightShiftGameState::StaticClass();
    bUseSeamlessTravel = true;
}

UClass* AEMRNightShiftGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    const AEMRPlayerState* EMRPlayerState = InController ? InController->GetPlayerState<AEMRPlayerState>() : nullptr;
    const UEMRLobbyCharacterCatalog* Catalog = GetPlayerCharacterCatalog();
    if (EMRPlayerState && Catalog)
    {
        const int32 SelectedIndex = EMRPlayerState->GetLobbyCharacterIndex();
        if (const FLobbyCharacterDefinition* Definition = Catalog->GetCharacterDefinition(SelectedIndex))
        {
            if (Definition->PlayerCharacterClass)
            {
                return Definition->PlayerCharacterClass;
            }
        }
    }

    return Super::GetDefaultPawnClassForController_Implementation(InController);
}

const UEMRLobbyCharacterCatalog* AEMRNightShiftGameMode::GetPlayerCharacterCatalog() const
{
    if (CachedPlayerCharacterCatalog.IsValid())
    {
        return CachedPlayerCharacterCatalog.Get();
    }

    CachedPlayerCharacterCatalog = PlayerCharacterCatalog.LoadSynchronous();
    return CachedPlayerCharacterCatalog.Get();
}

void AEMRNightShiftGameMode::BeginPlay()
{
    Super::BeginPlay();


    if (!HasAuthority())
    {
        return;
    }

    GetPlayerCharacterCatalog();

    UEMRAssetManager& AssetManager = UEMRAssetManager::Get();
    AssetManager.LoadPatients(FStreamableDelegate());
    AssetManager.LoadEconomySystemGenerics(FStreamableDelegate());
    AssetManager.LoadSubsystemConfig(FStreamableDelegate());

    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        if (ProgressionSubsystem->ApplyToGameState(GetNightShiftGameState()))
        {
            if (const AEMRNightShiftGameState* RestoredRunState = GetNightShiftGameState())
            {
                UE_LOG(
                    LogTemp,
                    Log,
                    TEXT("[NightShiftGameMode] BeginPlay - Restored run from ProgressionSubsystem (Phase=%d, Cycle=%d, NightShift=%d, TotalRevenue=%.2f)"),
                    static_cast<int32>(RestoredRunState->GetRunPhase()),
                    RestoredRunState->GetCurrentCycleIndex(),
                    RestoredRunState->GetNightShiftIndexInCycle(),
                    RestoredRunState->GetTotalRevenue());
            }

            ProgressionSubsystem->LogAuthoritativeState(TEXT("NightShift.BeginPlay.Restored"), GetNightShiftGameState());
            ProgressionSubsystem->StoreNightShiftStartSnapshot(GetNightShiftGameState());

            UE_LOG(LogTemp, Log, TEXT("[NightShiftGameMode] BeginPlay - Restored run from ProgressionSubsystem"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[RunPhaseDebug] NightShift BeginPlay - ApplyToGameState failed"));
        }
    }

    bNightShiftEndedByReputation = false;
    SpecialEventRuntimeState = FEMRSpecialEventRuntimeState();
    ActiveNightShiftForSpecialEvent.Reset();
    ActiveSpecialEventDefinition.Reset();
    RegisterReputationListener();

    HandleRestoredRunState();
    if (AEMRNightShiftGameState* RunGameState = GetNightShiftGameState())
    {
        const FString ExistingRunCode = RunGameState->GetSessionInviteCode().TrimStartAndEnd();
        if (ExistingRunCode.IsEmpty())
        {
            UEMRLobbyInviteCodeSubsystem* InviteCodeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbyInviteCodeSubsystem>() : nullptr;
            if (InviteCodeSubsystem)
            {
                FString SessionCode;
                if (!InviteCodeSubsystem->GetCurrentSessionInviteCode(SessionCode))
                {
                    const FString GeneratedCode = InviteCodeSubsystem->GenerateInviteCode();
                    if (!GeneratedCode.IsEmpty() && InviteCodeSubsystem->UpdateSessionInviteCode(GeneratedCode))
                    {
                        SessionCode = GeneratedCode;
                    }
                }

                SessionCode = SessionCode.TrimStartAndEnd();
                if (!SessionCode.IsEmpty())
                {
                    RunGameState->SetSessionInviteCode(SessionCode);
                    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
                    {
                        ProgressionSubsystem->CacheFromGameState(RunGameState);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[InviteCode] NightShiftGameMode could not resolve or generate session invite code."));
                }
            }
        }
    }
    ApplyRunUpgradesToNightShiftActors();
    ValidateCoffeeMachinePlacementForActiveUpgrade();
    ValidateOxygenMachinePlacementForActiveUpgrade();
    ValidateSnackMachinePlacementForActiveUpgrade();
    ValidateMagicBoxPlacementForActiveUpgrade();
    ValidateTreatmentBedPlacementForActiveUpgrade();
}

void AEMRNightShiftGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AEMRHubertDirector* ResolvedHubertDirector = HubertDirector.Get())
    {
        ResolvedHubertDirector->StopHubertRuntime();
    }

    UnregisterTelemetryListeners();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(NightShiftEndDelayHandle);
        World->GetTimerManager().ClearTimer(OvertimeStopDepartureTimerHandle);
    }
    OvertimeStopDepartureQueue.Reset();
    OvertimeStopDepartureIndex = 0;

    if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        if (UAbilitySystemComponent* TeamASC = RunGS->GetAbilitySystemComponent())
        {
            if (TeamReputationChangedHandle.IsValid())
            {
                TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetReputationAttribute()).Remove(TeamReputationChangedHandle);
            }

            if (TeamRevenueChangedHandle.IsValid())
            {
                TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetTotalRevenueAttribute()).Remove(TeamRevenueChangedHandle);
            }
        }
    }

    TeamReputationChangedHandle.Reset();
    TeamRevenueChangedHandle.Reset();

    Super::EndPlay(EndPlayReason);
}

void AEMRNightShiftGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
}

void AEMRNightShiftGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
}


bool AEMRNightShiftGameMode::ShouldUseSeamlessTravel() const
{
    return true;
}


AEMRNightShiftGameState* AEMRNightShiftGameMode::GetNightShiftGameState() const
{
    return GetGameState<AEMRNightShiftGameState>();
}


UEMRRunProgressionSubsystem* AEMRNightShiftGameMode::GetRunProgressionSubsystem() const
{
    if (!GetGameInstance())
    {
        return nullptr;
    }

    return GetGameInstance()->GetSubsystem<UEMRRunProgressionSubsystem>();
}

UEMRRunRulesSubsystem* AEMRNightShiftGameMode::GetRunRulesSubsystem() const
{
    if (RunRulesSubsystem.IsValid())
    {
        return RunRulesSubsystem.Get();
    }

    if (!GetGameInstance())
    {
        return nullptr;
    }

    RunRulesSubsystem = GetGameInstance()->GetSubsystem<UEMRRunRulesSubsystem>();
    return RunRulesSubsystem.Get();
}

AEMRHubertDirector* AEMRNightShiftGameMode::ResolveHubertDirector()
{
    if (!HasAuthority())
    {
        return nullptr;
    }

    if (AEMRHubertDirector* ExistingDirector = HubertDirector.Get())
    {
        return ExistingDirector;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AEMRHubertDirector> It(World); It; ++It)
    {
        if (AEMRHubertDirector* FoundDirector = *It)
        {
            HubertDirector = FoundDirector;
            return FoundDirector;
        }
    }

    if (!HubertDirectorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Hubert] NightShiftGameMode has no HubertDirectorClass and no placed director in map."));
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = nullptr;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEMRHubertDirector* SpawnedDirector = World->SpawnActor<AEMRHubertDirector>(HubertDirectorClass, FTransform::Identity, SpawnParameters);
    if (!SpawnedDirector)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Hubert] NightShiftGameMode failed to spawn director class=%s"), *GetNameSafe(HubertDirectorClass));
        return nullptr;
    }

    HubertDirector = SpawnedDirector;
    return SpawnedDirector;
}

const UEMRDifficultyTuningData* AEMRNightShiftGameMode::GetDifficultyTuning() const
{
    if (CachedDifficultyTuning.IsValid())
    {
        return CachedDifficultyTuning.Get();
    }

    TArray<const UEMRSubsystemConfig*> Configs;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(Configs) || Configs.Num() == 0)
    {
        return nullptr;
    }

    const UEMRSubsystemConfig* Config = Configs[0];
    if (Config)
    {
        CachedDifficultyTuning = Config->GetDifficultyTuning();
    }

    return CachedDifficultyTuning.Get();
}

void AEMRNightShiftGameMode::SyncRunStateToSubsystem()
{
    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        ProgressionSubsystem->CacheFromGameState(GetNightShiftGameState());
    }
}

bool AEMRNightShiftGameMode::TravelToHubLevel()
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameMode] TravelToHubLevel requested"));
    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] NightShiftGameMode.TravelToHubLevel requested Auth=%d NetMode=%d"),
        HasAuthority() ? 1 : 0,
        static_cast<int32>(GetNetMode()));

    UWorld* World = GetWorld();
    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!World || !RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][NightShiftGameMode] TravelToHubLevel aborted: missing World or GameState"));
        return false;
    }

    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        ProgressionSubsystem->ClearPendingNightShiftForTravel();
        ProgressionSubsystem->CacheFromGameState(RunGS);
    }

    FString LevelLongName;
    if (HubLevel.IsNull())
    {
        const UGameMapsSettings* GameMapsSettings = GetDefault<UGameMapsSettings>();
        LevelLongName = GameMapsSettings ? GameMapsSettings->GetGameDefaultMap() : FString();
    }
    else
    {
        const FSoftObjectPath LevelPath = HubLevel.ToSoftObjectPath();
        LevelLongName = LevelPath.GetLongPackageName();
    }

    if (LevelLongName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][NightShiftGameMode] Travel aborted: invalid Hub level"));
        return false;
    }

    const FString SanitizedLevelLongName = UWorld::RemovePIEPrefix(LevelLongName);

    const FString ClientTravelURL = SanitizedLevelLongName;
    const FString ServerTravelURL = SanitizedLevelLongName.Contains(TEXT("?"))
    ? SanitizedLevelLongName + TEXT("&listen")
    : SanitizedLevelLongName + TEXT("?listen");

    RunGS->SetPendingTravelURL(ClientTravelURL);
    NotifyControllersOfPendingTravel(ClientTravelURL);

    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] NightShiftGameMode.TravelToHubLevel ServerTravelURL=%s ClientTravelURL=%s RunPhase=%d Revenue=%.2f Quota=%.2f"),
        *ServerTravelURL,
        *ClientTravelURL,
        static_cast<int32>(RunGS->GetRunPhase()),
        RunGS->GetTotalRevenue(),
        RunGS->GetCurrentCycleQuota());
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameMode] ServerTravel -> %s"), *ServerTravelURL);
    World->ServerTravel(ServerTravelURL, true);
    return true;
}

void AEMRNightShiftGameMode::NotifyControllersOfPendingTravel(const FString& TravelURL)
{
    // ServerTravel is the authoritative travel path; this hook exists for UI/status notifications only.
    (void)TravelURL;
}

void AEMRNightShiftGameMode::HandleNightShiftOvertimeStarted()
{
    if (!HasAuthority())
    {
        return;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return;
    }

    if (RunGS->IsInNightShiftOvertime())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameMode] HandleNightShiftOvertimeStarted"));

    RunGS->SetNightShiftOvertimeActive(true);
    RunGS->SetNightShiftTimeExpired(true);
    RunGS->Multicast_PlayGlobalSound2D(OvertimeStartedAlertSound);
}

void AEMRNightShiftGameMode::NotifyNightShiftFinished()
{
    if (!HasAuthority())
    {
        return;
    }

    if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        if (RunGS->GetRunPhase() == EER_RunPhase::InNightShift && RunGS->IsInNightShiftOvertime())
        {
            RunGS->SetReputationFrozen(true, RunGS->GetReputation());
        }
    }

    BeginNightShiftFinish();
}

void AEMRNightShiftGameMode::ForceNightShiftFailureByReputationForTests()
{
    if (!HasAuthority())
    {
        return;
    }

    HandleNightShiftFailureByReputation();
}

void AEMRNightShiftGameMode::ForceNightShiftSuccessForTests()
{
    if (!HasAuthority())
    {
        return;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS || RunGS->GetRunPhase() != EER_RunPhase::InNightShift)
    {
        return;
    }

    const float TargetRevenue = FMath::Max(RunGS->GetTotalRevenue(), RunGS->GetCurrentCycleQuota());
    RunGS->SetTotalRevenue(TargetRevenue);

    NotifyNightShiftFinished();
}

void AEMRNightShiftGameMode::SetNightShiftEndDelayForTests(const float NewDelaySeconds)
{
    if (!HasAuthority())
    {
        return;
    }

    NightShiftEndDelaySeconds = FMath::Max(NewDelaySeconds, 0.f);
}

bool AEMRNightShiftGameMode::ApplyRunUpgradeStackForTests(const FGameplayTag UpgradeTag, const int32 StackDelta, FString& OutMessage)
{
    if (!HasAuthority())
    {
        OutMessage = TEXT("Action rejected: not authority.");
        return false;
    }

    if (!UpgradeTag.IsValid())
    {
        OutMessage = TEXT("Action rejected: invalid upgrade tag.");
        return false;
    }

    if (StackDelta <= 0)
    {
        OutMessage = TEXT("Action rejected: stack delta must be > 0.");
        return false;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        OutMessage = TEXT("Action rejected: NightShift game state unavailable.");
        return false;
    }

    const int32 NewStackCount = RunGS->AddActiveRunUpgradeStack(UpgradeTag, StackDelta);
    if (NewStackCount <= 0)
    {
        OutMessage = FString::Printf(TEXT("Failed to apply upgrade stack for %s."), *UpgradeTag.ToString());
        return false;
    }

    if (RunGS->GetRunPhase() == EER_RunPhase::InNightShift)
    {
        ApplyRunUpgradesToNightShiftActors();
    }

    OutMessage = FString::Printf(TEXT("Applied upgrade %s. New stack: %d"), *UpgradeTag.ToString(), NewStackCount);
    return true;
}

void AEMRNightShiftGameMode::BeginNightShiftFinish()
{
    if (!HasAuthority())
    {
        return;
    }

    if (bNightShiftFinishPending)
    {
        return;
    }

    bNightShiftFinishPending = true;

    if (AEMRHubertDirector* ResolvedHubertDirector = HubertDirector.Get())
    {
        ResolvedHubertDirector->StopHubertRuntime();
    }

    ClearSpecialEventRuntimeState();
    UnregisterTelemetryListeners();

    if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = GetWorld()->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
    {
        SpawnSubsystem->StopNightShiftSpawning();
    }
    if (UEMRPatientPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UEMRPatientPoolSubsystem>())
    {
        PoolSubsystem->InitializePool(0);
    }

    ScheduleOvertimeStopPatientDepartures();

    if (NightShiftEndDelaySeconds <= 0.f)
    {
        CompleteNightShiftFinish();
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            NightShiftEndDelayHandle,
            this,
            &ThisClass::CompleteNightShiftFinish,
            NightShiftEndDelaySeconds,
            false);
    }
}

void AEMRNightShiftGameMode::CompleteNightShiftFinish()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(OvertimeStopDepartureTimerHandle);
    }
    OvertimeStopDepartureQueue.Reset();
    OvertimeStopDepartureIndex = 0;

    bNightShiftFinishPending = false;
    HandleNightShiftFinished();
}


void AEMRNightShiftGameMode::HandleNightShiftFinished()
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameMode] HandleNightShiftFinished"));

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] NightShiftGameMode.HandleNightShiftFinished aborted: RunGS null"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] NightShiftGameMode.HandleNightShiftFinished begin Phase=%d Cycle=%d NightShiftIndex=%d Revenue=%.2f Quota=%.2f PendingTravel=%s"),
        static_cast<int32>(RunGS->GetRunPhase()),
        RunGS->GetCurrentCycleIndex(),
        RunGS->GetNightShiftIndexInCycle(),
        RunGS->GetTotalRevenue(),
        RunGS->GetCurrentCycleQuota(),
        RunGS->GetPendingTravelURL().IsEmpty() ? TEXT("<none>") : *RunGS->GetPendingTravelURL());

    UEMRRunRulesSubsystem* RunRules = GetRunRulesSubsystem();

    bool bSkippedRemainingNightShiftsInCycle = false;
    if (RunRules)
    {
        const int32 CompletedNightShiftIndex = RunGS->GetNightShiftIndexInCycle() + 1;
        bSkippedRemainingNightShiftsInCycle =
            RunGS->GetTotalRevenue() >= RunGS->GetCurrentCycleQuota()
            && CompletedNightShiftIndex < RunRules->GetNightShiftsPerCycle();
    }

    const FEMRNightShiftTelemetryRecord TelemetryRecord = BuildNightShiftTelemetryRecord(bSkippedRemainingNightShiftsInCycle);
    BroadcastNightShiftTelemetryToClients(TelemetryRecord);

    RunGS->SetNightShiftOvertimeActive(false);

    RunGS->ClearPendingTravelURL();

    if (RunRules)
    {
        RunRules->HandleNightShiftCompletion(RunGS);
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] NightShiftGameMode.HandleNightShiftFinished after RunRules Phase=%d RunFailed=%d FinalUnlocked=%d"),
            static_cast<int32>(RunGS->GetRunPhase()),
            RunGS->HasRunFailed() ? 1 : 0,
            RunGS->IsFinalMissionUnlocked() ? 1 : 0);
    }

    BuildAndPublishNightShiftSummary();

    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        const bool bNightShiftLose = bNightShiftEndedByReputation || RunGS->IsNightShiftFailedByReputation();
        ProgressionSubsystem->QueueHubertOutcomeAnnouncement(
            bNightShiftLose ? EEMRHubertOutcomeAnnouncement::NightShiftLose : EEMRHubertOutcomeAnnouncement::NightShiftWin);

        const bool bCycleQuotaReached = RunGS->GetTotalRevenue() >= RunGS->GetCurrentCycleQuota();
        bool bCycleOutcomeResolved = bCycleQuotaReached;
        if (!bCycleOutcomeResolved && RunRules)
        {
            bCycleOutcomeResolved = RunGS->GetNightShiftIndexInCycle() >= RunRules->GetNightShiftsPerCycle();
        }

        if (bCycleOutcomeResolved)
        {
            const bool bCycleWin = RunGS->GetRunPhase() != EER_RunPhase::RunFinished;
            ProgressionSubsystem->QueueHubertOutcomeAnnouncement(
                bCycleWin ? EEMRHubertOutcomeAnnouncement::CycleWin : EEMRHubertOutcomeAnnouncement::CycleLose);
        }
    }

    SyncRunStateToSubsystem();

    const bool bTravelTriggered = TravelToHubLevel();
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameMode] Travel to Hub triggered: %s"), bTravelTriggered ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] NightShiftGameMode.HandleNightShiftFinished end TravelTriggered=%d PendingTravel=%s"),
        bTravelTriggered ? 1 : 0,
        RunGS->GetPendingTravelURL().IsEmpty() ? TEXT("<none>") : *RunGS->GetPendingTravelURL());
}


void AEMRNightShiftGameMode::RegisterReputationListener()
{
    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return;
    }

    UAbilitySystemComponent* TeamASC = RunGS->GetAbilitySystemComponent();
    if (!TeamASC)
    {
        return;
    }

    if (TeamReputationChangedHandle.IsValid())
    {
        TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetReputationAttribute()).Remove(TeamReputationChangedHandle);
    }

    if (TeamRevenueChangedHandle.IsValid())
    {
        TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetTotalRevenueAttribute()).Remove(TeamRevenueChangedHandle);
    }

    TeamReputationChangedHandle = TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetReputationAttribute()).AddUObject(this, &ThisClass::HandleTeamReputationChanged);
    TeamRevenueChangedHandle = TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetTotalRevenueAttribute()).AddUObject(this, &ThisClass::HandleTelemetryRevenueChanged);
}


void AEMRNightShiftGameMode::HandleTeamReputationChanged(const FOnAttributeChangeData& Data)
{
    HandleTelemetryReputationChanged(Data);

    if (!HasAuthority() || bNightShiftEndedByReputation)
    {
        return;
    }

    if (const AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        if (RunGS->IsReputationFrozen() && !RunGS->IsNightShiftFailedByReputation())
        {
            return;
        }

        if (RunGS->GetRunPhase() != EER_RunPhase::InNightShift)
        {
            return;
        }
    }

    if (Data.NewValue > KINDA_SMALL_NUMBER)
    {
        return;
    }

    HandleNightShiftFailureByReputation();
}


void AEMRNightShiftGameMode::HandleNightShiftFailureByReputation()
{
    if (!HasAuthority())
    {
        return;
    }

    if (bNightShiftEndedByReputation)
    {
        return;
    }

    bNightShiftEndedByReputation = true;

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return;
    }

    RunGS->SetNightShiftFailedByReputation(true);
    RunGS->SetReputationLockedAtZero(true);

    PlayWatchReputationFailureAlertForAllPlayers();

    if (UAbilitySystemComponent* TeamASC = RunGS->GetAbilitySystemComponent())
    {
        TeamASC->SetNumericAttributeBase(UEMRTeamAttributeSet::GetReputationAttribute(), 0.f);
    }

    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        ProgressionSubsystem->RestoreNightShiftStartEconomy(RunGS);
        ProgressionSubsystem->LogAuthoritativeState(TEXT("NightShift.Fail.Reputation"), RunGS);
    }
    else
    {
        // Fallback: reset to current values (already consistent)
        RunGS->SetTotalRevenue(RunGS->GetTotalRevenue());
    }

    SyncRunStateToSubsystem();

    BeginNightShiftFinish();
}

void AEMRNightShiftGameMode::PlayWatchReputationFailureAlertForAllPlayers() const
{
    if (!HasAuthority())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AEMRPlayerCharacter> It(World); It; ++It)
    {
        if (AEMRPlayerCharacter* PlayerCharacter = *It)
        {
            PlayerCharacter->Client_PlayWatchReputationFailureSound();
        }
    }
}

void AEMRNightShiftGameMode::BuildAndPublishNightShiftSummary()
{
    if (!HasAuthority())
    {
        return;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return;
    }

    if (RunGS->GetRunPhase() == EER_RunPhase::RunFinished)
    {
        return;
    }

    FEMRNightShiftSummary Summary = RunGS->GetLastNightShiftSummary();

    const float NightShiftStartRevenue =
    (GetRunProgressionSubsystem() && GetRunProgressionSubsystem()->HasCachedState())
    ? GetRunProgressionSubsystem()->GetNightShiftStartTotalRevenue()
    : RunGS->GetCurrentCycleStartRevenue();

    Summary.NightShiftRevenue = FMath::Max(0.f, RunGS->GetTotalRevenue() - NightShiftStartRevenue);
    Summary.RevenueToCycleQuota = FMath::Max(0.f, RunGS->GetCurrentCycleQuota() - RunGS->GetTotalRevenue());
    Summary.bIsValid = true;

    RunGS->SetLastNightShiftSummary(Summary);
}

void AEMRNightShiftGameMode::HandleTelemetryPatientSpawned(AEMRPatient* Patient)
{
    if (!HasAuthority() || !bTelemetryAccumulatorActive || !Patient)
    {
        return;
    }

    AddCountForSplit(
        IsTelemetrySplitInOvertime(),
        NightShiftTelemetryAccumulator.PatientsSpawnedBeforeOvertime,
        NightShiftTelemetryAccumulator.PatientsSpawnedAfterOvertime);

    const TWeakObjectPtr<AEMRPatient> PatientKey(Patient);
    const double SpawnTimestamp = GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;
    NightShiftTelemetryAccumulator.SpawnTimestampByPatient.FindOrAdd(PatientKey) = SpawnTimestamp;
}

void AEMRNightShiftGameMode::HandleTelemetryExamCompleted(const FGameplayTag ExamTag)
{
    if (!HasAuthority() || !bTelemetryAccumulatorActive || !ExamTag.IsValid())
    {
        return;
    }

    NightShiftTelemetryAccumulator.ExamCounts.FindOrAdd(ExamTag) += 1;
}

void AEMRNightShiftGameMode::HandleTelemetryTreatmentResolved(AEMRPatient* Patient, const FGameplayTag TreatmentTag, const bool bPatientFullyCured)
{
    if (!HasAuthority() || !bTelemetryAccumulatorActive)
    {
        return;
    }

    if (TreatmentTag.IsValid())
    {
        NightShiftTelemetryAccumulator.TreatmentCounts.FindOrAdd(TreatmentTag) += 1;
    }

    if (!Patient || !bPatientFullyCured)
    {
        return;
    }

    AddCountForSplit(
        IsTelemetrySplitInOvertime(),
        NightShiftTelemetryAccumulator.PatientsFullyCuredBeforeOvertime,
        NightShiftTelemetryAccumulator.PatientsFullyCuredAfterOvertime);

    const TWeakObjectPtr<AEMRPatient> PatientKey(Patient);
    if (const double* SpawnTimestamp = NightShiftTelemetryAccumulator.SpawnTimestampByPatient.Find(PatientKey))
    {
        const double CureTimestamp = GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : *SpawnTimestamp;
        if (CureTimestamp >= *SpawnTimestamp)
        {
            NightShiftTelemetryAccumulator.TotalSpawnToCureSeconds += (CureTimestamp - *SpawnTimestamp);
            ++NightShiftTelemetryAccumulator.SpawnToCureSampleCount;
        }

        NightShiftTelemetryAccumulator.SpawnTimestampByPatient.Remove(PatientKey);
    }
}

void AEMRNightShiftGameMode::HandleTelemetryRevenueChanged(const FOnAttributeChangeData& Data)
{
    if (!HasAuthority() || !bTelemetryAccumulatorActive)
    {
        return;
    }

    const float RevenueDelta = FMath::Max(Data.NewValue - Data.OldValue, 0.0f);
    if (RevenueDelta <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    AddFloatForSplit(
        IsTelemetrySplitInOvertime(),
        RevenueDelta,
        NightShiftTelemetryAccumulator.RevenueBeforeOvertime,
        NightShiftTelemetryAccumulator.RevenueAfterOvertime);
}

void AEMRNightShiftGameMode::HandleTelemetryReputationChanged(const FOnAttributeChangeData& Data)
{
    if (!HasAuthority() || !bTelemetryAccumulatorActive)
    {
        return;
    }

    const float ReputationLoss = FMath::Max(Data.OldValue - Data.NewValue, 0.0f);
    if (ReputationLoss <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    AddFloatForSplit(
        IsTelemetrySplitInOvertime(),
        ReputationLoss,
        NightShiftTelemetryAccumulator.ReputationLostBeforeOvertime,
        NightShiftTelemetryAccumulator.ReputationLostAfterOvertime);
}

FEMRNightShiftTelemetryRecord AEMRNightShiftGameMode::BuildNightShiftTelemetryRecord(const bool bSkippedRemainingNightShiftsInCycle) const
{
    FEMRNightShiftTelemetryRecord Record;
    Record.RecordId = FGuid::NewGuid();
    Record.UtcTimestampIso8601 = FDateTime::UtcNow().ToIso8601();

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return Record;
    }

    Record.CycleNumber = FMath::Max(0, RunGS->GetCurrentCycleIndex() + 1);
    Record.NightShiftNumberInCycle = FMath::Max(0, RunGS->GetNightShiftIndexInCycle() + 1);
    Record.bSkippedRemainingNightShiftsInCycle = bSkippedRemainingNightShiftsInCycle;
    Record.CycleQuota = RunGS->GetCurrentCycleQuota();

    if (const UEMRNightShiftDefinition* CurrentDefinition = RunGS->GetCurrentNightShiftDefinition())
    {
        Record.DifficultyTier = CurrentDefinition->DifficultyTier;
    }

    const float OvertimeSeconds = FMath::Max(RunGS->GetNightShiftOvertimeElapsedSeconds(), 0.0f);
    Record.OvertimeSeconds = OvertimeSeconds;

    const UEMRRunRulesSubsystem* RunRulesSub = GetRunRulesSubsystem();
    const float BaseNightShiftDuration = RunRulesSub ? FMath::Max(RunRulesSub->GetNightShiftDurationSeconds(), 0.0f) : 0.0f;
    if (OvertimeSeconds > KINDA_SMALL_NUMBER)
    {
        Record.TotalNightShiftSeconds = BaseNightShiftDuration + OvertimeSeconds;
    }
    else
    {
        Record.TotalNightShiftSeconds = FMath::Max(BaseNightShiftDuration - RunGS->GetRemainingTimeInNightShift(), 0.0f);
    }

    Record.RevenueCollectedBeforeOvertime = FMath::Max(NightShiftTelemetryAccumulator.RevenueBeforeOvertime, 0.0f);
    Record.RevenueCollectedAfterOvertime = FMath::Max(NightShiftTelemetryAccumulator.RevenueAfterOvertime, 0.0f);
    Record.RevenueCollectedTotal = Record.RevenueCollectedBeforeOvertime + Record.RevenueCollectedAfterOvertime;

    Record.ReputationLostBeforeOvertime = FMath::Max(NightShiftTelemetryAccumulator.ReputationLostBeforeOvertime, 0.0f);
    Record.ReputationLostAfterOvertime = FMath::Max(NightShiftTelemetryAccumulator.ReputationLostAfterOvertime, 0.0f);
    Record.ReputationLostTotal = Record.ReputationLostBeforeOvertime + Record.ReputationLostAfterOvertime;

    Record.PatientsSpawnedBeforeOvertime = NightShiftTelemetryAccumulator.PatientsSpawnedBeforeOvertime;
    Record.PatientsSpawnedAfterOvertime = NightShiftTelemetryAccumulator.PatientsSpawnedAfterOvertime;
    Record.PatientsSpawnedTotal = Record.PatientsSpawnedBeforeOvertime + Record.PatientsSpawnedAfterOvertime;

    Record.PatientsFullyCuredBeforeOvertime = NightShiftTelemetryAccumulator.PatientsFullyCuredBeforeOvertime;
    Record.PatientsFullyCuredAfterOvertime = NightShiftTelemetryAccumulator.PatientsFullyCuredAfterOvertime;
    Record.PatientsFullyCuredTotal = Record.PatientsFullyCuredBeforeOvertime + Record.PatientsFullyCuredAfterOvertime;

    Record.PatientsLeftByPatienceBeforeOvertime = NightShiftTelemetryAccumulator.PatientsLeftByPatienceBeforeOvertime;
    Record.PatientsLeftByPatienceAfterOvertime = NightShiftTelemetryAccumulator.PatientsLeftByPatienceAfterOvertime;
    Record.PatientsLeftByPatienceTotal = Record.PatientsLeftByPatienceBeforeOvertime + Record.PatientsLeftByPatienceAfterOvertime;

    if (NightShiftTelemetryAccumulator.SpawnToCureSampleCount > 0)
    {
        Record.AverageSpawnToCureSeconds = static_cast<float>(
            NightShiftTelemetryAccumulator.TotalSpawnToCureSeconds / static_cast<double>(NightShiftTelemetryAccumulator.SpawnToCureSampleCount));
    }
    else
    {
        Record.AverageSpawnToCureSeconds = 0.0f;
    }

    Record.ExamCounts = BuildSortedTagCounts(NightShiftTelemetryAccumulator.ExamCounts);
    Record.TreatmentCounts = BuildSortedTagCounts(NightShiftTelemetryAccumulator.TreatmentCounts);

    return Record;
}

void AEMRNightShiftGameMode::BroadcastNightShiftTelemetryToClients(const FEMRNightShiftTelemetryRecord& Record) const
{
    if (!HasAuthority())
    {
        return;
    }

    if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        RunGS->SetLastNightShiftTelemetryRecord(Record);
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (AEMRPlayerController* EMRPlayerController = Cast<AEMRPlayerController>(It->Get()))
        {
            EMRPlayerController->Client_ReceiveNightShiftTelemetry(Record);
        }
    }
}

bool AEMRNightShiftGameMode::SendNightShiftTelemetrySnapshotToController(AEMRPlayerController* TargetController) const
{
    if (!HasAuthority() || !TargetController)
    {
        return false;
    }

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS || RunGS->GetRunPhase() != EER_RunPhase::InNightShift || !bTelemetryAccumulatorActive)
    {
        return false;
    }

    const FEMRNightShiftTelemetryRecord TelemetryRecord = BuildNightShiftTelemetryRecord(false);
    TargetController->Client_ReceiveNightShiftTelemetry(TelemetryRecord);
    return true;
}

void AEMRNightShiftGameMode::ResetTelemetryAccumulator()
{
    NightShiftTelemetryAccumulator.Reset();
    bTelemetryAccumulatorActive = false;
}

void AEMRNightShiftGameMode::RegisterTelemetryListeners()
{
    UnregisterTelemetryListeners();

    UWorld* World = GetWorld();
    UGameInstance* GameInstance = GetGameInstance();
    if (!World || !GameInstance)
    {
        return;
    }

    if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
    {
        SpawnedPatientDelegateHandle = SpawnSubsystem->OnPatientSpawnedNative().AddUObject(this, &ThisClass::HandleTelemetryPatientSpawned);
    }

    if (UEMRExamQueueSubsystem* ExamQueueSubsystem = GameInstance->GetSubsystem<UEMRExamQueueSubsystem>())
    {
        ExamCompletedDelegateHandle = ExamQueueSubsystem->OnExamCompletedNative().AddUObject(this, &ThisClass::HandleTelemetryExamCompleted);
    }

    if (UEMRTreatmentSubsystem* TreatmentSubsystem = GameInstance->GetSubsystem<UEMRTreatmentSubsystem>())
    {
        TreatmentResolvedDelegateHandle = TreatmentSubsystem->OnTreatmentResolvedNative().AddUObject(this, &ThisClass::HandleTelemetryTreatmentResolved);
    }

    bTelemetryAccumulatorActive = true;
}

void AEMRNightShiftGameMode::UnregisterTelemetryListeners()
{
    if (UWorld* World = GetWorld())
    {
        if (SpawnedPatientDelegateHandle.IsValid())
        {
            if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
            {
                SpawnSubsystem->OnPatientSpawnedNative().Remove(SpawnedPatientDelegateHandle);
            }
            SpawnedPatientDelegateHandle.Reset();
        }
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (ExamCompletedDelegateHandle.IsValid())
        {
            if (UEMRExamQueueSubsystem* ExamQueueSubsystem = GameInstance->GetSubsystem<UEMRExamQueueSubsystem>())
            {
                ExamQueueSubsystem->OnExamCompletedNative().Remove(ExamCompletedDelegateHandle);
            }
            ExamCompletedDelegateHandle.Reset();
        }

        if (TreatmentResolvedDelegateHandle.IsValid())
        {
            if (UEMRTreatmentSubsystem* TreatmentSubsystem = GameInstance->GetSubsystem<UEMRTreatmentSubsystem>())
            {
                TreatmentSubsystem->OnTreatmentResolvedNative().Remove(TreatmentResolvedDelegateHandle);
            }
            TreatmentResolvedDelegateHandle.Reset();
        }
    }

    bTelemetryAccumulatorActive = false;
}

bool AEMRNightShiftGameMode::IsTelemetrySplitInOvertime() const
{
    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    return RunGS && RunGS->IsInNightShiftOvertime();
}

void AEMRNightShiftGameMode::HandleRestoredRunState()
{
    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[NightShiftFlow][NightShiftGameMode] HandleRestoredRunState Phase=%d CurrentDefinition=%s PendingTravel=%s"),
        static_cast<int32>(RunGS->GetRunPhase()),
        *GetNameSafe(RunGS->GetCurrentNightShiftDefinition()),
        RunGS->GetPendingTravelURL().IsEmpty() ? TEXT("<none>") : *RunGS->GetPendingTravelURL());

    switch (RunGS->GetRunPhase())
    {
    case EER_RunPhase::InNightShift:
    {
        UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameMode] Resuming active NightShift"));
        if (UEMRNightShiftDefinition* ActiveDefinition = RunGS->GetCurrentNightShiftDefinition())
        {
            if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
            {
                    ProgressionSubsystem->StoreNightShiftStartSnapshot(RunGS);
            }

            RegisterReputationListener();
            StartNightShiftRuntimeSystems(ActiveDefinition);
        }
        else
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[NightShiftFlow][NightShiftGameMode] Missing NightShiftDefinition while resuming InNightShift (RemainingTime=%.2f, Overtime=%s, PendingTravel=%s)"),
                RunGS->GetRemainingTimeInNightShift(),
                RunGS->IsInNightShiftOvertime() ? TEXT("true") : TEXT("false"),
                RunGS->GetPendingTravelURL().IsEmpty() ? TEXT("<none>") : *RunGS->GetPendingTravelURL());
        }
        break;
    }

    default:
        UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameMode] Resuming phase %d"), static_cast<int32>(RunGS->GetRunPhase()));
        break;
    }
}

void AEMRNightShiftGameMode::StartNightShiftRuntimeSystems(UEMRNightShiftDefinition* NightShiftDefinition)
{
    if (!NightShiftDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][NightShiftGameMode] StartNightShiftRuntimeSystems called with null definition."));
        return;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[NightShiftFlow][NightShiftGameMode] StartNightShiftRuntimeSystems Definition=%s Difficulty=%s Hospital=%s"),
        *NightShiftDefinition->GetName(),
        *StaticEnum<EEMRNightShiftDifficultyTier>()->GetValueAsString(NightShiftDefinition->DifficultyTier),
        *NightShiftDefinition->HospitalLevel.ToSoftObjectPath().ToString());

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][NightShiftGameMode] StartNightShiftRuntimeSystems aborted: World is null."));
        return;
    }

    if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        const bool bShouldResetEndState = RunGS->GetRunPhase() == EER_RunPhase::InNightShift
            && !RunGS->IsInNightShiftOvertime()
            && RunGS->GetRemainingTimeInNightShift() > 0.f;
        if (bShouldResetEndState)
        {
            RunGS->ResetNightShiftEndState();
        }
    }

    ResetTelemetryAccumulator();
    RegisterTelemetryListeners();

    if (UEMRPatientPoolSubsystem* PatientPoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
    {
        TArray<TSubclassOf<AEMRPatient>> PatientClasses;

        if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
        {
            PatientClasses = RunGS->GetPatientBlueprintClasses();
        }

        if (PatientClasses.Num() == 0 && DefaultPatientBlueprintClass)
        {
            PatientClasses.Add(DefaultPatientBlueprintClass);
            UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameMode] Using DefaultPatientBlueprintClass fallback: %s"), *GetNameSafe(DefaultPatientBlueprintClass));
        }
        else if (PatientClasses.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][NightShiftGameMode] No configured patient classes in GameState and no DefaultPatientBlueprintClass fallback."));
        }

        TArray<FString> PatientClassNames;
        PatientClassNames.Reserve(PatientClasses.Num());
        for (const TSubclassOf<AEMRPatient>& PatientClass : PatientClasses)
        {
            PatientClassNames.Add(GetNameSafe(PatientClass.Get()));
        }

        UE_LOG(
            LogTemp,
            Log,
            TEXT("[NightShiftFlow][NightShiftGameMode] Configuring PatientPool classes=%d [%s]"),
            PatientClasses.Num(),
            PatientClassNames.Num() > 0 ? *FString::Join(PatientClassNames, TEXT(", ")) : TEXT("<none>"));

        PatientPoolSubsystem->SetPatientClasses(PatientClasses);
        PatientPoolSubsystem->PrewarmAutomaticSpawnResources();
        PatientPoolSubsystem->PrewarmForNightShift(NightShiftDefinition);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][NightShiftGameMode] PatientPoolSubsystem missing."));
    }

    if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
    {
        if (!SpawnSubsystem->StartNightShiftSpawning(NightShiftDefinition))
        {
            UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][NightShiftGameMode] Failed to start spawn subsystem for %s"), *NightShiftDefinition->GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][NightShiftGameMode] SpawnSubsystem missing."));
    }

    ScheduleSpecialEvent(NightShiftDefinition);

    if (AEMRHubertDirector* ResolvedHubertDirector = ResolveHubertDirector())
    {
        ResolvedHubertDirector->StartNightShiftRuntime();
    }
}

void AEMRNightShiftGameMode::ApplyRunUpgradesToNightShiftActors()
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s NightShiftGameMode ApplyRunUpgradesToNightShiftActors ignored: no authority."), NightShiftGameModeUpgradesFlowLogPrefix);
        return;
    }

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s NightShiftGameMode ApplyRunUpgradesToNightShiftActors aborted: RunGS missing."), NightShiftGameModeUpgradesFlowLogPrefix);
        return;
    }

    const int32 CoffeeUpgradeStackCount = RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::CoffeeMachine);
    const UEMRDifficultyTuningData* DifficultyTuning = RunGS->GetDifficultyTuningData();
    const FEMRCoffeeUpgradeTuning* CoffeeUpgradeTuning = DifficultyTuning
    ? &DifficultyTuning->GetCoffeeUpgradeTuning()
    : nullptr;
    int32 CoffeeMachineCount = 0;
    int32 EnabledCoffeeMachineCount = 0;
    for (TActorIterator<AEMRCoffeeMachineActor> It(GetWorld()); It; ++It)
    {
        AEMRCoffeeMachineActor* CoffeeMachine = *It;
        if (!CoffeeMachine)
        {
            continue;
        }

        ++CoffeeMachineCount;
        if (CoffeeUpgradeTuning)
        {
            CoffeeMachine->ApplyUpgradeTuning(*CoffeeUpgradeTuning);
        }
        const bool bShouldEnableMachine = CoffeeUpgradeStackCount >= CoffeeMachine->GetRequiredUpgradeStackCount();
        CoffeeMachine->SetCoffeeMachineEnabledByUpgrade(bShouldEnableMachine);
        if (bShouldEnableMachine)
        {
            ++EnabledCoffeeMachineCount;
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s NightShiftGameMode ApplyRunUpgradesToNightShiftActors coffeeUpgradeStacks=%d machineCount=%d enabledMachineCount=%d"),
        NightShiftGameModeUpgradesFlowLogPrefix,
        CoffeeUpgradeStackCount,
        CoffeeMachineCount,
        EnabledCoffeeMachineCount);

    const int32 SnackMachineUpgradeStackCount = RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::SnackMachine);
    int32 SnackMachineCount = 0;
    int32 EnabledSnackMachineCount = 0;
    for (TActorIterator<AEMRSnackMachineActor> It(GetWorld()); It; ++It)
    {
        AEMRSnackMachineActor* SnackMachine = *It;
        if (!SnackMachine)
        {
            continue;
        }

        ++SnackMachineCount;
        const FGameplayTag RequiredUpgradeTag = SnackMachine->GetRequiredUpgradeTag();
        const int32 RequiredStackCount = SnackMachine->GetRequiredUpgradeStackCount();
        const bool bShouldEnableMachine = !RequiredUpgradeTag.IsValid()
            || RunGS->GetActiveRunUpgradeStackCount(RequiredUpgradeTag) >= RequiredStackCount;
        SnackMachine->SetSnackMachineEnabledByUpgrade(bShouldEnableMachine);
        if (bShouldEnableMachine)
        {
            ++EnabledSnackMachineCount;
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s NightShiftGameMode ApplyRunUpgradesToNightShiftActors snackUpgradeStacks=%d machineCount=%d enabledMachineCount=%d"),
        NightShiftGameModeUpgradesFlowLogPrefix,
        SnackMachineUpgradeStackCount,
        SnackMachineCount,
        EnabledSnackMachineCount);

    const int32 OxygenMachineUpgradeStackCount = RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::OxygenMachine);
    int32 OxygenMachineCount = 0;
    int32 EnabledOxygenMachineCount = 0;
    for (TActorIterator<AEMROxygenMachine> It(GetWorld()); It; ++It)
    {
        AEMROxygenMachine* OxygenMachine = *It;
        if (!OxygenMachine)
        {
            continue;
        }

        ++OxygenMachineCount;
        const FGameplayTag RequiredUpgradeTag = OxygenMachine->GetRequiredUpgradeTag();
        const int32 RequiredStackCount = OxygenMachine->GetRequiredUpgradeStackCount();
        const bool bShouldEnableMachine = !RequiredUpgradeTag.IsValid()
            || RunGS->GetActiveRunUpgradeStackCount(RequiredUpgradeTag) >= RequiredStackCount;
        OxygenMachine->SetOxygenMachineEnabledByUpgrade(bShouldEnableMachine);
        if (bShouldEnableMachine)
        {
            ++EnabledOxygenMachineCount;
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s NightShiftGameMode ApplyRunUpgradesToNightShiftActors oxygenUpgradeStacks=%d machineCount=%d enabledMachineCount=%d"),
        NightShiftGameModeUpgradesFlowLogPrefix,
        OxygenMachineUpgradeStackCount,
        OxygenMachineCount,
        EnabledOxygenMachineCount);

    const int32 MagicBoxUpgradeStackCount = RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::MagicBox);
    int32 MagicBoxCount = 0;
    int32 EnabledMagicBoxCount = 0;
    for (TActorIterator<AEMRMagicBoxActor> It(GetWorld()); It; ++It)
    {
        AEMRMagicBoxActor* MagicBox = *It;
        if (!MagicBox)
        {
            continue;
        }

        ++MagicBoxCount;
        const FGameplayTag RequiredUpgradeTag = MagicBox->GetRequiredUpgradeTag();
        const int32 RequiredStackCount = MagicBox->GetRequiredUpgradeStackCount();
        const bool bShouldEnableMagicBox = !RequiredUpgradeTag.IsValid()
            || RunGS->GetActiveRunUpgradeStackCount(RequiredUpgradeTag) >= RequiredStackCount;
        MagicBox->SetMagicBoxEnabledByUpgrade(bShouldEnableMagicBox);
        if (bShouldEnableMagicBox)
        {
            ++EnabledMagicBoxCount;
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s NightShiftGameMode ApplyRunUpgradesToNightShiftActors magicBoxUpgradeStacks=%d boxCount=%d enabledBoxCount=%d"),
        NightShiftGameModeUpgradesFlowLogPrefix,
        MagicBoxUpgradeStackCount,
        MagicBoxCount,
        EnabledMagicBoxCount);

    const int32 TreatmentBedUpgradeStackCount = RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::TreatmentBed);
    int32 TreatmentBedCount = 0;
    int32 EnabledTreatmentBedCount = 0;
    for (TActorIterator<AEMRTreatmentBed> It(GetWorld()); It; ++It)
    {
        AEMRTreatmentBed* Bed = *It;
        if (!Bed)
        {
            continue;
        }

        ++TreatmentBedCount;
        const FGameplayTag RequiredUpgradeTag = Bed->GetRequiredUpgradeTag();
        const int32 RequiredStackCount = Bed->GetRequiredUpgradeStackCount();
        const bool bShouldEnableBed = !RequiredUpgradeTag.IsValid()
            || RunGS->GetActiveRunUpgradeStackCount(RequiredUpgradeTag) >= RequiredStackCount;
        Bed->SetTreatmentBedEnabledByUpgrade(bShouldEnableBed);
        if (bShouldEnableBed)
        {
            ++EnabledTreatmentBedCount;
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s NightShiftGameMode ApplyRunUpgradesToNightShiftActors treatmentBedUpgradeStacks=%d bedCount=%d enabledBedCount=%d"),
        NightShiftGameModeUpgradesFlowLogPrefix,
        TreatmentBedUpgradeStackCount,
        TreatmentBedCount,
        EnabledTreatmentBedCount);
}

void AEMRNightShiftGameMode::ValidateCoffeeMachinePlacementForActiveUpgrade() const
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s NightShiftGameMode ValidateCoffeeMachinePlacement ignored: no authority."), NightShiftGameModeUpgradesFlowLogPrefix);
        return;
    }

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    const int32 CoffeeUpgradeStackCount = RunGS ? RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::CoffeeMachine) : 0;
    if (!RunGS || CoffeeUpgradeStackCount <= 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s NightShiftGameMode ValidateCoffeeMachinePlacement skipped RunGS=%s coffeeUpgradeStacks=%d"),
            NightShiftGameModeUpgradesFlowLogPrefix,
            *GetNameSafe(RunGS),
            CoffeeUpgradeStackCount);
        return;
    }

    int32 CoffeeMachineCount = 0;
    for (TActorIterator<AEMRCoffeeMachineActor> It(GetWorld()); It; ++It)
    {
        if (*It)
        {
            ++CoffeeMachineCount;
        }
    }

    if (CoffeeMachineCount == 0)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][Coffee] Content bug: Run has coffee upgrade active but this map contains no coffee machine placement. Map=%s"),
            *GetWorld()->GetMapName());
    }
    else if (CoffeeMachineCount < CoffeeUpgradeStackCount)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][Coffee] Content bug: Coffee upgrade stacks exceed machine placements. Stacks=%d Placements=%d Map=%s"),
            CoffeeUpgradeStackCount,
            CoffeeMachineCount,
            *GetWorld()->GetMapName());
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s NightShiftGameMode ValidateCoffeeMachinePlacement success machineCount=%d stacks=%d map=%s"),
            NightShiftGameModeUpgradesFlowLogPrefix,
            CoffeeMachineCount,
            CoffeeUpgradeStackCount,
            *GetWorld()->GetMapName());
    }
}

void AEMRNightShiftGameMode::ValidateOxygenMachinePlacementForActiveUpgrade() const
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s NightShiftGameMode ValidateOxygenMachinePlacement ignored: no authority."), NightShiftGameModeUpgradesFlowLogPrefix);
        return;
    }

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s NightShiftGameMode ValidateOxygenMachinePlacement skipped: RunGS missing."), NightShiftGameModeUpgradesFlowLogPrefix);
        return;
    }

    const int32 OxygenUpgradeStackCount = RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::OxygenMachine);
    int32 TotalMachineCount = 0;
    int32 BaselineMachineCount = 0;
    int32 UpgradeControlledMachineCount = 0;
    for (TActorIterator<AEMROxygenMachine> It(GetWorld()); It; ++It)
    {
        const AEMROxygenMachine* OxygenMachine = *It;
        if (!OxygenMachine)
        {
            continue;
        }

        ++TotalMachineCount;
        if (OxygenMachine->GetRequiredUpgradeTag() == EMRTags::RunUpgrade::OxygenMachine
            && OxygenMachine->GetRequiredUpgradeStackCount() > 0)
        {
            ++UpgradeControlledMachineCount;
        }
        else if (!OxygenMachine->GetRequiredUpgradeTag().IsValid() || OxygenMachine->GetRequiredUpgradeStackCount() <= 0)
        {
            ++BaselineMachineCount;
        }
    }

    if (TotalMachineCount == 0)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][OxygenMachine] Content bug: map contains no oxygen machine placement. Map=%s"),
            *GetWorld()->GetMapName());
        return;
    }

    if (BaselineMachineCount <= 0)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][OxygenMachine] Content bug: map has no baseline oxygen machine placement (non-upgrade-gated). Map=%s"),
            *GetWorld()->GetMapName());
        return;
    }

    if (UpgradeControlledMachineCount < OxygenUpgradeStackCount)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][OxygenMachine] Content bug: oxygen-machine upgrade stacks exceed stack-gated placements. Stacks=%d Placements=%d Map=%s"),
            OxygenUpgradeStackCount,
            UpgradeControlledMachineCount,
            *GetWorld()->GetMapName());
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s NightShiftGameMode ValidateOxygenMachinePlacement success totalCount=%d baselineCount=%d stackControlledCount=%d stacks=%d map=%s"),
        NightShiftGameModeUpgradesFlowLogPrefix,
        TotalMachineCount,
        BaselineMachineCount,
        UpgradeControlledMachineCount,
        OxygenUpgradeStackCount,
        *GetWorld()->GetMapName());
}

void AEMRNightShiftGameMode::ValidateSnackMachinePlacementForActiveUpgrade() const
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s NightShiftGameMode ValidateSnackMachinePlacement ignored: no authority."), NightShiftGameModeUpgradesFlowLogPrefix);
        return;
    }

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    const int32 SnackUpgradeStackCount = RunGS ? RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::SnackMachine) : 0;
    if (!RunGS || SnackUpgradeStackCount <= 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s NightShiftGameMode ValidateSnackMachinePlacement skipped RunGS=%s snackUpgradeStacks=%d"),
            NightShiftGameModeUpgradesFlowLogPrefix,
            *GetNameSafe(RunGS),
            SnackUpgradeStackCount);
        return;
    }

    int32 SnackMachineCount = 0;
    int32 UpgradeControlledMachineCount = 0;
    for (TActorIterator<AEMRSnackMachineActor> It(GetWorld()); It; ++It)
    {
        const AEMRSnackMachineActor* SnackMachine = *It;
        if (!SnackMachine)
        {
            continue;
        }

        ++SnackMachineCount;
        if (SnackMachine->GetRequiredUpgradeTag() == EMRTags::RunUpgrade::SnackMachine)
        {
            ++UpgradeControlledMachineCount;
        }
    }

    if (SnackMachineCount == 0)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][SnackMachine] Content bug: Run has snack-machine upgrade active but this map contains no snack-machine placement. Map=%s"),
            *GetWorld()->GetMapName());
    }
    else if (UpgradeControlledMachineCount > 0 && UpgradeControlledMachineCount < SnackUpgradeStackCount)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][SnackMachine] Content bug: Snack-machine upgrade stacks exceed stack-gated placements. Stacks=%d Placements=%d Map=%s"),
            SnackUpgradeStackCount,
            UpgradeControlledMachineCount,
            *GetWorld()->GetMapName());
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s NightShiftGameMode ValidateSnackMachinePlacement success machineCount=%d stackControlledCount=%d stacks=%d map=%s"),
            NightShiftGameModeUpgradesFlowLogPrefix,
            SnackMachineCount,
            UpgradeControlledMachineCount,
            SnackUpgradeStackCount,
            *GetWorld()->GetMapName());
    }
}

void AEMRNightShiftGameMode::ValidateMagicBoxPlacementForActiveUpgrade() const
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s NightShiftGameMode ValidateMagicBoxPlacement ignored: no authority."), NightShiftGameModeUpgradesFlowLogPrefix);
        return;
    }

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    const int32 MagicBoxUpgradeStackCount = RunGS ? RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::MagicBox) : 0;
    if (!RunGS || MagicBoxUpgradeStackCount <= 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s NightShiftGameMode ValidateMagicBoxPlacement skipped RunGS=%s magicBoxUpgradeStacks=%d"),
            NightShiftGameModeUpgradesFlowLogPrefix,
            *GetNameSafe(RunGS),
            MagicBoxUpgradeStackCount);
        return;
    }

    int32 MagicBoxCount = 0;
    int32 UpgradeControlledBoxCount = 0;
    for (TActorIterator<AEMRMagicBoxActor> It(GetWorld()); It; ++It)
    {
        const AEMRMagicBoxActor* MagicBox = *It;
        if (!MagicBox)
        {
            continue;
        }

        ++MagicBoxCount;
        if (MagicBox->GetRequiredUpgradeTag() == EMRTags::RunUpgrade::MagicBox)
        {
            ++UpgradeControlledBoxCount;
        }
    }

    if (MagicBoxCount == 0)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][MagicBox] Content bug: Run has magic-box upgrade active but this map contains no magic-box placement. Map=%s"),
            *GetWorld()->GetMapName());
    }
    else if (UpgradeControlledBoxCount > 0 && UpgradeControlledBoxCount < MagicBoxUpgradeStackCount)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][MagicBox] Content bug: Magic-box upgrade stacks exceed stack-gated placements. Stacks=%d Placements=%d Map=%s"),
            MagicBoxUpgradeStackCount,
            UpgradeControlledBoxCount,
            *GetWorld()->GetMapName());
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s NightShiftGameMode ValidateMagicBoxPlacement success boxCount=%d stackControlledCount=%d stacks=%d map=%s"),
            NightShiftGameModeUpgradesFlowLogPrefix,
            MagicBoxCount,
            UpgradeControlledBoxCount,
            MagicBoxUpgradeStackCount,
            *GetWorld()->GetMapName());
    }
}

void AEMRNightShiftGameMode::ValidateTreatmentBedPlacementForActiveUpgrade() const
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s NightShiftGameMode ValidateTreatmentBedPlacement ignored: no authority."), NightShiftGameModeUpgradesFlowLogPrefix);
        return;
    }

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    const int32 TreatmentBedUpgradeStackCount = RunGS ? RunGS->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::TreatmentBed) : 0;
    if (!RunGS || TreatmentBedUpgradeStackCount <= 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s NightShiftGameMode ValidateTreatmentBedPlacement skipped RunGS=%s treatmentBedUpgradeStacks=%d"),
            NightShiftGameModeUpgradesFlowLogPrefix,
            *GetNameSafe(RunGS),
            TreatmentBedUpgradeStackCount);
        return;
    }

    int32 UpgradeControlledBedCount = 0;
    for (TActorIterator<AEMRTreatmentBed> It(GetWorld()); It; ++It)
    {
        const AEMRTreatmentBed* Bed = *It;
        if (Bed && Bed->GetRequiredUpgradeTag() == EMRTags::RunUpgrade::TreatmentBed)
        {
            ++UpgradeControlledBedCount;
        }
    }

    if (UpgradeControlledBedCount == 0)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][TreatmentBed] Content bug: Run has treatment-bed upgrade active but this map has no upgrade-controlled treatment bed placements. Map=%s"),
            *GetWorld()->GetMapName());
    }
    else if (UpgradeControlledBedCount < TreatmentBedUpgradeStackCount)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Upgrade][TreatmentBed] Content bug: Treatment-bed upgrade stacks exceed bed placements. Stacks=%d Placements=%d Map=%s"),
            TreatmentBedUpgradeStackCount,
            UpgradeControlledBedCount,
            *GetWorld()->GetMapName());
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s NightShiftGameMode ValidateTreatmentBedPlacement success placements=%d stacks=%d map=%s"),
            NightShiftGameModeUpgradesFlowLogPrefix,
            UpgradeControlledBedCount,
            TreatmentBedUpgradeStackCount,
            *GetWorld()->GetMapName());
    }
}

bool AEMRNightShiftGameMode::IsSpecialEventDifficultyEligible(const EEMRNightShiftDifficultyTier DifficultyTier) const
{
    return DifficultyTier != EEMRNightShiftDifficultyTier::Calm
    && DifficultyTier != EEMRNightShiftDifficultyTier::Standard;
}

void AEMRNightShiftGameMode::ScheduleSpecialEvent(UEMRNightShiftDefinition* NightShiftDefinition)
{
    ClearSpecialEventRuntimeState();

    if (!HasAuthority() || !NightShiftDefinition)
    {
        return;
    }

    ActiveNightShiftForSpecialEvent = NightShiftDefinition;
    if (!NightShiftDefinition->bAllowSpecialEvents)
    {
        return;
    }

    if (!IsSpecialEventDifficultyEligible(NightShiftDefinition->DifficultyTier))
    {
        UE_LOG(LogTemp, Log, TEXT("[SpecialEvent] Skipping scheduling on ineligible difficulty %s"),
            *StaticEnum<EEMRNightShiftDifficultyTier>()->GetValueAsString(NightShiftDefinition->DifficultyTier));
        return;
    }

    const UEMRSpecialEventDefinition* SelectedSpecialEvent = SelectSpecialEventForNightShift(*NightShiftDefinition);
    if (!SelectedSpecialEvent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SpecialEvent] No eligible special event found for NightShift '%s'."), *NightShiftDefinition->GetName());
        return;
    }

    ActiveSpecialEventDefinition = SelectedSpecialEvent;
    const FEMRNightShiftSpecialEventDefinition& EventDefinition = SelectedSpecialEvent->GetEventDefinition();
    if (!EventDefinition.bEnabled || EventDefinition.EventId.IsNone())
    {
        return;
    }

    const UEMRDifficultyTuningData* DifficultyTuning = GetDifficultyTuning();
    const float SpawnRateScalar = DifficultyTuning
    ? DifficultyTuning->GetSpecialEventSpawnRateScalar(NightShiftDefinition->DifficultyTier)
    : 1.0f;

    float ShiftDuration = 600.f;
    if (UEMRRunRulesSubsystem* RulesSubsystem = GetRunRulesSubsystem())
    {
        ShiftDuration = FMath::Max(RulesSubsystem->GetNightShiftDurationSeconds(), 1.0f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[SpecialEvent] Missing RunRulesSubsystem; using default duration."));
    }
    UE_LOG(LogTemp, Log, TEXT("[SpecialEvent] Using NightShift duration %.2f for scheduling."), ShiftDuration);
    const float EffectiveDuration = FMath::Max(EventDefinition.ActiveDurationSeconds, 0.1f);
    const float EffectiveAlertLead = FMath::Max(EventDefinition.AlertLeadSeconds, 0.0f);
    const float MaxEventStartSeconds = FMath::Max(ShiftDuration - EffectiveDuration, 0.0f);

    float ClampedWindowMinSeconds = FMath::Clamp(EventDefinition.StartWindowMinSeconds, 0.0f, MaxEventStartSeconds);
    float ClampedWindowMaxSeconds = FMath::Clamp(EventDefinition.StartWindowMaxSeconds, 0.0f, MaxEventStartSeconds);
    if (ClampedWindowMaxSeconds < ClampedWindowMinSeconds)
    {
        Swap(ClampedWindowMinSeconds, ClampedWindowMaxSeconds);
    }

    SpecialEventRuntimeState = FEMRSpecialEventRuntimeState();
    SpecialEventRuntimeState.bScheduled = true;
    SpecialEventRuntimeState.EventId = EventDefinition.EventId;
    SpecialEventRuntimeState.EffectiveSpawnRateMultiplier = FMath::Max(EventDefinition.BaseSpawnRateMultiplier * SpawnRateScalar, 0.0f);
    SpecialEventRuntimeState.EventStartSeconds = FMath::FRandRange(ClampedWindowMinSeconds, ClampedWindowMaxSeconds);
    SpecialEventRuntimeState.AlertStartSeconds = FMath::Max(SpecialEventRuntimeState.EventStartSeconds - EffectiveAlertLead, 0.0f);
    SpecialEventRuntimeState.EventEndSeconds = FMath::Min(SpecialEventRuntimeState.EventStartSeconds + EffectiveDuration, ShiftDuration);

    if (SpecialEventRuntimeState.EventEndSeconds <= SpecialEventRuntimeState.EventStartSeconds)
    {
        SpecialEventRuntimeState.EventEndSeconds = SpecialEventRuntimeState.EventStartSeconds + 0.1f;
    }

    UE_LOG(LogTemp, Log, TEXT("[SpecialEvent] Scheduled Event=%s Alert=%.2fs Start=%.2fs End=%.2fs SpawnMult=%.2f"),
        *SpecialEventRuntimeState.EventId.ToString(),
        SpecialEventRuntimeState.AlertStartSeconds,
        SpecialEventRuntimeState.EventStartSeconds,
        SpecialEventRuntimeState.EventEndSeconds,
        SpecialEventRuntimeState.EffectiveSpawnRateMultiplier);

    if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        if (EventDefinition.FlickerLightTag.IsNone())
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[SpecialEvent] Event '%s' has no FlickerLightTag; light flicker presentation will be skipped."),
                *EventDefinition.EventId.ToString());
        }

        RunGS->SetSpecialEventPhase(EEMRSpecialEventPhase::None, NAME_None, 0.0f);
        RunGS->SetSpecialEventPresentationData(
            EventDefinition.AlertTitle,
            EventDefinition.AlertDescription,
            EventDefinition.FlickerLightTag,
            EventDefinition.FlickerColor,
            EventDefinition.BaseLightFlickerRateHz);
    }

    FTimerManager& TimerManager = GetWorldTimerManager();
    if (SpecialEventRuntimeState.AlertStartSeconds <= 0.0f)
    {
        HandleSpecialEventAlertStarted();
    }
    else
    {
        TimerManager.SetTimer(
            SpecialEventRuntimeState.AlertTimerHandle,
            this,
            &ThisClass::HandleSpecialEventAlertStarted,
            SpecialEventRuntimeState.AlertStartSeconds,
            false);
    }

    TimerManager.SetTimer(
        SpecialEventRuntimeState.StartTimerHandle,
        this,
        &ThisClass::HandleSpecialEventStarted,
        SpecialEventRuntimeState.EventStartSeconds,
        false);

    TimerManager.SetTimer(
        SpecialEventRuntimeState.EndTimerHandle,
        this,
        &ThisClass::HandleSpecialEventEnded,
        SpecialEventRuntimeState.EventEndSeconds,
        false);
}

void AEMRNightShiftGameMode::CollectConfiguredSpecialEvents(TArray<const UEMRSpecialEventDefinition*>& OutSpecialEvents) const
{
    TArray<const UEMRSubsystemConfig*> Configs;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(Configs))
    {
        return;
    }

    for (const UEMRSubsystemConfig* Config : Configs)
    {
        if (!Config)
        {
            continue;
        }

        Config->GetSpecialEventDefinitions(OutSpecialEvents);
    }
}

const UEMRSpecialEventDefinition* AEMRNightShiftGameMode::SelectSpecialEventForNightShift(const UEMRNightShiftDefinition& NightShiftDefinition) const
{
    TArray<const UEMRSpecialEventDefinition*> CandidateEvents;
    CollectConfiguredSpecialEvents(CandidateEvents);

    if (CandidateEvents.IsEmpty())
    {
        return nullptr;
    }

    TArray<const UEMRSpecialEventDefinition*> EligibleEvents;
    for (const UEMRSpecialEventDefinition* CandidateEvent : CandidateEvents)
    {
        if (!CandidateEvent)
        {
            continue;
        }

        const FEMRNightShiftSpecialEventDefinition& EventDefinition = CandidateEvent->GetEventDefinition();
        if (!EventDefinition.bEnabled || EventDefinition.EventId.IsNone())
        {
            continue;
        }

        if (!CandidateEvent->SupportsDifficulty(NightShiftDefinition.DifficultyTier))
        {
            continue;
        }

        const FGameplayTagContainer& RequiredNightShiftTags = CandidateEvent->GetRequiredNightShiftTags();
        if (!RequiredNightShiftTags.IsEmpty() && !NightShiftDefinition.NightShiftTags.HasAll(RequiredNightShiftTags))
        {
            continue;
        }

        if (!NightShiftDefinition.SpecialEventPoolTags.IsEmpty())
        {
            const FGameplayTagContainer& EventPoolTags = CandidateEvent->GetEventPoolTags();
            if (!EventPoolTags.HasAny(NightShiftDefinition.SpecialEventPoolTags))
            {
                continue;
            }
        }

        EligibleEvents.Add(CandidateEvent);
    }

    if (EligibleEvents.IsEmpty())
    {
        return nullptr;
    }

    const int32 SelectedIndex = FMath::RandRange(0, EligibleEvents.Num() - 1);
    return EligibleEvents[SelectedIndex];
}

void AEMRNightShiftGameMode::ClearSpecialEventRuntimeState()
{
    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(SpecialEventRuntimeState.AlertTimerHandle);
        TimerManager.ClearTimer(SpecialEventRuntimeState.StartTimerHandle);
        TimerManager.ClearTimer(SpecialEventRuntimeState.EndTimerHandle);
    }

    ClearSpecialEventRuntimeEffects();

    if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        RunGS->SetSpecialEventPhase(EEMRSpecialEventPhase::None, NAME_None, 0.0f);
        RunGS->ClearSpecialEventPresentationData();
    }

    SpecialEventRuntimeState = FEMRSpecialEventRuntimeState();
    ActiveNightShiftForSpecialEvent.Reset();
    ActiveSpecialEventDefinition.Reset();
}

void AEMRNightShiftGameMode::HandleSpecialEventAlertStarted()
{
    if (!HasAuthority() || !SpecialEventRuntimeState.bScheduled || SpecialEventRuntimeState.bHasAlertStarted)
    {
        return;
    }

    SpecialEventRuntimeState.bHasAlertStarted = true;

    if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        RunGS->SetSpecialEventPhase(
            EEMRSpecialEventPhase::Alert,
            SpecialEventRuntimeState.EventId,
            RunGS->GetServerWorldTimeSeconds());
        RunGS->Multicast_PlayGlobalSound2D(SpecialEventAlertSound);
    }
}

void AEMRNightShiftGameMode::HandleSpecialEventStarted()
{
    if (!HasAuthority() || !SpecialEventRuntimeState.bScheduled || SpecialEventRuntimeState.bHasEventStarted)
    {
        return;
    }

    SpecialEventRuntimeState.bHasEventStarted = true;

    const UEMRSpecialEventDefinition* ActiveSpecialEvent = ActiveSpecialEventDefinition.Get();
    if (!ActiveSpecialEvent)
    {
        return;
    }

    ApplySpecialEventRuntimeEffects(ActiveSpecialEvent->GetEventDefinition());

    if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        RunGS->SetSpecialEventPhase(
            EEMRSpecialEventPhase::Active,
            SpecialEventRuntimeState.EventId,
            RunGS->GetServerWorldTimeSeconds());
    }
}

void AEMRNightShiftGameMode::HandleSpecialEventEnded()
{
    if (!HasAuthority() || !SpecialEventRuntimeState.bScheduled || SpecialEventRuntimeState.bHasEventEnded)
    {
        return;
    }

    SpecialEventRuntimeState.bHasEventEnded = true;
    ClearSpecialEventRuntimeEffects();

    if (AEMRNightShiftGameState* RunGS = GetNightShiftGameState())
    {
        RunGS->SetSpecialEventPhase(
            EEMRSpecialEventPhase::Completed,
            SpecialEventRuntimeState.EventId,
            RunGS->GetServerWorldTimeSeconds());
    }
}

void AEMRNightShiftGameMode::ApplySpecialEventRuntimeEffects(const FEMRNightShiftSpecialEventDefinition& EventDefinition)
{
    if (UWorld* World = GetWorld())
    {
        if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
        {
            SpawnSubsystem->SetSpecialEventSpawnRateMultiplier(SpecialEventRuntimeState.EffectiveSpawnRateMultiplier);
        }

        if (UEMRPatientPoolSubsystem* PatientPoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
        {
            if (EventDefinition.WeightedPathologies.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("[SpecialEvent] Event '%s' has no weighted pathologies; keeping baseline pathology distribution."),
                    *EventDefinition.EventId.ToString());
                PatientPoolSubsystem->ClearSpecialEventPathologyWeights();
            }
            else
            {
                PatientPoolSubsystem->SetSpecialEventPathologyWeights(
                    EventDefinition.WeightedPathologies,
                    EventDefinition.NonMatchingPathologyWeight);
            }
        }
    }
}

void AEMRNightShiftGameMode::ClearSpecialEventRuntimeEffects()
{
    if (UWorld* World = GetWorld())
    {
        if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
        {
            SpawnSubsystem->SetSpecialEventSpawnRateMultiplier(1.0f);
        }

        if (UEMRPatientPoolSubsystem* PatientPoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
        {
            PatientPoolSubsystem->ClearSpecialEventPathologyWeights();
        }
    }
}

void AEMRNightShiftGameMode::ScheduleOvertimeStopPatientDepartures()
{
    UWorld* World = GetWorld();
    if (!World || !HasAuthority())
    {
        return;
    }

    World->GetTimerManager().ClearTimer(OvertimeStopDepartureTimerHandle);
    OvertimeStopDepartureQueue.Reset();
    OvertimeStopDepartureIndex = 0;

    for (TActorIterator<AEMRPatient> It(World); It; ++It)
    {
        AEMRPatient* Patient = *It;
        if (!IsValid(Patient) || !Patient->GetPatientData())
        {
            continue;
        }

        const UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
        if (ASC && ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving))
        {
            continue;
        }

        OvertimeStopDepartureQueue.Add(Patient);
    }

    if (OvertimeStopDepartureQueue.IsEmpty())
    {
        return;
    }

    const float StagingWindowSeconds = FMath::Max(NightShiftEndDelaySeconds * 0.5f, 0.0f);
    if (StagingWindowSeconds <= KINDA_SMALL_NUMBER)
    {
        while (OvertimeStopDepartureIndex < OvertimeStopDepartureQueue.Num())
        {
            DispatchNextOvertimeStopPatientDeparture();
        }
        return;
    }

    DispatchNextOvertimeStopPatientDeparture();

    if (OvertimeStopDepartureIndex < OvertimeStopDepartureQueue.Num())
    {
        const int32 RemainingDispatchCount = OvertimeStopDepartureQueue.Num() - 1;
        const float DispatchIntervalSeconds = RemainingDispatchCount > 0
            ? (StagingWindowSeconds / static_cast<float>(RemainingDispatchCount))
            : StagingWindowSeconds;
        const float ClampedDispatchIntervalSeconds = FMath::Max(DispatchIntervalSeconds, 0.05f);

        World->GetTimerManager().SetTimer(
            OvertimeStopDepartureTimerHandle,
            this,
            &ThisClass::DispatchNextOvertimeStopPatientDeparture,
            ClampedDispatchIntervalSeconds,
            true);
    }
}

void AEMRNightShiftGameMode::DispatchNextOvertimeStopPatientDeparture()
{
    if (OvertimeStopDepartureIndex >= OvertimeStopDepartureQueue.Num())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(OvertimeStopDepartureTimerHandle);
        }
        return;
    }

    AEMRPatient* Patient = OvertimeStopDepartureQueue[OvertimeStopDepartureIndex].Get();
    ++OvertimeStopDepartureIndex;

    if (Patient)
    {
        HandlePatientDeparture(Patient, EEMRPatientLeaveReason::OvertimeStopShutdown);
    }

    if (OvertimeStopDepartureIndex >= OvertimeStopDepartureQueue.Num())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(OvertimeStopDepartureTimerHandle);
        }
    }
}

float AEMRNightShiftGameMode::GetPatienceReputationPenalty() const
{
    const AEMRNightShiftGameState* NightShiftGameState = GetNightShiftGameState();
    const UEMRNightShiftDefinition* ActiveNightShift =
    NightShiftGameState ? NightShiftGameState->GetCurrentNightShiftDefinition() : nullptr;

    const EEMRNightShiftDifficultyTier Difficulty =
    ActiveNightShift ? ActiveNightShift->DifficultyTier : EEMRNightShiftDifficultyTier::Standard;

    if (const UEMRDifficultyTuningData* DifficultyTuning = GetDifficultyTuning())
    {
        return DifficultyTuning->GetPatientReputationPenalty(Difficulty);
    }

    return DefaultReputationPenalty;
}

void AEMRNightShiftGameMode::HandlePatientOutOfPatience(AEMRPatient* Patient)
{
    HandlePatientDeparture(Patient, EEMRPatientLeaveReason::OutOfPatience);
}

void AEMRNightShiftGameMode::HandlePatientDeparture(AEMRPatient* Patient, const EEMRPatientLeaveReason Reason)
{
    if (!Patient || !HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] OutOfPatience ignored. Patient=%s HasAuthority=%s"),
            *GetNameSafe(Patient),
            HasAuthority() ? TEXT("true") : TEXT("false"));
        return;
    }

    const bool bCountAsPatienceLeave = Reason == EEMRPatientLeaveReason::OutOfPatience;
    const bool bApplyReputationPenalty = Reason == EEMRPatientLeaveReason::OutOfPatience;
    const TCHAR* ReasonText = bCountAsPatienceLeave ? TEXT("OutOfPatience") : TEXT("OvertimeStop");

    UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
    const bool bAlreadyLeaving = ASC && ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving);
    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] PatientDeparture start. Reason=%s Patient=%s AlreadyLeaving=%s"),
        ReasonText,
        *GetNameSafe(Patient),
        bAlreadyLeaving ? TEXT("true") : TEXT("false"));

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] PatientDeparture aborted. Reason=%s Patient=%s Missing GameInstance"),
            ReasonText,
            *GetNameSafe(Patient));
        return;
    }

    if (!bAlreadyLeaving)
    {
        if (bCountAsPatienceLeave)
        {
            if (AEMRNightShiftGameState* NightShiftGameState = GetNightShiftGameState())
            {
                NightShiftGameState->RegisterPatientLeftWithoutPay();
            }

            if (bTelemetryAccumulatorActive)
            {
                AddCountForSplit(
                    IsTelemetrySplitInOvertime(),
                    NightShiftTelemetryAccumulator.PatientsLeftByPatienceBeforeOvertime,
                    NightShiftTelemetryAccumulator.PatientsLeftByPatienceAfterOvertime);
                NightShiftTelemetryAccumulator.SpawnTimestampByPatient.Remove(Patient);
            }
        }

        if (bApplyReputationPenalty)
        {
            ApplyReputationPenaltyForPatientLeaving(Patient);
        }
    }

    if (bTelemetryAccumulatorActive)
    {
        NightShiftTelemetryAccumulator.SpawnTimestampByPatient.Remove(Patient);
    }

    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AEMRWaitingRoomArea> It(World); It; ++It)
        {
            if (AEMRWaitingRoomArea* WaitingArea = *It)
            {
                if (UEMRWaitingRoomManagerComponent* WaitingRoomManager = WaitingArea->GetManagerComponent())
                {
                    if (WaitingRoomManager->RemovePatientFromQueue(Patient))
                    {
                        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Removed from waiting room queue. Patient=%s Area=%s"),
                            *GetNameSafe(Patient),
                            *GetNameSafe(WaitingArea));
                        break;
                    }
                }
            }
        }
    }

    if (UEMRExamQueueSubsystem* ExamQueueSubsystem = GameInstance->GetSubsystem<UEMRExamQueueSubsystem>())
    {
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Removing patient from exam queues. Patient=%s"),
            *GetNameSafe(Patient));
        ExamQueueSubsystem->RemovePatientFromAllQueues(Patient);
    }

    if (UEMRTreatmentSubsystem* TreatmentSubsystem = GameInstance->GetSubsystem<UEMRTreatmentSubsystem>())
    {
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Requesting treatment abandonment. Patient=%s"),
            *GetNameSafe(Patient));
        TreatmentSubsystem->HandlePatientAbandonment(Patient);
    }

    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AEMRBaseMachine> It(World); It; ++It)
        {
            if (AEMRBaseMachine* Machine = *It)
            {
                if (Machine->GetAssignedPatient() == Patient)
                {
                    Machine->ClearOccupiedPatient(Patient);
                    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Cleared patient from machine. Patient=%s Machine=%s"),
                        *GetNameSafe(Patient),
                        *GetNameSafe(Machine));
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] PatientDeparture end. Reason=%s Patient=%s"), ReasonText, *GetNameSafe(Patient));
}


void AEMRNightShiftGameMode::ApplyReputationPenaltyForPatientLeaving(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float ReputationPenaltyAmount = GetPatienceReputationPenalty();
    if (ReputationPenaltyAmount <= 0.f)
    {
    	return;
    }
	

    AEMRNightShiftGameState* NightShiftGameState = World->GetGameState<AEMRNightShiftGameState>();
    UAbilitySystemComponent* TeamASC = NightShiftGameState ? NightShiftGameState->GetAbilitySystemComponent() : nullptr;
    if (!TeamASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftGameMode] Team ASC not available for reputation penalty"));
        return;
    }

    TArray<const UEMREconomySystemGenerics*> LoadedEconomy;
    if (!UEMRAssetManager::Get().CollectLoadedEconomySystemGenerics(LoadedEconomy) || LoadedEconomy.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftGameMode] EconomySystemGenerics not loaded for penalty"));
        return;
    }

    const UEMREconomySystemGenerics* EconomyConfig = LoadedEconomy[0];
    if (!EconomyConfig)
    {
        return;
    }

    const TSubclassOf<UGameplayEffect> RemoveReputationEffect = EconomyConfig->GetRemoveReputationEffect();
    if (!RemoveReputationEffect)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftGameMode] Missing RemoveReputation config"));
        return;
    }

    FGameplayEffectContextHandle ReputationPenaltyContext = TeamASC->MakeEffectContext();
    ReputationPenaltyContext.AddSourceObject(Patient);

    FGameplayEffectSpecHandle PenaltySpecHandle = TeamASC->MakeOutgoingSpec(RemoveReputationEffect, 1.0f, ReputationPenaltyContext);
    if (PenaltySpecHandle.IsValid())
    {
        PenaltySpecHandle.Data->SetSetByCallerMagnitude(EMRTags::SetByCaller::RemoveReputation, -ReputationPenaltyAmount);
        TeamASC->ApplyGameplayEffectSpecToSelf(*PenaltySpecHandle.Data.Get());
    }
}
