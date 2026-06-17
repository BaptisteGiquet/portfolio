#pragma once

#include "CoreMinimal.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Subsystems/WorldSubsystem.h"

#include "EMRPatientPoolSubsystem.generated.h"

class AEMRPatient;
class UEMRNightShiftDefinition;
class UEMRPatientData;
class UEMRDifficultyTuningData;
class AEMRPatientPoolAnchor;

USTRUCT(BlueprintType)
struct FEMRPatientPoolDebugInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Spawn")
    int32 TargetPoolSize = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Spawn")
    int32 InactivePatients = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Spawn")
    int32 ActiveTrackedPatients = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Spawn")
    int32 CachedPatientData = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Spawn")
    FString PatientClassName;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Spawn")
    FString CurrentNightShiftName;

};

/**
 * Pool of patient actors that keeps a stable set of instances alive across a run.
 */
UCLASS()
class LOD_API UEMRPatientPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

    /** Spawn or resize the pool so that at least Size patients are available for reuse. */
    void InitializePool(int32 Size);

    /** Acquire (or spawn) a patient actor. Activation is optional for seat-gated spawns. */
    AEMRPatient* AcquirePatient(bool bActivate = true);

    /** Acquire a hidden pooled shell for staged automatic spawn preparation. */
    AEMRPatient* AcquirePatientShell();

    /** Selects patient data for automatic spawn preparation using current weighting rules. */
    const UEMRPatientData* SelectPatientDataForAutomaticSpawn() const;

    /** Applies patient data and runtime effects for automatic staged spawn. */
    bool ApplyPatientDataForAutomaticSpawn(AEMRPatient& Patient, const UEMRPatientData& PatientData);

    /** Restarts patient AI state for staged automatic spawn. */
    void EnablePatientAIForAutomaticSpawn(AEMRPatient& Patient) const;

    /** Activate a pooled patient for use in the world. */
    void ActivatePatient(AEMRPatient& Patient, const FTransform& SpawnTransform);

    /** Return a patient actor to the pool. */
    void ReleasePatient(AEMRPatient* Patient);

    /** Prepares the pool for the provided NightShift definition. */
    void PrewarmForNightShift(const UEMRNightShiftDefinition* Definition);

    /** Kicks class and data warmup for automatic NightShift arrivals. */
    void PrewarmAutomaticSpawnResources();

    int32 GetAutomaticWarmActorTarget() const { return FMath::Max(AutomaticWarmActorTarget, 1); }

    /** (Server) Refresh patience drain effect on all active patients using the provided multiplier. */
    void ApplyPatienceDrainMultiplier(float Multiplier);

    void SetSpecialEventPathologyWeights(
        const TArray<FEMRWeightedPathologyEntry>& WeightedPathologies,
        float NonMatchingWeight);
    void ClearSpecialEventPathologyWeights();

    void SetPatientClasses(const TArray<TSubclassOf<AEMRPatient>>& InClasses);

    UFUNCTION(BlueprintCallable, Category = "EMR|NightShift|Spawning")
    FEMRPatientPoolDebugInfo GetDebugInfo() const;

private:
    void UpdateStorageLocationFromGameState();
    void DestroyPooledPatients();
    void RefreshPatientDataCache();
    void LoadDifficultyTuning();
    const UEMRPatientData* SelectPatientData() const;
    const UEMRPatientData* SelectPatientDataWeighted() const;
    TSubclassOf<AEMRPatient> SelectPatientClass() const;
    AEMRPatient* SpawnPatient() const;
    void DeactivatePatient(AEMRPatient& Patient) const;
    void ApplyPatienceDrainMultiplierToPatient(AEMRPatient& Patient);
    void WarmConfiguredPatientClassesAsync();
    void WarmPatientDataAssets();

	

    UPROPERTY()
    TArray<TWeakObjectPtr<AEMRPatient>> InactivePatients;

    UPROPERTY()
    TMap<TWeakObjectPtr<AEMRPatient>, TObjectPtr<const UEMRPatientData>> PatientDataByPatient;

    float CurrentPatienceDrainMultiplier = 1.0f;

    UPROPERTY()
    TArray<TObjectPtr<const UEMRPatientData>> CachedPatientData;

    UPROPERTY()
    TArray<TSubclassOf<AEMRPatient>> PatientClasses;

    /** Current NightShift definition used for spawn weighting. */
    UPROPERTY()
    TObjectPtr<const UEMRNightShiftDefinition> CurrentNightShiftDefinition = nullptr;

    UPROPERTY()
    TObjectPtr<const UEMRDifficultyTuningData> CachedDifficultyTuning = nullptr;

    int32 TargetPoolSize = 0;

    float BasePatienceDrainMultiplier = 1.0f;

    bool bHasWarnedMissingPatientData = false;

    TMap<FGameplayTag, float> ActiveSpecialEventPathologyWeights;
    float ActiveSpecialEventNonMatchingWeight = 1.0f;
    bool bSpecialEventPathologyOverrideActive = false;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Spawn", meta = (ClampMin = "0"))
    int32 DefaultPrewarmPoolSize = 12;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Spawn|Warmup", meta = (ClampMin = "1"))
    int32 AutomaticWarmActorTarget = 12;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Spawn|Warmup", meta = (ClampMin = "1"))
    int32 AutomaticWarmClassBatchSize = 8;

    mutable TWeakObjectPtr<AEMRPatientPoolAnchor> CachedPoolAnchor;

	FVector StorageLocation = FVector(-20000.f, -20000.f, -20000.f);
};
