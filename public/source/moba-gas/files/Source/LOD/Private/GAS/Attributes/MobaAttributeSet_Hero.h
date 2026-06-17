
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MobaAttributeSet_Hero.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
 	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
 	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
 	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
 	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


UCLASS()
class UMobaAttributeSet_Hero : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, Strength)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, Intelligence)
    ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, Experience)
    ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, PreviousLevelExperience)
    ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, NextLevelExperience)
    ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, Level)
    ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, MaxLevel)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, UpgradePoint)
    ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, Gold)

	ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, StrengthGrowthRate)
	ATTRIBUTE_ACCESSORS(UMobaAttributeSet_Hero, IntelligenceGrowthRate)
	
	virtual void GetLifetimeReplicatedProps( TArray< class FLifetimeProperty > & OutLifetimeProps ) const override;

	
private:
	UPROPERTY(ReplicatedUsing = OnRep_Strength, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData Strength;

	UPROPERTY(ReplicatedUsing = OnRep_Intelligence, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData Intelligence;
	
	UPROPERTY(ReplicatedUsing = OnRep_Experience, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData Experience;

	UPROPERTY(ReplicatedUsing = OnRep_PrevLevelExperience, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData PreviousLevelExperience;

	UPROPERTY(ReplicatedUsing = OnRep_NextLevelExperience, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData NextLevelExperience;

	UPROPERTY(ReplicatedUsing = OnRep_Level, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData Level;
	
	UPROPERTY(ReplicatedUsing = OnRep_MaxLevel, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData MaxLevel;

	UPROPERTY(ReplicatedUsing = OnRep_UpgradePoint, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData UpgradePoint;

	UPROPERTY(ReplicatedUsing = OnRep_Gold, BlueprintReadOnly, Category = "Attributes|Stats", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData Gold;


	UPROPERTY(ReplicatedUsing = OnRep_StrengthGrowthRate, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData StrengthGrowthRate;

	UPROPERTY(ReplicatedUsing = OnRep_IntelligenceGrowthRate, BlueprintReadOnly, Category = "Attributes|Level", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData IntelligenceGrowthRate;


	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_Experience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrevLevelExperience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_NextLevelExperience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Level(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxLevel(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_UpgradePoint(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Gold(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_StrengthGrowthRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_IntelligenceGrowthRate(const FGameplayAttributeData& OldValue);
};
