
#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "UObject/Object.h"
#include "MobaInventoryItem.generated.h"


struct FOnAttributeChangeData;
class UGameplayAbility;
class UAbilitySystemComponent;
class UMobaShopItem;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGrantedAbilityCanCastUpdatedDelegate, bool /* CanCast */)


USTRUCT()
struct FInventoryItemHandle
{
	GENERATED_BODY()

public:
	FInventoryItemHandle();
	static FInventoryItemHandle CreateHandle();
	static FInventoryItemHandle GetInvalidHandle();

	bool IsValid() const;
	uint32 GetHandleID() const { return HandleID; }
	
private:
	explicit FInventoryItemHandle(const uint32 ID);
	
	static uint32 GenerateNextUniqueID();
	static uint32 GetInvalidID();

	UPROPERTY()
	uint32 HandleID;
};

bool operator==(const FInventoryItemHandle& LeftHandSide, const FInventoryItemHandle& RightHandSide);
uint32 GetTypeHash(const FInventoryItemHandle& ItemKey);



UCLASS()
class LOD_API UMobaInventoryItem : public UObject
{
	GENERATED_BODY()

public:
	void InitializeItem(const FInventoryItemHandle& InItemHandle, const UMobaShopItem* InShopItem, UAbilitySystemComponent* InAbilitySystemComponent);
	bool IsValid() const;

	FOnGrantedAbilityCanCastUpdatedDelegate OnGrantedAbilityCanCastUpdated;
	
	FInventoryItemHandle GetInventoryItemHandle() const { return ItemHandle; }
	const UMobaShopItem* GetShopItem() const { return ShopItem; }
	int32 GetItemStackCount() const { return InventoryItemStackCount; }

	void AddToStackCount();
	void ReduceStackCount();
	void SetStackCount(const int32 NewStackCount);
	
	bool IsStackFull() const;
	bool IsForItem(const UMobaShopItem* Item) const;

	bool IsGrantingThisAbility(TSubclassOf<UGameplayAbility> AbilityClass);
	bool IsGrantingAnyAbility() const;
	
	void SetItemSlotNumber(const int32 NewSlot);
	int32 GetItemSlot() const { return ItemSlotNumber; }
	
	bool TryActivateGrantedAbility();
	void ApplyItemConsumeEffect();
	void RemoveGASModifications();

	float GetGrantedAbilityCooldownTimeRemaining() const;
	float GetGrantedAbilityCooldownDuration() const;
	float GetGrantedAbilityCost() const;

	bool CanCastGrantedAbility() const;
	
	FGameplayAbilitySpecHandle GetGrantedAbilitySpecHandle() const { return GrantedAbilitySpecHandle; }
	void SetGrantedAbilitySpecHandle(FGameplayAbilitySpecHandle SpecHandle) { GrantedAbilitySpecHandle = SpecHandle; }

private:
	void ApplyGASModifications();
	void GrantedAbilityManaUpdated(const FOnAttributeChangeData& ManaData);
	
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> OwnerASC;
	
	UPROPERTY()
	TObjectPtr<UMobaShopItem> ShopItem;

	int32 InventoryItemStackCount = 1;
	int32 ItemSlotNumber = 0;

	FInventoryItemHandle ItemHandle;
	FActiveGameplayEffectHandle AppliedEquipEffectHandle;
	FActiveGameplayEffectHandle AppliedConsumeEffectHandle;
	FGameplayAbilitySpecHandle GrantedAbilitySpecHandle;
};
