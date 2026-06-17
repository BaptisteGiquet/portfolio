#pragma once

#include "CoreMinimal.h"
#include "Data/EMRNightShiftDefinition.h"
#include "GameFramework/GameModeBase.h"
#include "Telemetry/EMRGameplayTelemetryTypes.h"
#include "TimerManager.h"
#include "EMRNightShiftGameMode.generated.h"

class AEMRPatient;
class UEMRNightShiftDefinition;
class AEMRNightShiftGameState;
class UEMRRunProgressionSubsystem;
class UEMRRunRulesSubsystem;
class UEMRNightShiftSpawnSubsystem;
class UEMRPatientPoolSubsystem;
class UEMRExamQueueSubsystem;
class UEMRTreatmentSubsystem;
class AEMRPlayerController;
class UEMRDifficultyTuningData;
class UEMRSpecialEventDefinition;
class UWorld;
class USoundBase;
class AEMRHubertDirector;
struct FOnAttributeChangeData;
class UEMRLobbyCharacterCatalog;

enum class EEMRPatientLeaveReason : uint8
{
    OutOfPatience,
    OvertimeStopShutdown
};

USTRUCT()
struct FEMRSpecialEventRuntimeState
{
    GENERATED_BODY()

    bool bScheduled = false;
    bool bHasAlertStarted = false;
    bool bHasEventStarted = false;
    bool bHasEventEnded = false;

    FName EventId = NAME_None;
    float AlertStartSeconds = 0.0f;
    float EventStartSeconds = 0.0f;
    float EventEndSeconds = 0.0f;

    float EffectiveSpawnRateMultiplier = 1.0f;

    FTimerHandle AlertTimerHandle;
    FTimerHandle StartTimerHandle;
    FTimerHandle EndTimerHandle;
};

/** GameMode used during NightShifts (objectives, overtime, extraction). */
UCLASS()
class LOD_API AEMRNightShiftGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEMRNightShiftGameMode();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    bool ShouldUseSeamlessTravel() const;

    virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

    UFUNCTION(BlueprintCallable, Category="EMR|Server")
    void NotifyNightShiftFinished();

    bool SendNightShiftTelemetrySnapshotToController(AEMRPlayerController* TargetController) const;

    /** Test hook used by automation/MCP scenarios to deterministically trigger the reputation-failure branch. */
    UFUNCTION(BlueprintCallable, Category="EMR|Tests")
    void ForceNightShiftFailureByReputationForTests();

    /** Test hook used by dev tools to force a successful NightShift end through the normal finish flow. */
    UFUNCTION(BlueprintCallable, Category="EMR|Tests")
    void ForceNightShiftSuccessForTests();

    /** Test hook used by automation/MCP scenarios to remove long finish delays. */
    UFUNCTION(BlueprintCallable, Category="EMR|Tests")
    void SetNightShiftEndDelayForTests(float NewDelaySeconds);

    bool ApplyRunUpgradeStackForTests(FGameplayTag UpgradeTag, int32 StackDelta, FString& OutMessage);

    void HandleNightShiftOvertimeStarted();
    void HandlePatientOutOfPatience(AEMRPatient* Patient);

    float GetPatienceReputationPenalty() const;
    bool IsNightShiftFinishPending() const { return bNightShiftFinishPending; }

protected:
    AEMRNightShiftGameState* GetNightShiftGameState() const;
    UEMRRunProgressionSubsystem* GetRunProgressionSubsystem() const;
    UEMRRunRulesSubsystem* GetRunRulesSubsystem() const;
    void SyncRunStateToSubsystem();

    void RegisterReputationListener();
    void HandleTeamReputationChanged(const FOnAttributeChangeData& Data);
    void HandleNightShiftFailureByReputation();
    void PlayWatchReputationFailureAlertForAllPlayers() const;

    bool TravelToHubLevel();
    void NotifyControllersOfPendingTravel(const FString& TravelURL);

    void BeginNightShiftFinish();
    void CompleteNightShiftFinish();
    void HandleNightShiftFinished();
    void HandleRestoredRunState();
    AEMRHubertDirector* ResolveHubertDirector();

    void StartNightShiftRuntimeSystems(UEMRNightShiftDefinition* NightShiftDefinition);
    void ApplyRunUpgradesToNightShiftActors();
    void ValidateCoffeeMachinePlacementForActiveUpgrade() const;
    void ValidateOxygenMachinePlacementForActiveUpgrade() const;
    void ValidateSnackMachinePlacementForActiveUpgrade() const;
    void ValidateMagicBoxPlacementForActiveUpgrade() const;
    void ValidateTreatmentBedPlacementForActiveUpgrade() const;
    void ScheduleOvertimeStopPatientDepartures();
    void DispatchNextOvertimeStopPatientDeparture();
    void HandlePatientDeparture(AEMRPatient* Patient, EEMRPatientLeaveReason Reason);
    void ApplyReputationPenaltyForPatientLeaving(AEMRPatient* Patient);
    const UEMRDifficultyTuningData* GetDifficultyTuning() const;
    void BuildAndPublishNightShiftSummary();

    UFUNCTION()
    void HandleTelemetryPatientSpawned(AEMRPatient* Patient);

    UFUNCTION()
    void HandleTelemetryExamCompleted(FGameplayTag ExamTag);

    UFUNCTION()
    void HandleTelemetryTreatmentResolved(AEMRPatient* Patient, FGameplayTag TreatmentTag, bool bPatientFullyCured);

    void HandleTelemetryRevenueChanged(const FOnAttributeChangeData& Data);
    void HandleTelemetryReputationChanged(const FOnAttributeChangeData& Data);
    FEMRNightShiftTelemetryRecord BuildNightShiftTelemetryRecord(bool bSkippedRemainingNightShiftsInCycle) const;
    void BroadcastNightShiftTelemetryToClients(const FEMRNightShiftTelemetryRecord& Record) const;
    void ResetTelemetryAccumulator();
    void RegisterTelemetryListeners();
    void UnregisterTelemetryListeners();
    bool IsTelemetrySplitInOvertime() const;

    bool IsSpecialEventDifficultyEligible(EEMRNightShiftDifficultyTier DifficultyTier) const;
    void ScheduleSpecialEvent(UEMRNightShiftDefinition* NightShiftDefinition);
    void ClearSpecialEventRuntimeState();
    void HandleSpecialEventAlertStarted();
    void HandleSpecialEventStarted();
    void HandleSpecialEventEnded();
    void ApplySpecialEventRuntimeEffects(const FEMRNightShiftSpecialEventDefinition& EventDefinition);
    void ClearSpecialEventRuntimeEffects();
    void CollectConfiguredSpecialEvents(TArray<const UEMRSpecialEventDefinition*>& OutSpecialEvents) const;
    const UEMRSpecialEventDefinition* SelectSpecialEventForNightShift(const UEMRNightShiftDefinition& NightShiftDefinition) const;

    // Data and configuration

    /** Hub map to load after a NightShift ends. Falls back to GameDefaultMap when unset. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="EMR|Config")
    TSoftObjectPtr<UWorld> HubLevel;

    UPROPERTY(EditDefaultsOnly, Category="EMR|NightShift", meta = (ClampMin = "0.0"))
    float NightShiftEndDelaySeconds = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category="EMR|NightShift|Audio")
    TObjectPtr<USoundBase> OvertimeStartedAlertSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="EMR|NightShift|Audio")
    TObjectPtr<USoundBase> SpecialEventAlertSound = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="EMR|Players")
    TSoftObjectPtr<UEMRLobbyCharacterCatalog> PlayerCharacterCatalog;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Patients")
    TSubclassOf<AEMRPatient> DefaultPatientBlueprintClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert")
    TSubclassOf<AEMRHubertDirector> HubertDirectorClass;

private:
    struct FEMRNightShiftTelemetryAccumulator
    {
        float RevenueBeforeOvertime = 0.0f;
        float RevenueAfterOvertime = 0.0f;
        float ReputationLostBeforeOvertime = 0.0f;
        float ReputationLostAfterOvertime = 0.0f;

        int32 PatientsSpawnedBeforeOvertime = 0;
        int32 PatientsSpawnedAfterOvertime = 0;
        int32 PatientsFullyCuredBeforeOvertime = 0;
        int32 PatientsFullyCuredAfterOvertime = 0;
        int32 PatientsLeftByPatienceBeforeOvertime = 0;
        int32 PatientsLeftByPatienceAfterOvertime = 0;

        TMap<FGameplayTag, int32> ExamCounts;
        TMap<FGameplayTag, int32> TreatmentCounts;
        TMap<TWeakObjectPtr<AEMRPatient>, double> SpawnTimestampByPatient;

        double TotalSpawnToCureSeconds = 0.0;
        int32 SpawnToCureSampleCount = 0;

        void Reset()
        {
            RevenueBeforeOvertime = 0.0f;
            RevenueAfterOvertime = 0.0f;
            ReputationLostBeforeOvertime = 0.0f;
            ReputationLostAfterOvertime = 0.0f;
            PatientsSpawnedBeforeOvertime = 0;
            PatientsSpawnedAfterOvertime = 0;
            PatientsFullyCuredBeforeOvertime = 0;
            PatientsFullyCuredAfterOvertime = 0;
            PatientsLeftByPatienceBeforeOvertime = 0;
            PatientsLeftByPatienceAfterOvertime = 0;
            ExamCounts.Reset();
            TreatmentCounts.Reset();
            SpawnTimestampByPatient.Reset();
            TotalSpawnToCureSeconds = 0.0;
            SpawnToCureSampleCount = 0;
        }
    };

    const UEMRLobbyCharacterCatalog* GetPlayerCharacterCatalog() const;

    UPROPERTY()
    mutable TWeakObjectPtr<UEMRRunRulesSubsystem> RunRulesSubsystem;

    UPROPERTY()
    mutable TWeakObjectPtr<const UEMRDifficultyTuningData> CachedDifficultyTuning;

    UPROPERTY()
    mutable TWeakObjectPtr<const UEMRLobbyCharacterCatalog> CachedPlayerCharacterCatalog;

    FDelegateHandle TeamReputationChangedHandle;
    FDelegateHandle TeamRevenueChangedHandle;
    FDelegateHandle SpawnedPatientDelegateHandle;
    FDelegateHandle ExamCompletedDelegateHandle;
    FDelegateHandle TreatmentResolvedDelegateHandle;
    bool bTelemetryAccumulatorActive = false;
    FEMRNightShiftTelemetryAccumulator NightShiftTelemetryAccumulator;
	
    bool bNightShiftEndedByReputation = false;
    bool bNightShiftFinishPending = false;
    FTimerHandle NightShiftEndDelayHandle;
    FTimerHandle OvertimeStopDepartureTimerHandle;
    TArray<TWeakObjectPtr<AEMRPatient>> OvertimeStopDepartureQueue;
    int32 OvertimeStopDepartureIndex = 0;
    FEMRSpecialEventRuntimeState SpecialEventRuntimeState;
    TWeakObjectPtr<const UEMRNightShiftDefinition> ActiveNightShiftForSpecialEvent;
    TWeakObjectPtr<const UEMRSpecialEventDefinition> ActiveSpecialEventDefinition;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRHubertDirector> HubertDirector;
};
