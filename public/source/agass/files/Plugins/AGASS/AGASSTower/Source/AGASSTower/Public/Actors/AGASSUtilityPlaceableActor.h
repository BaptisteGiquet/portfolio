#pragma once

#include "CoreMinimal.h"
#include "Actors/AGASSPlaceableItemActor.h"
#include "AGASSUtilityPlaceableActor.generated.h"

UCLASS(Abstract)
class AGASSTOWER_API AAGASSUtilityPlaceableActor : public AAGASSPlaceableItemActor
{
	GENERATED_BODY()

public:
	AAGASSUtilityPlaceableActor();

	virtual bool CanBeClaimedBy(const AController* Controller) const override;
	virtual bool ShouldProvideCarryPreviewCollision() const override;
	virtual void ConfigureCarryPreviewMesh(UStaticMeshComponent& PreviewMeshComponent) const override;

protected:
	virtual bool ShouldSimulatePhysicsWhenVisible() const override;
	virtual bool RequiresTouchingPlacementSupportByDefault() const override;
};
