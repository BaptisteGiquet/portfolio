#include "Actors/AGASSUtilityPlaceableActor.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

AAGASSUtilityPlaceableActor::AAGASSUtilityPlaceableActor()
{
	if (UStaticMeshComponent* const MutableItemMeshComponent = GetMutableItemMeshComponent())
	{
		MutableItemMeshComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_Yes;
	}
}

bool AAGASSUtilityPlaceableActor::CanBeClaimedBy(const AController* Controller) const
{
	if (!Super::CanBeClaimedBy(Controller))
	{
		return false;
	}

	const ACharacter* const Character = Controller != nullptr ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	const UPrimitiveComponent* const MovementBase = Character != nullptr ? Character->GetMovementBase() : nullptr;
	return MovementBase == nullptr || MovementBase != GetMovementBaseComponent();
}

bool AAGASSUtilityPlaceableActor::ShouldProvideCarryPreviewCollision() const
{
	return false;
}

void AAGASSUtilityPlaceableActor::ConfigureCarryPreviewMesh(UStaticMeshComponent& PreviewMeshComponent) const
{
	Super::ConfigureCarryPreviewMesh(PreviewMeshComponent);
}

bool AAGASSUtilityPlaceableActor::ShouldSimulatePhysicsWhenVisible() const
{
	return false;
}

bool AAGASSUtilityPlaceableActor::RequiresTouchingPlacementSupportByDefault() const
{
	return false;
}
