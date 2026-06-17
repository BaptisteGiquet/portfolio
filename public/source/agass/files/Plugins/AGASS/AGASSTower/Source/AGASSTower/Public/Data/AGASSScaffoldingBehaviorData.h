#pragma once

#include "CoreMinimal.h"
#include "Data/AGASSPlaceableBehaviorData.h"
#include "AGASSScaffoldingBehaviorData.generated.h"

class UStaticMesh;

UCLASS(BlueprintType)
class AGASSTOWER_API UAGASSScaffoldingBehaviorData : public UAGASSPlaceableBehaviorData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Scaffolding|Presentation")
	TSoftObjectPtr<UStaticMesh> StackedWorldStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Scaffolding|Placement", meta = (ClampMin = "0.0"))
	float GroundPlacementTraceDistance = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Scaffolding|Placement", meta = (ClampMin = "0.0"))
	float StackDetectionTraceSlack = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Scaffolding|PlacementDebug")
	bool bDrawPlacementDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Scaffolding|PlacementDebug", meta = (ClampMin = "0.0"))
	float PlacementDebugDrawDuration = 5.f;
};
