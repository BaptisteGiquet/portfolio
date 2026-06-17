#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameState.h"
#include "Subsystems/EMRNightShiftSpawnSubsystem.h"
#include "Subsystems/EMRPatientPoolSubsystem.h"
#include "Telemetry/EMRGameplayTelemetryTypes.h"
#include "EMRNightShiftGameState.generated.h"


class UEMRNightShiftDefinition;
class UEMRRunUpgradeDefinition;
class UGameplayEffect;
class UEMRAbilitySystemComponent;
class UEMRTeamAttributeSet;
class UEMRNightShiftSpawnSubsystem;
class UEMRPatientPoolSubsystem;
class UCurveFloat;
class UEMRDifficultyTuningData;
class USoundBase;
class AEMRPatient;
struct FOnAttributeChangeData;
enum class EEMRSpecialEventPhase : uint8;
enum class EEMRHubDecisionStage : uint8;

DECLARE_MULTICAST_DELEGATE(FOnNextNightShiftsAvailableChanged);
DECLARE_MULTICAST_DELEGATE(FOnNightShiftSelectionLockedChanged);
DECLARE_MULTICAST_DELEGATE(FOnNightShiftSelectionCommittedChanged);
DECLARE_MULTICAST_DELEGATE(FOnPendingTravelChanged);
DECLARE_MULTICAST_DELEGATE(FOnNightShiftOvertimeStarted);
DECLARE_MULTICAST_DELEGATE(FOnNightShiftTimeExpired);
DECLARE_MULTICAST_DELEGATE(FOnNightShiftFailedByReputation);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRunPhaseChanged, EER_RunPhase /*NewPhase*/, EER_RunPhase /*PreviousPhase*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNightShiftSummaryChanged, const struct FEMRNightShiftSummary&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNightShiftTelemetryPublished, const FEMRNightShiftTelemetryRecord&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSessionInviteCodeChanged, const FString&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCycleInfoUpdated, int32 /*CycleIndex*/, float /*CycleQuota*/, float /*CycleStartRevenue*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSpecialEventPhaseChanged, EEMRSpecialEventPhase /*NewPhase*/, FName /*EventId*/, float /*PhaseServerTimestamp*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHubDecisionStageChanged, EEMRHubDecisionStage /*NewStage*/, EEMRHubDecisionStage /*PreviousStage*/);
DECLARE_MULTICAST_DELEGATE(FOnHubUpgradeVoteStateChanged);
DECLARE_MULTICAST_DELEGATE(FOnActiveRunUpgradeTagsChanged);

USTRUCT(BlueprintType)
struct FEMRNightShiftSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="EMR|NightShift")
    int32 PatientsTreated = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|NightShift")
    int32 PatientsLeftWithoutPay = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|NightShift")
    float NightShiftRevenue = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|NightShift")
    float RevenueToCycleQuota = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|NightShift")
    bool bIsValid = false;
};

UENUM(BlueprintType)
enum class EER_RunPhase : uint8
{
	Hub         UMETA(DisplayName = "Hub"),
	InNightShift     UMETA(DisplayName = "In NightShift"),
	InterNightShift  UMETA(DisplayName = "Between NightShift"),
	EndOfCycle  UMETA(DisplayName = "End of cycle"),
	MissionFinal UMETA(DisplayName = "Final mission"),
	RunFinished UMETA(DisplayName = "Run finished")
};

UENUM(BlueprintType)
enum class EEMRHubDecisionStage : uint8
{
    NightShiftSelection UMETA(DisplayName = "NightShift Selection"),
    UpgradeVoting UMETA(DisplayName = "Upgrade Voting"),
    WaitingNightShiftTravel UMETA(DisplayName = "Waiting NightShift Travel")
};

USTRUCT(BlueprintType)
struct FEMRHubUpgradeVoteState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote")
    TArray<TObjectPtr<UEMRRunUpgradeDefinition>> OfferedUpgrades;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote")
    TArray<int32> VoteCounts;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote")
    float VoteEndServerTimeSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote")
    bool bVoteActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote")
    int32 WinningOfferIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote")
    int32 RequiredVoterCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote")
    int32 SubmittedVoterCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote")
    bool bAllConnectedPlayersVoted = false;
};

UENUM(BlueprintType)
enum class EEMRSpecialEventPhase : uint8
{
    None      UMETA(DisplayName = "None"),
    Alert     UMETA(DisplayName = "Alert"),
    Active    UMETA(DisplayName = "Active"),
    Completed UMETA(DisplayName = "Completed")
};


UCLASS()
class LOD_API AEMRNightShiftGameState : public AGameStateBase, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AEMRNightShiftGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void Tick(float DeltaSeconds) override;
	
	
    UFUNCTION(BlueprintCallable, Category="EMR|Economy")
    float GetTotalRevenue() const;

    void SetTotalRevenue(float NewTotalRevenue);

    UFUNCTION(BlueprintCallable, Category="EMR|Economy")
    float GetReputation() const;

    UFUNCTION(BlueprintCallable, Category="EMR|Economy")
    float GetCurrentCycleRevenue() const;
	
    UFUNCTION(BlueprintCallable, Category="EMR|State")
    EER_RunPhase GetRunPhase() const { return RunPhase; }

    UFUNCTION(BlueprintCallable, Category="EMR|Session|Invite")
    const FString& GetSessionInviteCode() const { return SessionInviteCode; }

    UFUNCTION(BlueprintCallable, Category="EMR|State")
    bool HasRunFailed() const { return bRunFailed; }

    UFUNCTION(BlueprintCallable, Category="EMR|State")
    bool IsFinalMissionUnlocked() const { return bFinalMissionUnlocked; }

    UFUNCTION(BlueprintCallable, Category="EMR|Cycle")
    int32 GetCurrentCycleIndex() const { return CurrentCycleIndex; }

    UFUNCTION(BlueprintCallable, Category="EMR|Cycle")
    int32 GetNightShiftIndexInCycle() const { return NightShiftIndexInCycle; }

    UFUNCTION(BlueprintCallable, Category="EMR|Cycle")
    float GetCurrentCycleQuota() const { return CurrentCycleQuota; }

    UFUNCTION(BlueprintCallable, Category="EMR|Cycle")
    float GetCurrentCycleStartRevenue() const { return CurrentCycleStartRevenue; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|Summary")
    const FEMRNightShiftSummary& GetLastNightShiftSummary() const { return LastNightShiftSummary; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|Summary")
    bool HasLastNightShiftSummary() const { return LastNightShiftSummary.bIsValid; }

    const FEMRNightShiftTelemetryRecord& GetLastNightShiftTelemetryRecord() const { return LastNightShiftTelemetryRecord; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    float GetRemainingTimeInNightShift() const { return RemainingTimeInNightShift; }


    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    UEMRNightShiftDefinition* GetCurrentNightShiftDefinition() const { return CurrentNightShiftDefinition; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    bool IsInNightShiftOvertime() const { return bIsInNightShiftOvertime; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    float GetNightShiftOvertimeElapsedSeconds() const { return NightShiftOvertimeElapsedSeconds; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    float GetOvertimeDifficultyMultiplier() const;

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    float GetOvertimeSpawnRateMultiplier() const;

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    float GetOvertimePatienceDrainMultiplier() const;

    UFUNCTION(BlueprintPure, Category="EMR|Difficulty")
    const UEMRDifficultyTuningData* GetDifficultyTuningData() const;

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|SpecialEvent")
    EEMRSpecialEventPhase GetSpecialEventPhase() const { return SpecialEventPhase; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|SpecialEvent")
    FName GetActiveSpecialEventId() const { return ActiveSpecialEventId; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|SpecialEvent")
    float GetSpecialEventPhaseServerTimestamp() const { return SpecialEventPhaseServerTimestamp; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|SpecialEvent")
    FText GetSpecialEventAlertTitle() const { return SpecialEventAlertTitle; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|SpecialEvent")
    FText GetSpecialEventAlertDescription() const { return SpecialEventAlertDescription; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|SpecialEvent")
    FName GetSpecialEventFlickerLightTag() const { return SpecialEventFlickerLightTag; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|SpecialEvent")
    FLinearColor GetSpecialEventFlickerColor() const { return SpecialEventFlickerColor; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift|SpecialEvent")
    float GetSpecialEventLightFlickerRateHz() const { return SpecialEventLightFlickerRateHz; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    TArray<UEMRNightShiftDefinition*> GetAvailableNextNightShifts() const;

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    bool IsNightShiftSelectionLocked() const { return bNightShiftSelectionLocked; }

    UFUNCTION(BlueprintCallable, Category="EMR|NightShift")
    bool IsNightShiftSelectionCommitted() const { return bNightShiftSelectionCommitted; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Hub|UpgradeVote")
    EEMRHubDecisionStage GetHubDecisionStage() const { return HubDecisionStage; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Hub|UpgradeVote")
    const FEMRHubUpgradeVoteState& GetHubUpgradeVoteState() const { return HubUpgradeVoteState; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Run|Upgrades")
    const FGameplayTagContainer& GetActiveRunUpgradeTags() const { return ActiveRunUpgradeTags; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Run|Upgrades")
    const TMap<FGameplayTag, int32>& GetActiveRunUpgradeStacks() const { return ActiveRunUpgradeStacks; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Run|Upgrades")
    bool HasActiveRunUpgradeTag(FGameplayTag UpgradeTag) const;

    UFUNCTION(BlueprintCallable, Category = "EMR|Run|Upgrades")
    int32 GetActiveRunUpgradeStackCount(FGameplayTag UpgradeTag) const;

    const TArray<TSubclassOf<AEMRPatient>>& GetPatientBlueprintClasses() const { return PatientBlueprintClasses; }

    void SetNightShiftSelectionLocked(bool bLocked);
    void SetNightShiftSelectionCommitted(bool bCommitted);

    FOnNextNightShiftsAvailableChanged& OnNextNightShiftsAvailableChanged() { return NextNightShiftsChangedDelegate; }
    FOnNightShiftSelectionLockedChanged& OnNightShiftSelectionLockedChanged() { return NightShiftSelectionLockedDelegate; }
    FOnNightShiftSelectionCommittedChanged& OnNightShiftSelectionCommittedChanged() { return NightShiftSelectionCommittedDelegate; }
    FOnPendingTravelChanged& OnPendingTravelChanged() { return PendingTravelChangedDelegate; }
    FOnNightShiftOvertimeStarted& OnNightShiftOvertimeStarted() { return NightShiftOvertimeStartedDelegate; }
    FOnNightShiftTimeExpired& OnNightShiftTimeExpired() { return NightShiftTimeExpiredDelegate; }
    FOnNightShiftFailedByReputation& OnNightShiftFailedByReputation() { return NightShiftFailedByReputationDelegate; }
    FOnRunPhaseChanged& OnRunPhaseChanged() { return RunPhaseChangedDelegate; }
    FOnSessionInviteCodeChanged& OnSessionInviteCodeChanged() { return SessionInviteCodeChangedDelegate; }
    FOnSpecialEventPhaseChanged& OnSpecialEventPhaseChanged() { return SpecialEventPhaseChangedDelegate; }
    FOnHubDecisionStageChanged& OnHubDecisionStageChanged() { return HubDecisionStageChangedDelegate; }
    FOnHubUpgradeVoteStateChanged& OnHubUpgradeVoteStateChanged() { return HubUpgradeVoteStateChangedDelegate; }
    FOnActiveRunUpgradeTagsChanged& OnActiveRunUpgradeTagsChanged() { return ActiveRunUpgradeTagsChangedDelegate; }

	
	// Functions call on GameMode
    void SetRunPhase(EER_RunPhase NewPhase);
    void SetSessionInviteCode(const FString& InCode);
    void SetRunFailed(bool bFailed);
    void SetFinalMissionUnlocked(bool bUnlocked);

    void SetCurrentCycleInfo(int32 NewCycleIndex, float NewCycleQuota, float NewCycleStartRevenue);
    void SetNightShiftIndexInCycle(int32 NewNightShiftIndex);

    void SetRemainingTimeInNightShift(float NewRemainingTime);

    void SetCurrentNightShiftDefinition(UEMRNightShiftDefinition* NewNightShift);

    void SetNightShiftOvertimeActive(bool bInOvertime);
    void SetNightShiftTimeExpired(bool bExpired);
    bool IsNightShiftTimeExpired() const { return bNightShiftTimeExpired; }
    void SetNightShiftFailedByReputation(bool bFailedByReputation);
    bool IsNightShiftFailedByReputation() const { return bNightShiftFailedByReputation; }
    void SetReputationLockedAtZero(bool bLocked);
    bool IsReputationLockedAtZero() const { return bReputationLockedAtZero; }
    void SetReputationFrozen(bool bFrozen, float InFrozenReputation);
    bool IsReputationFrozen() const { return bReputationFrozen; }
    float GetFrozenReputation() const { return FrozenReputation; }
    void ResetNightShiftEndState();
    void ResetNightShiftOvertimeProgress();
    void SetAvailableNextNightShifts(const TArray<UEMRNightShiftDefinition*>& NewList);
    void SetHubDecisionStage(EEMRHubDecisionStage NewStage);
    void SetHubUpgradeVoteState(const FEMRHubUpgradeVoteState& NewState);
    void SetActiveRunUpgradeTags(const FGameplayTagContainer& NewTags);
    void SetActiveRunUpgradeStacks(const TMap<FGameplayTag, int32>& NewStacks);
    bool AddActiveRunUpgradeTag(FGameplayTag UpgradeTag);
    int32 AddActiveRunUpgradeStack(FGameplayTag UpgradeTag, int32 StackDelta = 1);
    void SetSpecialEventPhase(EEMRSpecialEventPhase NewPhase, FName EventId, float PhaseServerTimestamp);
    void SetSpecialEventPresentationData(
        const FText& AlertTitle,
        const FText& AlertDescription,
        FName FlickerLightTag,
        const FLinearColor& FlickerColor,
        float LightFlickerRateHz);
    void ClearSpecialEventPresentationData();

    void ClearNightShiftSelectionLock();

    void SetPendingTravelURL(const FString& NewURL);
    void ClearPendingTravelURL();
    FString GetPendingTravelURL() const { return PendingTravelURL; }

    void ResetNightShiftSummary();
    void RegisterPatientPaid();
    void RegisterPatientLeftWithoutPay();
    void SetLastNightShiftSummary(const FEMRNightShiftSummary& Summary);
    void SetLastNightShiftTelemetryRecord(const FEMRNightShiftTelemetryRecord& Record);
    FOnNightShiftSummaryChanged& OnNightShiftSummaryChanged() { return NightShiftSummaryChangedDelegate; }
    FOnNightShiftTelemetryPublished& OnNightShiftTelemetryPublished() { return NightShiftTelemetryPublishedDelegate; }
    FOnCycleInfoUpdated& OnCycleInfoUpdated() { return CycleInfoUpdatedDelegate; }

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayGlobalSound2D(USoundBase* Sound);

	UPROPERTY(ReplicatedUsing = OnRep_NightShiftSelectionLocked, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
	bool bNightShiftSelectionLocked = false;

	UPROPERTY(ReplicatedUsing = OnRep_NightShiftSelectionCommitted, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
	bool bNightShiftSelectionCommitted = false;

    UPROPERTY(ReplicatedUsing = OnRep_HubDecisionStage, BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote", meta = (AllowPrivateAccess = "true"))
    EEMRHubDecisionStage HubDecisionStage = EEMRHubDecisionStage::NightShiftSelection;

    UPROPERTY(ReplicatedUsing = OnRep_HubUpgradeVoteState, BlueprintReadOnly, Category = "EMR|Hub|UpgradeVote", meta = (AllowPrivateAccess = "true"))
    FEMRHubUpgradeVoteState HubUpgradeVoteState;

    UPROPERTY(ReplicatedUsing = OnRep_ActiveRunUpgradeTags, BlueprintReadOnly, Category = "EMR|Run|Upgrades", meta = (AllowPrivateAccess = "true"))
    FGameplayTagContainer ActiveRunUpgradeTags;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Run|Upgrades", meta = (AllowPrivateAccess = "true"))
    TMap<FGameplayTag, int32> ActiveRunUpgradeStacks;

    UPROPERTY(ReplicatedUsing=OnRep_PendingTravelURL, BlueprintReadOnly, Category="EMR|Travel", meta=(AllowPrivateAccess="true"))
    FString PendingTravelURL;

    UPROPERTY(ReplicatedUsing=OnRep_SessionInviteCode, BlueprintReadOnly, Category="EMR|Session|Invite", meta=(AllowPrivateAccess="true"))
    FString SessionInviteCode;

    UPROPERTY(ReplicatedUsing=OnRep_LastNightShiftSummary, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    FEMRNightShiftSummary LastNightShiftSummary;

    UPROPERTY(ReplicatedUsing=OnRep_LastNightShiftTelemetryRecord, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    FEMRNightShiftTelemetryRecord LastNightShiftTelemetryRecord;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "EMR|ASC")
	TObjectPtr<UEMRAbilitySystemComponent> TeamASC;

	UPROPERTY(VisibleAnywhere, Category = "EMR|ASC")
	TObjectPtr<UEMRTeamAttributeSet> TeamAttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|ASC")
	TArray<TSubclassOf<UGameplayEffect>> InitTeamEffects;

	void HandleTeamReputationChanged(const FOnAttributeChangeData& Data);

	FDelegateHandle TeamReputationChangedHandle;

	// Phase and run state
	
	UPROPERTY(ReplicatedUsing = OnRep_RunPhase, BlueprintReadOnly, Category="EMR|State", meta=(AllowPrivateAccess="true"))
	EER_RunPhase RunPhase = EER_RunPhase::Hub;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|State", meta=(AllowPrivateAccess="true"))
	bool bRunFailed = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|State", meta=(AllowPrivateAccess="true"))
	bool bFinalMissionUnlocked = false;

	// Cycles and quotas
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|Cycle", meta=(AllowPrivateAccess="true"))
	int32 CurrentCycleIndex = 0;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|Cycle", meta=(AllowPrivateAccess="true"))
	int32 NightShiftIndexInCycle = 0;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentCycleQuota, BlueprintReadOnly, Category="EMR|Cycle", meta=(AllowPrivateAccess="true"))
	float CurrentCycleQuota = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|Cycle", meta=(AllowPrivateAccess="true"))
	float CurrentCycleStartRevenue = 0.f;

	// Current night shift

	UPROPERTY(ReplicatedUsing=OnRep_RemainingTimeInNightShift, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
	float RemainingTimeInNightShift = 0.f;



    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UEMRNightShiftDefinition> CurrentNightShiftDefinition = nullptr;

    UPROPERTY(ReplicatedUsing=OnRep_NightShiftOvertime, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    bool bIsInNightShiftOvertime = false;

    UPROPERTY(ReplicatedUsing=OnRep_NightShiftTimeExpired, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    bool bNightShiftTimeExpired = false;

    UPROPERTY(ReplicatedUsing=OnRep_NightShiftFailedByReputation, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    bool bNightShiftFailedByReputation = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    bool bReputationLockedAtZero = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    bool bReputationFrozen = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    float FrozenReputation = 0.f;

    /** Seconds elapsed since overtime started. Resets when overtime ends. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    float NightShiftOvertimeElapsedSeconds = 0.f;

    UPROPERTY(ReplicatedUsing=OnRep_SpecialEventPhase, BlueprintReadOnly, Category="EMR|NightShift|SpecialEvent", meta=(AllowPrivateAccess="true"))
    EEMRSpecialEventPhase SpecialEventPhase = EEMRSpecialEventPhase::None;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift|SpecialEvent", meta=(AllowPrivateAccess="true"))
    FName ActiveSpecialEventId = NAME_None;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift|SpecialEvent", meta=(AllowPrivateAccess="true"))
    float SpecialEventPhaseServerTimestamp = 0.f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift|SpecialEvent", meta=(AllowPrivateAccess="true"))
    FText SpecialEventAlertTitle;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift|SpecialEvent", meta=(AllowPrivateAccess="true"))
    FText SpecialEventAlertDescription;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift|SpecialEvent", meta=(AllowPrivateAccess="true"))
    FName SpecialEventFlickerLightTag = NAME_None;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift|SpecialEvent", meta=(AllowPrivateAccess="true"))
    FLinearColor SpecialEventFlickerColor = FLinearColor::Red;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="EMR|NightShift|SpecialEvent", meta=(AllowPrivateAccess="true"))
    float SpecialEventLightFlickerRateHz = 1.0f;

    UPROPERTY(ReplicatedUsing=OnRep_NextNightShiftsAvailable, BlueprintReadOnly, Category="EMR|NightShift", meta=(AllowPrivateAccess="true"))
    TArray<TObjectPtr<UEMRNightShiftDefinition>> NextNightShiftsAvailable;

    FOnCycleInfoUpdated CycleInfoUpdatedDelegate;
    FOnRunPhaseChanged RunPhaseChangedDelegate;

    FOnNextNightShiftsAvailableChanged NextNightShiftsChangedDelegate;
    FOnNightShiftSelectionLockedChanged NightShiftSelectionLockedDelegate;
    FOnNightShiftSelectionCommittedChanged NightShiftSelectionCommittedDelegate;
    FOnPendingTravelChanged PendingTravelChangedDelegate;
    FOnNightShiftOvertimeStarted NightShiftOvertimeStartedDelegate;
    FOnNightShiftTimeExpired NightShiftTimeExpiredDelegate;
    FOnNightShiftFailedByReputation NightShiftFailedByReputationDelegate;
    FOnNightShiftSummaryChanged NightShiftSummaryChangedDelegate;
    FOnNightShiftTelemetryPublished NightShiftTelemetryPublishedDelegate;
    FOnSessionInviteCodeChanged SessionInviteCodeChangedDelegate;
    FOnSpecialEventPhaseChanged SpecialEventPhaseChangedDelegate;
    FOnHubDecisionStageChanged HubDecisionStageChangedDelegate;
    FOnHubUpgradeVoteStateChanged HubUpgradeVoteStateChangedDelegate;
    FOnActiveRunUpgradeTagsChanged ActiveRunUpgradeTagsChangedDelegate;

    void RebuildActiveRunUpgradeTagsFromStacks();

	// OnRep handlers for UI

	UFUNCTION()
	void OnRep_RunPhase(EER_RunPhase PreviousPhase);

	UFUNCTION()
	void OnRep_CurrentCycleQuota();

	UFUNCTION()
	void OnRep_RemainingTimeInNightShift();

	UFUNCTION()
	void OnRep_NightShiftOvertime();

	UFUNCTION()
	void OnRep_NightShiftTimeExpired();

	UFUNCTION()
	void OnRep_NightShiftFailedByReputation();

	UFUNCTION()
	void OnRep_SpecialEventPhase(EEMRSpecialEventPhase PreviousPhase);

	UFUNCTION()
	void OnRep_NextNightShiftsAvailable();

public:
	UFUNCTION()
	void OnRep_NightShiftSelectionLocked();

	UFUNCTION()
	void OnRep_NightShiftSelectionCommitted();

	UFUNCTION()
	void OnRep_HubDecisionStage(EEMRHubDecisionStage PreviousStage);

	UFUNCTION()
	void OnRep_HubUpgradeVoteState();

	UFUNCTION()
	void OnRep_ActiveRunUpgradeTags();

	void OnRep_ActiveRunUpgradeStacks();

	UFUNCTION()
	void OnRep_PendingTravelURL();

	UFUNCTION()
	void OnRep_SessionInviteCode();

	UFUNCTION()
	void OnRep_LastNightShiftSummary();

	UFUNCTION()
	void OnRep_LastNightShiftTelemetryRecord();

	// Debug info

	UFUNCTION(BlueprintCallable, Category="EMR|Debug")
	bool HasReplicatedSpawnDebugInfo() const { return bHasReplicatedSpawnDebugInfo; }
	const FEMRNightShiftSpawnDebugInfo& GetReplicatedSpawnDebugInfo() const { return ReplicatedSpawnDebugInfo; }
	bool HasReplicatedPatientPoolDebugInfo() const { return bHasReplicatedPatientPoolDebugInfo; }
	const FEMRPatientPoolDebugInfo& GetReplicatedPatientPoolDebugInfo() const { return ReplicatedPatientPoolDebugInfo; }

    UFUNCTION(BlueprintPure, Category="EMR|Spawn")
    FVector GetPatientPoolLocation() const { return PatientPoolLocation; }

	const UEMRDifficultyTuningData* GetDifficultyTuning() const;

private:
    void UpdateDebugSnapshots();

    float CachedPatienceDrainMultiplier = 1.0f;

    UPROPERTY(Replicated)
    FEMRNightShiftSpawnDebugInfo ReplicatedSpawnDebugInfo;

    UPROPERTY(Replicated)
    FEMRPatientPoolDebugInfo ReplicatedPatientPoolDebugInfo;

    UPROPERTY(Replicated)
    bool bHasReplicatedSpawnDebugInfo = false;

    UPROPERTY(Replicated)
    bool bHasReplicatedPatientPoolDebugInfo = false;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Patients")
    TArray<TSubclassOf<AEMRPatient>> PatientBlueprintClasses;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Spawn")
    FVector PatientPoolLocation = FVector(-20000.f, -20000.f, -20000.f);

    UPROPERTY(EditDefaultsOnly, Category="EMR|Debug")
    float DebugSnapshotInterval = 0.5f;

    float DebugSnapshotAccumulator = 0.f;

    float GetDifficultySpecificOvertimeScalar(const TFunction<float(const UEMRDifficultyTuningData&, EEMRNightShiftDifficultyTier)>& Getter) const;


    void LoadDifficultyTuning() const;

    UPROPERTY(Transient)
    mutable TWeakObjectPtr<const UEMRDifficultyTuningData> CachedDifficultyTuning;
};
