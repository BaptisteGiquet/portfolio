#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EMRFrontendFunctionLibrary.generated.h"

class UEMRFrontendCommonActivatableWidgetBase;
/**
 * 
 */
UCLASS()
class LOD_API UEMRFrontendFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Frontend Function Library")
	static TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> GetFrontendSoftWidgetClassByTag(UPARAM(meta = (Categories = "EMR.UI.Widgets")) FGameplayTag InWidgetTag);
	
	
	UFUNCTION(BlueprintPure, Category = "Frontend Function Library")
	static TSoftObjectPtr<UTexture2D> GetOptionsSoftImageByTag(UPARAM(meta = (Categories = "EMR.UI.OptionsImages")) FGameplayTag InImageTag);	
};
