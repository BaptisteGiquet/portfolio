
#pragma once

#include "CoreMinimal.h"
#include "Data/Inventory/MobaShopItem.h"
#include "Engine/AssetManager.h"
#include "MobaAssetManager.generated.h"


struct FItemCollection;
class UMobaShopItem;

UCLASS()
class LOD_API UMobaAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UMobaAssetManager& Get();

	void LoadShopItems(const FStreamableDelegate& LoadFinishedCallback);
	bool CollectLoadedShopItems(TArray<const UMobaShopItem*>& OutLoadedShopItems);

	const FItemCollection* GetCombinationForItem(const UMobaShopItem* Item) const;
	const FItemCollection* GetIngredientForItem(const UMobaShopItem* Item) const;

private:
	void OnShopItemsLoadFinished(FStreamableDelegate Callback);
	void BuildItemMaps();
	void AddToCombinationMap(const UMobaShopItem* Ingredient, const UMobaShopItem* CombinationItem);

	UPROPERTY()
	TMap<const UMobaShopItem*, FItemCollection> ItemCombinationMap;

	UPROPERTY()
	TMap<const UMobaShopItem*, FItemCollection> ItemIngredientMap;
};
