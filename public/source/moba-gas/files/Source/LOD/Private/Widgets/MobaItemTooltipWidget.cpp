

#include "MobaItemTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/Inventory/MobaShopItem.h"


void UMobaItemTooltipWidget::SetItemToolTip(const UMobaShopItem* Item)
{
	Image_ItemIcon->SetBrushFromTexture(Item->GetItemIcon());
	Text_ItemTitle->SetText(Item->GetItemName());
	Text_ItemDescription->SetText(Item->GetItemDescription());
	Text_ItemPrice->SetText(FText::AsNumber(Item->GetItemPrice()));
}



void UMobaItemTooltipWidget::SetPrice(const float NewPrice)
{
	Text_ItemPrice->SetText(FText::AsNumber(NewPrice));
}
