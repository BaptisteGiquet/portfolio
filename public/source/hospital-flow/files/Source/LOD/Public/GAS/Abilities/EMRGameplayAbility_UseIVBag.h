#pragma once

#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_UseIVBag.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_UseIVBag : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_UseIVBag();

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
