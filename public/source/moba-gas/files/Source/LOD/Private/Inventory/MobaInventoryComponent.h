
#pragma once

#include "CoreMinimal.h"
#include "MobaInventoryItem.h"
#include "Components/ActorComponent.h"
#include "Data/Inventory/MobaShopItem.h"
#include "MobaInventoryComponent.generated.h"


class UMobaShopItem;
class UAbilitySystemComponent;


USTRUCT(BlueprintType)
struct FInventoryEntry
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FInventoryItemHandle Handle;
	
	UPROPERTY()
	TSoftObjectPtr<UMobaShopItem> ShopItemData;
	
	UPROPERTY()
	int32 StackCount = 1;
};


DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemAddedDelegate, const UMobaInventoryItem* /* NewInventoryItemAdded */)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemRemovedDelegate, const FInventoryItemHandle& /* ItemHandle */)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnItemStackCountChangedDelegate, const FInventoryItemHandle&, int32 /* NewStackCount */)
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemAbilityCommitted, const FInventoryItemHandle&, float /* CooldownTimeRemaining*/, float /* CooldownDuration */)

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LOD_API UMobaInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMobaInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void TryPurchaseItem(const UMobaShopItem* ItemToPurchase);
	void TrySellingItem(const FInventoryItemHandle& ItemToSellHandle);
	void TryActivateItem(const FInventoryItemHandle& ItemToActivateHandle);

	int32 GetCapacity() const { return InventoryCapacity; }
	FOnItemAddedDelegate OnItemAdded;
	FOnItemRemovedDelegate OnItemRemoved;
	FOnItemStackCountChangedDelegate OnItemStackCountChanged;
	FOnItemAbilityCommitted OnItemAbilityCommitted;

	void ItemSlotChanged(const FInventoryItemHandle& Handle, int32 NewSlotNumber);
	UMobaInventoryItem* GetInventoryItemByHandle(const FInventoryItemHandle& Handle) const;

	bool IsInventoryFullFor(const UMobaShopItem* Item) const;

	bool IsAllSlotsOccupied() const;
	UMobaInventoryItem* GetAvailableStackForItem(const UMobaShopItem* Item) const;

	bool FindIngredientsForItem(const UMobaShopItem* Item, TArray<UMobaInventoryItem*>& OutIngredients, const TArray<const UMobaShopItem*>& IngredientsToIgnore);
	UMobaInventoryItem* TryGetItemForShopItem(const UMobaShopItem* ShopItem) const;

	void TryActivateItemInSlot(int32 SlotNumber);

protected:
	virtual void BeginPlay() override;

private:
	void ItemAbilityCommitted(UGameplayAbility* CommittedAbility);

	
	UFUNCTION(Server, Reliable)
	void Server_PurchaseItem(const UMobaShopItem* ItemToPurchase);

	UFUNCTION(Server, Reliable)
	void Server_SellItem(FInventoryItemHandle ItemToSellHandle);
	
	UFUNCTION(Server, Reliable)
	void Server_ActivateItem(FInventoryItemHandle ItemHandle);

	void GrantItem(const UMobaShopItem* ItemToGrant);
	void ConsumeItem(UMobaInventoryItem* ItemToConsume);
	void RemoveItem(UMobaInventoryItem* ItemToRemove);
	bool TryItemCombination(const UMobaShopItem* ItemToCheck);

	float GetOwnerGoldAmount();

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int32 InventoryCapacity = 6;
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> OwnerASC;

	UPROPERTY()
	TMap<FInventoryItemHandle, UMobaInventoryItem*> InventoryMap;

	UFUNCTION()
	void OnRep_Inventory();
	
	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	TArray<FInventoryEntry> Inventory;
};
