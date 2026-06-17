#include "Actors/AGASSMadCopsOfficerCharacter.h"

#include "AIController.h"
#include "Actors/AGASSMadCopsSessionEventActor.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSMadCopsOfficerInteraction, Log, All);

AAGASSMadCopsOfficerCharacter::AAGASSMadCopsOfficerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.f);

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	if (UCapsuleComponent* const OfficerCapsuleComponent = GetCapsuleComponent())
	{
		OfficerCapsuleComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	if (UCharacterMovementComponent* const MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = true;
	}
}

void AAGASSMadCopsOfficerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, OwningMadCopsEvent);
}

void AAGASSMadCopsOfficerCharacter::SetOwningMadCopsEvent(AAGASSMadCopsSessionEventActor* InOwningEvent)
{
	OwningMadCopsEvent = InOwningEvent;
	SetOwner(InOwningEvent);
}

bool AAGASSMadCopsOfficerCharacter::CanAGASSInteract_Implementation(AController* InteractingController) const
{
	const APawn* const InteractingPawn = InteractingController != nullptr ? InteractingController->GetPawn() : nullptr;
	if (InteractingPawn == nullptr)
	{
		UE_LOG(
			LogAGASSMadCopsOfficerInteraction,
			Log,
			TEXT("Officer '%s' rejected interact check because InteractingPawn is null. Controller='%s'"),
			*GetNameSafe(this),
			*GetNameSafe(InteractingController));
		return false;
	}

	if (OwningMadCopsEvent == nullptr)
	{
		UE_LOG(
			LogAGASSMadCopsOfficerInteraction,
			Log,
			TEXT("Officer '%s' rejected interact check because OwningMadCopsEvent is null. Controller='%s' Pawn='%s'"),
			*GetNameSafe(this),
			*GetNameSafe(InteractingController),
			*GetNameSafe(InteractingPawn));
		return false;
	}

	if (!OwningMadCopsEvent->CanAcceptBribe())
	{
		UE_LOG(
			LogAGASSMadCopsOfficerInteraction,
			Log,
			TEXT("Officer '%s' rejected interact check because event '%s' is not accepting bribes. Controller='%s' Pawn='%s'"),
			*GetNameSafe(this),
			*GetNameSafe(OwningMadCopsEvent),
			*GetNameSafe(InteractingController),
			*GetNameSafe(InteractingPawn));
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator IgnoredViewRotation = FRotator::ZeroRotator;
	InteractingController->GetPlayerViewPoint(ViewLocation, IgnoredViewRotation);
	const float DistanceToOfficer = FVector::Dist(ViewLocation, GetActorLocation());
	const bool bCanInteract = DistanceToOfficer <= FMath::Max(InteractionDistance, 0.f);
	UE_LOG(
		LogAGASSMadCopsOfficerInteraction,
		Log,
		TEXT("Officer '%s' interact check from Controller='%s' Pawn='%s'. ViewLocation=%s OfficerLocation=%s Distance=%.1f AllowedDistance=%.1f Result=%d"),
		*GetNameSafe(this),
		*GetNameSafe(InteractingController),
		*GetNameSafe(InteractingPawn),
		*ViewLocation.ToCompactString(),
		*GetActorLocation().ToCompactString(),
		DistanceToOfficer,
		InteractionDistance,
		bCanInteract ? 1 : 0);
	return bCanInteract;
}

float AAGASSMadCopsOfficerCharacter::GetAGASSInteractHoldDurationSeconds_Implementation(AController* InteractingController) const
{
	return 0.f;
}

void AAGASSMadCopsOfficerCharacter::HandleAGASSInteract_Implementation(AController* InteractingController)
{
	if (!HasAuthority() || OwningMadCopsEvent == nullptr)
	{
		UE_LOG(
			LogAGASSMadCopsOfficerInteraction,
			Log,
			TEXT("Officer '%s' ignored HandleAGASSInteract. HasAuthority=%d OwningEvent='%s' Controller='%s'"),
			*GetNameSafe(this),
			HasAuthority() ? 1 : 0,
			*GetNameSafe(OwningMadCopsEvent),
			*GetNameSafe(InteractingController));
		return;
	}

	const bool bAccepted = OwningMadCopsEvent->TryAcceptBribe(InteractingController);
	if (bAccepted)
	{
		MulticastPlayBribeAcceptedSound(GetActorLocation());
	}
	UE_LOG(
		LogAGASSMadCopsOfficerInteraction,
		Log,
		TEXT("Officer '%s' forwarded bribe interaction to event '%s'. Controller='%s' Result=%d"),
		*GetNameSafe(this),
		*GetNameSafe(OwningMadCopsEvent),
		*GetNameSafe(InteractingController),
		bAccepted ? 1 : 0);
}

void AAGASSMadCopsOfficerCharacter::MulticastPlayBribeAcceptedSound_Implementation(const FVector_NetQuantize SoundLocation)
{
	if (BribeAcceptedSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BribeAcceptedSound, SoundLocation);
	}
}
