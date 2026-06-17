
#pragma once

#include "CoreMinimal.h"
#include "MobaInventoryItemSlotWidget.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/MobaInventoryItem.h"
#include "MobaInventoryWidget.generated.h"


class UMobaInventoryContextMenuWidget;
class UMobaInventoryComponent;
class UMobaInventoryItemSlotWidget;
class UWrapBox;
class UMobaInventoryItem;
class UTextBlock;

UCLASS()
class LOD_API UMobaInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent) override;
	
private:
	void ItemAdded(const UMobaInventoryItem* InventoryItem);
	void ItemRemoved(const FInventoryItemHandle& ItemHandle);
	void ItemStackCountChanged(const FInventoryItemHandle& InventoryItemHandle, int32 NewStackCount);
	void ItemAbilityCommitted(const FInventoryItemHandle& InventoryItemHandle, float CooldownTimeRemaining, float CooldownDuration);
	
	UMobaInventoryItemSlotWidget* GetNextAvailableSlot() const;
	void HandleItemDragDrop(UMobaInventoryItemSlotWidget* DestinationWidget, UMobaInventoryItemSlotWidget* SourceWidget);

	UFUNCTION()
	void UseFocusedItem();

	UFUNCTION()
	void SellFocusedItem();
	
	void SpawnContextMenu();
	void SetContextMenuVisible(const bool bIsContextMenuVisible);
	void ToggleContextMenu(const FInventoryItemHandle& ItemHandle);
	void ClearContextMenu();

	UPROPERTY()
	FInventoryItemHandle CurrentFocusedItemHandle;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> WrapBox_ItemList;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UMobaInventoryItemSlotWidget> InventoryItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UMobaInventoryContextMenuWidget> ContextMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UMobaInventoryContextMenuWidget> ContextMenuWidget;

	UPROPERTY()
	TObjectPtr<UMobaInventoryComponent> InventoryComponent;

	UPROPERTY()
	TArray<UMobaInventoryItemSlotWidget*> InventoryItemWidgets;

	UPROPERTY()
	TMap<FInventoryItemHandle, UMobaInventoryItemSlotWidget*> PopulatedItemEntryWidgets;
	
};
