#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AGASSRespawnTuningData.generated.h"

UCLASS(BlueprintType)
class AGASSTOWER_API UAGASSRespawnTuningData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelaySeconds = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Respawn")
	bool bEnableTimedFallDeath = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Respawn", meta = (ClampMin = "0.0"))
	float MaxDangerousFallSeconds = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Respawn", meta = (ClampMin = "0.0"))
	float MinDangerousFallSpeed = 200.f;
};
