
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MobaMMC_BaseAttackDamage.generated.h"


UCLASS()
class UMobaMMC_BaseAttackDamage : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMobaMMC_BaseAttackDamage();
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition AttackDamageCaptureDefinition;
	FGameplayEffectAttributeCaptureDefinition ArmorPenetrationCaptureDefinition;
	FGameplayEffectAttributeCaptureDefinition ArmorCaptureDefinition;
};
