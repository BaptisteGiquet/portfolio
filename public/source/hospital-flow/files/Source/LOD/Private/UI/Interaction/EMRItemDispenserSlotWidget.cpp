#include "UI/Interaction/EMRItemDispenserSlotWidget.h"

#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"

void UEMRItemDispenserSlotWidget::SetProductCodeText(const FText& InText)
{
    if (ProductCodeText)
    {
        ProductCodeText->SetText(InText);
    }
}

void UEMRItemDispenserSlotWidget::SetCostText(const FText& InText)
{
    if (CostText)
    {
        CostText->SetText(InText);
    }
}

void UEMRItemDispenserSlotWidget::SetCostColor(const FLinearColor& InColor)
{
    if (CostText)
    {
        CostText->SetColorAndOpacity(FSlateColor(InColor));
    }
}
