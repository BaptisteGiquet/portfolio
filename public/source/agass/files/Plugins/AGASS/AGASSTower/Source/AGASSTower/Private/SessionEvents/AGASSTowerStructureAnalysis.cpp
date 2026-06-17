#include "SessionEvents/AGASSTowerStructureAnalysis.h"

#include "Actors/AGASSPlaceableItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

namespace
{
bool IsCandidateTowerItem(const AAGASSPlaceableItemActor* Candidate)
{
	return Candidate != nullptr
		&& !Candidate->IsHeldHidden()
		&& Candidate->IsPlacementCommitted();
}

float ResolveItemTopZ(const AAGASSPlaceableItemActor& Item)
{
	if (const UStaticMeshComponent* const MeshComponent = Item.GetItemMeshComponent())
	{
		const FBoxSphereBounds& Bounds = MeshComponent->Bounds;
		return Bounds.Origin.Z + Bounds.BoxExtent.Z;
	}

	return Item.GetActorLocation().Z;
}

float ResolveItemBottomZ(const AAGASSPlaceableItemActor& Item)
{
	if (const UStaticMeshComponent* const MeshComponent = Item.GetItemMeshComponent())
	{
		const FBoxSphereBounds& Bounds = MeshComponent->Bounds;
		return Bounds.Origin.Z - Bounds.BoxExtent.Z;
	}

	return Item.GetActorLocation().Z;
}

AAGASSPlaceableItemActor* ResolveTowerRoot(AAGASSPlaceableItemActor* Candidate)
{
	AAGASSPlaceableItemActor* Current = Candidate;
	TSet<const AAGASSPlaceableItemActor*> VisitedItems;

	while (Current != nullptr && IsCandidateTowerItem(Current) && !VisitedItems.Contains(Current))
	{
		VisitedItems.Add(Current);

		AAGASSPlaceableItemActor* const SupportingItem = Current->GetSupportingTowerPlaceable();
		if (!IsCandidateTowerItem(SupportingItem))
		{
			break;
		}

		Current = SupportingItem;
	}

	return Current;
}
}

bool BuildTowerStructureSnapshot(AAGASSPlaceableItemActor& SeedItem, FAGASSTowerStructureSnapshot& OutSnapshot)
{
	OutSnapshot = FAGASSTowerStructureSnapshot();

	AAGASSPlaceableItemActor* const TowerRoot = ResolveTowerRoot(&SeedItem);
	if (!IsCandidateTowerItem(TowerRoot))
	{
		return false;
	}

	TArray<AAGASSPlaceableItemActor*> PendingItems;
	TSet<AAGASSPlaceableItemActor*> VisitedItems;
	PendingItems.Add(TowerRoot);

	FVector LocationAccumulator = FVector::ZeroVector;
	bool bHasVerticalBounds = false;

	while (!PendingItems.IsEmpty())
	{
		AAGASSPlaceableItemActor* const CurrentItem = PendingItems.Pop(EAllowShrinking::No);
		if (!IsCandidateTowerItem(CurrentItem) || VisitedItems.Contains(CurrentItem))
		{
			continue;
		}

		VisitedItems.Add(CurrentItem);
		OutSnapshot.Items.Add(CurrentItem);
		LocationAccumulator += CurrentItem->GetActorLocation();

		const float ItemTopZ = ResolveItemTopZ(*CurrentItem);
		const float ItemBottomZ = ResolveItemBottomZ(*CurrentItem);
		if (!bHasVerticalBounds || ItemTopZ > OutSnapshot.TowerTopZ)
		{
			OutSnapshot.TowerTopZ = ItemTopZ;
			OutSnapshot.TowerTopLocation = FVector(CurrentItem->GetActorLocation().X, CurrentItem->GetActorLocation().Y, ItemTopZ);
		}

		if (!bHasVerticalBounds || ItemBottomZ < OutSnapshot.TowerBottomZ)
		{
			OutSnapshot.TowerBottomZ = ItemBottomZ;
		}

		bHasVerticalBounds = true;

		if (AAGASSPlaceableItemActor* const SupportingItem = CurrentItem->GetSupportingTowerPlaceable())
		{
			PendingItems.Add(SupportingItem);
		}

		TArray<AAGASSPlaceableItemActor*> SupportedItems;
		CurrentItem->GetSupportedTowerPlaceables(SupportedItems);
		PendingItems.Append(SupportedItems);
	}

	if (OutSnapshot.Items.IsEmpty() || !bHasVerticalBounds)
	{
		OutSnapshot = FAGASSTowerStructureSnapshot();
		return false;
	}

	OutSnapshot.TowerCenter = LocationAccumulator / static_cast<float>(OutSnapshot.Items.Num());
	OutSnapshot.HeightCentimeters = FMath::Max(OutSnapshot.TowerTopZ - OutSnapshot.TowerBottomZ, 0.f);
	return true;
}

bool ResolveTallestEligibleTower(UWorld& World, const float MinimumHeightCentimeters, FAGASSTowerStructureSnapshot& OutSnapshot)
{
	OutSnapshot = FAGASSTowerStructureSnapshot();

	TSet<const AAGASSPlaceableItemActor*> ProcessedRoots;
	bool bFoundTower = false;

	for (TActorIterator<AAGASSPlaceableItemActor> It(&World); It; ++It)
	{
		AAGASSPlaceableItemActor* const Candidate = *It;
		if (!IsCandidateTowerItem(Candidate))
		{
			continue;
		}

		AAGASSPlaceableItemActor* const TowerRoot = ResolveTowerRoot(Candidate);
		if (!IsCandidateTowerItem(TowerRoot) || ProcessedRoots.Contains(TowerRoot))
		{
			continue;
		}

		ProcessedRoots.Add(TowerRoot);

		FAGASSTowerStructureSnapshot CandidateSnapshot;
		if (!BuildTowerStructureSnapshot(*TowerRoot, CandidateSnapshot)
			|| CandidateSnapshot.HeightCentimeters < MinimumHeightCentimeters)
		{
			continue;
		}

		if (!bFoundTower
			|| CandidateSnapshot.HeightCentimeters > OutSnapshot.HeightCentimeters
			|| (FMath::IsNearlyEqual(CandidateSnapshot.HeightCentimeters, OutSnapshot.HeightCentimeters, 0.5f)
				&& CandidateSnapshot.TowerTopZ > OutSnapshot.TowerTopZ))
		{
			OutSnapshot = CandidateSnapshot;
			bFoundTower = true;
		}
	}

	return bFoundTower;
}

void ResolveImpactSelection(
	const FAGASSTowerStructureSnapshot& Snapshot,
	const FVector& ImpactPoint,
	const float ImpactRadius,
	TArray<TObjectPtr<AAGASSPlaceableItemActor>>& OutAffectedItems)
{
	OutAffectedItems.Reset();

	if (Snapshot.Items.IsEmpty())
	{
		return;
	}

	const float ImpactRadiusSq = FMath::Square(FMath::Max(ImpactRadius, 0.f));
	TSet<AAGASSPlaceableItemActor*> PrimaryImpactedItems;
	AAGASSPlaceableItemActor* ClosestItem = nullptr;
	float ClosestDistanceSq = TNumericLimits<float>::Max();

	for (AAGASSPlaceableItemActor* const Item : Snapshot.Items)
	{
		if (!IsCandidateTowerItem(Item))
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(Item->GetActorLocation(), ImpactPoint);
		if (DistanceSq <= ImpactRadiusSq)
		{
			PrimaryImpactedItems.Add(Item);
		}

		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestItem = Item;
		}
	}

	if (PrimaryImpactedItems.IsEmpty() && ClosestItem != nullptr)
	{
		PrimaryImpactedItems.Add(ClosestItem);
	}

	TSet<AAGASSPlaceableItemActor*> AffectedItems = PrimaryImpactedItems;
	for (AAGASSPlaceableItemActor* const PrimaryItem : PrimaryImpactedItems)
	{
		if (PrimaryItem == nullptr)
		{
			continue;
		}

		if (AAGASSPlaceableItemActor* const SupportingItem = PrimaryItem->GetSupportingTowerPlaceable())
		{
			AffectedItems.Add(SupportingItem);
		}

		TArray<AAGASSPlaceableItemActor*> SupportedItems;
		PrimaryItem->GetSupportedTowerPlaceables(SupportedItems);
		for (AAGASSPlaceableItemActor* const SupportedItem : SupportedItems)
		{
			AffectedItems.Add(SupportedItem);
		}
	}

	for (AAGASSPlaceableItemActor* const AffectedItem : AffectedItems)
	{
		if (IsCandidateTowerItem(AffectedItem))
		{
			OutAffectedItems.Add(AffectedItem);
		}
	}
}
