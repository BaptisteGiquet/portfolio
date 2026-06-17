#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "EMRGameplayAbility_PerformLabAnalyzerTests.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_PerformLabAnalyzerTests : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_PerformLabAnalyzerTests();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
