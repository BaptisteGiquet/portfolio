

#include "Shop/EMRItemData.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"


FItemCollection::FItemCollection()
{
	
}



FItemCollection::FItemCollection(const TArray<const UEMRItemData*>& InItems)
{
	Items = InItems;
}



void FItemCollection::AddItem(const UEMRItemData* NewItem, const bool bAddUnique)
{
	if (bAddUnique && Contains(NewItem)) { return; }

	Items.Add(NewItem);
}



bool FItemCollection::Contains(const UEMRItemData* Item) const
{
	return Items.Contains(Item);
}



const TArray<const UEMRItemData*>& FItemCollection::GetItems() const
{
	return Items;
}



FPrimaryAssetId UEMRItemData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(GetShopItemAssetType(), GetFName());
}



FPrimaryAssetType UEMRItemData::GetShopItemAssetType()
{
	return FPrimaryAssetType("ShopItem");
}



UTexture2D* UEMRItemData::GetItemIcon() const
{
    return ItemIcon.LoadSynchronous();
}


UStaticMesh* UEMRItemData::GetWorldItemMesh() const
{
	return WorldItemMesh.LoadSynchronous();
}

UStaticMesh* UEMRItemData::GetFilledOverlayMesh() const
{
	return FilledOverlayMesh.LoadSynchronous();
}

UMaterialInterface* UEMRItemData::GetFilledOverlayMaterial() const
{
	return FilledOverlayMaterial.LoadSynchronous();
}

USoundBase* UEMRItemData::GetInteractionSound() const
{
	return InteractionSound.LoadSynchronous();
}

USoundBase* UEMRItemData::GetUseInteractionSound() const
{
	return UseInteractionSound.LoadSynchronous();
}
