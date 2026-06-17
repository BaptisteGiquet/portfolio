

#include "MobaShopItemSlotWidget.h"

#include "Components/ListView.h"
#include "Data/Inventory/MobaShopItem.h"
#include "Framework/MobaAssetManager.h"


void UMobaShopItemSlotWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	InitWithShopItem(Cast<UMobaShopItem>(ListItemObject));

	ParentNodeListView = Cast<UListView>(IUserListEntry::GetOwningListView());
}


UUserWidget* UMobaShopItemSlotWidget::CreateTreeNodeWidget() const
{
	UMobaShopItemSlotWidget* Copy = CreateWidget<UMobaShopItemSlotWidget>(GetOwningPlayer(), GetClass(), NAME_None);
	Copy->CopyFromSource(this);
	return Copy;
}


TArray<const IMobaTreeNodeInterface*> UMobaShopItemSlotWidget::GetParentNodes() const
{
	if (!ShopItem)
	{
		return TArray<const IMobaTreeNodeInterface*>();
	}
	
	const FItemCollection* ParentItemsCollection = UMobaAssetManager::Get().GetCombinationForItem(ShopItem);
	if (!ParentItemsCollection)
	{
		return TArray<const IMobaTreeNodeInterface*>();
	}

	return ConvertItemsToInterfaces(ParentItemsCollection->GetItems());
}


TArray<const IMobaTreeNodeInterface*> UMobaShopItemSlotWidget::ConvertItemsToInterfaces(const TArray<const UMobaShopItem*>& Items) const
{
	TArray<const IMobaTreeNodeInterface*> ReturnInterfaces;
	if (!ParentNodeListView) { return ReturnInterfaces; }

	 for (const UMobaShopItem* Item : Items)
	 {
	 	const UMobaShopItemSlotWidget* ItemSlotWidget = Cast<UMobaShopItemSlotWidget>(ParentNodeListView->GetEntryWidgetFromItem(Item));
	 	if (ItemSlotWidget)
	 	{
	 		ReturnInterfaces.Add(ItemSlotWidget);
	 	}
	 }

	return ReturnInterfaces;
}


TArray<const IMobaTreeNodeInterface*> UMobaShopItemSlotWidget::GetChildNodes() const
{
	if (!ShopItem)
	{
		return TArray<const IMobaTreeNodeInterface*>();
	}
	
	const FItemCollection* IngredientItemsCollection = UMobaAssetManager::Get().GetIngredientForItem(ShopItem);
	if (!IngredientItemsCollection)
	{
		return TArray<const IMobaTreeNodeInterface*>();
	}

	return ConvertItemsToInterfaces(IngredientItemsCollection->GetItems());
}


UObject* UMobaShopItemSlotWidget::GetNodeObject() const
{
	if (!ShopItem) return nullptr;
	
	return ShopItem;
}


TObjectPtr<UMobaShopItem> UMobaShopItemSlotWidget::GetShopItem() const
{
	if (!ShopItem) return nullptr;
	
	return ShopItem;
}


void UMobaShopItemSlotWidget::CopyFromSource(const UMobaShopItemSlotWidget* SourceWidget)
{
	OnItemPurchasedIssued = SourceWidget->OnItemPurchasedIssued;
	OnItemSelected = SourceWidget->OnItemSelected;
	InitWithShopItem(SourceWidget->GetShopItem());
	ParentNodeListView = SourceWidget->ParentNodeListView;
}


void UMobaShopItemSlotWidget::InitWithShopItem(UMobaShopItem* InShopItem)
{
	ShopItem = InShopItem;
	if (!ShopItem) { return; }
	
	SetItemIconTexture(ShopItem->GetItemIcon());

	SetupItemTooltipWidget(ShopItem);
}


void UMobaShopItemSlotWidget::LeftButtonClicked()
{
	Super::LeftButtonClicked();

	OnItemSelected.Broadcast(this);
}



void UMobaShopItemSlotWidget::RightButtonClicked()
{
	Super::RightButtonClicked();

	OnItemPurchasedIssued.Broadcast(GetShopItem());
}
