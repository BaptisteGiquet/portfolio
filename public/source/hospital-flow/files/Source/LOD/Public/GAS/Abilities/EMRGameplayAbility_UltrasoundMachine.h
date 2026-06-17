#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_UltrasoundMachine.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_UltrasoundMachine : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_UltrasoundMachine();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;
};
