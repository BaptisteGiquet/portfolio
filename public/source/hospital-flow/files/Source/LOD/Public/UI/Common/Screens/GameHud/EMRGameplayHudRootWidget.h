#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRGameplayHudRootWidget.generated.h"

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRGameplayHudRootWidget : public UEMRFrontendCommonActivatableWidgetBase
{
    GENERATED_BODY()

protected:
    virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
};
