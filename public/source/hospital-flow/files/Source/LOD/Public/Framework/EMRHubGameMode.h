#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameModeBase.h"
#include "EMRHubGameMode.generated.h"

class AEMRPatient;
class UEMRNightShiftDefinition;
class AEMRNightShiftGameState;
class UEMRRunProgressionSubsystem;
class UEMRRunRulesSubsystem;
class UEMRPatientPoolSubsystem;
class AEMRPlayerController;
class UWorld;
class UEMRLobbyCharacterCatalog;
class AEMRHubGameState;
class AEMRHubPlayerSlot;
class AController;
class APlayerState;
class UEMRRunUpgradeDefinition;
class AEMRHubertDirector;
struct FEMRDeveloperToolUpgradeOption;
enum class EER_RunPhase : uint8;

/**
 * GameMode used in the Hub level to manage the run loop (cycle setup, mission selection,
 * travel to NightShift levels).
 */
UCLASS()
class LOD_API AEMRHubGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
    AEMRHubGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void HandleSeamlessTravelPlayer(AController*& C) override;
    virtual void Logout(AController* Exiting) override;

    /** Ask the GameMode to start a NightShift using the selected definition. */
    UFUNCTION(BlueprintCallable, Category="EMR|Server")
    void StartNightShiftFromSelection(UEMRNightShiftDefinition* SelectedNightShift);

    void CastOrUpdateUpgradeVote(APlayerState* VoterState, int32 OfferIndex);
    void RequestStartHubUpgradeVoteNow(APlayerState* RequestingPlayerState);
    bool RerollActiveHubUpgradeVoteOffersForTests(FString& OutMessage);
    bool ApplySpecificRunUpgradeForTests(FGameplayTag UpgradeTag, FString& OutMessage);
    bool CollectRunUpgradeOptionsForTests(TArray<FEMRDeveloperToolUpgradeOption>& OutOptions, FString& OutMessage) const;

    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

protected:
    AEMRNightShiftGameState* GetNightShiftGameState() const;
    UEMRRunProgressionSubsystem* GetRunProgressionSubsystem() const;
    UEMRRunRulesSubsystem* GetRunRulesSubsystem() const;
    void SyncRunStateToSubsystem();
    UClass* ResolvePlayerStateClass() const;

    void StartNewRun();
    void SetupCycle(int32 CycleIndex);

    /** Build the list of available NightShifts for the next selection in the hub. */
    void PrepareNextNightShiftSelection();

    void StartHubUpgradeVote(UEMRNightShiftDefinition* SelectedNightShift);
    void HandleHubUpgradeVoteTimerExpired();
    int32 ResolveWinningUpgradeOfferIndex() const;
    void RefreshHubUpgradeVoteParticipationState();
    void RecomputeHubUpgradeVoteParticipationCounts(int32& OutRequiredVoterCount, int32& OutSubmittedVoterCount) const;
    bool CanStartHubUpgradeVoteNow() const;
    void ApplyWinningUpgradeAndStartNightShift();
    FString BuildStableHubVoterKey(const APlayerState* VoterState) const;

    void StartNightShift(UEMRNightShiftDefinition* NightShiftDefinition);
    bool TravelToNightShiftLevel(UEMRNightShiftDefinition* NightShiftDefinition);
    bool TravelToEndSessionHubLevel();
    void NotifyControllersOfPendingTravel(const FString& TravelURL);
    void ScheduleEndSessionReturnIfNeeded();
    void HandleEndSessionReturnDelayElapsed();

    void HandleRestoredRunState();

    void HandleHubSlotRegistered(AEMRHubPlayerSlot* SlotActor);
    void AssignSlotsForExistingPlayers();
    void RemovePendingPlayerState(APlayerState* PlayerState);
    void TryAssignPendingPlayers();
    bool IsRunPhaseEligibleForHubSlotAssignment(EER_RunPhase InRunPhase) const;
    AEMRHubertDirector* ResolveHubertDirector();

    AEMRHubGameState* GetHubGameState() const;

    /** Pool of NightShift definitions (contracts) that can be offered to the players in the hub. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Config")
    TArray<TSoftObjectPtr<UEMRNightShiftDefinition>> NightShiftDefinitionsPool;

    /** Avoid immediately re-offering the same NightShifts between two consecutive selections. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Config")
    bool bAvoidRepeatingLastSelection = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Players")
    TSoftObjectPtr<UEMRLobbyCharacterCatalog> PlayerCharacterCatalog;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Config|Upgrades")
    TArray<TSoftObjectPtr<UEMRRunUpgradeDefinition>> RunUpgradePool;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Config|Upgrades", meta = (ClampMin = "1.0"))
    float HubUpgradeVoteDurationSeconds = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Travel")
    TSoftObjectPtr<UWorld> EndSessionHubLevel;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Travel", meta = (DeprecatedProperty, DeprecationMessage = "Use EndSessionHubLevel"))
    TSoftObjectPtr<UWorld> EndSessionLobbyLevel;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Travel", meta = (ClampMin = "0.0"))
    float EndSessionReturnDelaySeconds = 4.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert")
    TSubclassOf<AEMRHubertDirector> HubertDirectorClass;

private:
    const UEMRLobbyCharacterCatalog* GetPlayerCharacterCatalog() const;

    /** Cache of the last NightShift selection (not replicated). Used to avoid immediate re-rolls. */
    TArray<TWeakObjectPtr<UEMRNightShiftDefinition>> LastNightShiftSelection;

    UPROPERTY()
    TArray<TWeakObjectPtr<APlayerState>> PendingPlayerStates;

    UPROPERTY()
    mutable TWeakObjectPtr<UEMRRunRulesSubsystem> RunRulesSubsystem;

    UPROPERTY()
    mutable TWeakObjectPtr<const UEMRLobbyCharacterCatalog> CachedPlayerCharacterCatalog;

    UPROPERTY()
    TObjectPtr<UEMRNightShiftDefinition> PendingNightShiftSelectionForVote = nullptr;

    UPROPERTY()
    TMap<FString, int32> VoteIndexByVoterId;

    UPROPERTY()
    TArray<int32> ActiveVoteCounts;

    FTimerHandle EndSessionReturnTimerHandle;
    FTimerHandle HubUpgradeVoteTimerHandle;
    bool bEndSessionReturnPending = false;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRHubertDirector> HubertDirector;
};

