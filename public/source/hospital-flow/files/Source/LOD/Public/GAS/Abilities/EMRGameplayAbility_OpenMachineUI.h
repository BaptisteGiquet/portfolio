#pragma once

#include "CoreMinimal.h"
#include "EMRGameplayAbility.h"
#include "EMRGameplayAbility_OpenMachineUI.generated.h"

class AEMRPatient;
class AEMRPlayerController;
/**
 * 
 */
UCLASS()
class LOD_API UEMRGameplayAbility_OpenMachineUI : public UEMRGameplayAbility
{
	GENERATED_BODY()

public:
	UEMRGameplayAbility_OpenMachineUI();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
