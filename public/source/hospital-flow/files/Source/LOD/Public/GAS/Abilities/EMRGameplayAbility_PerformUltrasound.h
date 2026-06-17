#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "EMRGameplayAbility_PerformUltrasound.generated.h"

UCLASS()
class LOD_API UEMRGameplayAbility_PerformUltrasound : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_PerformUltrasound();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound", meta = (Categories = "EMR.Abilities.Exam"))
    FGameplayTag UltrasoundExamTag;
};
