#include "Framework/EMRHubGameMode.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Data/EMRRunUpgradeDefinition.h"
#include "Developer/EMRDeveloperToolsTypes.h"
#include "Framework/EMRAssetManager.h"
#include "Framework/EMRHubertDirector.h"
#include "Framework/EMRHubGameState.h"
#include "Framework/EMRHubPlayerSlot.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/EMRTags.h"
#include "Lobby/EMRLobbyCharacterCatalog.h"
#include "Characters/Player/EMRPlayerState.h"
#include "Subsystems/EMRRunRulesSubsystem.h"
#include "Characters/Player/EMRPlayerController.h"
#include "Subsystems/EMRRunProgressionSubsystem.h"
#include "Subsystems/EMRNightShiftSpawnSubsystem.h"
#include "Subsystems/EMRPatientPoolSubsystem.h"
#include "Subsystems/EMRLobbyInviteCodeSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

namespace
{
    constexpr TCHAR HubGameModeUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");
}

AEMRHubGameMode::AEMRHubGameMode()
{
    bUseSeamlessTravel = true;
    PlayerStateClass = ResolvePlayerStateClass();
    PlayerControllerClass = AEMRPlayerController::StaticClass();
}

UClass* AEMRHubGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
    const AEMRPlayerState* EMRPlayerState = InController ? InController->GetPlayerState<AEMRPlayerState>() : nullptr;
    const UEMRLobbyCharacterCatalog* Catalog = GetPlayerCharacterCatalog();
    if (EMRPlayerState && Catalog)
    {
        int32 SelectedIndex = EMRPlayerState->GetLobbyCharacterIndex();
        if (!Catalog->Characters.IsValidIndex(SelectedIndex))
        {
            SelectedIndex = Catalog->DefaultCharacterIndex;
        }

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

const UEMRLobbyCharacterCatalog* AEMRHubGameMode::GetPlayerCharacterCatalog() const
{
    if (CachedPlayerCharacterCatalog.IsValid())
    {
        return CachedPlayerCharacterCatalog.Get();
    }

    CachedPlayerCharacterCatalog = PlayerCharacterCatalog.LoadSynchronous();
    return CachedPlayerCharacterCatalog.Get();
}

UClass* AEMRHubGameMode::ResolvePlayerStateClass() const
{
    return AEMRPlayerState::StaticClass();
}

void AEMRHubGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

}

void AEMRHubGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("[HubGameMode] BeginPlay - HasAuthority: %s"), HasAuthority() ? TEXT("YES") : TEXT("NO"));

    if (!HasAuthority())
    {
        return;
    }

    GetPlayerCharacterCatalog();

    if (UEMRPatientPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UEMRPatientPoolSubsystem>())
    {
        PoolSubsystem->InitializePool(0);
    }

    if (AEMRHubGameState* HubGameState = GetHubGameState())
    {
        HubGameState->OnHubSlotRegistered.AddUObject(this, &ThisClass::HandleHubSlotRegistered);
    }
	
	
    bool bRestoredRun = false;
    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        if (ProgressionSubsystem->ApplyToGameState(GetNightShiftGameState()))
        {
            if (const AEMRNightShiftGameState* RestoredRunState = GetNightShiftGameState())
            {
                UE_LOG(
                    LogTemp,
                    Log,
                    TEXT("[HubGameMode] BeginPlay - Restored run from ProgressionSubsystem (Phase=%d, Cycle=%d, NightShift=%d, TotalRevenue=%.2f)"),
                    static_cast<int32>(RestoredRunState->GetRunPhase()),
                    RestoredRunState->GetCurrentCycleIndex(),
                    RestoredRunState->GetNightShiftIndexInCycle(),
                    RestoredRunState->GetTotalRevenue());
            }

            bRestoredRun = true;
        }
    }

    if (bRestoredRun)
    {
        if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
        {
            UE_LOG(LogTemp, Log, TEXT("[RunPhaseDebug] Hub BeginPlay restored - Phase=%d"),
                static_cast<int32>(GetNightShiftGameState() ? GetNightShiftGameState()->GetRunPhase() : EER_RunPhase::Hub));
            ProgressionSubsystem->LogAuthoritativeState(TEXT("Hub.BeginPlay.Restored"), GetNightShiftGameState());
            ProgressionSubsystem->CacheFromGameState(GetNightShiftGameState());
        }

        HandleRestoredRunState();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[HubGameMode] BeginPlay - Starting new run"));
        StartNewRun();
    }

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
                    UE_LOG(LogTemp, Warning, TEXT("[InviteCode] HubGameMode could not resolve or generate session invite code."));
                }
            }
        }
    }

    AssignSlotsForExistingPlayers();
    TryAssignPendingPlayers();

    if (AEMRHubertDirector* ResolvedHubertDirector = ResolveHubertDirector())
    {
        ResolvedHubertDirector->StartHubRuntime();
    }
}

void AEMRHubGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!HasAuthority() || !NewPlayer)
    {
        return;
    }

    APlayerState* PlayerState = NewPlayer->PlayerState;
    if (!PlayerState)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HubGameMode] PostLogin - PlayerState missing for %s"), *GetNameSafe(NewPlayer));
        return;
    }

    AEMRHubGameState* HubGameState = GetHubGameState();
    if (!HubGameState || !IsRunPhaseEligibleForHubSlotAssignment(HubGameState->GetRunPhase()))
    {
        UE_LOG(LogTemp, Log, TEXT("[HubGameMode] PostLogin - Hub seating skipped (RunPhase=%d)"), HubGameState ? static_cast<int32>(HubGameState->GetRunPhase()) : -1);
        RefreshHubUpgradeVoteParticipationState();
        return;
    }

    if (!HubGameState->AssignSlotToPlayer(PlayerState))
    {
        PendingPlayerStates.AddUnique(PlayerState);
    }

    RefreshHubUpgradeVoteParticipationState();
}

void AEMRHubGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
    Super::HandleSeamlessTravelPlayer(C);

    if (!HasAuthority() || !C)
    {
        return;
    }

    APlayerState* PlayerState = C->PlayerState;
    if (!PlayerState)
    {
        return;
    }

    if (AEMRHubGameState* HubGameState = GetHubGameState())
    {
        if (!HubGameState->AssignSlotToPlayer(PlayerState))
        {
            PendingPlayerStates.AddUnique(PlayerState);
        }
    }

    RefreshHubUpgradeVoteParticipationState();
}

void AEMRHubGameMode::Logout(AController* Exiting)
{
    APlayerState* PlayerState = Exiting ? Exiting->PlayerState : nullptr;

    if (HasAuthority() && PlayerState)
    {
        const FString ExitingVoterKey = BuildStableHubVoterKey(PlayerState);
        if (!ExitingVoterKey.IsEmpty())
        {
            VoteIndexByVoterId.Remove(ExitingVoterKey);
        }

        RemovePendingPlayerState(PlayerState);
        if (AEMRHubGameState* HubGameState = GetHubGameState())
        {
            HubGameState->ClearSlotForPlayer(PlayerState);
        }
    }

    Super::Logout(Exiting);
    RefreshHubUpgradeVoteParticipationState();
}


AEMRNightShiftGameState* AEMRHubGameMode::GetNightShiftGameState() const
{
    return GetGameState<AEMRNightShiftGameState>();
}


UEMRRunProgressionSubsystem* AEMRHubGameMode::GetRunProgressionSubsystem() const
{
    if (!GetGameInstance())
    {
        return nullptr;
    }

    return GetGameInstance()->GetSubsystem<UEMRRunProgressionSubsystem>();
}


UEMRRunRulesSubsystem* AEMRHubGameMode::GetRunRulesSubsystem() const
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


void AEMRHubGameMode::SyncRunStateToSubsystem()
{
    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        ProgressionSubsystem->CacheFromGameState(GetNightShiftGameState());
    }
}


void AEMRHubGameMode::StartNewRun()
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] StartNewRun"));

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HubGameMode] StartNewRun: Run GameState is null"));
        return;
    }

    if (UEMRRunRulesSubsystem* RunRules = GetRunRulesSubsystem())
    {
        RunRules->InitializeNewRun(RunGS);
    }

    SetupCycle(0);
    PrepareNextNightShiftSelection();
}


void AEMRHubGameMode::SetupCycle(int32 CycleIndex)
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] SetupCycle %d"), CycleIndex);

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return;
    }

    if (UEMRRunRulesSubsystem* RunRules = GetRunRulesSubsystem())
    {
        RunRules->SetupCycle(RunGS, CycleIndex);
    }

    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        ProgressionSubsystem->LogAuthoritativeState(TEXT("Hub.SetupCycle"), RunGS);
    }

    SyncRunStateToSubsystem();
}

void AEMRHubGameMode::PrepareNextNightShiftSelection()
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] PrepareNextNightShiftSelection - Starting"));

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HubGameMode] PrepareNextNightShiftSelection - No GameState!"));
        return;
    }

    TArray<UEMRNightShiftDefinition*> LoadedNightShifts;

    for (const TSoftObjectPtr<UEMRNightShiftDefinition>& SoftNightShift : NightShiftDefinitionsPool)
    {
        if (UEMRNightShiftDefinition* NightShift = SoftNightShift.LoadSynchronous())
        {
            LoadedNightShifts.Add(NightShift);
        }
        else if (!SoftNightShift.IsNull())
        {
            UE_LOG(
                                LogTemp,
                                Warning,
                                TEXT("[HubGameMode] PrepareNextNightShiftSelection - Failed to load NightShift asset '%s'"),
                                *SoftNightShift.ToSoftObjectPath().ToString());
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[HubGameMode] PrepareNextNightShiftSelection - Loaded %d NightShifts from pool"), LoadedNightShifts.Num());

    if (LoadedNightShifts.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HubGameMode] PrepareNextNightShiftSelection: no NightShift definitions in pool"));
        RunGS->SetAvailableNextNightShifts({});
        SyncRunStateToSubsystem();
        return;
    }

    TArray<UEMRNightShiftDefinition*> AvailableNightShifts = LoadedNightShifts;
    if (bAvoidRepeatingLastSelection && LastNightShiftSelection.Num() > 0)
    {
        AvailableNightShifts.RemoveAll([this](const UEMRNightShiftDefinition* Candidate)
                {
                        return LastNightShiftSelection.ContainsByPredicate([Candidate](const TWeakObjectPtr<UEMRNightShiftDefinition>& Last)
                        {
                                return Last.IsValid() && Last.Get() == Candidate;
                        });
                });

        if (AvailableNightShifts.Num() == 0)
        {
            AvailableNightShifts = LoadedNightShifts;
        }
    }

    TSet<EEMRNightShiftRevenuePotential> UniquePotentials;
    for (const UEMRNightShiftDefinition* NightShift : LoadedNightShifts)
    {
        if (NightShift)
        {
            UniquePotentials.Add(NightShift->RevenuePotential);
        }
    }

    const int32 RequiredByPotential = UniquePotentials.Num();
    const int32 TargetSelectionCount = FMath::Clamp(RequiredByPotential, 1, LoadedNightShifts.Num());

    auto BuildBuckets = [](const TArray<UEMRNightShiftDefinition*>& Source)
    {
        TMap<EEMRNightShiftRevenuePotential, TArray<UEMRNightShiftDefinition*>> Buckets;
        for (UEMRNightShiftDefinition* NightShift : Source)
        {
            if (NightShift)
            {
                Buckets.FindOrAdd(NightShift->RevenuePotential).Add(NightShift);
            }
        }
        return Buckets;
    };

    auto GetRandomFromBucket = [](const TArray<UEMRNightShiftDefinition*>& Bucket) -> UEMRNightShiftDefinition*
    {
        if (Bucket.Num() == 0)
        {
            return nullptr;
        }

        const int32 Index = FMath::RandRange(0, Bucket.Num() - 1);
        return Bucket[Index];
    };

    const TMap<EEMRNightShiftRevenuePotential, TArray<UEMRNightShiftDefinition*>> PreferredBuckets = BuildBuckets(AvailableNightShifts);
    const TMap<EEMRNightShiftRevenuePotential, TArray<UEMRNightShiftDefinition*>> FallbackBuckets  = BuildBuckets(LoadedNightShifts);

    TArray<UEMRNightShiftDefinition*> Selection;
    Selection.Reserve(TargetSelectionCount);

    const EEMRNightShiftRevenuePotential OrderedPotentials[] = {EEMRNightShiftRevenuePotential::Low, EEMRNightShiftRevenuePotential::Medium, EEMRNightShiftRevenuePotential::High, EEMRNightShiftRevenuePotential::VeryHigh};
    for (EEMRNightShiftRevenuePotential Potential : OrderedPotentials)
    {
        const TArray<UEMRNightShiftDefinition*>* PotentialBucket = PreferredBuckets.Find(Potential);
        const TArray<UEMRNightShiftDefinition*>* FallbackBucket   = FallbackBuckets.Find(Potential);

        const TArray<UEMRNightShiftDefinition*>* SelectedBucket = (PotentialBucket && PotentialBucket->Num() > 0)
                        ? PotentialBucket
                        : FallbackBucket;

        if (SelectedBucket && SelectedBucket->Num() > 0)
        {
            if (UEMRNightShiftDefinition* Candidate = GetRandomFromBucket(*SelectedBucket))
            {
                if (!Selection.Contains(Candidate))
                {
                    Selection.Add(Candidate);
                }
            }
        }
    }

    TArray<UEMRNightShiftDefinition*> RemainingPreferred = AvailableNightShifts;
    RemainingPreferred.RemoveAll([&Selection](const UEMRNightShiftDefinition* Candidate)
        {
                return Selection.Contains(Candidate);
        });

    auto PullFromPool = [&Selection, &GetRandomFromBucket](TArray<UEMRNightShiftDefinition*>& Pool) -> bool
    {
        if (Pool.Num() == 0)
        {
            return false;
        }

        if (UEMRNightShiftDefinition* Candidate = GetRandomFromBucket(Pool))
        {
            Selection.Add(Candidate);
            Pool.Remove(Candidate);
            return true;
        }

        return false;
    };

    while (Selection.Num() < TargetSelectionCount && PullFromPool(RemainingPreferred))
    {
    }

    if (Selection.Num() < TargetSelectionCount)
    {
        TArray<UEMRNightShiftDefinition*> RemainingFallback = LoadedNightShifts;
        RemainingFallback.RemoveAll([&Selection](const UEMRNightShiftDefinition* Candidate)
                {
                        return Selection.Contains(Candidate);
                });

        while (Selection.Num() < TargetSelectionCount && PullFromPool(RemainingFallback))
        {
        }
    }

    LastNightShiftSelection.Reset();
    for (UEMRNightShiftDefinition* Chosen : Selection)
    {
        LastNightShiftSelection.Add(Chosen);
    }

    UE_LOG(LogTemp, Log, TEXT("[HubGameMode] PrepareNextNightShiftSelection - Setting %d NightShifts in GameState"), Selection.Num());
    RunGS->SetAvailableNextNightShifts(Selection);
    RunGS->SetNightShiftSelectionLocked(false);
    RunGS->SetHubDecisionStage(EEMRHubDecisionStage::NightShiftSelection);

    FEMRHubUpgradeVoteState ClearedVoteState;
    RunGS->SetHubUpgradeVoteState(ClearedVoteState);

    PendingNightShiftSelectionForVote = nullptr;
    VoteIndexByVoterId.Reset();
    ActiveVoteCounts.Reset();
    GetWorldTimerManager().ClearTimer(HubUpgradeVoteTimerHandle);

    SyncRunStateToSubsystem();

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] PrepareNextNightShiftSelection - Complete. GameState now has %d available NightShifts"),
                RunGS->GetAvailableNextNightShifts().Num());
}

void AEMRHubGameMode::StartNightShiftFromSelection(UEMRNightShiftDefinition* SelectedNightShift)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s StartNightShiftFromSelection requested Selected=%s"),
        HubGameModeUpgradesFlowLogPrefix,
        *GetNameSafe(SelectedNightShift));

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s StartNightShiftFromSelection ignored (no authority)."), HubGameModeUpgradesFlowLogPrefix);
        return;
    }

    if (!SelectedNightShift)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s StartNightShiftFromSelection rejected: SelectedNightShift is null."), HubGameModeUpgradesFlowLogPrefix);
        UE_LOG(LogTemp, Warning, TEXT("StartNightShiftFromSelection: SelectedNightShift is null"));
        return;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s StartNightShiftFromSelection rejected: NightShiftGameState is null."), HubGameModeUpgradesFlowLogPrefix);
        UE_LOG(LogTemp, Warning, TEXT("StartNightShiftFromSelection: NightShiftGameState is null"));
        return;
    }

    const TArray<UEMRNightShiftDefinition*>& OfferedNightShifts = RunGS->GetAvailableNextNightShifts();
    if (!OfferedNightShifts.Contains(SelectedNightShift))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s StartNightShiftFromSelection rejected: selected nightshift is not in offers. OfferedCount=%d Selected=%s"),
            HubGameModeUpgradesFlowLogPrefix,
            OfferedNightShifts.Num(),
            *GetNameSafe(SelectedNightShift));
        UE_LOG(LogTemp, Warning, TEXT("StartNightShiftFromSelection: %s not in current offers"), *SelectedNightShift->GetName());
        return;
    }

    if (RunGS->GetHubDecisionStage() != EEMRHubDecisionStage::NightShiftSelection)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s StartNightShiftFromSelection rejected: invalid hub decision stage. Stage=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            static_cast<int32>(RunGS->GetHubDecisionStage()));
        UE_LOG(LogTemp, Warning, TEXT("StartNightShiftFromSelection: hub stage is not in nightshift selection."));
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s StartNightShiftFromSelection accepted. Starting upgrade vote for %s"),
        HubGameModeUpgradesFlowLogPrefix,
        *GetNameSafe(SelectedNightShift));
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] Selection accepted: %s"), *SelectedNightShift->GetName());
    StartHubUpgradeVote(SelectedNightShift);
}

void AEMRHubGameMode::StartHubUpgradeVote(UEMRNightShiftDefinition* SelectedNightShift)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s StartHubUpgradeVote entry Selected=%s"),
        HubGameModeUpgradesFlowLogPrefix,
        *GetNameSafe(SelectedNightShift));

    if (!HasAuthority() || !SelectedNightShift)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s StartHubUpgradeVote aborted HasAuthority=%d SelectedValid=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            HasAuthority() ? 1 : 0,
            SelectedNightShift ? 1 : 0);
        return;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s StartHubUpgradeVote fallback: RunGS missing, starting NightShift immediately (%s)."),
            HubGameModeUpgradesFlowLogPrefix,
            *GetNameSafe(SelectedNightShift));
        StartNightShift(SelectedNightShift);
        return;
    }

    if (RunGS->GetHubDecisionStage() != EEMRHubDecisionStage::NightShiftSelection)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s StartHubUpgradeVote aborted: stage is not NightShiftSelection. Stage=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            static_cast<int32>(RunGS->GetHubDecisionStage()));
        return;
    }

    PendingNightShiftSelectionForVote = SelectedNightShift;
    VoteIndexByVoterId.Reset();
    GetWorldTimerManager().ClearTimer(HubUpgradeVoteTimerHandle);

    TArray<UEMRRunUpgradeDefinition*> LoadedUpgrades;
    LoadedUpgrades.Reserve(RunUpgradePool.Num());
    for (const TSoftObjectPtr<UEMRRunUpgradeDefinition>& UpgradeRef : RunUpgradePool)
    {
        if (UEMRRunUpgradeDefinition* UpgradeDefinition = UpgradeRef.LoadSynchronous())
        {
            LoadedUpgrades.Add(UpgradeDefinition);
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s StartHubUpgradeVote loaded upgrades PoolCount=%d LoadedCount=%d"),
        HubGameModeUpgradesFlowLogPrefix,
        RunUpgradePool.Num(),
        LoadedUpgrades.Num());

    if (LoadedUpgrades.IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s StartHubUpgradeVote no loaded upgrades configured -> immediate NightShift travel."),
            HubGameModeUpgradesFlowLogPrefix);
        UE_LOG(LogTemp, Warning, TEXT("[HubGameMode] No run upgrades configured, starting NightShift immediately."));
        RunGS->SetHubDecisionStage(EEMRHubDecisionStage::WaitingNightShiftTravel);
        SyncRunStateToSubsystem();
        StartNightShift(SelectedNightShift);
        return;
    }

    TArray<UEMRRunUpgradeDefinition*> CandidateUpgrades;
    TSet<FGameplayTag> AddedUpgradeTags;
    TSet<const UEMRRunUpgradeDefinition*> AddedUpgradeDefinitions;
    const UEMRDifficultyTuningData* DifficultyTuning = RunGS->GetDifficultyTuningData();
    const int32 SelectedMapTreatmentBedUpgradeCapacity = (DifficultyTuning && SelectedNightShift)
    ? DifficultyTuning->GetMaxTreatmentBedUpgradeCountForNightShift(SelectedNightShift)
    : 0;
    const int32 SelectedMapCoffeeMachineUpgradeCapacity = (DifficultyTuning && SelectedNightShift)
    ? DifficultyTuning->GetMaxCoffeeMachineUpgradeCountForNightShift(SelectedNightShift)
    : 0;
    const int32 SelectedMapSnackMachineUpgradeCapacity = (DifficultyTuning && SelectedNightShift)
    ? DifficultyTuning->GetMaxSnackMachineUpgradeCountForNightShift(SelectedNightShift)
    : 0;
    const int32 SelectedMapMagicBoxUpgradeCapacity = (DifficultyTuning && SelectedNightShift)
    ? DifficultyTuning->GetMaxMagicBoxUpgradeCountForNightShift(SelectedNightShift)
    : 0;
    const int32 SelectedMapOxygenMachineTotalCount = (DifficultyTuning && SelectedNightShift)
    ? DifficultyTuning->GetMaxOxygenMachineCountForNightShift(SelectedNightShift)
    : 1;
    const int32 SelectedMapOxygenMachineUpgradeCapacity = FMath::Max(0, SelectedMapOxygenMachineTotalCount - 1);
    const float ItemDispenserSalesDiscountPctPerStack = DifficultyTuning
    ? FMath::Max(0.0f, DifficultyTuning->GetItemDispenserSalesUpgradeTuning().DiscountPercentPerStack)
    : 0.0f;
    for (UEMRRunUpgradeDefinition* UpgradeDefinition : LoadedUpgrades)
    {
        if (!UpgradeDefinition)
        {
            continue;
        }

        int32 CurrentPurchaseCount = 0;
        if (UpgradeDefinition->UpgradeTag.IsValid())
        {
            CurrentPurchaseCount = RunGS->GetActiveRunUpgradeStackCount(UpgradeDefinition->UpgradeTag);
            if (UpgradeDefinition->UpgradeTag == EMRTags::RunUpgrade::ItemDispenserSales && ItemDispenserSalesDiscountPctPerStack > 0.0f)
            {
                const float CurrentTotalDiscountPercent = static_cast<float>(CurrentPurchaseCount) * ItemDispenserSalesDiscountPctPerStack;
                if (CurrentTotalDiscountPercent >= 100.0f)
                {
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("%s StartHubUpgradeVote skipping item dispenser sales at threshold current=%d perStackPct=%.2f totalPct=%.2f"),
                        HubGameModeUpgradesFlowLogPrefix,
                        CurrentPurchaseCount,
                        ItemDispenserSalesDiscountPctPerStack,
                        CurrentTotalDiscountPercent);
                    continue;
                }
            }

            if (UpgradeDefinition->MaxPurchaseCount > 0 && CurrentPurchaseCount >= UpgradeDefinition->MaxPurchaseCount)
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("%s StartHubUpgradeVote skipping upgrade at max purchases tag=%s current=%d max=%d"),
                    HubGameModeUpgradesFlowLogPrefix,
                    *UpgradeDefinition->UpgradeTag.ToString(),
                    CurrentPurchaseCount,
                    UpgradeDefinition->MaxPurchaseCount);
                continue;
            }

            if (UpgradeDefinition->UpgradeTag == EMRTags::RunUpgrade::TreatmentBed)
            {
                const int32 EffectiveMaxPurchase = UpgradeDefinition->MaxPurchaseCount > 0
                    ? FMath::Min(UpgradeDefinition->MaxPurchaseCount, SelectedMapTreatmentBedUpgradeCapacity)
                    : SelectedMapTreatmentBedUpgradeCapacity;
                if (CurrentPurchaseCount >= EffectiveMaxPurchase)
                {
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("%s StartHubUpgradeVote skipping treatment-bed upgrade at effective cap current=%d mapCapacity=%d effectiveMax=%d map=%s"),
                        HubGameModeUpgradesFlowLogPrefix,
                        CurrentPurchaseCount,
                        SelectedMapTreatmentBedUpgradeCapacity,
                        EffectiveMaxPurchase,
                        *GetNameSafe(SelectedNightShift));
                    continue;
                }
            }

            if (UpgradeDefinition->UpgradeTag == EMRTags::RunUpgrade::CoffeeMachine)
            {
                const int32 EffectiveMaxPurchase = UpgradeDefinition->MaxPurchaseCount > 0
                    ? FMath::Min(UpgradeDefinition->MaxPurchaseCount, SelectedMapCoffeeMachineUpgradeCapacity)
                    : SelectedMapCoffeeMachineUpgradeCapacity;
                if (CurrentPurchaseCount >= EffectiveMaxPurchase)
                {
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("%s StartHubUpgradeVote skipping coffee-machine upgrade at effective cap current=%d mapCapacity=%d effectiveMax=%d map=%s"),
                        HubGameModeUpgradesFlowLogPrefix,
                        CurrentPurchaseCount,
                        SelectedMapCoffeeMachineUpgradeCapacity,
                        EffectiveMaxPurchase,
                        *GetNameSafe(SelectedNightShift));
                    continue;
                }
            }

            if (UpgradeDefinition->UpgradeTag == EMRTags::RunUpgrade::SnackMachine)
            {
                const int32 EffectiveMaxPurchase = UpgradeDefinition->MaxPurchaseCount > 0
                    ? FMath::Min(UpgradeDefinition->MaxPurchaseCount, SelectedMapSnackMachineUpgradeCapacity)
                    : SelectedMapSnackMachineUpgradeCapacity;
                if (CurrentPurchaseCount >= EffectiveMaxPurchase)
                {
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("%s StartHubUpgradeVote skipping snack-machine upgrade at effective cap current=%d mapCapacity=%d effectiveMax=%d map=%s"),
                        HubGameModeUpgradesFlowLogPrefix,
                        CurrentPurchaseCount,
                        SelectedMapSnackMachineUpgradeCapacity,
                        EffectiveMaxPurchase,
                        *GetNameSafe(SelectedNightShift));
                    continue;
                }
            }

            if (UpgradeDefinition->UpgradeTag == EMRTags::RunUpgrade::MagicBox)
            {
                const int32 EffectiveMaxPurchase = UpgradeDefinition->MaxPurchaseCount > 0
                    ? FMath::Min(UpgradeDefinition->MaxPurchaseCount, SelectedMapMagicBoxUpgradeCapacity)
                    : SelectedMapMagicBoxUpgradeCapacity;
                if (CurrentPurchaseCount >= EffectiveMaxPurchase)
                {
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("%s StartHubUpgradeVote skipping magic-box upgrade at effective cap current=%d mapCapacity=%d effectiveMax=%d map=%s"),
                        HubGameModeUpgradesFlowLogPrefix,
                        CurrentPurchaseCount,
                        SelectedMapMagicBoxUpgradeCapacity,
                        EffectiveMaxPurchase,
                        *GetNameSafe(SelectedNightShift));
                    continue;
                }
            }

            if (UpgradeDefinition->UpgradeTag == EMRTags::RunUpgrade::OxygenMachine)
            {
                const int32 EffectiveMaxPurchase = UpgradeDefinition->MaxPurchaseCount > 0
                    ? FMath::Min(UpgradeDefinition->MaxPurchaseCount, SelectedMapOxygenMachineUpgradeCapacity)
                    : SelectedMapOxygenMachineUpgradeCapacity;
                if (CurrentPurchaseCount >= EffectiveMaxPurchase)
                {
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("%s StartHubUpgradeVote skipping oxygen-machine upgrade at effective cap current=%d totalMapCount=%d effectiveMax=%d map=%s"),
                        HubGameModeUpgradesFlowLogPrefix,
                        CurrentPurchaseCount,
                        SelectedMapOxygenMachineTotalCount,
                        EffectiveMaxPurchase,
                        *GetNameSafe(SelectedNightShift));
                    continue;
                }
            }

            if (AddedUpgradeTags.Contains(UpgradeDefinition->UpgradeTag))
            {
                continue;
            }

            AddedUpgradeTags.Add(UpgradeDefinition->UpgradeTag);
        }
        else
        {
            if (UpgradeDefinition->MaxPurchaseCount > 0)
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("%s StartHubUpgradeVote upgrade has max purchase count but invalid tag; cap cannot be enforced asset=%s"),
                    HubGameModeUpgradesFlowLogPrefix,
                    *GetNameSafe(UpgradeDefinition));
            }

            if (AddedUpgradeDefinitions.Contains(UpgradeDefinition))
            {
                continue;
            }

            AddedUpgradeDefinitions.Add(UpgradeDefinition);
        }

        CandidateUpgrades.Add(UpgradeDefinition);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s StartHubUpgradeVote candidate upgrades CandidateCount=%d ActiveTagCount=%d"),
        HubGameModeUpgradesFlowLogPrefix,
        CandidateUpgrades.Num(),
        RunGS->GetActiveRunUpgradeTags().Num());

    for (int32 Index = 0; Index < CandidateUpgrades.Num() - 1; ++Index)
    {
        const int32 SwapIndex = FMath::RandRange(Index, CandidateUpgrades.Num() - 1);
        CandidateUpgrades.Swap(Index, SwapIndex);
    }

    constexpr int32 DesiredOfferCount = 3;
    TArray<UEMRRunUpgradeDefinition*> OfferedUpgrades;
    OfferedUpgrades.Reserve(DesiredOfferCount);

    const int32 UniqueCount = FMath::Min(DesiredOfferCount, CandidateUpgrades.Num());
    for (int32 Index = 0; Index < UniqueCount; ++Index)
    {
        OfferedUpgrades.Add(CandidateUpgrades[Index]);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s StartHubUpgradeVote offers generated OfferCount=%d DesiredOfferCount=%d"),
        HubGameModeUpgradesFlowLogPrefix,
        OfferedUpgrades.Num(),
        DesiredOfferCount);

    if (OfferedUpgrades.Num() < DesiredOfferCount)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s StartHubUpgradeVote unique upgrade pool smaller than desired offers. OfferCount=%d DesiredOfferCount=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            OfferedUpgrades.Num(),
            DesiredOfferCount);
    }

    if (OfferedUpgrades.IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s StartHubUpgradeVote could not produce offers -> immediate NightShift travel."),
            HubGameModeUpgradesFlowLogPrefix);
        UE_LOG(LogTemp, Warning, TEXT("[HubGameMode] Could not produce upgrade offers, starting NightShift immediately."));
        RunGS->SetHubDecisionStage(EEMRHubDecisionStage::WaitingNightShiftTravel);
        SyncRunStateToSubsystem();
        StartNightShift(SelectedNightShift);
        return;
    }

    ActiveVoteCounts.SetNumZeroed(OfferedUpgrades.Num());

    FEMRHubUpgradeVoteState VoteState;
    VoteState.OfferedUpgrades = OfferedUpgrades;
    VoteState.VoteCounts = ActiveVoteCounts;
    VoteState.VoteEndServerTimeSeconds = RunGS->GetServerWorldTimeSeconds() + FMath::Max(HubUpgradeVoteDurationSeconds, 1.0f);
    VoteState.bVoteActive = true;
    VoteState.WinningOfferIndex = INDEX_NONE;
    VoteState.RequiredVoterCount = 0;
    VoteState.SubmittedVoterCount = 0;
    VoteState.bAllConnectedPlayersVoted = false;

    RunGS->SetHubUpgradeVoteState(VoteState);
    RunGS->SetHubDecisionStage(EEMRHubDecisionStage::UpgradeVoting);
    RunGS->SetNightShiftSelectionLocked(true);
    RefreshHubUpgradeVoteParticipationState();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s StartHubUpgradeVote started vote Stage=%d Duration=%.2f VoteEndServerTime=%.2f"),
        HubGameModeUpgradesFlowLogPrefix,
        static_cast<int32>(RunGS->GetHubDecisionStage()),
        FMath::Max(HubUpgradeVoteDurationSeconds, 1.0f),
        VoteState.VoteEndServerTimeSeconds);

    GetWorldTimerManager().SetTimer(
        HubUpgradeVoteTimerHandle,
        this,
        &ThisClass::HandleHubUpgradeVoteTimerExpired,
        FMath::Max(HubUpgradeVoteDurationSeconds, 1.0f),
        false);

    SyncRunStateToSubsystem();
}

void AEMRHubGameMode::HandleHubUpgradeVoteTimerExpired()
{
    UE_LOG(LogTemp, Warning, TEXT("%s HandleHubUpgradeVoteTimerExpired triggered."), HubGameModeUpgradesFlowLogPrefix);

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s HandleHubUpgradeVoteTimerExpired ignored (no authority)."), HubGameModeUpgradesFlowLogPrefix);
        return;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s HandleHubUpgradeVoteTimerExpired aborted: RunGS null."), HubGameModeUpgradesFlowLogPrefix);
        return;
    }

    FEMRHubUpgradeVoteState VoteState = RunGS->GetHubUpgradeVoteState();
    if (!VoteState.bVoteActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s HandleHubUpgradeVoteTimerExpired ignored: vote already inactive."), HubGameModeUpgradesFlowLogPrefix);
        return;
    }

    const int32 WinningOfferIndex = ResolveWinningUpgradeOfferIndex();
    VoteState.bVoteActive = false;
    VoteState.WinningOfferIndex = WinningOfferIndex;
    VoteState.VoteCounts = ActiveVoteCounts;
    VoteState.RequiredVoterCount = 0;
    VoteState.SubmittedVoterCount = 0;
    VoteState.bAllConnectedPlayersVoted = false;
    RunGS->SetHubUpgradeVoteState(VoteState);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s HandleHubUpgradeVoteTimerExpired resolved winner index=%d OfferCount=%d"),
        HubGameModeUpgradesFlowLogPrefix,
        WinningOfferIndex,
        VoteState.OfferedUpgrades.Num());

    ApplyWinningUpgradeAndStartNightShift();
}

void AEMRHubGameMode::CastOrUpdateUpgradeVote(APlayerState* VoterState, int32 OfferIndex)
{
    if (!HasAuthority() || !VoterState)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CastOrUpdateUpgradeVote ignored HasAuthority=%d VoterValid=%d OfferIndex=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            HasAuthority() ? 1 : 0,
            VoterState ? 1 : 0,
            OfferIndex);
        return;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS || RunGS->GetHubDecisionStage() != EEMRHubDecisionStage::UpgradeVoting)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CastOrUpdateUpgradeVote rejected: invalid stage or RunGS missing. Stage=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            RunGS ? static_cast<int32>(RunGS->GetHubDecisionStage()) : -1);
        return;
    }

    const FEMRHubUpgradeVoteState CurrentState = RunGS->GetHubUpgradeVoteState();
    if (!CurrentState.bVoteActive || !CurrentState.OfferedUpgrades.IsValidIndex(OfferIndex))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CastOrUpdateUpgradeVote rejected: vote inactive or invalid offer. VoteActive=%d OfferIndex=%d OfferCount=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            CurrentState.bVoteActive ? 1 : 0,
            OfferIndex,
            CurrentState.OfferedUpgrades.Num());
        return;
    }

    const FString VoterKey = BuildStableHubVoterKey(VoterState);
    if (VoterKey.IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CastOrUpdateUpgradeVote rejected: empty voter key for playerstate=%s"),
            HubGameModeUpgradesFlowLogPrefix,
            *GetNameSafe(VoterState));
        return;
    }

    if (ActiveVoteCounts.Num() != CurrentState.OfferedUpgrades.Num())
    {
        ActiveVoteCounts = CurrentState.VoteCounts;
        ActiveVoteCounts.SetNum(CurrentState.OfferedUpgrades.Num());
    }

    int32 PreviousVoteIndex = INDEX_NONE;
    if (const int32* ExistingVote = VoteIndexByVoterId.Find(VoterKey))
    {
        PreviousVoteIndex = *ExistingVote;
    }

    if (PreviousVoteIndex == OfferIndex)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CastOrUpdateUpgradeVote no-op: voter %s re-selected same offer index=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            *VoterKey,
            OfferIndex);
        return;
    }

    if (ActiveVoteCounts.IsValidIndex(PreviousVoteIndex))
    {
        ActiveVoteCounts[PreviousVoteIndex] = FMath::Max(0, ActiveVoteCounts[PreviousVoteIndex] - 1);
    }

    if (ActiveVoteCounts.IsValidIndex(OfferIndex))
    {
        ActiveVoteCounts[OfferIndex] += 1;
    }

    VoteIndexByVoterId.Add(VoterKey, OfferIndex);

    FEMRHubUpgradeVoteState UpdatedState = CurrentState;
    UpdatedState.VoteCounts = ActiveVoteCounts;
    RunGS->SetHubUpgradeVoteState(UpdatedState);
    RefreshHubUpgradeVoteParticipationState();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CastOrUpdateUpgradeVote applied voter=%s previous=%d new=%d"),
        HubGameModeUpgradesFlowLogPrefix,
        *VoterKey,
        PreviousVoteIndex,
        OfferIndex);
}

bool AEMRHubGameMode::RerollActiveHubUpgradeVoteOffersForTests(FString& OutMessage)
{
    if (!HasAuthority())
    {
        OutMessage = TEXT("Action rejected: not authority.");
        return false;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        OutMessage = TEXT("Action rejected: NightShift game state unavailable.");
        return false;
    }

    if (RunGS->GetHubDecisionStage() != EEMRHubDecisionStage::UpgradeVoting)
    {
        OutMessage = TEXT("Action unavailable: Hub reroll only works during UpgradeVoting.");
        return false;
    }

    const FEMRHubUpgradeVoteState CurrentVoteState = RunGS->GetHubUpgradeVoteState();
    if (!CurrentVoteState.bVoteActive)
    {
        OutMessage = TEXT("Action unavailable: no active upgrade vote to reroll.");
        return false;
    }

    UEMRNightShiftDefinition* PendingSelection = PendingNightShiftSelectionForVote;
    if (!PendingSelection)
    {
        OutMessage = TEXT("Action failed: pending Nightshift selection for vote is missing.");
        return false;
    }

    RunGS->SetHubDecisionStage(EEMRHubDecisionStage::NightShiftSelection);
    StartHubUpgradeVote(PendingSelection);

    const FEMRHubUpgradeVoteState NewVoteState = RunGS->GetHubUpgradeVoteState();
    const bool bSuccess =
    RunGS->GetHubDecisionStage() == EEMRHubDecisionStage::UpgradeVoting
    && NewVoteState.bVoteActive
    && NewVoteState.OfferedUpgrades.Num() > 0;

    if (!bSuccess)
    {
        OutMessage = TEXT("Hub upgrade reroll failed to start a new active vote.");
        return false;
    }

    OutMessage = FString::Printf(TEXT("Rerolled hub upgrades (%d offer(s))."), NewVoteState.OfferedUpgrades.Num());
    return true;
}

bool AEMRHubGameMode::ApplySpecificRunUpgradeForTests(const FGameplayTag UpgradeTag, FString& OutMessage)
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

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        OutMessage = TEXT("Action rejected: NightShift game state unavailable.");
        return false;
    }

    bool bFoundInPool = false;
    for (const TSoftObjectPtr<UEMRRunUpgradeDefinition>& UpgradeRef : RunUpgradePool)
    {
        const UEMRRunUpgradeDefinition* UpgradeDefinition = UpgradeRef.LoadSynchronous();
        if (UpgradeDefinition && UpgradeDefinition->UpgradeTag == UpgradeTag)
        {
            bFoundInPool = true;
            break;
        }
    }

    if (!bFoundInPool)
    {
        OutMessage = FString::Printf(TEXT("Action rejected: upgrade tag not found in RunUpgradePool (%s)."), *UpgradeTag.ToString());
        return false;
    }

    const int32 NewStackCount = RunGS->AddActiveRunUpgradeStack(UpgradeTag, 1);
    if (NewStackCount <= 0)
    {
        OutMessage = FString::Printf(TEXT("Failed to apply upgrade %s."), *UpgradeTag.ToString());
        return false;
    }

    OutMessage = FString::Printf(TEXT("Applied upgrade %s (new stack: %d)."), *UpgradeTag.ToString(), NewStackCount);
    return true;
}

bool AEMRHubGameMode::CollectRunUpgradeOptionsForTests(TArray<FEMRDeveloperToolUpgradeOption>& OutOptions, FString& OutMessage) const
{
    if (!HasAuthority())
    {
        OutMessage = TEXT("Action rejected: not authority.");
        return false;
    }

    OutOptions.Reset();

    TSet<FGameplayTag> AddedTags;
    for (const TSoftObjectPtr<UEMRRunUpgradeDefinition>& UpgradeRef : RunUpgradePool)
    {
        const UEMRRunUpgradeDefinition* UpgradeDefinition = UpgradeRef.LoadSynchronous();
        if (!UpgradeDefinition || !UpgradeDefinition->UpgradeTag.IsValid())
        {
            continue;
        }

        if (AddedTags.Contains(UpgradeDefinition->UpgradeTag))
        {
            continue;
        }
        AddedTags.Add(UpgradeDefinition->UpgradeTag);

        FEMRDeveloperToolUpgradeOption Option;
        Option.UpgradeTag = UpgradeDefinition->UpgradeTag;
        Option.UpgradeId = UpgradeDefinition->UpgradeId;

        const FString DisplayName = !UpgradeDefinition->DisplayName.IsEmpty()
            ? UpgradeDefinition->DisplayName.ToString()
            : (UpgradeDefinition->UpgradeId != NAME_None ? UpgradeDefinition->UpgradeId.ToString() : GetNameSafe(UpgradeDefinition));
        Option.DisplayLabel = FString::Printf(TEXT("%s [%s]"), *DisplayName, *UpgradeDefinition->UpgradeTag.ToString());
        OutOptions.Add(MoveTemp(Option));
    }

    OutOptions.Sort([](const FEMRDeveloperToolUpgradeOption& A, const FEMRDeveloperToolUpgradeOption& B)
    {
        return A.DisplayLabel < B.DisplayLabel;
    });

    OutMessage = FString::Printf(TEXT("Collected %d upgrade option(s)."), OutOptions.Num());
    return true;
}

void AEMRHubGameMode::RefreshHubUpgradeVoteParticipationState()
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

    FEMRHubUpgradeVoteState VoteState = RunGS->GetHubUpgradeVoteState();
    if (!VoteState.bVoteActive)
    {
        return;
    }

    int32 RequiredVoterCount = 0;
    int32 SubmittedVoterCount = 0;
    RecomputeHubUpgradeVoteParticipationCounts(RequiredVoterCount, SubmittedVoterCount);

    const bool bAllConnectedPlayersVoted = RequiredVoterCount > 0 && SubmittedVoterCount >= RequiredVoterCount;
    const bool bChanged = VoteState.RequiredVoterCount != RequiredVoterCount
    || VoteState.SubmittedVoterCount != SubmittedVoterCount
    || VoteState.bAllConnectedPlayersVoted != bAllConnectedPlayersVoted;
    if (!bChanged)
    {
        return;
    }

    VoteState.RequiredVoterCount = RequiredVoterCount;
    VoteState.SubmittedVoterCount = SubmittedVoterCount;
    VoteState.bAllConnectedPlayersVoted = bAllConnectedPlayersVoted;
    RunGS->SetHubUpgradeVoteState(VoteState);
}

void AEMRHubGameMode::RecomputeHubUpgradeVoteParticipationCounts(int32& OutRequiredVoterCount, int32& OutSubmittedVoterCount) const
{
    OutRequiredVoterCount = 0;
    OutSubmittedVoterCount = 0;

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return;
    }

    for (const APlayerState* PlayerState : RunGS->PlayerArray)
    {
        const FString VoterKey = BuildStableHubVoterKey(PlayerState);
        if (VoterKey.IsEmpty())
        {
            continue;
        }

        ++OutRequiredVoterCount;
        if (VoteIndexByVoterId.Contains(VoterKey))
        {
            ++OutSubmittedVoterCount;
        }
    }
}

bool AEMRHubGameMode::CanStartHubUpgradeVoteNow() const
{
    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return false;
    }

    const FEMRHubUpgradeVoteState& VoteState = RunGS->GetHubUpgradeVoteState();
    return VoteState.bVoteActive
    && VoteState.bAllConnectedPlayersVoted
    && VoteState.RequiredVoterCount > 0
    && VoteState.SubmittedVoterCount >= VoteState.RequiredVoterCount;
}

void AEMRHubGameMode::RequestStartHubUpgradeVoteNow(APlayerState* RequestingPlayerState)
{
    if (!HasAuthority() || !RequestingPlayerState)
    {
        return;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return;
    }

    if (RunGS->GetHubDecisionStage() != EEMRHubDecisionStage::UpgradeVoting)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s RequestStartHubUpgradeVoteNow rejected: wrong stage=%d requestor=%s"),
            HubGameModeUpgradesFlowLogPrefix,
            static_cast<int32>(RunGS->GetHubDecisionStage()),
            *GetNameSafe(RequestingPlayerState));
        return;
    }

    RefreshHubUpgradeVoteParticipationState();
    if (!CanStartHubUpgradeVoteNow())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s RequestStartHubUpgradeVoteNow rejected: not all connected players voted requestor=%s"),
            HubGameModeUpgradesFlowLogPrefix,
            *GetNameSafe(RequestingPlayerState));
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s RequestStartHubUpgradeVoteNow accepted requestor=%s"),
        HubGameModeUpgradesFlowLogPrefix,
        *GetNameSafe(RequestingPlayerState));

    GetWorldTimerManager().ClearTimer(HubUpgradeVoteTimerHandle);
    HandleHubUpgradeVoteTimerExpired();
}

int32 AEMRHubGameMode::ResolveWinningUpgradeOfferIndex() const
{
    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return INDEX_NONE;
    }

    const FEMRHubUpgradeVoteState& VoteState = RunGS->GetHubUpgradeVoteState();
    const int32 OfferCount = VoteState.OfferedUpgrades.Num();
    if (OfferCount <= 0)
    {
        return INDEX_NONE;
    }

    TArray<int32> LocalVoteCounts = ActiveVoteCounts;
    if (LocalVoteCounts.Num() != OfferCount)
    {
        LocalVoteCounts = VoteState.VoteCounts;
        LocalVoteCounts.SetNum(OfferCount);
    }

    int32 TotalVotes = 0;
    int32 MaxVotes = -1;
    TArray<int32> TopIndices;
    TopIndices.Reserve(OfferCount);

    for (int32 OfferIndex = 0; OfferIndex < OfferCount; ++OfferIndex)
    {
        const int32 VoteCount = FMath::Max(0, LocalVoteCounts[OfferIndex]);
        TotalVotes += VoteCount;

        if (VoteCount > MaxVotes)
        {
            MaxVotes = VoteCount;
            TopIndices.Reset();
            TopIndices.Add(OfferIndex);
        }
        else if (VoteCount == MaxVotes)
        {
            TopIndices.Add(OfferIndex);
        }
    }

    if (TotalVotes <= 0 || TopIndices.IsEmpty())
    {
        const int32 RandomFallbackIndex = FMath::RandRange(0, OfferCount - 1);
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s ResolveWinningUpgradeOfferIndex no votes or no top indices -> random index=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            RandomFallbackIndex);
        return RandomFallbackIndex;
    }

    const int32 RandomTopIndex = FMath::RandRange(0, TopIndices.Num() - 1);
    const int32 WinningIndex = TopIndices[RandomTopIndex];
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s ResolveWinningUpgradeOfferIndex votes resolved totalVotes=%d maxVotes=%d tiedCount=%d winner=%d"),
        HubGameModeUpgradesFlowLogPrefix,
        TotalVotes,
        MaxVotes,
        TopIndices.Num(),
        WinningIndex);
    return WinningIndex;
}

void AEMRHubGameMode::ApplyWinningUpgradeAndStartNightShift()
{
    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s ApplyWinningUpgradeAndStartNightShift aborted: RunGS null."), HubGameModeUpgradesFlowLogPrefix);
        return;
    }

    const FEMRHubUpgradeVoteState VoteState = RunGS->GetHubUpgradeVoteState();
    if (VoteState.OfferedUpgrades.IsValidIndex(VoteState.WinningOfferIndex))
    {
        if (const UEMRRunUpgradeDefinition* WinningUpgrade = VoteState.OfferedUpgrades[VoteState.WinningOfferIndex])
        {
            if (WinningUpgrade->UpgradeTag.IsValid())
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("%s ApplyWinningUpgradeAndStartNightShift applying winning upgrade index=%d tag=%s"),
                    HubGameModeUpgradesFlowLogPrefix,
                    VoteState.WinningOfferIndex,
                    *WinningUpgrade->UpgradeTag.ToString());
                const int32 NewStackCount = RunGS->AddActiveRunUpgradeStack(WinningUpgrade->UpgradeTag, 1);
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("%s ApplyWinningUpgradeAndStartNightShift applied upgrade tag=%s newStack=%d"),
                    HubGameModeUpgradesFlowLogPrefix,
                    *WinningUpgrade->UpgradeTag.ToString(),
                    NewStackCount);
            }
            else
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("%s ApplyWinningUpgradeAndStartNightShift winning upgrade has invalid tag. index=%d asset=%s"),
                    HubGameModeUpgradesFlowLogPrefix,
                    VoteState.WinningOfferIndex,
                    *GetNameSafe(WinningUpgrade));
            }
        }
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s ApplyWinningUpgradeAndStartNightShift invalid winning index=%d offerCount=%d"),
            HubGameModeUpgradesFlowLogPrefix,
            VoteState.WinningOfferIndex,
            VoteState.OfferedUpgrades.Num());
    }

    RunGS->SetHubDecisionStage(EEMRHubDecisionStage::WaitingNightShiftTravel);
    SyncRunStateToSubsystem();

    UEMRNightShiftDefinition* NightShiftToStart = PendingNightShiftSelectionForVote;
    PendingNightShiftSelectionForVote = nullptr;
    VoteIndexByVoterId.Reset();
    ActiveVoteCounts.Reset();

    if (NightShiftToStart)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s ApplyWinningUpgradeAndStartNightShift launching nightshift=%s"),
            HubGameModeUpgradesFlowLogPrefix,
            *GetNameSafe(NightShiftToStart));
        StartNightShift(NightShiftToStart);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s ApplyWinningUpgradeAndStartNightShift no pending nightshift to start."), HubGameModeUpgradesFlowLogPrefix);
    }
}

FString AEMRHubGameMode::BuildStableHubVoterKey(const APlayerState* VoterState) const
{
    if (!VoterState)
    {
        return FString();
    }

    const FUniqueNetIdRepl& UniqueNetId = VoterState->GetUniqueId();
    if (UniqueNetId.IsValid())
    {
        return UniqueNetId->ToString();
    }

    return FString::Printf(TEXT("PlayerState_%s_%p"), *GetNameSafe(VoterState), VoterState);
}

void AEMRHubGameMode::StartNightShift(UEMRNightShiftDefinition* NightShiftDefinition)
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] StartNightShift %s"), NightShiftDefinition ? *NightShiftDefinition->GetName() : TEXT("nullptr"));

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS || !NightShiftDefinition)
    {
        return;
    }

    RunGS->ResetNightShiftSummary();
    RunGS->SetCurrentNightShiftDefinition(NightShiftDefinition);
    RunGS->SetNightShiftOvertimeActive(false);
    RunGS->SetRunPhase(EER_RunPhase::InNightShift);
    UE_LOG(LogTemp, Log, TEXT("[RunPhaseDebug] Hub StartNightShift - RunPhase now %d"), static_cast<int32>(RunGS->GetRunPhase()));

    float NightShiftDuration = 600.f;
    if (UEMRRunRulesSubsystem* RulesSubsystem = GetRunRulesSubsystem())
    {
        NightShiftDuration = FMath::Max(RulesSubsystem->GetNightShiftDurationSeconds(), 1.0f);
        UE_LOG(
            LogTemp,
            Log,
            TEXT("[NightShiftFlow][HubGameMode] StartNightShift - Duration=%.2f (RunRules=%.2f)"),
            NightShiftDuration,
            RulesSubsystem->GetNightShiftDurationSeconds());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][HubGameMode] Missing RunRulesSubsystem; using default duration."));
    }

    RunGS->SetRemainingTimeInNightShift(NightShiftDuration);

    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        ProgressionSubsystem->StoreNightShiftStartSnapshot(RunGS);
    }

    SyncRunStateToSubsystem();

    const bool bTravelTriggered = TravelToNightShiftLevel(NightShiftDefinition);
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] Travel triggered: %s"), bTravelTriggered ? TEXT("YES") : TEXT("NO"));
}


bool AEMRHubGameMode::TravelToNightShiftLevel(UEMRNightShiftDefinition* NightShiftDefinition)
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] TravelToNightShiftLevel requested"));
    if (!NightShiftDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][HubGameMode] Travel aborted: null definition"));
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        return false;
    }

    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        ProgressionSubsystem->SetPendingNightShiftForTravel(NightShiftDefinition);
        ProgressionSubsystem->CacheFromGameState(GetNightShiftGameState());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][HubGameMode] ProgressionSubsystem not found"));
    }

    if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
    {
        SpawnSubsystem->StopNightShiftSpawning();
    }

    const FSoftObjectPath LevelPath = NightShiftDefinition->HospitalLevel.ToSoftObjectPath();
    const FString LevelLongName = LevelPath.GetLongPackageName();

    if (LevelLongName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[NightShiftFlow][HubGameMode] Travel aborted: invalid level for %s"), *NightShiftDefinition->GetName());
        return false;
    }

    const FString SanitizedLevelLongName = UWorld::RemovePIEPrefix(LevelLongName);

    const FString ClientTravelURL = SanitizedLevelLongName;
    const FString ServerTravelURL = SanitizedLevelLongName.Contains(TEXT("?"))
    ? SanitizedLevelLongName + TEXT("&listen")
    : SanitizedLevelLongName + TEXT("?listen");

    RunGS->SetPendingTravelURL(ClientTravelURL);
    NotifyControllersOfPendingTravel(ClientTravelURL);

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] ServerTravel -> %s"), *ServerTravelURL);
    World->ServerTravel(ServerTravelURL, true);
    return true;
}

bool AEMRHubGameMode::TravelToEndSessionHubLevel()
{
    FString LevelLongName;
    if (EndSessionHubLevel.IsNull())
    {
        LevelLongName = TEXT("/Game/EmergencyRoom/Maps/HubMap");
    }
    else
    {
        const FSoftObjectPath LevelPath = EndSessionHubLevel.ToSoftObjectPath();
        LevelLongName = LevelPath.GetLongPackageName();
    }

    if (LevelLongName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HubGameMode] TravelToEndSessionHubLevel aborted: invalid map"));
        return false;
    }

    UWorld* World = GetWorld();
    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!World || !RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HubGameMode] TravelToEndSessionHubLevel aborted World=%s RunGS=%s"),
            *GetNameSafe(World),
            *GetNameSafe(RunGS));
        return false;
    }

    if (!RunGS->GetPendingTravelURL().IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("[HubGameMode] End-session Hub travel skipped because a pending travel already exists (%s)."),
            *RunGS->GetPendingTravelURL());
        return false;
    }

    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetRunProgressionSubsystem())
    {
        ProgressionSubsystem->ResetCachedState();
    }

    const FString SanitizedLevelLongName = UWorld::RemovePIEPrefix(LevelLongName);
    const FString ClientTravelURL = SanitizedLevelLongName;
    const FString ServerTravelURL = SanitizedLevelLongName.Contains(TEXT("?"))
    ? SanitizedLevelLongName + TEXT("&listen")
    : SanitizedLevelLongName + TEXT("?listen");

    RunGS->SetPendingTravelURL(ClientTravelURL);
    NotifyControllersOfPendingTravel(ClientTravelURL);

    UE_LOG(LogTemp, Log, TEXT("[HubGameMode] ServerTravel end-session Hub reset -> %s"), *ServerTravelURL);
    World->ServerTravel(ServerTravelURL, true);
    return true;
}

void AEMRHubGameMode::NotifyControllersOfPendingTravel(const FString& TravelURL)
{
    // ServerTravel is the authoritative travel path; this hook exists for UI/status notifications only.
    (void)TravelURL;
}

void AEMRHubGameMode::ScheduleEndSessionReturnIfNeeded()
{
    if (!HasAuthority() || bEndSessionReturnPending)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] HubGameMode.ScheduleEndSessionReturnIfNeeded skip Auth=%d Pending=%d"),
            HasAuthority() ? 1 : 0,
            bEndSessionReturnPending ? 1 : 0);
        return;
    }

    const AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] HubGameMode.ScheduleEndSessionReturnIfNeeded skip RunGS null"));
        return;
    }

    const bool bIsEndOfSession = RunGS->GetRunPhase() == EER_RunPhase::MissionFinal
    || RunGS->GetRunPhase() == EER_RunPhase::RunFinished;
    if (!bIsEndOfSession)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] HubGameMode.ScheduleEndSessionReturnIfNeeded skip non-end phase=%d"),
            static_cast<int32>(RunGS->GetRunPhase()));
        return;
    }

    bEndSessionReturnPending = true;
    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] HubGameMode.ScheduleEndSessionReturnIfNeeded scheduled delay=%.2f phase=%d"),
        EndSessionReturnDelaySeconds,
        static_cast<int32>(RunGS->GetRunPhase()));

    if (EndSessionReturnDelaySeconds <= 0.0f)
    {
        HandleEndSessionReturnDelayElapsed();
        return;
    }

    GetWorldTimerManager().SetTimer(
        EndSessionReturnTimerHandle,
        this,
        &ThisClass::HandleEndSessionReturnDelayElapsed,
        EndSessionReturnDelaySeconds,
        false);
}

void AEMRHubGameMode::HandleEndSessionReturnDelayElapsed()
{
    bEndSessionReturnPending = false;
    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] HubGameMode.HandleEndSessionReturnDelayElapsed firing travel"));
    TravelToEndSessionHubLevel();
}

void AEMRHubGameMode::HandleRestoredRunState()
{
    AEMRNightShiftGameState* RunGS = GetNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] HubGameMode.HandleRestoredRunState aborted RunGS null"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] HubGameMode.HandleRestoredRunState phase=%d cycle=%d nightShift=%d revenue=%.2f quota=%.2f pendingTravel=%s"),
        static_cast<int32>(RunGS->GetRunPhase()),
        RunGS->GetCurrentCycleIndex(),
        RunGS->GetNightShiftIndexInCycle(),
        RunGS->GetTotalRevenue(),
        RunGS->GetCurrentCycleQuota(),
        RunGS->GetPendingTravelURL().IsEmpty() ? TEXT("<none>") : *RunGS->GetPendingTravelURL());

    switch (RunGS->GetRunPhase())
    {
    case EER_RunPhase::Hub:
        UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] Resuming in Hub phase"));

        if (RunGS->GetAvailableNextNightShifts().Num() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] NightShifts already prepared (%d), skipping"),
                RunGS->GetAvailableNextNightShifts().Num());
        }
        else
        {
            PrepareNextNightShiftSelection();
        }
        break;

    case EER_RunPhase::MissionFinal:
    case EER_RunPhase::RunFinished:
        UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] Resuming end-of-session phase %d"), static_cast<int32>(RunGS->GetRunPhase()));
        ScheduleEndSessionReturnIfNeeded();
        break;

    default:
        UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][HubGameMode] Resuming phase %d"), static_cast<int32>(RunGS->GetRunPhase()));
        break;
    }
}

AEMRHubertDirector* AEMRHubGameMode::ResolveHubertDirector()
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
        UE_LOG(LogTemp, Warning, TEXT("[Hubert] HubGameMode has no HubertDirectorClass and no placed director in map."));
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = nullptr;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEMRHubertDirector* SpawnedDirector = World->SpawnActor<AEMRHubertDirector>(HubertDirectorClass, FTransform::Identity, SpawnParameters);
    if (!SpawnedDirector)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Hubert] HubGameMode failed to spawn director class=%s"), *GetNameSafe(HubertDirectorClass));
        return nullptr;
    }

    HubertDirector = SpawnedDirector;
    return SpawnedDirector;
}

AEMRHubGameState* AEMRHubGameMode::GetHubGameState() const
{
    return GetGameState<AEMRHubGameState>();
}

void AEMRHubGameMode::HandleHubSlotRegistered(AEMRHubPlayerSlot* SlotActor)
{
    if (!HasAuthority() || !SlotActor)
    {
        return;
    }

    TryAssignPendingPlayers();
}

void AEMRHubGameMode::AssignSlotsForExistingPlayers()
{
    if (!HasAuthority())
    {
        return;
    }

    AEMRHubGameState* HubGameState = GetHubGameState();
    if (!HubGameState || !IsRunPhaseEligibleForHubSlotAssignment(HubGameState->GetRunPhase()))
    {
        return;
    }

    for (APlayerState* PlayerState : HubGameState->PlayerArray)
    {
        if (!PlayerState)
        {
            continue;
        }

        if (HubGameState->FindSlotForPlayer(PlayerState))
        {
            RemovePendingPlayerState(PlayerState);
            continue;
        }

        if (!HubGameState->AssignSlotToPlayer(PlayerState))
        {
            PendingPlayerStates.AddUnique(PlayerState);
            continue;
        }

        RemovePendingPlayerState(PlayerState);
    }
}

void AEMRHubGameMode::RemovePendingPlayerState(APlayerState* PlayerState)
{
    if (!PlayerState)
    {
        return;
    }

    PendingPlayerStates.Remove(PlayerState);
}

void AEMRHubGameMode::TryAssignPendingPlayers()
{
    if (!HasAuthority())
    {
        return;
    }

    AEMRHubGameState* HubGameState = GetHubGameState();
    if (!HubGameState || !IsRunPhaseEligibleForHubSlotAssignment(HubGameState->GetRunPhase()))
    {
        return;
    }

    for (int32 Index = PendingPlayerStates.Num() - 1; Index >= 0; --Index)
    {
        APlayerState* PlayerState = PendingPlayerStates[Index].Get();
        if (!PlayerState)
        {
            PendingPlayerStates.RemoveAt(Index);
            continue;
        }

        if (HubGameState->AssignSlotToPlayer(PlayerState))
        {
            PendingPlayerStates.RemoveAt(Index);
        }
    }
}

bool AEMRHubGameMode::IsRunPhaseEligibleForHubSlotAssignment(const EER_RunPhase InRunPhase) const
{
    return InRunPhase == EER_RunPhase::Hub
    || InRunPhase == EER_RunPhase::MissionFinal
    || InRunPhase == EER_RunPhase::RunFinished;
}
