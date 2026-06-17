#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Data/EMRNightShiftDefinition.h"
#include "EMRDifficultyTuningData.generated.h"

/** Simple map wrapper to expose scalar tuning values per NightShift difficulty. */
USTRUCT(BlueprintType)
struct FEMRDifficultyScalarMap
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    TMap<EEMRNightShiftDifficultyTier, float> Values;

    float GetValue(const EEMRNightShiftDifficultyTier Difficulty, const float DefaultValue = 1.0f) const
    {
        if (const float* FoundValue = Values.Find(Difficulty))
        {
            return *FoundValue;
        }

        return DefaultValue;
    }
};

/** Map wrapper to expose player-count based tuning values. */
USTRUCT(BlueprintType)
struct FEMRPlayerCountScalarMap
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    TMap<int32, float> Values;

    /** Returns the value for the closest lower-or-equal player count, or DefaultValue if none is defined. */
    float GetValue(const int32 PlayerCount, const float DefaultValue = 1.0f) const
    {
        float BestValue = DefaultValue;
        int32 BestKey = TNumericLimits<int32>::Min();

        for (const TPair<int32, float>& Entry : Values)
        {
            if (Entry.Key <= PlayerCount && Entry.Key > BestKey)
            {
                BestKey = Entry.Key;
                BestValue = Entry.Value;
            }
        }

        return BestValue;
    }
};

/** Map wrapper to expose spawn weights per patient severity. */
USTRUCT(BlueprintType)
struct FEMRPatientSeverityWeightMap
{
    GENERATED_BODY();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty", meta = (Categories = "EMR.Patient.Severity"))
    TMap<FGameplayTag, float> Values;

    float GetValue(const FGameplayTag& PatientSeverity, const float DefaultValue = 1.0f) const
    {
        if (const float* FoundValue = Values.Find(PatientSeverity))
        {
            return *FoundValue;
        }

        return DefaultValue;
    }
};

/** Map wrapper to expose per-patient-severity reward values. */
USTRUCT(BlueprintType)
struct FEMRPatientSeverityRewardMap
{
    GENERATED_BODY();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty", meta = (Categories = "EMR.Patient.Severity"))
    TMap<FGameplayTag, float> Values;

    float GetValue(const FGameplayTag& PatientSeverity, const float DefaultValue = 0.0f) const
    {
        if (const float* FoundValue = Values.Find(PatientSeverity))
        {
            return *FoundValue;
        }

        return DefaultValue;
    }
};

/** Map wrapper to expose per-patient-severity scalar values (e.g., patience). */
USTRUCT(BlueprintType)
struct FEMRPatientSeverityScalarMap
{
    GENERATED_BODY();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty", meta = (Categories = "EMR.Patient.Severity"))
    TMap<FGameplayTag, float> Values;

    float GetValue(const FGameplayTag& PatientSeverity, const float DefaultValue = 0.0f) const
    {
        if (const float* FoundValue = Values.Find(PatientSeverity))
        {
            return *FoundValue;
        }

        return DefaultValue;
    }
};

/** Map wrapper to expose per-exam money costs. */
USTRUCT(BlueprintType)
struct FEMRExamMoneyCostMap
{
    GENERATED_BODY();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty", meta = (Categories = "EMR.Abilities.Exam"))
    TMap<FGameplayTag, float> Values;

    float GetValue(const FGameplayTag& ExamTag, const float DefaultValue = 0.0f) const
    {
        if (const float* FoundValue = Values.Find(ExamTag))
        {
            return *FoundValue;
        }

        return DefaultValue;
    }
};

/** Map wrapper to expose spawn weights per NightShift difficulty and patient severity. */
USTRUCT(BlueprintType)
struct FEMRNightShiftPatientWeightMap
{
    GENERATED_BODY();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    TMap<EEMRNightShiftDifficultyTier, FEMRPatientSeverityWeightMap> Values;

    float GetValue(const EEMRNightShiftDifficultyTier NightShiftDifficulty, const FGameplayTag& PatientSeverity, const float DefaultValue = 1.0f) const
    {
        if (const FEMRPatientSeverityWeightMap* DifficultyWeights = Values.Find(NightShiftDifficulty))
        {
            return DifficultyWeights->GetValue(PatientSeverity, DefaultValue);
        }

        return DefaultValue;
    }
};

USTRUCT(BlueprintType)
struct FEMRCoffeeUpgradeTuning
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|Coffee", meta = (ClampMin = "1"))
    int32 MaxServingsPerFill = 5;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|Coffee", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PatienceRestorePercentOfMax = 0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|Coffee", meta = (ClampMin = "0.1"))
    float BrewFillDurationSeconds = 3.0f;
};

USTRUCT(BlueprintType)
struct FEMRSnackMachineUpgradeTuning
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine", meta = (ClampMin = "0.1"))
    float TripRequestMinSeconds = 40.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine", meta = (ClampMin = "0.1"))
    float TripRequestMaxSeconds = 90.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine", meta = (ClampMin = "0.1"))
    float SnackUseMinSeconds = 4.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine", meta = (ClampMin = "0.1"))
    float SnackUseMaxSeconds = 8.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine", meta = (ClampMin = "0.0"))
    float RevenuePerTrip = 20.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine", meta = (ClampMin = "0.0"))
    float PatienceRestoreAmount = 10.0f;
};

USTRUCT(BlueprintType)
struct FEMRItemDispenserSalesUpgradeTuning
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|ItemDispenserSales", meta = (ClampMin = "0.0"))
    float DiscountPercentPerStack = 30.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|ItemDispenserSales")
    FLinearColor DiscountedPriceColor = FLinearColor(0.2f, 0.9f, 0.3f, 1.0f);
};

USTRUCT(BlueprintType)
struct FEMRMagicBoxUpgradeTuning
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|MagicBox", meta = (ClampMin = "0.1"))
    float TreatmentDurationSeconds = 60.0f;
};

USTRUCT(BlueprintType)
struct FEMRTreatmentBedUpgradeMapCapacity
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|TreatmentBed")
    TSoftObjectPtr<UWorld> HospitalLevel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|TreatmentBed", meta = (ClampMin = "0"))
    int32 MaxTreatmentBedUpgradeCount = 0;
};

USTRUCT(BlueprintType)
struct FEMRCoffeeMachineUpgradeMapCapacity
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|Coffee")
    TSoftObjectPtr<UWorld> HospitalLevel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|Coffee", meta = (ClampMin = "0"))
    int32 MaxCoffeeMachineUpgradeCount = 0;
};

USTRUCT(BlueprintType)
struct FEMRSnackMachineUpgradeMapCapacity
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine")
    TSoftObjectPtr<UWorld> HospitalLevel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine", meta = (ClampMin = "0"))
    int32 MaxSnackMachineUpgradeCount = 0;
};

USTRUCT(BlueprintType)
struct FEMRMagicBoxUpgradeMapCapacity
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|MagicBox")
    TSoftObjectPtr<UWorld> HospitalLevel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|MagicBox", meta = (ClampMin = "0"))
    int32 MaxMagicBoxUpgradeCount = 0;
};

USTRUCT(BlueprintType)
struct FEMROxygenMachineUpgradeMapCapacity
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|OxygenMachine")
    TSoftObjectPtr<UWorld> HospitalLevel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|OxygenMachine", meta = (ClampMin = "1"))
    int32 MaxOxygenMachineCount = 1;
};

/**
 * Global tuning asset to centralize per-difficulty scalars.
 * Replaces integer-indexed curves for coarse tuning (spawn, patience drain, rewards...).
 */
UCLASS(BlueprintType)
class LOD_API UEMRDifficultyTuningData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    static FPrimaryAssetType GetDifficultyTuningAssetType();

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetSpawnRateMultiplier(const EEMRNightShiftDifficultyTier Difficulty) const
    {
        return SpawnRateMultipliers.GetValue(Difficulty, 1.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetBaseSpawnRatePerSecond() const { return BaseSpawnRatePerSecond; }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetPatienceDrainMultiplier(const EEMRNightShiftDifficultyTier Difficulty) const
    {
        return PatienceDrainMultipliers.GetValue(Difficulty, 1.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetPatientReputationPenalty(const EEMRNightShiftDifficultyTier Difficulty) const
    {
        return PatientReputationPenalties.GetValue(Difficulty, DefaultPatientReputationPenalty);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetIntravenousMedicationCompletionDelay(const EEMRNightShiftDifficultyTier Difficulty) const
    {
        return IntravenousMedicationCompletionDelaySeconds.GetValue(Difficulty, 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetOxygenTreatmentCompletionDelay(const EEMRNightShiftDifficultyTier Difficulty) const
    {
        return OxygenTreatmentCompletionDelaySeconds.GetValue(Difficulty, 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetInitialAutomaticSpawnDelaySeconds(const EEMRNightShiftDifficultyTier Difficulty) const
    {
        return InitialAutomaticSpawnDelaySeconds.GetValue(Difficulty, 0.0f);
    }


    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetPlayerCountMultiplier(const int32 PlayerCount) const
    {
        return PlayerCountMultipliers.GetValue(PlayerCount, 1.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetPatientSpawnWeight(const EEMRNightShiftDifficultyTier NightShiftDifficulty, const FGameplayTag& PatientSeverity) const
    {
        return PatientSpawnWeights.GetValue(NightShiftDifficulty, PatientSeverity, 1.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetOvertimeSpawnRateScalar(const EEMRNightShiftDifficultyTier Difficulty) const
    {
        return OvertimeSpawnRateScalars.GetValue(Difficulty, 1.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetOvertimePatienceDrainScalar(const EEMRNightShiftDifficultyTier Difficulty) const
    {
        return OvertimePatienceDrainScalars.GetValue(Difficulty, 1.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|SpecialEvent")
    float GetSpecialEventSpawnRateScalar(const EEMRNightShiftDifficultyTier Difficulty) const
    {
        return SpecialEventSpawnRateScalars.GetValue(Difficulty, 1.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetPatientMoneyReward(const FGameplayTag& PatientSeverity) const
    {
        return PatientMoneyRewards.GetValue(PatientSeverity, 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetPatientReputationReward(const FGameplayTag& PatientSeverity) const
    {
        return PatientReputationRewards.GetValue(PatientSeverity, 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetPatientInitialPatience(const FGameplayTag& PatientSeverity) const
    {
        return PatientInitialPatience.GetValue(PatientSeverity, DefaultPatientInitialPatience);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    float GetExamMoneyCost(const FGameplayTag& ExamTag) const
    {
        return ExamMoneyCosts.GetValue(ExamTag, 0.0f);
    }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty")
    const UCurveFloat* GetOvertimeDifficultyCurve() const { return OvertimeDifficultyCurve; }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|Upgrades|Coffee")
    const FEMRCoffeeUpgradeTuning& GetCoffeeUpgradeTuning() const { return CoffeeUpgradeTuning; }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|Upgrades")
    const FEMRSnackMachineUpgradeTuning& GetSnackMachineUpgradeTuning() const { return SnackMachineUpgradeTuning; }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|Upgrades")
    const FEMRItemDispenserSalesUpgradeTuning& GetItemDispenserSalesUpgradeTuning() const { return ItemDispenserSalesUpgradeTuning; }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|Upgrades|MagicBox")
    const FEMRMagicBoxUpgradeTuning& GetMagicBoxUpgradeTuning() const { return MagicBoxUpgradeTuning; }

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|Upgrades")
    int32 GetMaxTreatmentBedUpgradeCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const;

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|Upgrades")
    int32 GetMaxCoffeeMachineUpgradeCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const;

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|Upgrades")
    int32 GetMaxSnackMachineUpgradeCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const;

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|Upgrades|MagicBox")
    int32 GetMaxMagicBoxUpgradeCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const;

    UFUNCTION(BlueprintPure, Category = "EMR|Difficulty|Upgrades|OxygenMachine")
    int32 GetMaxOxygenMachineCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const;

    /** Difficulty -> spawn rate multiplier (applied on top of curves). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRDifficultyScalarMap SpawnRateMultipliers;

    /** Difficulty -> patience drain multiplier (baseline before overtime). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRDifficultyScalarMap PatienceDrainMultipliers;

    /** Difficulty -> reputation penalty when a patient leaves without treatment. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRDifficultyScalarMap PatientReputationPenalties;

    /** Difficulty -> delay before IV treatment completion after success. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRDifficultyScalarMap IntravenousMedicationCompletionDelaySeconds;

    /** Difficulty -> delay before oxygen treatment completion after success. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRDifficultyScalarMap OxygenTreatmentCompletionDelaySeconds;

    /** Difficulty -> delay before first automatic patient spawn at shift start. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRDifficultyScalarMap InitialAutomaticSpawnDelaySeconds;



    /** Baseline penalty used when no difficulty-specific value is provided. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty", meta = (ClampMin = "0.0"))
    float DefaultPatientReputationPenalty = 5.0f;

    /** Baseline spawn rate per second before difficulty/player modifiers. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty", meta = (ClampMin = "0.0"))
    float BaseSpawnRatePerSecond = 0.0f;

    /** PlayerCount -> spawn multiplier (replaces integer-indexed curves). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRPlayerCountScalarMap PlayerCountMultipliers;

    /** NightShift difficulty -> patient severity spawn weighting map. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRNightShiftPatientWeightMap PatientSpawnWeights;

    /** NightShift difficulty -> overtime spawn rate penalty. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRDifficultyScalarMap OvertimeSpawnRateScalars;

    /** NightShift difficulty -> overtime patience drain penalty. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRDifficultyScalarMap OvertimePatienceDrainScalars;

    /** NightShift difficulty -> special-event spawn multiplier scalar. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|SpecialEvent")
    FEMRDifficultyScalarMap SpecialEventSpawnRateScalars;

    /** Patient severity -> full money reward. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRPatientSeverityRewardMap PatientMoneyRewards;

    /** Patient severity -> full reputation reward. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRPatientSeverityRewardMap PatientReputationRewards;

    /** Patient severity -> initial patience value. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRPatientSeverityScalarMap PatientInitialPatience;

    /** Exam tag -> flat money cost applied on exam completion. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    FEMRExamMoneyCostMap ExamMoneyCosts;

    /** Baseline patience used when no severity-specific value is provided. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty", meta = (ClampMin = "0.0"))
    float DefaultPatientInitialPatience = 0.0f;

    /** Curve defining how overtime intensity scales over time. Value is multiplied by the per-feature scalars. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty")
    TObjectPtr<UCurveFloat> OvertimeDifficultyCurve = nullptr;

    /** Shared tuning for coffee-machine upgrade behavior. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|Coffee")
    FEMRCoffeeUpgradeTuning CoffeeUpgradeTuning;

    /** Shared tuning for snack-machine upgrade behavior. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine")
    FEMRSnackMachineUpgradeTuning SnackMachineUpgradeTuning;

    /** Shared tuning for item-dispenser sales upgrade behavior. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades")
    FEMRItemDispenserSalesUpgradeTuning ItemDispenserSalesUpgradeTuning;

    /** Shared tuning for magic-box upgrade behavior. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|MagicBox")
    FEMRMagicBoxUpgradeTuning MagicBoxUpgradeTuning;

    /** Per-map capacity for treatment-bed upgrade stacks. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades")
    TArray<FEMRTreatmentBedUpgradeMapCapacity> TreatmentBedUpgradeMapCapacities;

    /** Per-map capacity for coffee-machine upgrade stacks. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|Coffee")
    TArray<FEMRCoffeeMachineUpgradeMapCapacity> CoffeeMachineUpgradeMapCapacities;

    /** Per-map capacity for snack-machine upgrade stacks. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|SnackMachine")
    TArray<FEMRSnackMachineUpgradeMapCapacity> SnackMachineUpgradeMapCapacities;

    /** Per-map capacity for magic-box upgrade stacks. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|MagicBox")
    TArray<FEMRMagicBoxUpgradeMapCapacity> MagicBoxUpgradeMapCapacities;

    /** Per-map total oxygen machine count (includes the baseline oxygen machine). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Difficulty|Upgrades|OxygenMachine")
    TArray<FEMROxygenMachineUpgradeMapCapacity> OxygenMachineUpgradeMapCapacities;
};
