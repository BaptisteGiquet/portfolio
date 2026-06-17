

#include "MobaInventoryItem.h"

#include "AbilitySystemComponent.h"
#include "Data/Inventory/MobaShopItem.h"
#include "GAS/MobaAbilitySystemStatics.h"
#include "GAS/Attributes/MobaAttributeSet.h"


FInventoryItemHandle::FInventoryItemHandle()
{
	HandleID = GetInvalidID();
}


FInventoryItemHandle FInventoryItemHandle::CreateHandle()
{
	return FInventoryItemHandle(GenerateNextUniqueID());
}


FInventoryItemHandle::FInventoryItemHandle(const uint32 ID)
{
	HandleID = ID;
}


uint32 FInventoryItemHandle::GenerateNextUniqueID()
{
	static uint32 StaticID = 1;
	// return StaticID then increment for the next item
	return StaticID++;
}


FInventoryItemHandle FInventoryItemHandle::GetInvalidHandle()
{
	static FInventoryItemHandle InvalidHandle = FInventoryItemHandle();
	return InvalidHandle;
}


bool FInventoryItemHandle::IsValid() const
{
	return HandleID != GetInvalidID();
}






void UMobaInventoryItem::InitializeItem(const FInventoryItemHandle& InItemHandle, const UMobaShopItem* InShopItem, UAbilitySystemComponent* InAbilitySystemComponent)
{
	ItemHandle = InItemHandle;
	ShopItem = const_cast<UMobaShopItem*>(InShopItem);
	OwnerASC = InAbilitySystemComponent;
	
	
	if (OwnerASC)
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet::GetManaAttribute()).AddUObject(this, &ThisClass::GrantedAbilityManaUpdated);
	}

	ApplyGASModifications();
}


bool UMobaInventoryItem::IsValid() const
{
	return ShopItem != nullptr;
}


uint32 FInventoryItemHandle::GetInvalidID()
{
	return 0;
}


bool operator==(const FInventoryItemHandle& LeftHandSide, const FInventoryItemHandle& RightHandSide)
{
	return LeftHandSide.GetHandleID() == RightHandSide.GetHandleID();
}


uint32 GetTypeHash(const FInventoryItemHandle& ItemKey)
{
	return ItemKey.GetHandleID();
}


void UMobaInventoryItem::AddToStackCount()
{
	if (!IsStackFull())
	{
		++InventoryItemStackCount;
	}
}


void UMobaInventoryItem::ReduceStackCount()
{
	if (InventoryItemStackCount > 0)
	{
		--InventoryItemStackCount;
	}
}


void UMobaInventoryItem::SetStackCount(const int32 NewStackCount)
{
	if (NewStackCount > 0 && NewStackCount <= GetShopItem()->GetItemMaxStackCount())
	{
		InventoryItemStackCount = NewStackCount;	
	}
}


bool UMobaInventoryItem::IsStackFull() const
{
	return InventoryItemStackCount >= GetShopItem()->GetItemMaxStackCount();
}


bool UMobaInventoryItem::IsForItem(const UMobaShopItem* Item) const
{
	if (!Item) { return false; }

	return GetShopItem() == Item;
}


bool UMobaInventoryItem::IsGrantingThisAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!ShopItem) { return false; }

	const TSubclassOf<UGameplayAbility> GrantedAbility = ShopItem->GetGrantedAbility();
	if (GrantedAbility == AbilityClass)
	{
		return true;
	}

	return false;
}


bool UMobaInventoryItem::IsGrantingAnyAbility() const
{
	if (!ShopItem) { return false; }

	const TSubclassOf<UGameplayAbility> GrantedAbility = ShopItem->GetGrantedAbility();
	if (GrantedAbility)
	{
		return true;
	}

	return false;
}


void UMobaInventoryItem::SetItemSlotNumber(const int32 NewSlot)
{
	ItemSlotNumber = NewSlot;
}


void UMobaInventoryItem::ApplyGASModifications()
{
	if (!OwnerASC || !ShopItem) { return; }

	if (!OwnerASC->GetOwner() || !OwnerASC->GetOwner()->HasAuthority()) { return; }
	
	const TSubclassOf<UGameplayEffect> EquipEffectClass = GetShopItem()->GetEquippedEffect();
	if (EquipEffectClass)
	{
		const FGameplayEffectSpecHandle EquipEffectSpecHandle = OwnerASC->MakeOutgoingSpec(EquipEffectClass, 1, OwnerASC->MakeEffectContext());
		if (!EquipEffectSpecHandle.IsValid()) { return; }
		
		FGameplayEffectSpec* EquipEffectSpec = EquipEffectSpecHandle.Data.Get();

		const TMap<FGameplayTag, float>& ItemEquipEffectChange = GetShopItem()->GetItemEquipEffectChangeMap();
		for (const TPair<FGameplayTag, float>& AttributeChange : ItemEquipEffectChange)
		{
			if (AttributeChange.Key.IsValid())
			{
				EquipEffectSpec->SetSetByCallerMagnitude(AttributeChange.Key, AttributeChange.Value);
			}
		}
	
		AppliedEquipEffectHandle = OwnerASC->ApplyGameplayEffectSpecToSelf(*EquipEffectSpec);
	}

	
	TSubclassOf<UGameplayAbility> GrantedAbilityClass = GetShopItem()->GetGrantedAbility();
	if (!GrantedAbilityClass) { return; }
	
	GrantedAbilitySpecHandle = OwnerASC->GiveAbility(FGameplayAbilitySpec(GrantedAbilityClass));
}


void UMobaInventoryItem::GrantedAbilityManaUpdated(const FOnAttributeChangeData& ManaData)
{
	OnGrantedAbilityCanCastUpdated.Broadcast(CanCastGrantedAbility());
}


bool UMobaInventoryItem::TryActivateGrantedAbility()
{
	if (!GrantedAbilitySpecHandle.IsValid()) { return false; }
	
	if (OwnerASC && OwnerASC->TryActivateAbility(GrantedAbilitySpecHandle))
	{
		return true;
	}

	return false;
}


void UMobaInventoryItem::ApplyItemConsumeEffect()
{
	if (!ShopItem) { return; }
	
	const TSubclassOf<UGameplayEffect> ConsumeEffectClass = GetShopItem()->GetItemConsumedEffect();
	if (ConsumeEffectClass)
	{
		const FGameplayEffectSpecHandle ConsumeEffectSpecHandle = OwnerASC->MakeOutgoingSpec(ConsumeEffectClass, 1, OwnerASC->MakeEffectContext());
		if (!ConsumeEffectSpecHandle.IsValid()) { return; }
		
		FGameplayEffectSpec* ConsumeEffectSpec = ConsumeEffectSpecHandle.Data.Get();

		const TMap<FGameplayTag, float>& ItemConsumeEffectChangeMap = GetShopItem()->GetItemConsumeEffectChangeMap();
		for (const TPair<FGameplayTag, float>& AttributeChange : ItemConsumeEffectChangeMap)
		{
			if (AttributeChange.Key.IsValid())
			{
				ConsumeEffectSpec->SetSetByCallerMagnitude(AttributeChange.Key, AttributeChange.Value);
			}
		}
	
		AppliedConsumeEffectHandle = OwnerASC->ApplyGameplayEffectSpecToSelf(*ConsumeEffectSpec);
	}
}


void UMobaInventoryItem::RemoveGASModifications()
{
	if (!OwnerASC) { return; }

	OwnerASC->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet::GetManaAttribute()).RemoveAll(this);

	if (OwnerASC->GetOwner()->HasAuthority())
	{
		if (GrantedAbilitySpecHandle.IsValid())
		{
			OwnerASC->SetRemoveAbilityOnEnd(GrantedAbilitySpecHandle);
		}
	
		if (AppliedEquipEffectHandle.IsValid())
		{
			OwnerASC->RemoveActiveGameplayEffect(AppliedEquipEffectHandle);
		}	
	}
}


float UMobaInventoryItem::GetGrantedAbilityCooldownTimeRemaining() const
{
	if (!IsGrantingAnyAbility() || !GetShopItem()) { return 0.f; }

	return UMobaAbilitySystemStatics::GetCooldownRemainingForAbility(GetShopItem()->GetGrantedAbilityCDO(), *OwnerASC);
}


float UMobaInventoryItem::GetGrantedAbilityCooldownDuration() const
{
	if (!IsGrantingAnyAbility() || !GetShopItem()) { return 0.f; }

	return UMobaAbilitySystemStatics::GetCooldownDurationForAbility(GetShopItem()->GetGrantedAbilityCDO(), *OwnerASC, 1);
}


float UMobaInventoryItem::GetGrantedAbilityCost() const
{
	if (!IsGrantingAnyAbility() || !GetShopItem()) { return 0.f; }

	return UMobaAbilitySystemStatics::GetManaCostForAbility(GetShopItem()->GetGrantedAbilityCDO(), *OwnerASC, 1);
}


bool UMobaInventoryItem::CanCastGrantedAbility() const
{
	if (!IsGrantingAnyAbility() || !OwnerASC) { return false; }

	if (OwnerASC->GetOwner() && OwnerASC->GetOwner()->HasAuthority())
	{
		FGameplayAbilitySpec* AbilitySpec = OwnerASC->FindAbilitySpecFromHandle(GrantedAbilitySpecHandle);
		if (AbilitySpec)
		{
			return UMobaAbilitySystemStatics::HasEnoughResourceToCastAbility(*AbilitySpec, *OwnerASC);	
		}	
	}

	// On Client
	return UMobaAbilitySystemStatics::HasEnoughResourceToCastAbilityStatic(GetShopItem()->GetGrantedAbilityCDO(), *OwnerASC);
}

