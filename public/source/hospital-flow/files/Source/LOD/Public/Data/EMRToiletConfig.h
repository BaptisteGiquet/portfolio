#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EMRToiletConfig.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType)
class LOD_API UEMRToiletConfig : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    static FPrimaryAssetType GetToiletConfigAssetType();

    UFUNCTION(BlueprintPure, Category="EMR|Toilet")
    float GetTripRequestMinSeconds() const { return TripRequestMinSeconds; }

    UFUNCTION(BlueprintPure, Category="EMR|Toilet")
    float GetTripRequestMaxSeconds() const { return TripRequestMaxSeconds; }

    UFUNCTION(BlueprintPure, Category="EMR|Toilet")
    float GetToiletUseMinSeconds() const { return ToiletUseMinSeconds; }

    UFUNCTION(BlueprintPure, Category="EMR|Toilet")
    float GetToiletUseMaxSeconds() const { return ToiletUseMaxSeconds; }

    UFUNCTION(BlueprintPure, Category="EMR|Toilet")
    float GetDirtBaseChance() const { return DirtBaseChance; }

    UFUNCTION(BlueprintPure, Category="EMR|Toilet")
    float GetDirtChanceIncrement() const { return DirtChanceIncrement; }

    UFUNCTION(BlueprintPure, Category="EMR|Toilet")
    float GetReputationDrainPerSecond() const { return ReputationDrainPerSecond; }

    UFUNCTION(BlueprintPure, Category="EMR|Toilet")
    TSubclassOf<UGameplayEffect> GetReputationDrainEffect() const { return ReputationDrainEffect; }

private:
    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet", meta=(ClampMin="0.0"))
    float TripRequestMinSeconds = 30.0f;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet", meta=(ClampMin="0.0"))
    float TripRequestMaxSeconds = 90.0f;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet", meta=(ClampMin="0.0"))
    float ToiletUseMinSeconds = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet", meta=(ClampMin="0.0"))
    float ToiletUseMaxSeconds = 30.0f;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DirtBaseChance = 0.05f;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet", meta=(ClampMin="0.0", ClampMax="1.0"))
    float DirtChanceIncrement = 0.05f;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet", meta=(ClampMin="0.0"))
    float ReputationDrainPerSecond = 0.2f;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet")
    TSubclassOf<UGameplayEffect> ReputationDrainEffect = nullptr;
};
