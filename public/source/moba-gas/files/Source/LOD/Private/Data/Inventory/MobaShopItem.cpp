

#include "MobaShopItem.h"

#include "Abilities/GameplayAbility.h"


FItemCollection::FItemCollection()
{
	
}



FItemCollection::FItemCollection(const TArray<const UMobaShopItem*>& InItems)
{
	Items = InItems;
}



void FItemCollection::AddItem(const UMobaShopItem* NewItem, const bool bAddUnique)
{
	if (bAddUnique && Contains(NewItem)) { return; }

	Items.Add(NewItem);
}



bool FItemCollection::Contains(const UMobaShopItem* Item) const
{
	return Items.Contains(Item);
}



const TArray<const UMobaShopItem*>& FItemCollection::GetItems() const
{
	return Items;
}



FPrimaryAssetId UMobaShopItem::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(GetShopItemAssetType(), GetFName());
}



FPrimaryAssetType UMobaShopItem::GetShopItemAssetType()
{
	return FPrimaryAssetType("ShopItem");
}



UTexture2D* UMobaShopItem::GetItemIcon() const
{
	return ItemIcon.LoadSynchronous();
}


UGameplayAbility* UMobaShopItem::GetGrantedAbilityCDO() const
{
	if (ItemAbilityGranted)
	{
		return Cast<UGameplayAbility>(ItemAbilityGranted->GetDefaultObject());
	}

	return nullptr;
}
