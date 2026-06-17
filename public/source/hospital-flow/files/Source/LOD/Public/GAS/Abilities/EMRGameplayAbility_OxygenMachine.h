#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_OxygenMachine.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_OxygenMachine : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_OxygenMachine();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;
};
