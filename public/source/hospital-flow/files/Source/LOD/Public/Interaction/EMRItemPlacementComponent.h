#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Components/ActorComponent.h"
#include "EMRItemPlacementComponent.generated.h"

class ACharacter;
class AEMRItemActor;
class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct FEMRItemPlacementResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bHasHit = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsValid = false;

	UPROPERTY(BlueprintReadOnly)
	FHitResult SurfaceHit;

	UPROPERTY(BlueprintReadOnly)
	FTransform Transform;
};

UCLASS(ClassGroup = (EMR), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRItemPlacementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEMRItemPlacementComponent();

	bool QueryPlacement(const ACharacter& Character,
		const AEMRItemActor& Item,
		const FVector& ViewLocation,
		const FVector& ViewDirection,
		FEMRItemPlacementResult& OutResult) const;

	bool BuildPlacementFromHit(const ACharacter& Character,
		const AEMRItemActor& Item,
		const FHitResult& SurfaceHit,
		FEMRItemPlacementResult& OutResult) const;

private:
	bool ValidatePlacementHit(const ACharacter& Character, const FHitResult& HitResult, FTransform& OutTransform) const;
	bool HasClearanceForPlacement(
		const UWorld& World,
		const FTransform& DesiredTransform,
		const AEMRItemActor& Item,
		const FVector& HalfExtent,
		const FVector& BoundsOrigin,
		const AActor* ActorToIgnore,
		const AActor* SurfaceActorToIgnore,
		const UPrimitiveComponent* SurfaceComponentToIgnore) const;
	void GetItemBounds(const AEMRItemActor& Item, FVector& OutOrigin, FVector& OutExtent) const;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement")
	float MaxPlacementDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement")
	float MaxSurfaceAngleDegrees = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement")
	float PlacementTraceLength = 4000.f;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement")
	FVector MinimumHalfExtent = FVector(20.f);

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement")
	float PlacementClearancePadding = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement")
	bool bDrawDebug = false;
};
