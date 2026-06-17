#pragma once

#include "CoreMinimal.h"
#include "EMRGameplayAbility.h"
#include "EMRGameplayAbility_UseCarriedObject.generated.h"

class UEMRCarryableComponent;
/**
 * 
 */
UCLASS()
class LOD_API UEMRGameplayAbility_UseCarriedObject : public UEMRGameplayAbility
{
	GENERATED_BODY()

	UEMRGameplayAbility_UseCarriedObject();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UEMRCarryableComponent* GetCarriedComponent(const ACharacter& Character) const;
};
