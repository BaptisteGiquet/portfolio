
#pragma once

#include "GameplayTagContainer.h"
#include "MobaGameplayTypes.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
	None			UMETA(DisplayName = "None"),
	BasicAttack		UMETA(DisplayName = "Basic Attack"),
	Aim				UMETA(DisplayName = "Aim"),
	AbilityOne		UMETA(DisplayName = "Ability One"),
	AbilityTwo		UMETA(DisplayName = "Ability Two"),
	AbilityThree	UMETA(DisplayName = "Ability Three"),
	AbilityFour		UMETA(DisplayName = "Ability Four"),
	AbilityFive		UMETA(DisplayName = "Ability Five"),
	AbilitySix		UMETA(DisplayName = "Ability Six"),
	Confirm		UMETA(DisplayName = "Confirm"),
	Cancel		UMETA(DisplayName = "Cancel"),
};


UENUM(BlueprintType)
enum class EMobaTeam : uint8
{
	None = 255 UMETA(DisplayName = "None"),
	TeamOne = 0 UMETA(DisplayName = "Team One"),
	TeamTwo = 1 UMETA(DisplayName = "Team Two")
};


USTRUCT(BlueprintType)
struct FGameplayAbilityProperties
{
	GENERATED_BODY()

public:
	FGameplayAbilityProperties();
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (AllowAbstract = false))
	TArray<TSubclassOf<UGameplayEffect>> DamageEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (InlineEditConditionToggle))
	bool bSendPush;
	
	UPROPERTY(EditDefaultsOnly, Category="Ability", meta = (EditCondition = "bSendPush"))
	FGameplayTag PushTargetEventTag;

	UPROPERTY(EditDefaultsOnly, Category="Ability", meta = (EditCondition = "bSendPush"))
	FGameplayTag PushSelfEventTag;
};




USTRUCT(BlueprintType)
struct FHeroBaseStats : public FTableRowBase
{
	GENERATED_BODY()

public:
	FHeroBaseStats();
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> CharacterClass;

	UPROPERTY(EditAnywhere)
	float MaxHealth;

	UPROPERTY(EditAnywhere)
	float MaxMana;

	UPROPERTY(EditAnywhere)
	float Strength;

	UPROPERTY(EditAnywhere)
	float Intelligence;

	UPROPERTY(EditAnywhere)
	float Armor;

	UPROPERTY(EditAnywhere)
	float MagicResistance;
	
	
	UPROPERTY(EditAnywhere)
	float AttackDamage;
	
	UPROPERTY(EditAnywhere)
	float AbilityPower;

	UPROPERTY(EditAnywhere)
	float AttackSpeed;
	
	UPROPERTY(EditAnywhere)
	float ArmorPenetration;

	UPROPERTY(EditAnywhere)
	float MagicResistancePenetration;

	UPROPERTY(EditAnywhere)
	float MoveSpeed;

	UPROPERTY(EditAnywhere)
	float CooldownReduction;

	
	UPROPERTY(EditAnywhere)
	float StrengthGrowthRate;

	UPROPERTY(EditAnywhere)
	float IntelligenceGrowthRate;
};
