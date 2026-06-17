

#include "MobaInventoryItemDragDropOp.h"

#include "Widgets/Inventory/MobaInventoryItemSlotWidget.h"
#include "Widgets/MobaItemWidget.h"



void UMobaInventoryItemDragDropOp::SetDraggedItem(UMobaInventoryItemSlotWidget* DraggedItem)
{
	Payload = DraggedItem;

	if (DragVisualClass)
	{
		UMobaItemWidget* DragItemWidget = CreateWidget<UMobaItemWidget>(GetWorld(), DragVisualClass);
		if (DragItemWidget)
		{
			DragItemWidget->SetItemIconTexture(DraggedItem->GetIconTexture());
			DefaultDragVisual = DragItemWidget;
		}
	}
}
