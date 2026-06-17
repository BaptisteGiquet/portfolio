
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MobaShopItem.generated.h"

class UMobaShopItem;
class UGameplayAbility;
class UGameplayEffect;

USTRUCT()
struct FItemCollection
{
	GENERATED_BODY()
public:
	FItemCollection();
	FItemCollection(const TArray<const UMobaShopItem*>& InItem);
	void AddItem(const UMobaShopItem* NewItem, const bool bAddUnique = false);
	bool Contains(const UMobaShopItem* Item) const;
	const TArray<const UMobaShopItem*>& GetItems() const;
	
private:
	UPROPERTY()
	TArray<const UMobaShopItem*> Items;
};


UCLASS()
class LOD_API UMobaShopItem : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	static FPrimaryAssetType GetShopItemAssetType();

	UTexture2D* GetItemIcon() const;
	FText GetItemName() const { return ItemName; }
	FText GetItemDescription() const { return ItemDescription; }
	bool IsItemConsumable() const { return bIsItemConsumable; }
	TSubclassOf<UGameplayEffect> GetItemConsumedEffect() const { return ItemConsumedEffect; }
	bool IsItemStackable() const { return bIsItemStackable; }
	int32 GetItemMaxStackCount() const { return ItemMaxStackCount; }
	int32 GetItemPrice() const { return ItemPrice; }
	int32 GetItemSellPrice() const { return ItemPrice / 2; }
	TSubclassOf<UGameplayEffect> GetEquippedEffect() const { return ItemEquippedEffect; }
	TSubclassOf<UGameplayAbility> GetGrantedAbility() const { return ItemAbilityGranted; }
	const TArray<TSoftObjectPtr<UMobaShopItem>>& GetIngredientItems() const { return IngredientItems; }
	TMap<FGameplayTag, float> GetItemEquipEffectChangeMap() const { return ItemEquipEffectChangeMap; }
	TMap<FGameplayTag, float> GetItemConsumeEffectChangeMap() const { return ItemConsumeEffectChangeMap; }

	UGameplayAbility* GetGrantedAbilityCDO() const;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	TSoftObjectPtr<UTexture2D> ItemIcon = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	FText ItemName = FText();

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	FText ItemDescription = FText();

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem", meta = (InlineEditConditionToggle))
	bool bIsItemConsumable = false;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem", meta = (EditCondition = "bIsItemConsumable"))
	TSubclassOf<UGameplayEffect> ItemConsumedEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem", meta = (InlineEditConditionToggle))
	bool bIsItemStackable = false;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem", meta = (EditCondition = "bIsItemStackable"))
	int32 ItemMaxStackCount = 0;
	
	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	int32 ItemPrice = 0.f;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	TSubclassOf<UGameplayEffect> ItemEquippedEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	TMap<FGameplayTag, float> ItemEquipEffectChangeMap;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	TMap<FGameplayTag, float> ItemConsumeEffectChangeMap;

	
	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	TSubclassOf<UGameplayAbility> ItemAbilityGranted = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "ShopItem")
	TArray<TSoftObjectPtr<UMobaShopItem>> IngredientItems;
};
