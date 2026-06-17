

#include "MobaInventoryComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Data/MobaAbilitySystemGenerics.h"
#include "Data/Inventory/MobaShopItem.h"
#include "Framework/MobaAssetManager.h"
#include "GAS/MobaGameplayTags.h"
#include "GAS/Attributes/MobaAttributeSet_Hero.h"
#include "Interfaces/MobaAbilityInterface.h"
#include "Net/UnrealNetwork.h"


UMobaInventoryComponent::UMobaInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	SetIsReplicated(true);
}


void UMobaInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (OwnerASC)
	{
		OwnerASC->AbilityCommittedCallbacks.AddUObject(this, &ThisClass::ItemAbilityCommitted);	
	}
}


void UMobaInventoryComponent::ItemAbilityCommitted(UGameplayAbility* CommittedAbility)
{
	if (!CommittedAbility) { return; }
	
	float CooldownTimeRemaining = 0.f;
	float CooldownDuration = 0.f;
	CommittedAbility->GetCooldownTimeRemainingAndDuration(CommittedAbility->GetCurrentAbilitySpecHandle(), CommittedAbility->GetCurrentActorInfo(), CooldownTimeRemaining, CooldownDuration);
	
	for (TPair<FInventoryItemHandle, UMobaInventoryItem*>& ItemPair : InventoryMap)
	{
		FInventoryItemHandle ItemHandle = ItemPair.Key;
		UMobaInventoryItem* Item = ItemPair.Value;
		if (!Item) { continue; }

		if (Item->IsGrantingThisAbility(CommittedAbility->GetClass()))
		{
			OnItemAbilityCommitted.Broadcast(ItemHandle, CooldownTimeRemaining, CooldownDuration);
		}
	}
}


void UMobaInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMobaInventoryComponent, Inventory, COND_OwnerOnly, REPNOTIFY_Always)
}



void UMobaInventoryComponent::TryPurchaseItem(const UMobaShopItem* ItemToPurchase)
{
	if (!OwnerASC) { return; }

	Server_PurchaseItem(ItemToPurchase);
}


void UMobaInventoryComponent::TrySellingItem(const FInventoryItemHandle& ItemToSellHandle)
{
	Server_SellItem(ItemToSellHandle);
}


void UMobaInventoryComponent::TryActivateItem(const FInventoryItemHandle& ItemToActivateHandle)
{
	const UMobaInventoryItem* InventoryItem = GetInventoryItemByHandle(ItemToActivateHandle);
	if (!InventoryItem) { return; }

	Server_ActivateItem(ItemToActivateHandle);
}


void UMobaInventoryComponent::Server_ActivateItem_Implementation(FInventoryItemHandle ItemHandle)
{
	UMobaInventoryItem* InventoryItem = GetInventoryItemByHandle(ItemHandle);
	if (!InventoryItem) { return; }

	InventoryItem->TryActivateGrantedAbility();

	const UMobaShopItem* Item = InventoryItem->GetShopItem();
	if (!Item) { return; }

	if (Item->IsItemConsumable())
	{
		ConsumeItem(InventoryItem);
	}
}


void UMobaInventoryComponent::ConsumeItem(UMobaInventoryItem* ItemToConsume)
{
	if (!GetOwner()->HasAuthority()) { return; }
	
	if (!ItemToConsume) { return; }

	ItemToConsume->ApplyItemConsumeEffect();
	ItemToConsume->ReduceStackCount();

	const int32 NewItemStackCount = ItemToConsume->GetItemStackCount();
	const FInventoryItemHandle ItemHandle = ItemToConsume->GetInventoryItemHandle();
	
	if (NewItemStackCount <= 0)
	{
		RemoveItem(ItemToConsume);
	}
	else
	{
		OnItemStackCountChanged.Broadcast(ItemHandle, NewItemStackCount);
		for (FInventoryEntry& Entry : Inventory)
		{
			if (Entry.Handle == ItemHandle)
			{
				Entry.StackCount = NewItemStackCount;
				break;
			}
		}
	}
}


void UMobaInventoryComponent::RemoveItem(UMobaInventoryItem* ItemToRemove)
{
	if (!GetOwner()->HasAuthority()) { return; }

	const FInventoryItemHandle ItemHandle = ItemToRemove->GetInventoryItemHandle();

	ItemToRemove->RemoveGASModifications();
	OnItemRemoved.Broadcast(ItemHandle);
	InventoryMap.Remove(ItemHandle);

	for (FInventoryEntry& Entry : Inventory)
	{
		if (Entry.Handle == ItemHandle)
		{
			Entry.StackCount = 0;
			break;
		}
	}
}


bool UMobaInventoryComponent::TryItemCombination(const UMobaShopItem* ItemToCheck)
{
	if (!GetOwner()->HasAuthority()) { return false; }

	const FItemCollection* CombinationItems = UMobaAssetManager::Get().GetCombinationForItem(ItemToCheck);
	if (!CombinationItems) { return false; }

	TArray<const UMobaShopItem*> IngredientsToIgnore;
	IngredientsToIgnore.Add(ItemToCheck);
	
	for (const UMobaShopItem* CombinationItem : CombinationItems->GetItems())
	{
		TArray<UMobaInventoryItem*> Ingredients;
		
		if (!FindIngredientsForItem(CombinationItem, Ingredients, IngredientsToIgnore))
		{
			// Player doesn't have all the ingredients for this combination
			continue;
		}

		for (UMobaInventoryItem* Ingredient : Ingredients)
		{
			RemoveItem(Ingredient);
		}

		GrantItem(CombinationItem);
		return true;
	}

	return false;
}


void UMobaInventoryComponent::ItemSlotChanged(const FInventoryItemHandle& Handle, int32 NewSlotNumber)
{
	UMobaInventoryItem* FoundItem = GetInventoryItemByHandle(Handle);
	if (FoundItem)
	{
		FoundItem->SetItemSlotNumber(NewSlotNumber);
	}
}


UMobaInventoryItem* UMobaInventoryComponent::GetInventoryItemByHandle(const FInventoryItemHandle& Handle) const
{
	UMobaInventoryItem* const* FoundItem = InventoryMap.Find(Handle);
	if (FoundItem)
	{
		return *FoundItem;
	}
	
	return nullptr;
}


bool UMobaInventoryComponent::IsInventoryFullFor(const UMobaShopItem* Item) const
{
	if (!Item) { return false; }

	if (IsAllSlotsOccupied())
	{
		return GetAvailableStackForItem(Item) == nullptr;
	}

	return false;
}


bool UMobaInventoryComponent::IsAllSlotsOccupied() const
{
	return InventoryMap.Num() >= InventoryCapacity;
}


UMobaInventoryItem* UMobaInventoryComponent::GetAvailableStackForItem(const UMobaShopItem* Item) const
{
	if (!Item || !Item->IsItemStackable()) { return nullptr; }

	for (const TPair<FInventoryItemHandle, UMobaInventoryItem*>& ItemPair : InventoryMap)
	{
		if (ItemPair.Value && ItemPair.Value->IsForItem(Item) && !ItemPair.Value->IsStackFull())
		{
			return ItemPair.Value;
		}
	}

	return nullptr;
}


bool UMobaInventoryComponent::FindIngredientsForItem(const UMobaShopItem* Item, TArray<UMobaInventoryItem*>& OutIngredients, const TArray<const UMobaShopItem*>& IngredientsToIgnore)
{
	const FItemCollection* IngredientItems = UMobaAssetManager::Get().GetIngredientForItem(Item);
	if (!IngredientItems) { return false; }

	bool bAllFound = true;
	for (const UMobaShopItem* Ingredient : IngredientItems->GetItems())
	{
		if (IngredientsToIgnore.Contains(Ingredient))
		{
			continue;
		}
		
		UMobaInventoryItem* FoundItem = TryGetItemForShopItem(Ingredient);
		if (!FoundItem)
		{
			bAllFound = false;
			break;
		}

		OutIngredients.Add(FoundItem);
	}

	return bAllFound;
}


UMobaInventoryItem* UMobaInventoryComponent::TryGetItemForShopItem(const UMobaShopItem* ShopItem) const
{
	if (!ShopItem) { return nullptr; }

	for (const TPair<FInventoryItemHandle, UMobaInventoryItem*> ItemHandlePair : InventoryMap)
	{
		if (ItemHandlePair.Value && ItemHandlePair.Value->GetShopItem() == ShopItem)
		{
			return ItemHandlePair.Value;
		}
	}

	return nullptr;
}

void UMobaInventoryComponent::TryActivateItemInSlot(int32 SlotNumber)
{
	for (TPair<FInventoryItemHandle, UMobaInventoryItem*> ItemPair : InventoryMap)
	{
		if (ItemPair.Value->GetItemSlot() == SlotNumber)
		{
			Server_ActivateItem(ItemPair.Key);
		}
	}
}


void UMobaInventoryComponent::Server_SellItem_Implementation(FInventoryItemHandle ItemToSellHandle)
{
	UMobaInventoryItem* InventoryItemToSell = GetInventoryItemByHandle(ItemToSellHandle);
	if (!InventoryItemToSell || !InventoryItemToSell->IsValid()) { return; }

	const float SellPrice = InventoryItemToSell->GetShopItem()->GetItemSellPrice();

	const IMobaAbilityInterface* OwnerAbilityInterface = Cast<IMobaAbilityInterface>(OwnerASC);
	if (!OwnerAbilityInterface) { return; }
	
	const UMobaAbilitySystemGenerics* OwnerAbilitySystemGenerics = OwnerAbilityInterface->GetAbilitySystemGenerics();
	if (!OwnerAbilitySystemGenerics) { return; }

	const TSubclassOf<UGameplayEffect> GrantGoldEffectClass = OwnerAbilitySystemGenerics->GetGrantGoldEffect();
	if (!GrantGoldEffectClass) { return; }
	
	const FGameplayEffectSpecHandle GrantGoldEffectSpecHandle = OwnerASC->MakeOutgoingSpec(GrantGoldEffectClass, 1.f, OwnerASC->MakeEffectContext());
	if (!GrantGoldEffectSpecHandle.IsValid()) { return; }
	
	const float GoldAmountToAdd = SellPrice * InventoryItemToSell->GetItemStackCount();
	GrantGoldEffectSpecHandle.Data->SetSetByCallerMagnitude(MobaDataTags::Data_Shop_Item_Cost, GoldAmountToAdd);

	OwnerASC->ApplyGameplayEffectSpecToSelf(*GrantGoldEffectSpecHandle.Data.Get());

	RemoveItem(InventoryItemToSell);
}


void UMobaInventoryComponent::Server_PurchaseItem_Implementation(const UMobaShopItem* ItemToPurchase)
{
	if (!ItemToPurchase || !OwnerASC) { return; }

	if (GetOwnerGoldAmount() < ItemToPurchase->GetItemPrice())
	{
		UE_LOG(LogTemp, Warning, TEXT("Don't have enough gold"));
		return;
	}

	const IMobaAbilityInterface* OwnerAbilityInterface = Cast<IMobaAbilityInterface>(OwnerASC);
	if (!OwnerAbilityInterface) { return; }
	
	const UMobaAbilitySystemGenerics* OwnerAbilitySystemGenerics = OwnerAbilityInterface->GetAbilitySystemGenerics();
	if (!OwnerAbilitySystemGenerics) { return; }

	const TSubclassOf<UGameplayEffect> SpendGoldEffectClass = OwnerAbilitySystemGenerics->GetSpendGoldEffect();
	if (!SpendGoldEffectClass) { return; }
	
	const FGameplayEffectSpecHandle SpendGoldEffectSpecHandle = OwnerASC->MakeOutgoingSpec(SpendGoldEffectClass, 1.f, OwnerASC->MakeEffectContext());
	if (!SpendGoldEffectSpecHandle.IsValid()) { return; }
	
	const float GoldAmountToRemove = -ItemToPurchase->GetItemPrice();
	SpendGoldEffectSpecHandle.Data->SetSetByCallerMagnitude(MobaDataTags::Data_Shop_Item_Cost, GoldAmountToRemove);

	if (!IsInventoryFullFor(ItemToPurchase))
	{
		OwnerASC->ApplyGameplayEffectSpecToSelf(*SpendGoldEffectSpecHandle.Data.Get());
		GrantItem(ItemToPurchase);
		return;
	}

	if (TryItemCombination(ItemToPurchase))
	{
		OwnerASC->ApplyGameplayEffectSpecToSelf(*SpendGoldEffectSpecHandle.Data.Get());
	}
}



void UMobaInventoryComponent::GrantItem(const UMobaShopItem* ItemToGrant)
{
	if (!GetOwner()->HasAuthority()) { return; }

	UMobaInventoryItem* InventoryStackItem = GetAvailableStackForItem(ItemToGrant);
	
	if (InventoryStackItem)
	{
		InventoryStackItem->AddToStackCount();
		OnItemStackCountChanged.Broadcast(InventoryStackItem->GetInventoryItemHandle(), InventoryStackItem->GetItemStackCount());

		for (FInventoryEntry& InventoryEntry : Inventory)
		{
			if (InventoryEntry.Handle == InventoryStackItem->GetInventoryItemHandle())
			{
				InventoryEntry.StackCount = InventoryStackItem->GetItemStackCount();
				break;
			}
		}
	}
	else
	{
		if (TryItemCombination(ItemToGrant))
		{
			return;
		}
		
		UMobaInventoryItem* InventoryItem = NewObject<UMobaInventoryItem>();
		FInventoryItemHandle InventoryItemHandle = FInventoryItemHandle::CreateHandle();

		InventoryItem->InitializeItem(InventoryItemHandle, ItemToGrant, OwnerASC);
		InventoryMap.Add(InventoryItemHandle, InventoryItem);
		OnItemAdded.Broadcast(InventoryItem);
		

		FInventoryEntry NewEntry;
		NewEntry.Handle = InventoryItemHandle;
		NewEntry.ShopItemData = const_cast<UMobaShopItem*>(ItemToGrant);
		NewEntry.StackCount = 1;

		Inventory.Add(NewEntry);
	}
}



float UMobaInventoryComponent::GetOwnerGoldAmount()
{
	if (!OwnerASC) { return 0.f; }
	
	bool bFound = false;
	const float OwnerGoldAmount = OwnerASC->GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetGoldAttribute(), bFound);
	if (bFound)
	{
		return OwnerGoldAmount;
	}

	return 0.f;
}



void UMobaInventoryComponent::OnRep_Inventory()
{
	for (const FInventoryEntry& InventoryEntry : Inventory)
	{
		if (!InventoryMap.Contains(InventoryEntry.Handle))
		{
			UMobaInventoryItem* LocalItem = NewObject<UMobaInventoryItem>(this);

			const UMobaShopItem* ShopItem = InventoryEntry.ShopItemData.LoadSynchronous();

			LocalItem->InitializeItem(InventoryEntry.Handle, ShopItem, OwnerASC);

			InventoryMap.Add(InventoryEntry.Handle, LocalItem);

			OnItemAdded.Broadcast(LocalItem);
		}
		
		UMobaInventoryItem* FoundItem = InventoryMap.FindRef(InventoryEntry.Handle);
		if (FoundItem)
		{
			const int32 NewItemStackCount = InventoryEntry.StackCount;
		
			if (InventoryEntry.StackCount <= 0)
			{
				OnItemRemoved.Broadcast(InventoryEntry.Handle);
				RemoveItem(GetInventoryItemByHandle(InventoryEntry.Handle));
			}
			else
			{
				FoundItem->SetStackCount(NewItemStackCount);
				OnItemStackCountChanged.Broadcast(InventoryEntry.Handle, NewItemStackCount);
			}
		}
	}
}




