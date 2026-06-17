#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "EMRPatientUIData.generated.h"

/**
 * Struct with all PatientData for UI only
 */

USTRUCT(BlueprintType)
struct FEMRPatientUIData
{
    GENERATED_BODY()

    // Identity
    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|Identity")
    FText FullName;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|Identity")
    int32 Age = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|Identity")
    FGameplayTag BloodType;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|Identity")
    FGameplayTag PatientType;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|Identity")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|Visibility")
    bool bFolderDisplayRegistered = false;

	
    // Medical
    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|Medical")
    FGameplayTagContainer Pathologies;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|Medical")
    FGameplayTagContainer Symptoms;

	
    // CurrentPhase
    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|Phase")
    FGameplayTag CurrentPhase;

	
    // CurrentVitals
    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|VitalStats")
    float HeartRate = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|VitalStats")
    float BloodPressureSystolic = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|VitalStats")
    float BloodPressureDiastolic = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|VitalStats")
    float OxygenSaturation = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|VitalStats")
    float RespiratoryRate = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|VitalStats")
    float Temperature = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|VitalStats")
    float GlasgowScore = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|VitalStats")
    float Patience = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|UI|VitalStats")
    float MaxPatience = 0.0f;

	
    FEMRPatientUIData() = default;
	
    bool IsValid() const
    {
        return !FullName.IsEmpty() && Age > 0;
    }

	
    FORCEINLINE float GetPatienceRatio() const
    {
        return MaxPatience > 0.0f ? FMath::Clamp(Patience / MaxPatience, 0.0f, 1.0f) : 0.0f;
    }


    FText GetFormattedBloodPressure() const
    {
        return FText::Format(
            FText::FromString("{0}/{1}"),
            FText::AsNumber(FMath::RoundToInt(BloodPressureSystolic)),
            FText::AsNumber(FMath::RoundToInt(BloodPressureDiastolic))
        );
    }
};
