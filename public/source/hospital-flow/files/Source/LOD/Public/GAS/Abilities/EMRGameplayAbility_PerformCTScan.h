#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "EMRGameplayAbility_PerformCTScan.generated.h"

class AEMRPatient;

UCLASS()
class LOD_API UEMRGameplayAbility_PerformCTScan : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_PerformCTScan();

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan", meta = (Categories = "EMR.Abilities.Exam"))
    FGameplayTag CTScanExamTag;
};
