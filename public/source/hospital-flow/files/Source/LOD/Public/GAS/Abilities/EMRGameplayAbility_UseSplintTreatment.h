#pragma once

#include "CoreMinimal.h"
#include "EMRGameplayAbility.h"

#include "EMRGameplayAbility_UseSplintTreatment.generated.h"

class AEMRPatient;
class AEMRItemActor;
class APlayerController;

UCLASS()
class LOD_API UEMRGameplayAbility_UseSplintTreatment : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_UseSplintTreatment();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
    FHitResult GetHitResultFromLineTrace(APlayerController* InPlayerController) const;
    USkeletalMeshComponent* FindPatientMeshForSocket(AEMRPatient& Patient) const;
    bool AttachSplintToPatient(AEMRItemActor& SplintActor, AEMRPatient& Patient) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Treatment|Splint", meta = (AllowPrivateAccess = "true"))
    FName SplintSocketName = NAME_None;
};
