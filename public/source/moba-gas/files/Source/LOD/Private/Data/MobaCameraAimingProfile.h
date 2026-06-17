
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MobaCameraAimingProfile.generated.h"


UCLASS(BlueprintType)
class LOD_API UMobaCameraAimingProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Aim")
	float InTransitionSpeed = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Aim")
	float OutTransitionSpeed = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Aim")
	FVector TargetCameraOffset;
	
};
