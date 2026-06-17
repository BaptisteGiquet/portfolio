#include "EMRPlayerAnimInstance.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"



UEMRPlayerAnimInstance::UEMRPlayerAnimInstance()
{
    IsMoving = false;
    GroundSpeed = 0.f;
    Direction = 0.f;
    LookTargetLocation = FVector::ZeroVector;
    ActiveSeatedAnimation = nullptr;
}




void UEMRPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	PlayerCharacter = Cast<AEMRPlayerCharacter>(TryGetPawnOwner());
	if (PlayerCharacter)
	{
		CharacterMovementComponent = PlayerCharacter->GetCharacterMovement();
	}
}

void UEMRPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
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
	}
	
    if (PlayerCharacter)
    {
        const FVector2D InputAxis = PlayerCharacter->GetLastMoveInputAxis();
        if (!InputAxis.IsNearlyZero())
        {
            Direction = FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(InputAxis.X, InputAxis.Y)));
        }
        else
        {
            FVector DirectionSource = FVector::ZeroVector;
            if (CharacterMovementComponent && GroundSpeed > 3.f)
            {
                DirectionSource = CharacterMovementComponent->Velocity.GetSafeNormal2D();
            }

            if (DirectionSource.IsNearlyZero())
            {
                Direction = 0.f;
            }
            else
            {
                float ReferenceYaw = PlayerCharacter->GetActorRotation().Yaw;
                if (PlayerCharacter->IsLocallyControlled())
                {
                    if (const AController* Controller = PlayerCharacter->GetController())
                    {
                        ReferenceYaw = Controller->GetControlRotation().Yaw;
                    }
                }

                const FRotator ReferenceRotation(0.f, ReferenceYaw, 0.f);
                const FVector ReferenceForward = FRotationMatrix(ReferenceRotation).GetUnitAxis(EAxis::X);
                const FVector ReferenceRight = FRotationMatrix(ReferenceRotation).GetUnitAxis(EAxis::Y);

                const float ForwardValue = FVector::DotProduct(DirectionSource, ReferenceForward);
                const float RightValue = FVector::DotProduct(DirectionSource, ReferenceRight);
                Direction = FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(RightValue, ForwardValue)));
            }
        }

        IsSeated = PlayerCharacter->IsSeated();
        SeatedAnimationTag = PlayerCharacter->GetSeatedAnimationTag();
        LookTargetLocation = IsSeated ? PlayerCharacter->GetLookTargetLocation() : FVector::ZeroVector;
        bLookingAtWatch = PlayerCharacter->IsLookingAtWatch();
    }
    else
    {
        Direction = 0.f;
        LookTargetLocation = FVector::ZeroVector;
        bLookingAtWatch = false;
    }

    if (IsSeated)
    {
        if (const TObjectPtr<UAnimSequenceBase>* FoundSequence = SeatedAnimationMap.Find(SeatedAnimationTag))
        {
            ActiveSeatedAnimation = *FoundSequence;
        }
        else
        {
            ActiveSeatedAnimation = DefaultSeatedAnimation;
        }
    }
    else
    {
        ActiveSeatedAnimation = nullptr;
    }
}
