#pragma once

#include "CoreMinimal.h"
#include "EMRRunRulesData.generated.h"

/**
 * Designer-editable run rules loaded by the RunRulesSubsystem.
 * Allows tuning progression without touching code.
 */
UCLASS(BlueprintType)
class LOD_API UEMRRunRulesData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    static FPrimaryAssetType GetRunRulesAssetType();

    /** Number of NightShifts that compose a cycle. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="EMR|RunRules", meta=(ClampMin="1"))
    int32 NightShiftsPerCycle = 3;

    /** Total number of cycles before the final mission is unlocked. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="EMR|RunRules", meta=(ClampMin="1"))
    int32 MaxCycles = 3;

    /** Default cycle quotas for the run. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="EMR|RunRules")
    TArray<float> DefaultCycleQuotas;

    /** Fallback quota increment used when DefaultCycleQuotas does not define the current cycle. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="EMR|RunRules", meta=(ClampMin="0.0"))
    float FallbackCycleQuotaStep = 20000.0f;

    /** Global NightShift duration in seconds. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="EMR|RunRules", meta=(ClampMin="1.0"))
    float NightShiftDurationSeconds = 600.f;
};
