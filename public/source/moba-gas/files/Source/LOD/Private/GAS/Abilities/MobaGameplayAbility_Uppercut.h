
#pragma once

#include "CoreMinimal.h"
#include "MobaGameplayAbility.h"
#include "MobaGameplayAbility_Uppercut.generated.h"


UCLASS()
class LOD_API UMobaGameplayAbility_Uppercut : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaGameplayAbility_Uppercut();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
private:
	void PlayUppercutMontageAndWaitTask();
	void WaitUppercutPhaseChangeTask();
	void WaitTargetsPositionsForUppercutLaunchTask();
	
	void StartClicConfirmWindow();

	UFUNCTION()
	void ApplyAllGameplayEffectsToTarget(FGameplayEventData EventData);
	
	UFUNCTION()
	void OnUppercutPhaseChangedGameplayEventReceived(FGameplayEventData EventData);

	UFUNCTION()
	void HandleBasicAttackPressed();
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> UppercutMontage;

	FName NextUppercutPhase = NAME_None;
};
