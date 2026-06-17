#pragma once

#include "CoreMinimal.h"
#include "AGASSPlaceableItemRuntimeData.generated.h"

class UAGASSPlaceableBehaviorData;
class UStaticMesh;

USTRUCT()
struct AGASSTOWER_API FAGASSPlaceableItemRuntimeData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bUseRuntimeData = false;

	UPROPERTY()
	TSoftObjectPtr<UStaticMesh> WorldStaticMesh;

	UPROPERTY()
	TSoftObjectPtr<UAGASSPlaceableBehaviorData> BehaviorTuning;

	UPROPERTY()
	int32 SellValue = 0;

	UPROPERTY()
	bool bOverrideSimulatedMass = false;

	UPROPERTY()
	float SimulatedMassKg = 10.f;

	bool IsConfigured() const
	{
		return bUseRuntimeData;
	}

	bool HasConfiguredMesh() const
	{
		return bUseRuntimeData && !WorldStaticMesh.IsNull();
	}

	bool operator==(const FAGASSPlaceableItemRuntimeData& Other) const
	{
		return bUseRuntimeData == Other.bUseRuntimeData
			&& WorldStaticMesh == Other.WorldStaticMesh
			&& BehaviorTuning == Other.BehaviorTuning
			&& SellValue == Other.SellValue
			&& bOverrideSimulatedMass == Other.bOverrideSimulatedMass
			&& FMath::IsNearlyEqual(SimulatedMassKg, Other.SimulatedMassKg);
	}

	bool operator!=(const FAGASSPlaceableItemRuntimeData& Other) const
	{
		return !(*this == Other);
	}
};
