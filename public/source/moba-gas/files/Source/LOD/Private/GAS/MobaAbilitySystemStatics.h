
#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MobaAbilitySystemStatics.generated.h"


class UGameplayAbility;
struct FGameplayAbilitySpec;

UCLASS()
class LOD_API UMobaAbilitySystemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static float GetStaticCooldownDurationForAbility(const UGameplayAbility* Ability);
	static float GetStaticCostForAbility(const UGameplayAbility* Ability);

	static bool HasEnoughResourceToCastAbility(const FGameplayAbilitySpec& AbilitySpec, const UAbilitySystemComponent& ASC);
	static bool HasEnoughResourceToCastAbilityStatic(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC);
	static bool IsAbilityAtMaxLevel(const FGameplayAbilitySpec& AbilitySpec);
	static float GetManaCostForAbility (const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, const int32 AbilityLevel);
	static float GetCooldownDurationForAbility (const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, const int32 AbilityLevel);
	static float GetCooldownRemainingForAbility (const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC);

	static bool IsHero(const AActor* ActorToCheck);
	static bool ActorHasTag(const AActor* ActorToCheck, const FGameplayTag& Tag);

	static bool IsActorDead(const AActor* ActorToCheck);
};
