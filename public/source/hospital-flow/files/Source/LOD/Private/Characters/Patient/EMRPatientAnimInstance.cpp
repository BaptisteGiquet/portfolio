#include "EMRPatientAnimInstance.h"

#include "Characters/Patient/EMRPatient.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"



UEMRPatientAnimInstance::UEMRPatientAnimInstance()
{
	IsMoving = false;
	GroundSpeed = 0.f;
    ActiveSeatedAnimation = nullptr;
}


void UEMRPatientAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	PatientCharacter = Cast<AEMRPatient>(TryGetPawnOwner());
	if (PatientCharacter)
	{
		CharacterMovementComponent = PatientCharacter->GetCharacterMovement();
	}
}

void UEMRPatientAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
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

    if (PatientCharacter)
    {
        IsSeated = PatientCharacter->IsSeated();
        SeatedAnimationTag = PatientCharacter->GetSeatedAnimationTag();
    }
    else
    {
        IsSeated = false;
        SeatedAnimationTag = FGameplayTag();
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

