#include "UI/Common/Screens/GameHud/EMRGameplayHudRootWidget.h"

#include "Input/CommonUIInputTypes.h"
#include "Input/UIActionBindingHandle.h"

TOptional<FUIInputConfig> UEMRGameplayHudRootWidget::GetDesiredInputConfig() const
{
    return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently);
}
