

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "EMRFrontendCommonActivatableWidgetBase.generated.h"

class AEMRFrontendPlayerController;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendCommonActivatableWidgetBase : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintPure)
	AEMRFrontendPlayerController* GetOwningFrontendPlayerController();
	
private:
	TWeakObjectPtr<AEMRFrontendPlayerController> CachedOwningFrontendPC;
};
