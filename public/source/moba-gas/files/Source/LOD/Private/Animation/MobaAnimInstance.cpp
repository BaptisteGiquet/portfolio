
#include "Animation/MobaAnimInstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/MobaCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "GAS/MobaGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/MobaPlayerCharacter.h"


void UMobaAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwnerCharacter = Cast<AMobaCharacter>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		CharacterMovement = OwnerCharacter->GetCharacterMovement();

		OwnerPlayerPawn = Cast<AMobaPlayerCharacter>(OwnerCharacter);
		
		UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter);
		if (OwnerASC)
		{
			OwnerASC->RegisterGameplayTagEvent(MobaStatusTags::Status_Aiming, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UMobaAnimInstance::OnAimingTagAdded);
		}
	}
	
}



	
void UMobaAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (CharacterMovement)
	{
		CharacterVelocity = CharacterMovement->Velocity;
		GroundSpeed = CharacterVelocity.Size2D();
		bIsMoving = (GroundSpeed != 0);
		bShouldAnimateFullBody = (GroundSpeed != 0) || (bIsAiming);
		bIsInAir = (CharacterMovement->IsFalling());
	}
	if (OwnerCharacter)
	{
		HandleLeaning(DeltaSeconds);
		HandleAimOffset(DeltaSeconds);
		HandleMovementDirection(DeltaSeconds);
	}
}




void UMobaAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
}




void UMobaAnimInstance::HandleMovementDirection(float DeltaSeconds)
{
	const float RawDirection = UKismetAnimationLibrary::CalculateDirection(CharacterVelocity, FRotator(0.f, CurrentYaw, 0.f));
	const FRotator InterpFrom = FRotator(0.f, MovementDirectionInDegrees, 0.f);
	const FRotator InterpTo = FRotator(0.f, RawDirection, 0.f);
	MovementDirectionInDegrees = FMath::RInterpTo(InterpFrom, InterpTo, DeltaSeconds, MovementDirectionInterpSpeed).Yaw;
}




void UMobaAnimInstance::HandleLeaning(float DeltaSeconds)
{
	CurrentYaw = OwnerCharacter->GetActorRotation().Yaw;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentYaw, PreviousYaw);
	const float YawRate = (DeltaYaw / DeltaSeconds);
	PreviousYaw = CurrentYaw;

	float LeanTarget = 0.f;
	LeanTarget = FMath::Clamp(YawRate / MaxYawRateForFullLean, -1.f, 1.f);
	LeanAmount = FMath::FInterpTo(LeanAmount, LeanTarget, DeltaSeconds, LeanInterpRate);
}




void UMobaAnimInstance::HandleAimOffset(float DeltaSeconds)
{
	const FRotator CameraRotation = OwnerCharacter->GetBaseAimRotation();
	const FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
	const FRotator AimDeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(CameraRotation, CharacterRotation);

	const float TargetAimYaw = -FMath::Clamp(AimDeltaRotation.Yaw * AimOffsetStrength, -AimOffsetYawClamp, AimOffsetYawClamp);
	const float TargetAimPitch = FMath::Clamp(AimDeltaRotation.Pitch * AimOffsetStrength, -AimOffsetPitchClamp, AimOffsetPitchClamp);

	AimOffsetYaw = FMath::FInterpTo(AimOffsetYaw, TargetAimYaw, DeltaSeconds, AimOffsetInterpSpeed);
	AimOffsetPitch = FMath::FInterpTo(AimOffsetPitch, TargetAimPitch, DeltaSeconds, AimOffsetInterpSpeed);
}




void UMobaAnimInstance::OnAimingTagAdded(const FGameplayTag AimingTag, int32 AimingTagCount)
{
	bIsAiming = AimingTagCount > 0;
}
