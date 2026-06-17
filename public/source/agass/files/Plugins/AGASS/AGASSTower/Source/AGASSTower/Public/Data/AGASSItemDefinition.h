#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AGASSItemDefinition.generated.h"

class AAGASSCarryableItemActor;
class AAGASSPlaceableItemActor;
class APawn;
class UAGASSPlaceableBehaviorData;
class UStaticMesh;

UENUM(BlueprintType)
enum class EAGASSPlacementValidationReason : uint8
{
	Valid UMETA(DisplayName = "Valid"),
	OutOfRange UMETA(DisplayName = "Out Of Range"),
	BlockingOverlap UMETA(DisplayName = "Blocking Overlap"),
	ItemRuleFailed UMETA(DisplayName = "Item Rule Failed")
};

UCLASS(BlueprintType, Blueprintable)
class AGASSTOWER_API UAGASSItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Runtime")
	TSoftClassPtr<AAGASSCarryableItemActor> ItemActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Presentation")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Presentation")
	TSoftObjectPtr<UStaticMesh> WorldStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Placement")
	TSoftClassPtr<AAGASSPlaceableItemActor> PlaceableActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Placement")
	TSoftObjectPtr<UAGASSPlaceableBehaviorData> BehaviorTuning;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Economy", meta = (ClampMin = "0"))
	int32 PurchaseCost = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Economy", meta = (ClampMin = "0"))
	int32 SellValue = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Placement", meta = (ClampMin = "1.0"))
	float PlacementRotationStepDegrees = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Placement", meta = (ClampMin = "0.0"))
	FVector PlacementValidationHalfExtent = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Placement", meta = (ClampMin = "0.0"))
	float PlacementValidationInset = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Physics")
	bool bEnterDormancyWhenSettled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Physics")
	bool bOverrideSimulatedMass = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Physics", meta = (EditCondition = "bOverrideSimulatedMass", ClampMin = "0.001"))
	float SimulatedMassKg = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Physics", meta = (ClampMin = "0.0"))
	float SettledDormancyDelaySeconds = 0.25f;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AGASS|Placement")
	bool AllowsPlacementAtTransform(const AAGASSPlaceableItemActor* PlaceableActor, const APawn* PlacementOwner, const FTransform& CandidateTransform) const;
	virtual bool AllowsPlacementAtTransform_Implementation(const AAGASSPlaceableItemActor* PlaceableActor, const APawn* PlacementOwner, const FTransform& CandidateTransform) const;

	UFUNCTION(BlueprintCallable, Category = "AGASS|Runtime")
	UClass* ResolveItemActorClass() const;
};
