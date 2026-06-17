#pragma once

#include "CoreMinimal.h"
#include "EMRGameplayAbility.h"

#include "EMRGameplayAbility_UseCastTreatment.generated.h"

class AEMRPatient;
class AEMRItemActor;
class APlayerController;

UCLASS()
class LOD_API UEMRGameplayAbility_UseCastTreatment : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_UseCastTreatment();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
    FHitResult GetHitResultFromLineTrace(APlayerController* InPlayerController) const;
    USkeletalMeshComponent* FindPatientMeshForSocket(AEMRPatient& Patient) const;
    bool AttachCastToPatient(AEMRItemActor& CastActor, AEMRPatient& Patient) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Treatment|Cast", meta = (AllowPrivateAccess = "true"))
    FName CastSocketName = NAME_None;
};
