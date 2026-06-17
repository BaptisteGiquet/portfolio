#pragma once

#include "CoreMinimal.h"
#include "EMRGameplayAbility.h"

#include "EMRGameplayAbility_UseIntravenousMedication.generated.h"

class UAbilityTask_WaitGameplayEvent;
class AEMRPatient;

UCLASS()
class LOD_API UEMRGameplayAbility_UseIntravenousMedication : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_UseIntravenousMedication();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
    UFUNCTION()
    void OnPatientInteractionReceived(FGameplayEventData Payload);

    bool TryCompleteTreatment(AEMRPatient& Patient) const;

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent> InteractionTask;
};
