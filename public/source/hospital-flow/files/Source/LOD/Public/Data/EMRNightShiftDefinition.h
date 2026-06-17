#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EMRNightShiftDefinition.generated.h"

class UWorld;
class UCurveFloat;

/**
 * Qualitative revenue potential for a NightShift.
 * This is used for UI and balancing, not as exact numeric values.
 */
UENUM(BlueprintType)
enum class EEMRNightShiftRevenuePotential : uint8
{
    Low         UMETA(DisplayName = "Low"),
    Medium      UMETA(DisplayName = "Medium"),
    High        UMETA(DisplayName = "High"),
    VeryHigh    UMETA(DisplayName = "Very High")
};


UENUM(BlueprintType)
enum class EEMRNightShiftDifficultyTier : uint8
{
    Calm        UMETA(DisplayName  = "Calm"),
    Standard    UMETA(DisplayName  = "Standard"),
    Intensifying UMETA(DisplayName = "Intensifying"),
    Critical    UMETA(DisplayName  = "Critical"),
    Catastrophic UMETA(DisplayName = "Catastrophic")
};


USTRUCT(BlueprintType)
struct FEMRNightShiftSpawnModifier
{
    GENERATED_BODY()

	/** Gameplay tag to identify and enable/disable this modifier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Spawn", meta = (Categories = "EMR.NightShift.Modifier"))
	FGameplayTag ModifierTag;

	/** If true, this modifier is active from the start of the night shift. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Spawn")
	bool bEnabledByDefault = false;

	/** Additional spawn rate curve (additive to base spawn). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Spawn", meta = (AssetBundles = "SpawnCurves"))
	TSoftObjectPtr<UCurveFloat> SpawnRateCurve;
};


USTRUCT(BlueprintType)
struct FEMRNightShiftSpawnProfile
{
    GENERATED_BODY()

	/** Optional modifiers (events, contextual spawn boosts). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Spawn")
    TArray<FEMRNightShiftSpawnModifier> SpawnModifiers;
};

USTRUCT(BlueprintType)
struct FEMRWeightedPathologyEntry
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (Categories = "EMR.Patient.Pathology"))
    FGameplayTag PathologyTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (ClampMin = "0.0"))
    float Weight = 1.0f;
};

USTRUCT(BlueprintType)
struct FEMRNightShiftSpecialEventDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent")
    bool bEnabled = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent")
    FName EventId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent")
    FText AlertTitle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent")
    FText AlertDescription;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (ClampMin = "0.0"))
    float StartWindowMinSeconds = 60.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (ClampMin = "0.0"))
    float StartWindowMaxSeconds = 240.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (ClampMin = "0.0"))
    float AlertLeadSeconds = 6.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (ClampMin = "0.0"))
    float ActiveDurationSeconds = 90.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (ClampMin = "0.0"))
    float BaseSpawnRateMultiplier = 1.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (ClampMin = "0.0"))
    float NonMatchingPathologyWeight = 0.15f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent")
    TArray<FEMRWeightedPathologyEntry> WeightedPathologies;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (ClampMin = "0.0"))
    float BaseLightFlickerRateHz = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent")
    FName FlickerLightTag = TEXT("EMR_SpecialEventLight");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent")
    FLinearColor FlickerColor = FLinearColor::Red;
};



/**
 * Data asset describing a single NightShift/contract offered by AI Boss.
 * A NightShift is basically: "One night in a specific hospital with a specific context".
 */
UCLASS(BlueprintType)
class LOD_API UEMRNightShiftDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
	/** Optional helper to get a stable identifier for this asset (can be used in save systems or logging). */
	UFUNCTION(BlueprintCallable, Category = "NightShift")
	FName GetEffectiveNightShiftId() const;

	
    /** Optional internal id if we don't use direct name. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NightShift")
    FName NightShiftId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NightShift")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NightShift", meta=(MultiLine="true"))
    FText Description;

	
    // Hospital map to load for this NightShift.     
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NightShift")
    TSoftObjectPtr<UWorld> HospitalLevel;

    /** Overall difficulty tier used by spawn systems and UI. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NightShift")
    EEMRNightShiftDifficultyTier DifficultyTier = EEMRNightShiftDifficultyTier::Standard;

    /**
     * Qualitative revenue potential.
     * This does NOT directly define quotas, it only helps choose NightShifts and tune spawn logic.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NightShift")
    EEMRNightShiftRevenuePotential RevenuePotential = EEMRNightShiftRevenuePotential::Medium;

	
    /**
     * Optional tags to characterize this NightShift.
     * Example: NightShift.MatchFootball, NightShift.Strike, NightShift.Epidemic, NightShift.NightShift, etc.
     * These can drive patient composition, events, music, etc.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NightShift", meta = (Categories = "EMR.GameMode.NightShift"))
    FGameplayTagContainer NightShiftTags;

	
    /** Spawn configuration profile for this NightShift. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Spawn")
    FEMRNightShiftSpawnProfile SpawnProfile;

    /** When false, no random SpecialEvent is selected for this NightShift. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent")
    bool bAllowSpecialEvents = true;

    /** Optional tags used to filter which SpecialEvent assets can be selected. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (Categories = "EMR.GameMode.NightShift.EventPool"))
    FGameplayTagContainer SpecialEventPoolTags;

    /** Optional tag to define the reception layout for this NightShift. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Patients", meta = (Categories = "EMR.NightShift.Reception"))
    FGameplayTag ReceptionLayoutTag;
};
