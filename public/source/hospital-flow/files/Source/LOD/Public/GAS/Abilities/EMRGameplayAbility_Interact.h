#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_Interact.generated.h"

class APlayerController;
class ACharacter;
/**
 *
 */
UCLASS()
class LOD_API UEMRGameplayAbility_Interact : public UEMRGameplayAbility
{
	GENERATED_BODY()

public:
    UEMRGameplayAbility_Interact();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
    /** Radius for the detection sweep. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction", meta=(AllowPrivateAccess="true"))
    float TraceRadius = 30.f;

    /** Maximum distance at which we consider interactables. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction", meta=(AllowPrivateAccess="true"))
    float TraceDistance = 900.f;

    /** Maximum view angle (degrees) from the camera forward vector to keep a candidate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction", meta=(AllowPrivateAccess="true"))
    float MaxInteractAngle = 25.f;

    FHitResult GetHitResultFromSphereTrace(APlayerController* InPlayerController) const;
};
