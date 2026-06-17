#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EMRLoadingScreenSettings.generated.h"


UCLASS(Config = Game, DefaultConfig)
class LOD_API UEMRLoadingScreenSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	TSubclassOf<UUserWidget> GetLoadingScreenWidgetClassChecked() const;
	
	
	UPROPERTY(Config, EditAnywhere, Category = "EMR|Loading Screen")
	TSoftClassPtr<UUserWidget> LoadingScreenClass;

	UPROPERTY(Config, EditAnywhere, Category = "EMR|Loading Screen")
	int32 LoadingScreenZOrder = 10000;
	
	UPROPERTY(Config, EditAnywhere, Category = "EMR|Loading Screen")
	float HoldLoadingScreenAdditionalSecs = 3.f;
	
	UPROPERTY(Config, EditAnywhere, Category = "EMR|Loading Screen")
	bool bShowLoadingScreenInEditor = false;

	UPROPERTY(Config, EditAnywhere, Category = "EMR|Loading Screen")
	bool bHoldLoadingScreenAdditionalSecsEvenInEditor = false;

	UPROPERTY(Config, EditAnywhere, Category = "EMR|Loading Screen")
	bool bForceTickLoadingScreenEvenInEditor = true;

	UPROPERTY(Config, EditAnywhere, Category = "EMR|Loading Screen")
	float LoadingScreenHeartbeatHangDuration = 0.0f;
};
