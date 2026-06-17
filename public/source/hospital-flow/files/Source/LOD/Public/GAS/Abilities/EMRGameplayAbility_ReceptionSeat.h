#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_ReceptionSeat.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_ReceptionSeat : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_ReceptionSeat();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
