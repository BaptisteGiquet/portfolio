#include "Animation/AGASSCharacterAnimInstance.h"

#include "AGASSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAGASSCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	AGASSCharacter = Cast<AAGASSCharacter>(TryGetPawnOwner());
}

void UAGASSCharacterAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (AGASSCharacter == nullptr)
	{
		AGASSCharacter = Cast<AAGASSCharacter>(TryGetPawnOwner());
	}

	if (AGASSCharacter == nullptr)
	{
		GroundSpeed = 0.f;
		bIsInAir = false;
		bHasCarriedUsableItem = false;
		CarriedUsableAnimationId = NAME_None;
		return;
	}

	FVector HorizontalVelocity = AGASSCharacter->GetVelocity();
	HorizontalVelocity.Z = 0.f;
	GroundSpeed = HorizontalVelocity.Size();

	if (const UCharacterMovementComponent* const MovementComponent = AGASSCharacter->GetCharacterMovement())
	{
		bIsInAir = MovementComponent->IsFalling();
	}
	else
	{
		bIsInAir = false;
	}

	bHasCarriedUsableItem = AGASSCharacter->HasCarriedUsableItem();
	CarriedUsableAnimationId = AGASSCharacter->GetCarriedUsableAnimationId();
}
