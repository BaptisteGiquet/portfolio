
#pragma once

#include "CoreMinimal.h"
#include "MobaGameplayAbility.h"
#include "MobaGameplayAbility_Laser.generated.h"


class AMobaTargetActor_Line;

UCLASS()
class LOD_API UMobaGameplayAbility_Laser : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
private:
	UFUNCTION()
	void FireLaser(FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageEnded();

	UFUNCTION()
	void OnLaserCanceled();

	UFUNCTION()
	void TargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	void ManaUpdated(const FOnAttributeChangeData& ManaChangeData);
	
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<AMobaTargetActor_Line> LaserTargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	FName TargetActorAttachSocketName = TEXT("Laser");
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> OngoingManaCostEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> LaserMontage = nullptr;

	FActiveGameplayEffectHandle OngoingManaCostEffectHandle;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetRange = 4000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float CylinderDetectionRadius = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetingFrequency = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	bool bShouldDrawDebug = false;
};
