#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/MobaGameplayAbility.h"
#include "MobaGameplayAbility_Combo.generated.h"

class UAnimMontage;

UCLASS()
class UMobaGameplayAbility_Combo : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaGameplayAbility_Combo();
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
private:
	void PlayComboMontageAndWaitTask();
	void WaitComboChangeTask();
	void WaitSameInputPressTask();
	
	void TryCommitCombo();
	TSubclassOf<UGameplayEffect> GetGameplayEffectForCurrentCombo();
	
	UFUNCTION()
	void DoDamage(FGameplayEventData EventData);
	
	UFUNCTION()
	void HandleNewInputPress(float TimeWaited);
	
	UFUNCTION()
	void OnComboChangeGameplayEventReceived(FGameplayEventData Payload);
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Gameplay Effect")
	TSubclassOf<UGameplayEffect> DefaultEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Gameplay Effect")
	TMap<FName, TSubclassOf<UGameplayEffect>> ComboEffectMap;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> ComboMontage;

	UPROPERTY(VisibleAnywhere, Category = "Ability|Animation")
	FName NextComboName;
};
