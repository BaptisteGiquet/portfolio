#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_UseCoffeePitcher.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_UseCoffeePitcher : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_UseCoffeePitcher();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};

