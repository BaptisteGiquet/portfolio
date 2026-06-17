
#pragma once

#include "CoreMinimal.h"
#include "MobaShopItemSlotWidget.h"
#include "Blueprint/UserWidget.h"
#include "MobaShopWidget.generated.h"


class UMobaTreeWidget;
class UMobaInventoryComponent;
class UMobaShopItemSlotWidget;
class UMobaShopItem;
class UTileView;

UCLASS()
class LOD_API UMobaShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	
private:
	void LoadShopItems();
	void ShowItemCombination(const UMobaShopItemSlotWidget* SelectedItemWidget);
	void OnShopItemsLoadFinished();
	void OnShopItemWidgetGenerated(UUserWidget& NewWidget);
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTileView> TileView_ShopItemList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMobaTreeWidget> Widget_CombinationTree;

	UPROPERTY()
	TMap<TObjectPtr<UMobaShopItem>, TObjectPtr<UMobaShopItemSlotWidget>> ShopItemsMap;

	UPROPERTY()
	TObjectPtr<UMobaInventoryComponent> OwnerInventoryComponent;
};
