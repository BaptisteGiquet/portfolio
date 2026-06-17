#include "UI/Machines/LabAnalyzer/EMRLabAnalyzerValidationErrorWidget.h"

#include "CommonTextBlock.h"

void UEMRLabAnalyzerValidationErrorWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ClearMessage();
}

void UEMRLabAnalyzerValidationErrorWidget::ShowMessage(const FText& InMessage)
{
    if (InMessage.IsEmpty())
    {
        ClearMessage();
        return;
    }

    if (CommonTextBlock_Message)
    {
        CommonTextBlock_Message->SetText(InMessage);
    }

    SetVisibility(ESlateVisibility::Visible);
}

void UEMRLabAnalyzerValidationErrorWidget::ClearMessage()
{
    if (CommonTextBlock_Message)
    {
        CommonTextBlock_Message->SetText(FText::GetEmpty());
    }

    SetVisibility(ESlateVisibility::Collapsed);
}
