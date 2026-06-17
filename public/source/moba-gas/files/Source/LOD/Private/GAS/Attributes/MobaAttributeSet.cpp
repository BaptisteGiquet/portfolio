

#include "GAS/Attributes/MobaAttributeSet.h"

#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

void UMobaAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, Health, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, Mana, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, MaxMana, COND_None, REPNOTIFY_Always)

	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, AttackDamage, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, Armor, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, MagicResistance, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet, CooldownReduction, COND_None, REPNOTIFY_Always)
}




// This function is called just before an attribute is about to be changed
void UMobaAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	// NewValue will be the new Health value, so we clamp it before
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());	
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());	
	}
}




void UMobaAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

		SetCachedHealthPercentage(GetHealth() / GetMaxHealth());
	}

	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));

		SetCachedManaPercentage(GetMana() / GetMaxMana());
	}
}


void UMobaAttributeSet::RescaleHealth()
{
	if (!GetOwningActor() || !GetOwningActor()->HasAuthority()) { return; }
	
	if (GetCachedHealthPercentage() == 0.f || GetHealth() == 0.f) { return; }

	SetHealth(GetMaxHealth() * GetCachedHealthPercentage());
}


void UMobaAttributeSet::RescaleMana()
{
	if (!GetOwningActor() || !GetOwningActor()->HasAuthority()) { return; }
	
	if (GetCachedManaPercentage() == 0.f || GetMana() == 0.f) { return; }

	SetMana(GetMaxMana() * GetCachedManaPercentage());
}


void UMobaAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, Health, OldValue)
}




void UMobaAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MaxHealth, OldValue)
}




void UMobaAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, Mana, OldValue)
}




void UMobaAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MaxMana, OldValue)
}




void UMobaAttributeSet::OnRep_AttackDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AttackDamage, OldValue)
}




void UMobaAttributeSet::OnRep_AbilityPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AbilityPower, OldValue)
}




void UMobaAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AttackSpeed, OldValue)
}




void UMobaAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, Armor, OldValue)
}




void UMobaAttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, ArmorPenetration, OldValue)
}





void UMobaAttributeSet::OnRep_MagicResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MagicResistance, OldValue)
}




void UMobaAttributeSet::OnRep_MagicResistancePenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MagicResistancePenetration, OldValue)
}




void UMobaAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, MoveSpeed, OldValue)
}




void UMobaAttributeSet::OnRep_CooldownReduction(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, CooldownReduction, OldValue)
}



void UMobaAttributeSet::OnRep_AttackDamageGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AttackDamageGrowthRate, OldValue)
}



void UMobaAttributeSet::OnRep_AbilityPowerGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet, AbilityPowerGrowthRate, OldValue)
}


