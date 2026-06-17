
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "EMRTeamAttributeSet.generated.h"

UCLASS()
class UEMRTeamAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	ATTRIBUTE_ACCESSORS_BASIC(UEMRTeamAttributeSet, TotalRevenue)
	ATTRIBUTE_ACCESSORS_BASIC(UEMRTeamAttributeSet, Reputation)

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	



private:
	UPROPERTY(ReplicatedUsing = OnRep_TotalRevenue, BlueprintReadOnly, Category = "EMR|Attributes", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData TotalRevenue;

	UPROPERTY(ReplicatedUsing = OnRep_Reputation, BlueprintReadOnly, Category = "EMR|Attributes", meta=(AllowPrivateAccess = "true"))
	FGameplayAttributeData Reputation;

	
	UFUNCTION()
	void OnRep_TotalRevenue(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Reputation(const FGameplayAttributeData& OldValue);
};
