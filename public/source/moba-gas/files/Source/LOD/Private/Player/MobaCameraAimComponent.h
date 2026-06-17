
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "MobaCameraAimComponent.generated.h"


class UCameraComponent;
class UMobaAbilitySystemComponent;
class AMobaPlayerCharacter;
class UMobaCameraAimingProfile;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOD_API UMobaCameraAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMobaCameraAimComponent();
	void MoveCameraToTargetLocation(float DeltaTime);
	bool IsCameraTransitionFinished();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	void FindAimingProfileBasedOnChildGameplayTag();

private:
	void OnAimingTagChanged(const FGameplayTag AimingTag, const int32 AimingTagCount);
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Aim", meta = (Categories = "Ability.Aiming"))
	TMap<FGameplayTag, TObjectPtr<UMobaCameraAimingProfile>> CameraAimProfileMap;

	
	UPROPERTY(Transient)
	TObjectPtr<AMobaPlayerCharacter> OwnerCharacter = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UMobaAbilitySystemComponent> OwnerASC = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCameraComponent> OwnerCamera;

	UPROPERTY(Transient)
	TObjectPtr<UMobaCameraAimingProfile> CameraAimingProfile = nullptr;
	
	FVector CameraDefaultRelativeLocation = FVector::ZeroVector;
	FVector CachedCameraRelativeLocation = FVector::ZeroVector;
	FVector CameraTargetRelativeLocation = FVector::ZeroVector;
	FVector NewCameraRelativeLocation = FVector::ZeroVector;
	
	float LerpAlpha = 0.f;
	float AimTransitionSpeed = 0.f;
	
	bool bIsAiming = false;
	FGameplayTag LastAimingTag;
};
