#include "UI/Frontend/Screens/MainMenu/EMRCreditsScreenWidget.h"

#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"

void UEMRCreditsScreenWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    BackActionHandle = RegisterUIActionBinding(FBindUIActionArgs(ICommonInputModule::GetSettings().GetDefaultBackAction(),
        true,
        FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)));
}

void UEMRCreditsScreenWidget::OnBackBoundActionTriggered()
{
    DeactivateWidget();
}
