#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "EMRGameplayAbility_UseLabAnalyzerTube.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_UseLabAnalyzerTube : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_UseLabAnalyzerTube();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
