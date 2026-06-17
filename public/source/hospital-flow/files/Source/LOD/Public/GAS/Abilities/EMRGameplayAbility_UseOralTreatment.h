#pragma once

#include "CoreMinimal.h"
#include "EMRGameplayAbility.h"
#include "Data/EMRTreatmentForPathologyMapping.h"
#include "EMRGameplayAbility_UseOralTreatment.generated.h"

class UEMRSubsystemConfig;

/**
 * 
 */
UCLASS()
class LOD_API UEMRGameplayAbility_UseOralTreatment : public UEMRGameplayAbility
{
	GENERATED_BODY()

public:
	UEMRGameplayAbility_UseOralTreatment();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	FHitResult GetHitResultFromLineTrace(APlayerController* InPlayerController) const;
	
};
