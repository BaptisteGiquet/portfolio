
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MobaMMC_ScalingByLevel.generated.h"


UCLASS()
class UMobaMMC_ScalingByLevel : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMobaMMC_ScalingByLevel();
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	UPROPERTY(EditDefaultsOnly)
	FGameplayAttribute RateAttribute;

	FGameplayEffectAttributeCaptureDefinition RateAttributeDefinition;

	FGameplayEffectAttributeCaptureDefinition LevelCaptureDefinition;
};
