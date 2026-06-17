// Copyright Lorenzo Deluca

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"

#include "EMRPatientIdentityPoolData.generated.h"

UCLASS()
class LOD_API UEMRPatientIdentityPoolData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    static FPrimaryAssetType GetIdentityPoolAssetType();

    UFUNCTION(BlueprintPure, Category = "EMR|Identity")
    FText GetRandomName() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Identity")
    int32 GetRandomAge() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Identity")
    FGameplayTag GetRandomBloodType() const;

private:
    UPROPERTY(EditAnywhere, Category = "EMR|Identity")
    TArray<FText> AvailableNames;

    UPROPERTY(EditAnywhere, Category = "EMR|Identity")
    int32 MinAge = 0;

    UPROPERTY(EditAnywhere, Category = "EMR|Identity")
    int32 MaxAge = 0;

    UPROPERTY(EditAnywhere, Category = "EMR|Identity", meta = (Categories = "EMR.Patient.BloodType"))
    FGameplayTagContainer AvailableBloodTypes;
};
