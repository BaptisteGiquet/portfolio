#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AGASSPlaceableBehaviorData.generated.h"

UCLASS(Abstract, BlueprintType)
class AGASSTOWER_API UAGASSPlaceableBehaviorData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Placement", meta = (ClampMin = "0.0", DisplayName = "Placement Touch Tolerance", ToolTip = "Extra margin added when checking whether the item is touching another mesh for placement. Higher values make touch-gated placement more forgiving."))
	float PlacementTouchTolerance = 4.f;
};
