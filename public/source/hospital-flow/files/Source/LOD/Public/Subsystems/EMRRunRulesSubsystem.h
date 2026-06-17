
#pragma once

#include "CoreMinimal.h"

#include "Templates/Function.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Framework/EMRNightShiftGameState.h"
#include "EMRRunRulesSubsystem.generated.h"


class AEMRNightShiftGameState;
class UEMRRunRulesData;

/**
 * Authority-only subsystem that centralizes the run rules shared across Hub and NightShift maps.
 * Lives for the lifetime of the GameInstance so GameModes can query the same rule owner after travels.
 */
UCLASS()
class LOD_API UEMRRunRulesSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Prepare a new run with clean state. */
    void InitializeNewRun(AEMRNightShiftGameState* RunState);

    /** Configure quotas and indices for a given cycle. */
    void SetupCycle(AEMRNightShiftGameState* RunState, int32 CycleIndex) const;

    /**
     * Handle state transitions after a NightShift ends.
     * Returns the resulting run phase to help callers decide what to do next.
     */
    EER_RunPhase HandleNightShiftCompletion(AEMRNightShiftGameState* RunState) const;

    /** Evaluate if the current cycle succeeds or fails and update the run state accordingly. */
    void EvaluateEndOfCycle(AEMRNightShiftGameState* RunState) const;

    int32 GetNightShiftsPerCycle() const { return NightShiftsPerCycle; }
    int32 GetMaxCycles() const { return MaxCycles; }
    float GetNightShiftDurationSeconds() const { return NightShiftDurationSeconds; }

private:
    void EnsureRunRulesDataReady(TFunction<void()> Callback) const;
    void RequestRunRulesDataLoad() const;
    void HandleRunRulesDataLoaded() const;
    void FlushPendingCallbacks() const;

    void ApplyRunRulesData() const;

    void HandleRunFailed(AEMRNightShiftGameState* RunState) const;
    void HandleFinalMissionUnlocked(AEMRNightShiftGameState* RunState) const;

private:
    /** Number of NightShifts that compose a cycle. */
    UPROPERTY(VisibleDefaultsOnly, Category="EMR|Config")
    mutable int32 NightShiftsPerCycle = 1;

    /** Total number of cycles before the final mission is unlocked. */
    UPROPERTY(VisibleDefaultsOnly, Category="EMR|Config")
    mutable int32 MaxCycles = 3;

    /** Default cycle quotas for the run. */
    UPROPERTY(VisibleDefaultsOnly, Category="EMR|Config")
    mutable TArray<float> DefaultCycleQuotas;

    /** Fallback quota increment used when DefaultCycleQuotas does not cover a cycle. */
    UPROPERTY(VisibleDefaultsOnly, Category="EMR|Config")
    mutable float FallbackCycleQuotaStep = 20000.0f;

    /** Global NightShift duration in seconds. */
    UPROPERTY(VisibleDefaultsOnly, Category="EMR|Config")
    mutable float NightShiftDurationSeconds = 600.f;

    mutable TArray<TFunction<void()>> PendingReadyCallbacks;
    mutable bool bRunRulesDataLoadRequested = false;
    mutable bool bRunRulesDataLoaded = false;
};

