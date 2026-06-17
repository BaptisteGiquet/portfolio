#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_IntravenousMedicationStand.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_IntravenousMedicationStand : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_IntravenousMedicationStand();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;
};
