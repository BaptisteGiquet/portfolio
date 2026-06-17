#include "UI/Interaction/EMRItemDispenserOrderWidget.h"

#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"

void UEMRItemDispenserOrderWidget::SetQuantityText(const FText& InText)
{
    if (TextBlock_OrderQuantity)
    {
        TextBlock_OrderQuantity->SetText(InText);
    }
}

void UEMRItemDispenserOrderWidget::SetTotalCostText(const FText& InText)
{
    if (TextBlock_OrderTotalCost)
    {
        TextBlock_OrderTotalCost->SetText(InText);
    }
}

void UEMRItemDispenserOrderWidget::SetTotalCostColor(const FLinearColor& InColor)
{
    if (TextBlock_OrderTotalCost)
    {
        TextBlock_OrderTotalCost->SetColorAndOpacity(FSlateColor(InColor));
    }
}
