
#include "MobaAssetManager.h"

#include "Data/Inventory/MobaShopItem.h"


UMobaAssetManager& UMobaAssetManager::Get()
{
	UMobaAssetManager* Singleton = Cast<UMobaAssetManager>(GEngine->AssetManager.Get());
	if (Singleton)
	{
		return *Singleton;
	}

	UE_LOG(LogLoad, Fatal, TEXT("Asset Manager needs to be of type UMobaAssetManager"))
	return (*NewObject<UMobaAssetManager>());
}



void UMobaAssetManager::LoadShopItems(const FStreamableDelegate& LoadFinishedCallback)
{
	LoadPrimaryAssetsWithType(UMobaShopItem::GetShopItemAssetType(), TArray<FName>(), FStreamableDelegate::CreateUObject(this, &UMobaAssetManager::OnShopItemsLoadFinished, LoadFinishedCallback));
}



bool UMobaAssetManager::CollectLoadedShopItems(TArray<const UMobaShopItem*>& OutLoadedShopItems)
{
	TArray<UObject*> LoadedObjects;
	const bool bIsLoaded = GetPrimaryAssetObjectList(UMobaShopItem::GetShopItemAssetType(), LoadedObjects);

	if (bIsLoaded)
	{
		for (UObject* ObjectLoaded : LoadedObjects)
		{
			OutLoadedShopItems.Add(Cast<UMobaShopItem>(ObjectLoaded));
		}
	}

	return bIsLoaded;
}


const FItemCollection* UMobaAssetManager::GetCombinationForItem(const UMobaShopItem* Item) const
{
	if (!Item) return nullptr;
	
	return ItemCombinationMap.Find(Item);
}


const FItemCollection* UMobaAssetManager::GetIngredientForItem(const UMobaShopItem* Item) const
{
	if (!Item) return nullptr;
	
	return ItemIngredientMap.Find(Item);
}


void UMobaAssetManager::OnShopItemsLoadFinished(FStreamableDelegate Callback)
{
	Callback.ExecuteIfBound();
	BuildItemMaps();
}



void UMobaAssetManager::BuildItemMaps()
{
	TArray<const UMobaShopItem*> LoadedShopItems;
	if (CollectLoadedShopItems(LoadedShopItems))
	{
		for (const UMobaShopItem* Item : LoadedShopItems)
		{
			if (Item->GetIngredientItems().Num() == 0)
			{
				continue;
			}

			TArray<const UMobaShopItem*> ItemIngredients;
			for (const TSoftObjectPtr<UMobaShopItem>& Ingredient : Item->GetIngredientItems())
			{
				const UMobaShopItem* IngredientItem = Ingredient.LoadSynchronous();
				
				ItemIngredients.Add(IngredientItem);
				AddToCombinationMap(IngredientItem, Item);
			}

			ItemIngredientMap.Add(Item, FItemCollection{ItemIngredients});
		}
	}
}



void UMobaAssetManager::AddToCombinationMap(const UMobaShopItem* Ingredient, const UMobaShopItem* CombinationItem)
{
	
	FItemCollection* Combinations = ItemCombinationMap.Find(Ingredient);

	if (Combinations)
	{
		if (!Combinations->Contains(CombinationItem))
		{
			Combinations->AddItem(CombinationItem);
		}	
	}
	else
	{
		ItemCombinationMap.Add(Ingredient, FItemCollection{TArray<const UMobaShopItem*>{CombinationItem}});
	}
}


