#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_CTScanMachine.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_CTScanMachine : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_CTScanMachine();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;
};
