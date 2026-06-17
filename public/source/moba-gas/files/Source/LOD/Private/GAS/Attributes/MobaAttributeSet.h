
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MobaAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class UMobaAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	// Use a predefine macro to have getter and setter 
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, Health)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, MaxHealth)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, CachedHealthPercentage)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, Mana)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, MaxMana)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, CachedManaPercentage)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, Armor)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, MagicResistance)
	
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, AttackDamage)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, AbilityPower)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, AttackSpeed)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, ArmorPenetration)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, MagicResistancePenetration)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, MoveSpeed)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, CooldownReduction)

	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, AttackDamageGrowthRate)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet, AbilityPowerGrowthRate)
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	void RescaleHealth();
	void RescaleMana();
	
private:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);

	
	UFUNCTION()
	void OnRep_AttackDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AbilityPower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MagicResistance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MagicResistancePenetration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CooldownReduction(const FGameplayAttributeData& OldValue);


	UFUNCTION()
	void OnRep_AttackDamageGrowthRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AbilityPowerGrowthRate(const FGameplayAttributeData& OldValue);

	
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Attributes|Health", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData Health;

	UPROPERTY(ReplicatedUsing = OnRep_MaxHealth, BlueprintReadOnly, Category = "Attributes|Health", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData MaxHealth;

	UPROPERTY(ReplicatedUsing = OnRep_Mana, BlueprintReadOnly, Category = "Attributes|Mana", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData Mana;
	
	UPROPERTY(ReplicatedUsing = OnRep_MaxMana, BlueprintReadOnly, Category = "Attributes|Mana", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData MaxMana;

	UPROPERTY()
	FGameplayAttributeData CachedHealthPercentage;

	UPROPERTY()
	FGameplayAttributeData CachedManaPercentage;

	
	UPROPERTY(ReplicatedUsing = OnRep_AttackDamage, BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData AttackDamage;

	UPROPERTY(ReplicatedUsing = OnRep_AbilityPower, BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData AbilityPower;

	UPROPERTY(ReplicatedUsing = OnRep_AttackSpeed, BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData AttackSpeed;

	UPROPERTY(ReplicatedUsing = OnRep_Armor, BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData Armor;

	UPROPERTY(ReplicatedUsing = OnRep_ArmorPenetration, BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData ArmorPenetration;

	UPROPERTY(ReplicatedUsing = OnRep_MagicResistance, BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData MagicResistance;

	UPROPERTY(ReplicatedUsing = OnRep_MagicResistancePenetration, BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData MagicResistancePenetration;

	UPROPERTY(ReplicatedUsing = OnRep_MoveSpeed, BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData MoveSpeed;

	UPROPERTY(ReplicatedUsing = OnRep_CooldownReduction, BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData CooldownReduction;


	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData AttackDamageGrowthRate;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData AbilityPowerGrowthRate;
};
