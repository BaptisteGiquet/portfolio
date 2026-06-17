

#include "GAS/MobaMMC_ScalingByLevel.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Attributes/MobaAttributeSet_Hero.h"
#include "GAS/MobaAbilitySystemComponent.h"

UMobaMMC_ScalingByLevel::UMobaMMC_ScalingByLevel()
{
	LevelCaptureDefinition.AttributeToCapture = UMobaAttributeSet_Hero::GetLevelAttribute();
	LevelCaptureDefinition.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;

	RelevantAttributesToCapture.Add(LevelCaptureDefinition);
}

float UMobaMMC_ScalingByLevel::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const UAbilitySystemComponent* InstigatorASC = Spec.GetContext().GetInstigatorAbilitySystemComponent();
	if (!InstigatorASC) { return 0.f; }

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	float Level = 0;
	GetCapturedAttributeMagnitude(LevelCaptureDefinition, Spec, EvaluateParameters, Level);

	bool bFound = false;
	const float RateAttributeMagnitude = InstigatorASC->GetGameplayAttributeValue(RateAttribute, bFound);
	if (!bFound) { return 0.f; }

	return RateAttributeMagnitude * (Level - 1.f);
}
