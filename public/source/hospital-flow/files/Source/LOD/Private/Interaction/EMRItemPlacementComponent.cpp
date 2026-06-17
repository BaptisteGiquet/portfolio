#include "Interaction/EMRItemPlacementComponent.h"

#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/BodySetup.h"
#include "Shop/EMRItemActor.h"

UEMRItemPlacementComponent::UEMRItemPlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UEMRItemPlacementComponent::QueryPlacement(const ACharacter& Character,
	const AEMRItemActor& Item,
	const FVector& ViewLocation,
	const FVector& ViewDirection,
	FEMRItemPlacementResult& OutResult) const
{
	OutResult = FEMRItemPlacementResult();

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector SafeViewDirection = ViewDirection.GetSafeNormal();
	if (SafeViewDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (SafeViewDirection * PlacementTraceLength);

	if (bDrawDebug)
	{
		DrawDebugLine(World, TraceStart, TraceEnd, FColor::Green, false, 1.0f, 0, 2.f);
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ItemPlacementTrace), false, &Character);
	QueryParams.AddIgnoredActor(&Item);

	const bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	if (!bHit || !HitResult.bBlockingHit)
	{
		return true;
	}

	return BuildPlacementFromHit(Character, Item, HitResult, OutResult);
}

bool UEMRItemPlacementComponent::BuildPlacementFromHit(const ACharacter& Character,
	const AEMRItemActor& Item,
	const FHitResult& SurfaceHit,
	FEMRItemPlacementResult& OutResult) const
{
	OutResult = FEMRItemPlacementResult();

	if (!SurfaceHit.bBlockingHit)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	OutResult.bHasHit = true;
	OutResult.SurfaceHit = SurfaceHit;

	FTransform CandidateTransform;
	bool bValid = ValidatePlacementHit(Character, SurfaceHit, CandidateTransform);

	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = MinimumHalfExtent;
	GetItemBounds(Item, BoundsOrigin, BoundsExtent);
	const float UpOffset = BoundsExtent.Z - BoundsOrigin.Z;
	CandidateTransform.AddToTranslation(SurfaceHit.Normal * UpOffset);
	OutResult.Transform = CandidateTransform;

	if (bValid)
	{
		const FVector ClearanceHalfExtent = BoundsExtent + FVector(PlacementClearancePadding);
		bValid = HasClearanceForPlacement(
			*World,
			CandidateTransform,
			Item,
			ClearanceHalfExtent,
			BoundsOrigin,
			&Character,
			SurfaceHit.GetActor(),
			SurfaceHit.GetComponent());
	}

	OutResult.bIsValid = bValid;
	return true;
}

bool UEMRItemPlacementComponent::ValidatePlacementHit(const ACharacter& Character, const FHitResult& HitResult, FTransform& OutTransform) const
{
	const FVector Forward = Character.GetBaseAimRotation().Vector();
	const FVector ProjectedForward = FVector::VectorPlaneProject(Forward, HitResult.Normal).GetSafeNormal();
	const FVector ForwardToUse = ProjectedForward.IsNearlyZero() ? FVector::ForwardVector : ProjectedForward;
	const FRotator PlacementRotation = FRotationMatrix::MakeFromXZ(ForwardToUse, HitResult.Normal).Rotator();
	OutTransform = FTransform(PlacementRotation, HitResult.Location);

	const float DistanceFromCharacter = FVector::Dist(Character.GetActorLocation(), HitResult.Location);
	if (DistanceFromCharacter > MaxPlacementDistance)
	{
		return false;
	}

	const float SurfaceDot = FVector::DotProduct(HitResult.Normal.GetSafeNormal(), FVector::UpVector);
	const float SurfaceAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(SurfaceDot, -1.f, 1.f)));
	if (SurfaceAngle > MaxSurfaceAngleDegrees)
	{
		return false;
	}

	return true;
}

bool UEMRItemPlacementComponent::HasClearanceForPlacement(
	const UWorld& World,
	const FTransform& DesiredTransform,
	const AEMRItemActor& Item,
	const FVector& HalfExtent,
	const FVector& BoundsOrigin,
	const AActor* ActorToIgnore,
	const AActor* SurfaceActorToIgnore,
	const UPrimitiveComponent* SurfaceComponentToIgnore) const
{
	const FCollisionShape CollisionShape = FCollisionShape::MakeBox(HalfExtent);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ItemPlacementOverlap), false, ActorToIgnore);
	QueryParams.AddIgnoredActor(&Item);
	QueryParams.AddIgnoredActor(SurfaceActorToIgnore);
	QueryParams.AddIgnoredComponent(SurfaceComponentToIgnore);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	const FVector QueryCenter = DesiredTransform.TransformPosition(BoundsOrigin);
	const bool bBlocking = World.OverlapAnyTestByObjectType(
		QueryCenter,
		DesiredTransform.GetRotation(),
		ObjectParams,
		CollisionShape,
		QueryParams);

	if (bDrawDebug)
	{
		const FColor DebugColor = bBlocking ? FColor::Red : FColor::Green;
		DrawDebugBox(&World, QueryCenter, HalfExtent, DesiredTransform.GetRotation(), DebugColor, false, 1.0f);
	}

	return !bBlocking;
}

void UEMRItemPlacementComponent::GetItemBounds(const AEMRItemActor& Item, FVector& OutOrigin, FVector& OutExtent) const
{
	const UStaticMeshComponent* MeshComponent = Item.FindComponentByClass<UStaticMeshComponent>();
	FVector AbsScale(1.f, 1.f, 1.f);

	if (MeshComponent)
	{
		const FVector Scale = MeshComponent->GetComponentScale();
		AbsScale = FVector(FMath::Abs(Scale.X), FMath::Abs(Scale.Y), FMath::Abs(Scale.Z));

		if (const UStaticMesh* Mesh = MeshComponent->GetStaticMesh())
		{
			if (const UBodySetup* BodySetup = Mesh->GetBodySetup())
			{
				const FKAggregateGeom& AggGeom = BodySetup->AggGeom;
				if (AggGeom.GetElementCount() > 0)
				{
					const FBox Box = AggGeom.CalcAABB(FTransform::Identity);
					const FVector Extent = Box.GetExtent();
					if (!Extent.IsNearlyZero())
					{
						OutOrigin = Box.GetCenter() * AbsScale;
						OutExtent = Extent * AbsScale;
						return;
					}
				}
			}

			const FBoxSphereBounds MeshBounds = Mesh->GetBounds();
			const FVector MeshExtent = MeshBounds.BoxExtent;
			if (!MeshExtent.IsNearlyZero())
			{
				OutOrigin = MeshBounds.Origin * AbsScale;
				OutExtent = MeshExtent * AbsScale;
				return;
			}
		}
	}

	OutOrigin = FVector::ZeroVector;
	OutExtent = MinimumHalfExtent * AbsScale;
}
