#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"
#include "EMRFrontendDeveloperSettings.generated.h"

class UEMRFrontendCommonActivatableWidgetBase;
class UEMRFPSCounterWidget;
class UEMRGameplayKeybindHelperWidget;
/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Frontend UI Settings"))
class LOD_API UEMRFrontendDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "EMR|Widget Reference", meta = (ForceInlineRow, Categories = "EMR.UI.Widgets"))
	TMap<FGameplayTag, TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase>> FrontendWidgetMap;
	
	UPROPERTY(Config, EditAnywhere, Category = "EMR|Options Image Reference", meta = (ForceInlineRow, Categories = "EMR.UI.OptionsImages"))
	TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>> OptionsScreenSoftImageMap;

	UPROPERTY(Config, EditAnywhere, Category = "EMR|Debug UI")
	TSoftClassPtr<UEMRFPSCounterWidget> FPSStatsWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category = "EMR|Gameplay UI")
	TSoftClassPtr<UEMRGameplayKeybindHelperWidget> GameplayKeybindHelperWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category = "EMR|Gameplay UI")
	int32 GameplayKeybindHelperZOrder = 4200;
};
