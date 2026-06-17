

#include "UI/EMRItemWidget.h"

#include "Components/Image.h"

void UEMRItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
}



FReply UEMRItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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



FReply UEMRItemWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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



void UEMRItemWidget::RightButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("RightButtonClicked"))
}



void UEMRItemWidget::LeftButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("LeftButtonClicked"))
}


void UEMRItemWidget::SetItemIconTexture(UTexture2D* ItemIconTexture)
{
	Image_ItemIcon->SetBrushFromTexture(ItemIconTexture);	
}

