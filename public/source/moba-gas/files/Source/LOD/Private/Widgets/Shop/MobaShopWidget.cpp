

#include "MobaShopWidget.h"

#include "MobaShopItemSlotWidget.h"
#include "Components/TileView.h"
#include "Framework/MobaAssetManager.h"
#include "Inventory/MobaInventoryComponent.h"
#include "Widgets/MobaTreeWidget.h"

void UMobaShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetIsFocusable(true);
	LoadShopItems();
	
	// This is called whenever the TileView creates a visual row widget for an item.
	TileView_ShopItemList->OnEntryWidgetGenerated().AddUObject(this, &UMobaShopWidget::OnShopItemWidgetGenerated);

	if (GetOwningPlayerPawn())
	{
		OwnerInventoryComponent = GetOwningPlayerPawn()->GetComponentByClass<UMobaInventoryComponent>();
	}
}



void UMobaShopWidget::LoadShopItems()
{
	UMobaAssetManager::Get().LoadShopItems(FStreamableDelegate::CreateUObject(this, &UMobaShopWidget::OnShopItemsLoadFinished));
}



void UMobaShopWidget::OnShopItemsLoadFinished()
{
	TArray<const UMobaShopItem*> LoadedShopItems;
	UMobaAssetManager::Get().CollectLoadedShopItems(LoadedShopItems);

	for (const UMobaShopItem* ShopItem : LoadedShopItems)
	{
		// Populate the TileView with the loaded items.
		if (TileView_ShopItemList)
		{
			TileView_ShopItemList->AddItem(const_cast<UMobaShopItem*>(ShopItem));	
		}
	}
}



void UMobaShopWidget::OnShopItemWidgetGenerated(UUserWidget& NewWidget)
{
	UMobaShopItemSlotWidget* ShopItemWidget = Cast<UMobaShopItemSlotWidget>(&NewWidget);
	if (ShopItemWidget)
	{
		ShopItemsMap.Add(ShopItemWidget->GetShopItem(), ShopItemWidget);
		ShopItemWidget->OnItemSelected.AddUObject(this, &ThisClass::ShowItemCombination);
		
		if (OwnerInventoryComponent)
		{
			ShopItemWidget->OnItemPurchasedIssued.AddUObject(OwnerInventoryComponent, &UMobaInventoryComponent::TryPurchaseItem);
		}
	}
}


void UMobaShopWidget::ShowItemCombination(const UMobaShopItemSlotWidget* SelectedItemWidget)
{
	if (!Widget_CombinationTree) { return; }

	Widget_CombinationTree->DrawTreeFromNode(SelectedItemWidget);
}