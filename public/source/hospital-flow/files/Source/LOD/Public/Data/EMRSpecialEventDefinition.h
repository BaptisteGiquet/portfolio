#pragma once

#include "CoreMinimal.h"
#include "Data/EMRNightShiftDefinition.h"
#include "EMRSpecialEventDefinition.generated.h"

UCLASS(BlueprintType)
class LOD_API UEMRSpecialEventDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    static FPrimaryAssetType GetSpecialEventAssetType();

    UFUNCTION(BlueprintPure, Category = "EMR|SpecialEvent")
    bool SupportsDifficulty(EEMRNightShiftDifficultyTier DifficultyTier) const;

    UFUNCTION(BlueprintPure, Category = "EMR|SpecialEvent")
    const FEMRNightShiftSpecialEventDefinition& GetEventDefinition() const { return EventDefinition; }

    UFUNCTION(BlueprintPure, Category = "EMR|SpecialEvent")
    const FGameplayTagContainer& GetEventPoolTags() const { return EventPoolTags; }

    UFUNCTION(BlueprintPure, Category = "EMR|SpecialEvent")
    const FGameplayTagContainer& GetRequiredNightShiftTags() const { return RequiredNightShiftTags; }

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (AllowPrivateAccess = "true"))
    FEMRNightShiftSpecialEventDefinition EventDefinition;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (AllowPrivateAccess = "true", Categories = "EMR.GameMode.NightShift.EventPool"))
    FGameplayTagContainer EventPoolTags;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (AllowPrivateAccess = "true", Categories = "EMR.GameMode.NightShift"))
    FGameplayTagContainer RequiredNightShiftTags;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|SpecialEvent", meta = (AllowPrivateAccess = "true"))
    TArray<EEMRNightShiftDifficultyTier> SupportedDifficulties =
    {
        EEMRNightShiftDifficultyTier::Intensifying,
        EEMRNightShiftDifficultyTier::Critical,
        EEMRNightShiftDifficultyTier::Catastrophic
    };
};
