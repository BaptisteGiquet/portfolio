

#include "MobaInventoryContextMenuWidget.h"



FOnButtonClickedEvent& UMobaInventoryContextMenuWidget::GetSellButtonClickedEvent() const
{
	return Button_SellButton->OnClicked;
}


FOnButtonClickedEvent& UMobaInventoryContextMenuWidget::GetUseButtonClickedEvent() const
{
	return Button_UseButton->OnClicked;
}
