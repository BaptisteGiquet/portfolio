#pragma once

#include "CoreMinimal.h"
#include "EMRGameplayAbility.h"
#include "EMRGameplayAbility_PickItem.generated.h"

class AEMRItemActor;
/**
 * 
 */
UCLASS()
class LOD_API UEMRGameplayAbility_PickItem : public UEMRGameplayAbility
{
	GENERATED_BODY()

public:
	UEMRGameplayAbility_PickItem();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	void HandleClinicItem(AEMRItemActor& Item, ACharacter& Character, AActor* CurrentlyCarried) const;
	
	AActor* FindCarriedActor(const ACharacter& Character) const;
	FVector ComputeDropLocation(const ACharacter& Character) const;
};
