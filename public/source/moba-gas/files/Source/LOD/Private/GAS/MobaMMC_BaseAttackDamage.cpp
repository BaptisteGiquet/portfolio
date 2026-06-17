

#include "GAS/MobaMMC_BaseAttackDamage.h"

#include "Attributes/MobaAttributeSet.h"


UMobaMMC_BaseAttackDamage::UMobaMMC_BaseAttackDamage()
{
	AttackDamageCaptureDefinition.AttributeToCapture = UMobaAttributeSet::GetAttackDamageAttribute();
	AttackDamageCaptureDefinition.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;

	ArmorPenetrationCaptureDefinition.AttributeToCapture = UMobaAttributeSet::GetArmorPenetrationAttribute();
	ArmorPenetrationCaptureDefinition.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;

	ArmorCaptureDefinition.AttributeToCapture = UMobaAttributeSet::GetArmorAttribute();
	ArmorCaptureDefinition.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;

	RelevantAttributesToCapture.Add(AttackDamageCaptureDefinition);
	RelevantAttributesToCapture.Add(ArmorCaptureDefinition);
	RelevantAttributesToCapture.Add(ArmorPenetrationCaptureDefinition);
}



float UMobaMMC_BaseAttackDamage::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	float SourceAttackDamage = 0.f;
	GetCapturedAttributeMagnitude(AttackDamageCaptureDefinition, Spec, EvaluateParameters, SourceAttackDamage);

	float SourceArmorPenetration = 0.f;
	GetCapturedAttributeMagnitude(ArmorPenetrationCaptureDefinition, Spec, EvaluateParameters, SourceArmorPenetration);
	
	float TargetArmor = 0.f;
	GetCapturedAttributeMagnitude(ArmorCaptureDefinition, Spec, EvaluateParameters, TargetArmor);


	constexpr float ArmorPenetrationMinPercent = 0.0f;
	constexpr float ArmorPenetrationMaxPercent = 100.0f;
	constexpr float ArmorMitigationConstant = 100.0f;
	
	const float ClampedArmorPenetrationPercent = FMath::Clamp(SourceArmorPenetration, ArmorPenetrationMinPercent, ArmorPenetrationMaxPercent);
	float EffectiveTargetArmor = 0.f;

	if (TargetArmor > 0.f)
	{
		EffectiveTargetArmor = TargetArmor - (TargetArmor * ClampedArmorPenetrationPercent) / ArmorPenetrationMaxPercent;	
	}
	else
	{
		EffectiveTargetArmor = TargetArmor;
	}
	
	
	const float MitigatedDamage = SourceAttackDamage * (1.f - EffectiveTargetArmor / (EffectiveTargetArmor + ArmorMitigationConstant));

	return -MitigatedDamage;
}
