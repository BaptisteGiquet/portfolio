#include "UI/Interaction/EMRInteractableNameWidget.h"

#include "Components/TextBlock.h"

void UEMRInteractableNameWidget::UpdateInteractableInfo(const FText& InteractableName)
{
    if (Text_InteractableName)
    {
        Text_InteractableName->SetText(InteractableName);
        Text_InteractableName->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void UEMRInteractableNameWidget::ClearInteractableInfo()
{
    if (Text_InteractableName)
    {
        Text_InteractableName->SetText(FText::GetEmpty());
        Text_InteractableName->SetVisibility(ESlateVisibility::Collapsed);
    }
}
