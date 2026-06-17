
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimInstance.h"
#include "MobaAnimInstance.generated.h"


class AMobaPlayerCharacter;
class UCharacterMovementComponent;

UCLASS()
class UMobaAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetGroundSpeed() const { return GroundSpeed; };

	UFUNCTION(BlueprintPure, Category = "Animation|Movement")
	float GetLeanAmount() const { return LeanAmount; };

	
private:
	void HandleMovementDirection(float DeltaSeconds);
	void HandleLeaning(float DeltaSeconds);
	void HandleAimOffset(float DeltaSeconds);
	
	void OnAimingTagAdded(const FGameplayTag AimingTag, int32 AimingTagCount);
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Movement")
	float MovementDirectionInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Lean")
	float LeanInterpRate = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Lean")
	float MovementThreshold = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Lean", meta=(AllowPrivateAccess = "true"))
	float MaxYawRateForFullLean = 360.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Aim", meta=(AllowPrivateAccess = "true"))
	float AimOffsetInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Aim", meta=(AllowPrivateAccess = "true"))
	float AimOffsetStrength = 20.f;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Variables",  meta=(AllowPrivateAccess = "true"))
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Variables",  meta=(AllowPrivateAccess = "true"))
	TObjectPtr<AMobaPlayerCharacter> OwnerPlayerPawn= nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Variables", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterMovementComponent> CharacterMovement = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta=(AllowPrivateAccess = "true"))
	float GroundSpeed = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta=(AllowPrivateAccess = "true"))
	FVector CharacterVelocity;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta=(AllowPrivateAccess = "true"))
	float MovementDirectionInDegrees;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta=(AllowPrivateAccess = "true"))
	bool bIsMoving = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta=(AllowPrivateAccess = "true"))
	bool bIsInAir = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta=(AllowPrivateAccess = "true"))
	bool bIsAiming = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta=(AllowPrivateAccess = "true"))
	bool bShouldAnimateFullBody = true;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta=(AllowPrivateAccess = "true"))
	float CurrentYaw = 0.f;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta=(AllowPrivateAccess = "true"))
	float CurrentPitch = 0.f;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Lean", meta=(AllowPrivateAccess = "true"))
	float LeanAmount = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Aim", meta=(AllowPrivateAccess = "true"))
	float AimOffsetYaw = 0.f;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Aim", meta=(AllowPrivateAccess = "true"))
	float AimOffsetPitch = 0.f;

	float AimOffsetYawClamp = 180.f;
	float AimOffsetPitchClamp = 90.f;
	
	float PreviousYaw = 0.f;
};
