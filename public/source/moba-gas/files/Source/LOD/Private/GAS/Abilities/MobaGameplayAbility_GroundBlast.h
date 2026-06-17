
#pragma once

#include "CoreMinimal.h"
#include "MobaGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "MobaGameplayAbility_GroundBlast.generated.h"


class AMobaTargetActor_PickGroundTarget;

UCLASS()
class LOD_API UMobaGameplayAbility_GroundBlast : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaGameplayAbility_GroundBlast();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	void PlayGroundBlastMontageTask();
	void SetupTargetActorVariables(AGameplayAbilityTargetActor* TargetActorSpawned);
	void HandlePushingTargets(const FGameplayAbilityTargetDataHandle& TargetData, AActor* Target);
	void WaitGroundBlastTargetDataTask();

	UFUNCTION()
	void OnTargetConfirmed(const FGameplayAbilityTargetDataHandle& TargetData);

	UFUNCTION()
	void OnTargetCanceled(const FGameplayAbilityTargetDataHandle& TargetData);
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	TSubclassOf<AMobaTargetActor_PickGroundTarget> TargetActorClass; 
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> GroundBlastTargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> GroundBlastCastingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|VFX")
	FGameplayTag BlastGameplayCueTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	float TargetingAreaRadius = 300.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	float TargetingMaxRange = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	bool bShouldTargetHostile = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	bool bShouldTargetFriendly = false;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	bool bShouldDrawDebug = false;
};
