
#pragma once

#include "CoreMinimal.h"
#include "MobaGameplayAbility.h"
#include "MobaGameplayAbility_BlackHole.generated.h"


class UAbilityTask_WaitTargetData;
class AMobaTargetActor_BlackHole;
class UAbilityTask_PlayMontageAndWait;
class AMobaTargetActor_PickGroundTarget;

UCLASS()
class LOD_API UMobaGameplayAbility_BlackHole : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;


private:
	UFUNCTION()
	void OnMontageEnded();

	UFUNCTION()
	void PlaceBlackHole(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	
	UFUNCTION()
	void PlacementCanceled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void OnValidData(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	
	UFUNCTION()
	void OnCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void SpawnBlackHoleTargetActor();
	
	void AddAimEffect();
	void RemoveAimEffect();

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	TSubclassOf<AMobaTargetActor_PickGroundTarget> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	TSubclassOf<AMobaTargetActor_BlackHole> BlackHoleTargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	TSubclassOf<UGameplayEffect> AimEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	TSubclassOf<UGameplayEffect> BlackHoleDamageEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> BlackHoleTargetingMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> HoldBlackHoleMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Animation")
	TObjectPtr<UAnimMontage> FinalBlowMontage = nullptr;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	float BlackHoleAreaRadius = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	float TargetingMaxRange = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	float BlackHolePullSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	float BlackHoleDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Targeting")
	bool bShouldDrawDebug = false;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> PlayCastBlackHoleMontageTask = nullptr;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitTargetData> BlackHoleTargetingTask = nullptr;

	UPROPERTY()
	FGameplayAbilityTargetDataHandle CachedTargetDataHandle;
	
	FActiveGameplayEffectHandle AimEffectHandle;

	FTimerHandle SpawnBlackHoleTimerHandle;
};
