#include "Data/AGASSItemDefinition.h"

#include "Actors/AGASSPlaceableItemActor.h"

bool UAGASSItemDefinition::AllowsPlacementAtTransform_Implementation(const AAGASSPlaceableItemActor* PlaceableActor, const APawn* PlacementOwner, const FTransform& CandidateTransform) const
{
	return true;
}

UClass* UAGASSItemDefinition::ResolveItemActorClass() const
{
	if (UClass* const ResolvedItemActorClass = ItemActorClass.LoadSynchronous())
	{
		return ResolvedItemActorClass;
	}

	if (UClass* const ResolvedPlaceableActorClass = PlaceableActorClass.LoadSynchronous())
	{
		return ResolvedPlaceableActorClass;
	}

	return AAGASSPlaceableItemActor::StaticClass();
}
