#include "GAS/Abilities/MobaGameplayTypes.h"



FGameplayAbilityProperties::FGameplayAbilityProperties()
{
	bSendPush = false;
	PushTargetEventTag = FGameplayTag();
	PushSelfEventTag = FGameplayTag();
}


FHeroBaseStats::FHeroBaseStats()
{
	CharacterClass = nullptr;
	MaxHealth = 0.f;
	MaxMana = 0.f;
	Strength = 0.f;
	Intelligence = 0.f;
	Armor = 0.f;
	MagicResistance = 0.f;
	AttackDamage = 0.f;
	AbilityPower = 0.f;
	AttackSpeed = 0.f;
	ArmorPenetration = 0.f;
	MagicResistancePenetration = 0.f;
	MoveSpeed = 0.f;
	CooldownReduction = 0.f;
	StrengthGrowthRate = 0.f;
	IntelligenceGrowthRate = 0.f;
}
