// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SlashAnimInstance.h"
#include "Characters/SlashMyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Slash/DebugMacros.h"


USlashAnimInstance::USlashAnimInstance()
{
	IsMoving = false;
	GroundSpeed = 0.f;
	EquipmentState = EEquipmentState::EES_Unequipped;
}


void USlashAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	SlashCharacter = Cast<ASlashMyCharacter>(TryGetPawnOwner());
	if (SlashCharacter)
	{
		CharacterMovementComponent = SlashCharacter->GetCharacterMovement();
	}
}

void USlashAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (CharacterMovementComponent)
	{
		FVector Velocity = CharacterMovementComponent->Velocity;
		GroundSpeed = UKismetMathLibrary::VSizeXY(Velocity);
		if (GroundSpeed > 0)
		{
			IsMoving = true;
		}
		else
		{
			IsMoving = false;
		}
		IsFalling = CharacterMovementComponent->IsFalling();
		EquipmentState = SlashCharacter->GetEquipmentState();
		CharacterState = SlashCharacter->GetCharacterState();
	}
}

