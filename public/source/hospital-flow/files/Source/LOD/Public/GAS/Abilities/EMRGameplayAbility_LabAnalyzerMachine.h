#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "EMRGameplayAbility_LabAnalyzerMachine.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_LabAnalyzerMachine : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_LabAnalyzerMachine();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
