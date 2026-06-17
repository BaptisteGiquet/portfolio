#pragma once

#include "CoreMinimal.h"

class AAGASSPlaceableItemActor;
class UWorld;

struct FAGASSTowerStructureSnapshot
{
	TArray<TObjectPtr<AAGASSPlaceableItemActor>> Items;
	FVector TowerCenter = FVector::ZeroVector;
	FVector TowerTopLocation = FVector::ZeroVector;
	float TowerTopZ = 0.f;
	float TowerBottomZ = 0.f;
	float HeightCentimeters = 0.f;
};

bool BuildTowerStructureSnapshot(AAGASSPlaceableItemActor& SeedItem, FAGASSTowerStructureSnapshot& OutSnapshot);
bool ResolveTallestEligibleTower(UWorld& World, float MinimumHeightCentimeters, FAGASSTowerStructureSnapshot& OutSnapshot);
void ResolveImpactSelection(
	const FAGASSTowerStructureSnapshot& Snapshot,
	const FVector& ImpactPoint,
	float ImpactRadius,
	TArray<TObjectPtr<AAGASSPlaceableItemActor>>& OutAffectedItems);
