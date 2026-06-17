#include "UI/Frontend/Utils/EMRFrontendFunctionLibrary.h"

#include "UI/Frontend/EMRFrontendDeveloperSettings.h"


TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> UEMRFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(UPARAM(meta = (Categories = "EMR.UI.Widgets")) FGameplayTag InWidgetTag)
{
	const UEMRFrontendDeveloperSettings* FrontendDeveloperSettings = GetDefault<UEMRFrontendDeveloperSettings>();
	if (!FrontendDeveloperSettings) return nullptr;
	
	checkf(FrontendDeveloperSettings->FrontendWidgetMap.Contains(InWidgetTag), TEXT("Could not find the corresponding widget under the tag %s"), *InWidgetTag.ToString());

	return FrontendDeveloperSettings->FrontendWidgetMap.FindRef(InWidgetTag);
}


TSoftObjectPtr<UTexture2D> UEMRFrontendFunctionLibrary::GetOptionsSoftImageByTag(FGameplayTag InImageTag)
{
	const UEMRFrontendDeveloperSettings* FrontendDeveloperSettings = GetDefault<UEMRFrontendDeveloperSettings>();
	if (!FrontendDeveloperSettings) return nullptr;
	
	checkf(FrontendDeveloperSettings->OptionsScreenSoftImageMap.Contains(InImageTag), TEXT("Could not find the corresponding image under the tag %s"), *InImageTag.ToString());

	return FrontendDeveloperSettings->OptionsScreenSoftImageMap.FindRef(InImageTag);
}
