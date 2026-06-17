#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Characters/Patient/EMRPatientIdentityPoolData.h"
#include "EMRPatientData.generated.h"

class UGameplayEffect;
class UGameplayAbility;

USTRUCT(BlueprintType)
struct FEMRAttributeValue
{
    GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Attribute")
	FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Attribute")
    float Value = 0.0f;
};


USTRUCT(BlueprintType)
struct FPatientAttributeRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Attribute")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Attribute")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Attribute")
    FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|Attribute")
    float Value = 0.0f;
};



UCLASS()
class LOD_API UEMRPatientData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	static FPrimaryAssetType GetPatientAssetType();
	
	
    // Identity Getters
    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    FText GetRandomName() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    int32 GetRandomAge() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    FGameplayTag GetRandomBloodType() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    FGameplayTag GetParticularity() const { return Type; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    FGameplayTag GetSeverity() const { return Severity; }

    // Medical Getters
    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    FGameplayTagContainer GetPathologies() const { return Pathologies; }

	UFUNCTION(BlueprintPure, Category = "EMR|Patient")
	FGameplayTagContainer GetSymptoms() const { return Symptoms; }

	
    // Visual Getters
    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    TSoftObjectPtr<UTexture2D> GetPortrait() const { return Portrait; }

	// GAS Getters
    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    TArray<FEMRAttributeValue> GetDefaultVitalStats() const;

	UFUNCTION(BlueprintPure, Category = "EMR|Patient")
	TArray<TSubclassOf<UGameplayAbility>> GetStartupAbilities() const { return StartupAbilities; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    TArray<TSubclassOf<UGameplayEffect>> GetStartupEffects() const { return StartupEffects; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    TSubclassOf<UGameplayEffect> GetInitializationEffect() const { return InitPatientEffect; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    TSubclassOf<UGameplayEffect> GetAddTagToPatientEffect() const { return AddTagToPatientEffect; }

    UFUNCTION(BlueprintPure, Category = "EMR|Patient")
    TSubclassOf<UGameplayEffect> GetPatienceDrainEffect() const { return PatienceDrainEffect; }

	
private:
    // Identity
    UPROPERTY(EditAnywhere, Category = "EMR|Identity")
    TObjectPtr<const UEMRPatientIdentityPoolData> IdentityPool;

    UPROPERTY(EditAnywhere, Category = "EMR|Identity", meta = (Categories = "EMR.Patient.Type"))
    FGameplayTag Type;

    UPROPERTY(EditAnywhere, Category = "EMR|Identity", meta = (Categories = "EMR.Patient.Severity"))
    FGameplayTag Severity;


    // Medical
    UPROPERTY(EditAnywhere, Category = "EMR|Medical", meta = (Categories = "EMR.Patient.Pathology"))
    FGameplayTagContainer Pathologies;

	UPROPERTY(EditAnywhere, Category = "EMR|Medical", meta = (Categories = "EMR.Patient.Symptom"))
	FGameplayTagContainer Symptoms;
	

    // Visual
    UPROPERTY(EditAnywhere, Category = "EMR|Visual")
    TSoftObjectPtr<UTexture2D> Portrait;


    // GAS
    UPROPERTY(EditAnywhere, Category = "EMR|GAS")
    TObjectPtr<UDataTable> PatientAttributesTable;
	
	UPROPERTY(EditAnywhere, Category = "EMR|GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditAnywhere, Category = "EMR|GAS")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;
	
    UPROPERTY(EditDefaultsOnly, Category = "EMR|GAS", meta = (DisplayName = "Effect to initialize Patient Initial stats"))
    TSubclassOf<UGameplayEffect> InitPatientEffect;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|GAS")
    TSubclassOf<UGameplayEffect> AddTagToPatientEffect;

    /** GameplayEffect driving patience drain over time. Its level is scaled by the overtime multiplier. */
    UPROPERTY(EditDefaultsOnly, Category = "EMR|GAS")
    TSubclassOf<UGameplayEffect> PatienceDrainEffect;
};
