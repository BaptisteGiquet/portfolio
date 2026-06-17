

#include "MobaItemWidget.h"

#include "MobaItemTooltipWidget.h"
#include "Components/Image.h"

void UMobaItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
}



FReply UMobaItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply SuperReply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		return FReply::Handled().SetUserFocus(TakeWidget());
	}

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return FReply::Handled().SetUserFocus(TakeWidget()).DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return SuperReply;
}



FReply UMobaItemWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply SuperReply = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	
	if (HasAnyUserFocus())
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			RightButtonClicked();
			return FReply::Handled();
		}

		if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			LeftButtonClicked();
			return FReply::Handled();
		}
	}
	
	return SuperReply;
}



void UMobaItemWidget::RightButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("RightButtonClicked"))
}



void UMobaItemWidget::LeftButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("LeftButtonClicked"))
}


void UMobaItemWidget::SetItemIconTexture(UTexture2D* ItemIconTexture)
{
	Image_ItemIcon->SetBrushFromTexture(ItemIconTexture);	
}



UMobaItemTooltipWidget* UMobaItemWidget::SetupItemTooltipWidget(const UMobaShopItem* Item)
{
	if (!Item || !GetOwningPlayer() || !ItemTooltipClass) { return nullptr; }


	UMobaItemTooltipWidget* ItemTooltipWidget = CreateWidget<UMobaItemTooltipWidget>(GetOwningPlayer(), ItemTooltipClass);
	if (ItemTooltipWidget)
	{
		ItemTooltipWidget->SetItemToolTip(Item);
		SetToolTip(ItemTooltipWidget);
	}
	
	return ItemTooltipWidget;
}
