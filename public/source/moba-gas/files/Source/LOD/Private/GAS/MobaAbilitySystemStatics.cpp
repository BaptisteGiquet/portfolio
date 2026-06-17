

#include "MobaAbilitySystemStatics.h"

#include "AbilitySystemGlobals.h"
#include "MobaGameplayTags.h"
#include "Algo/MaxElement.h"
#include "GAS/MobaAbilitySystemComponent.h"

float UMobaAbilitySystemStatics::GetStaticCooldownDurationForAbility(const UGameplayAbility* Ability)
{
	if (!IsValid(Ability)) return 0;

	const UGameplayEffect* CooldownEffect = Ability->GetCooldownGameplayEffect();
	if (!IsValid(CooldownEffect)) return 0.f;
	
	float CooldownDuration = 0.f;
	CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(Ability->GetAbilityLevel(), CooldownDuration);

	return CooldownDuration;
}



float UMobaAbilitySystemStatics::GetStaticCostForAbility(const UGameplayAbility* Ability)
{
	if (!IsValid(Ability)) return 0.f;

	const UGameplayEffect* CostEffect = Ability->GetCostGameplayEffect();
	if (!IsValid(CostEffect) || CostEffect->Modifiers.Num() == 0) return 0.f;


	float AbilityCost = 0.f;
	const int32 FirstModifier = 0;
	CostEffect->Modifiers[FirstModifier].ModifierMagnitude.GetStaticMagnitudeIfPossible(Ability->GetAbilityLevel(), AbilityCost);

	// Cost is a negative value so we get the absolute value
	return FMath::Abs(AbilityCost);
}



bool UMobaAbilitySystemStatics::IsAbilityAtMaxLevel(const FGameplayAbilitySpec& AbilitySpec)
{
	return AbilitySpec.Level >= 4;
}



bool UMobaAbilitySystemStatics::HasEnoughResourceToCastAbility(const FGameplayAbilitySpec& AbilitySpec, const UAbilitySystemComponent& ASC)
{
	const UGameplayAbility* AbilityCDO = AbilitySpec.Ability;
	if (AbilityCDO)
	{
		const FGameplayAbilitySpecHandle AbilitySpecHandle = AbilitySpec.Handle;
		const FGameplayAbilityActorInfo* ActorInfo = ASC.AbilityActorInfo.Get();
		
		return AbilityCDO->CheckCost(AbilitySpecHandle, ActorInfo);
	}

	return false;
}


bool UMobaAbilitySystemStatics::HasEnoughResourceToCastAbilityStatic(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC)
{
	if (AbilityCDO)
	{
		const FGameplayAbilitySpecHandle AbilitySpecHandle = FGameplayAbilitySpecHandle();
		const FGameplayAbilityActorInfo* ActorInfo = ASC.AbilityActorInfo.Get();
		
		return AbilityCDO->CheckCost(AbilitySpecHandle, ActorInfo);
	}

	return false;
}


float UMobaAbilitySystemStatics::GetManaCostForAbility(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, const int32 AbilityLevel)
{
	if (!AbilityCDO) { return 0.f; }
	
	float AbilityManaCost = 0.f;
	const UGameplayEffect* CostGameplayEffect = AbilityCDO->GetCostGameplayEffect();
	if (!CostGameplayEffect) { return 0.f; }

	const FGameplayEffectSpecHandle CostGameplayEffectSpec = ASC.MakeOutgoingSpec(CostGameplayEffect->GetClass(), AbilityLevel, ASC.MakeEffectContext());
	
	const int32 FirstModifier = 0;
	CostGameplayEffect->Modifiers[FirstModifier].ModifierMagnitude.AttemptCalculateMagnitude(*CostGameplayEffectSpec.Data.Get(), AbilityManaCost);
	
	return FMath::Abs(AbilityManaCost);
}



float UMobaAbilitySystemStatics::GetCooldownDurationForAbility(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, const int32 AbilityLevel)
{
	if (!AbilityCDO) { return 0.f; }
	
	const UGameplayEffect* CooldownGameplayEffect = AbilityCDO->GetCooldownGameplayEffect();
	if (!CooldownGameplayEffect) { return 0.f; }

	const FGameplayEffectSpecHandle CooldownGameplayEffectSpec = ASC.MakeOutgoingSpec(CooldownGameplayEffect->GetClass(), AbilityLevel, ASC.MakeEffectContext());
	
	float AbilityCooldownDuration = 0.f;
	CooldownGameplayEffect->DurationMagnitude.AttemptCalculateMagnitude(*CooldownGameplayEffectSpec.Data.Get(), AbilityCooldownDuration);
	
	return FMath::Abs(AbilityCooldownDuration);
}



float UMobaAbilitySystemStatics::GetCooldownRemainingForAbility(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC)
{
	if (!AbilityCDO) { return 0.f; }

	const FGameplayTagContainer* CooldownTags = AbilityCDO->GetCooldownTags();
	if (CooldownTags->IsEmpty()) { return 0.f; }

	const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);

	TArray<float> RemainingTimesOfActiveCooldownEffects = ASC.GetActiveEffectsTimeRemaining(CooldownQuery);
	if (RemainingTimesOfActiveCooldownEffects.IsEmpty()) { return 0.f; }

	return *Algo::MaxElement(RemainingTimesOfActiveCooldownEffects);
}



bool UMobaAbilitySystemStatics::IsHero(const AActor* ActorToCheck)
{
	return ActorHasTag(ActorToCheck,MobaCharacterTypeTags::Character_Hero);
}


bool UMobaAbilitySystemStatics::ActorHasTag(const AActor* ActorToCheck, const FGameplayTag& Tag)
{
	if (!ActorToCheck) { return false; }

	const UAbilitySystemComponent* ActorASC =  UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActorToCheck); 
	if (!ActorASC) { return false; }

	return ActorASC->HasMatchingGameplayTag(Tag);
}


bool UMobaAbilitySystemStatics::IsActorDead(const AActor* ActorToCheck)
{
	return ActorHasTag(ActorToCheck, MobaStatusTags::Status_Dead);
}
