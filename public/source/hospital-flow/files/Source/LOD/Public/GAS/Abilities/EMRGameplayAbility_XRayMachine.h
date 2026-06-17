#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_XRayMachine.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_XRayMachine : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_XRayMachine();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;
};
