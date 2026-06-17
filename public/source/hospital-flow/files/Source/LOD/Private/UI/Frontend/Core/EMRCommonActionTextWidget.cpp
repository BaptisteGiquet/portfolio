#include "UI/Frontend/Core/EMRCommonActionTextWidget.h"

#include "CommonActionWidget.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "CommonUITypes.h"

void UEMRCommonActionTextWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (!CommonActionWidget_Action)
    {
        return;
    }

    if (!ActionRow.IsNull())
    {
        CommonActionWidget_Action->SetInputAction(ActionRow);
    }

    // Hide icon visuals and drive a text-only display.
    CommonActionWidget_Action->SetHidden(true);
    CommonActionWidget_Action->OnInputIconUpdated.AddDynamic(this, &ThisClass::HandleInputIconUpdated);
    CommonActionWidget_Action->OnInputMethodChanged.AddDynamic(this, &ThisClass::HandleInputMethodChanged);

    UpdateDisplayedText();
}

void UEMRCommonActionTextWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (CommonActionWidget_Action && !ActionRow.IsNull())
    {
        CommonActionWidget_Action->SetInputAction(ActionRow);
    }

    UpdateDisplayedText();
}

void UEMRCommonActionTextWidget::HandleInputIconUpdated()
{
    UpdateDisplayedText();
}

void UEMRCommonActionTextWidget::HandleInputMethodChanged(bool /*bUsingGamepad*/)
{
    UpdateDisplayedText();
}

void UEMRCommonActionTextWidget::UpdateDisplayedText()
{
    if (!CommonActionWidget_Action || !CommonTextBlock_ActionText)
    {
        return;
    }

    const UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer());
    if (CommonInputSubsystem && CommonInputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad)
    {
        CommonTextBlock_ActionText->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    CommonTextBlock_ActionText->SetVisibility(ESlateVisibility::HitTestInvisible);

    const FCommonInputActionDataBase* ActionData = ActionRow.GetRow<FCommonInputActionDataBase>(TEXT("CommonActionText"));
    if (ActionData)
    {
        const FCommonInputTypeInfo& InputInfo = ActionData->GetCurrentInputTypeInfo(CommonInputSubsystem);
        const FKey BoundKey = InputInfo.GetKey();
        if (BoundKey.IsValid())
        {
            CommonTextBlock_ActionText->SetText(BoundKey.GetDisplayName());
            return;
        }
    }

    CommonTextBlock_ActionText->SetText(CommonActionWidget_Action->GetDisplayText());
}
