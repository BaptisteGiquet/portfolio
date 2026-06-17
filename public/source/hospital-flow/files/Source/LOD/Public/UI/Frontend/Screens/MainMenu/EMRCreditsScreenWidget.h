#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRCreditsScreenWidget.generated.h"

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRCreditsScreenWidget : public UEMRFrontendCommonActivatableWidgetBase
{
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;

private:
    void OnBackBoundActionTriggered();

    FUIActionBindingHandle BackActionHandle;
};
