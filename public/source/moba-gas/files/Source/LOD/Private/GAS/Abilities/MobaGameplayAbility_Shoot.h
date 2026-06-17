
#pragma once

#include "CoreMinimal.h"
#include "MobaGameplayAbility.h"
#include "MobaGameplayAbility_Shoot.generated.h"


class AMobaProjectileActor;

UCLASS()
class LOD_API UMobaGameplayAbility_Shoot : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaGameplayAbility_Shoot();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	
private:
	UFUNCTION()
	void StartShooting(FGameplayEventData EventData);

	UFUNCTION()
	void StopShooting(FGameplayEventData EventData);

	UFUNCTION()
	void ShootProjectile(FGameplayEventData EventData);

	AActor* GetAimTargetIfValid() const;

	void FindAimTarget();
	void StartAimTargetCheckTimer();
	void StopAimTargetCheckTimer();
	bool HasValidTarget() const;
	bool IsTargetInRange() const;

	void TargetDeadTagUpdated(const FGameplayTag DeadTag, int32 DeadTagCount);

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> ShootMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	TSubclassOf<AMobaProjectileActor> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	TSubclassOf<UGameplayEffect> ProjectileHitEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	float ShootProjectileSpeed = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	float ShootProjectileMaxDistance = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float AimTargetCheckFrequency = 0.1f;
	
	UPROPERTY()
	AActor* AimTarget = nullptr;

	UPROPERTY()
	UAbilitySystemComponent* AimTargetASC = nullptr;

	FTimerHandle AimTargetCheckTimerHandle;
};
