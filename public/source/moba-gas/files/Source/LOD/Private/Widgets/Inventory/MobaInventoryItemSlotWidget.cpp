

#include "MobaInventoryItemSlotWidget.h"

#include "MobaInventoryItemDragDropOp.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/Inventory/MobaShopItem.h"
#include "Inventory/MobaInventoryItem.h"
#include "Widgets/MobaItemTooltipWidget.h"


void UMobaInventoryItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EmptySlot();
}


void UMobaInventoryItemSlotWidget::LeftButtonClicked()
{
	Super::LeftButtonClicked();

	if (!IsSlotEmpty())
	{
		OnLeftButtonClicked.Broadcast(GetInventoryItemHandle());
	}
}

void UMobaInventoryItemSlotWidget::RightButtonClicked()
{
	Super::RightButtonClicked();

	if (!IsSlotEmpty())
	{
		OnRightButtonClicked.Broadcast(GetInventoryItemHandle());
	}
}


void UMobaInventoryItemSlotWidget::UpdateInventoryItem(const UMobaInventoryItem* NewItem)
{
	UnbindCanCastGrantedAbilityDelegate();
	
	InventoryItem = const_cast<UMobaInventoryItem*>(NewItem);

	if (!InventoryItem || !InventoryItem->IsValid() || InventoryItem->GetItemStackCount() <= 0)
	{
		EmptySlot();
		return;
	}

	const UMobaShopItem* NewInventoryItem = InventoryItem->GetShopItem();

	SetItemIconTexture(NewInventoryItem->GetItemIcon());
	UMobaItemTooltipWidget* ItemToolTip = SetupItemTooltipWidget(NewInventoryItem);

	if (ItemToolTip)
	{
		ItemToolTip->SetPrice(NewInventoryItem->GetItemSellPrice());
	}

	
	if (NewInventoryItem->IsItemStackable())
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Visible);
		UpdateStackCount();
	}
	else
	{
		Text_StackCount->SetVisibility(ESlateVisibility::Hidden);
	}

	ClearCooldown();

	if (InventoryItem->IsGrantingAnyAbility())
	{
		UpdateCanCastDisplay(InventoryItem->CanCastGrantedAbility());
		
		const float GrantedAbilityCooldownRemaining = InventoryItem->GetGrantedAbilityCooldownTimeRemaining();
		const float GrantedAbilityCooldownDuration = InventoryItem->GetGrantedAbilityCooldownDuration();

		Text_CooldownDuration->SetVisibility(GrantedAbilityCooldownDuration == 0.f ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
		Text_CooldownDuration->SetText(FText::AsNumber(GrantedAbilityCooldownDuration));
		
		if (GrantedAbilityCooldownRemaining > 0.f)
		{
			StartCooldown(GrantedAbilityCooldownRemaining, GrantedAbilityCooldownDuration);
		}
		
		const float GrantedAbilityCost = InventoryItem->GetGrantedAbilityCost();
		Text_ManaCost->SetVisibility(GrantedAbilityCost == 0.f ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
		Text_ManaCost->SetText(FText::AsNumber(GrantedAbilityCost));
		BindCanCastGrantedAbilityDelegate();
	}
	else
	{
		UpdateCanCastDisplay(true);
		
		Text_CooldownDuration->SetVisibility(ESlateVisibility::Hidden);
		Text_ManaCost->SetVisibility(ESlateVisibility::Hidden);
		Text_CooldownCounter->SetVisibility(ESlateVisibility::Hidden);
	}
}


bool UMobaInventoryItemSlotWidget::IsSlotEmpty() const
{
	return !InventoryItem || !InventoryItem->IsValid();
}


void UMobaInventoryItemSlotWidget::EmptySlot()
{
	ClearCooldown();
	UnbindCanCastGrantedAbilityDelegate();
	
	InventoryItem = nullptr;
	SetItemIconTexture(EmptyTexture);

	Text_StackCount->SetVisibility(ESlateVisibility::Hidden);
	Text_CooldownCounter->SetVisibility(ESlateVisibility::Hidden);
	Text_CooldownDuration->SetVisibility(ESlateVisibility::Hidden);
	Text_ManaCost->SetVisibility(ESlateVisibility::Hidden);
}


void UMobaInventoryItemSlotWidget::SetSlotNumber(const int32 NewSlotNumber)
{
	SlotNumber = NewSlotNumber;
}


UTexture2D* UMobaInventoryItemSlotWidget::GetIconTexture() const
{
	if (InventoryItem && InventoryItem->GetShopItem())
	{
		return InventoryItem->GetShopItem()->GetItemIcon();
	}

	return nullptr;
}


void UMobaInventoryItemSlotWidget::UpdateStackCount()
{
	if (InventoryItem)
	{
		Text_StackCount->SetText(FText::AsNumber(InventoryItem->GetItemStackCount()));
	}
}


FInventoryItemHandle UMobaInventoryItemSlotWidget::GetInventoryItemHandle() const
{
	if (!IsSlotEmpty())
	{
		return InventoryItem->GetInventoryItemHandle();	
	}

	return FInventoryItemHandle::GetInvalidHandle();
}


void UMobaInventoryItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!IsSlotEmpty() && DragDropOperationClass)
	{
		UMobaInventoryItemDragDropOp* DragDropOp = NewObject<UMobaInventoryItemDragDropOp>(this, DragDropOperationClass);
		if (DragDropOp)
		{
			DragDropOp->SetDraggedItem(this);
			OutOperation = DragDropOp;
		}
	}
}


bool UMobaInventoryItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UMobaInventoryItemSlotWidget* OtherWidget = Cast<UMobaInventoryItemSlotWidget>(InOperation->Payload);
	if (OtherWidget && !OtherWidget->IsSlotEmpty())
	{
		OnInventoryItemDropped.Broadcast(this, OtherWidget);
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}


void UMobaInventoryItemSlotWidget::SetItemIconTexture(UTexture2D* ItemIconTexture)
{
	if (GetItemIcon())
	{
		GetItemIcon()->GetDynamicMaterial()->SetTextureParameterValue(IconMaterialParamName, ItemIconTexture);
		return;
	}

	Super::SetItemIconTexture(ItemIconTexture);
}


void UMobaInventoryItemSlotWidget::UpdateCanCastDisplay(const bool bCanCast)
{
	GetItemIcon()->GetDynamicMaterial()->SetScalarParameterValue(CanCastAbilityParamName, bCanCast ? 1.f : 0.f);
}


void UMobaInventoryItemSlotWidget::BindCanCastGrantedAbilityDelegate()
{
	if (InventoryItem)
	{
		InventoryItem->OnGrantedAbilityCanCastUpdated.AddUObject(this, &ThisClass::UpdateCanCastDisplay);
	}
}


void UMobaInventoryItemSlotWidget::UnbindCanCastGrantedAbilityDelegate()
{
	if (InventoryItem)
	{
		InventoryItem->OnGrantedAbilityCanCastUpdated.RemoveAll(this);
	}
}


void UMobaInventoryItemSlotWidget::StartCooldown(float CooldownTimeRemaining, float CooldownDuration)
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Cooldown"))

	if (!GetWorld()) { return; }
	
	Text_CooldownDuration->SetText(FText::AsNumber(CooldownDuration));
	CachedCooldownDuration = CooldownDuration;
	CachedCooldownTimeRemaining = CooldownTimeRemaining;
	
	Text_CooldownCounter->SetVisibility(ESlateVisibility::Visible);
	
	GetWorld()->GetTimerManager().ClearTimer(CooldownDurationTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(CooldownDurationTimerHandle, this, &ThisClass::CooldownFinished, CooldownTimeRemaining, false);

	GetWorld()->GetTimerManager().ClearTimer(CooldownUpdateTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(CooldownUpdateTimerHandle, this, &UMobaInventoryItemSlotWidget::UpdateCooldown, CooldownUpdateFrequency, true, 0.f);


}


void UMobaInventoryItemSlotWidget::CooldownFinished()
{
	if (!GetWorld()) { return; }

	if (GetItemIcon())
	{
		GetItemIcon()->GetDynamicMaterial()->SetScalarParameterValue(CooldownPercentParamName, 0.f);
		GetItemIcon()->GetDynamicMaterial()->SetScalarParameterValue(CanCastAbilityParamName, 1.f);
	}
	
	CachedCooldownDuration = 0.f;
	CachedCooldownTimeRemaining = 0.f;
	GetWorld()->GetTimerManager().ClearTimer(CooldownUpdateTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(CooldownDurationTimerHandle);
	Text_CooldownCounter->SetVisibility(ESlateVisibility::Hidden);
}


void UMobaInventoryItemSlotWidget::UpdateCooldown()
{
	CachedCooldownTimeRemaining -= CooldownUpdateFrequency;

	const FNumberFormattingOptions* FormattingOptions = bIsCooldownTwoDigit ? &TwoDigitNumberFormattingOptions : &WholeNumberFormattingOptions;
	
	Text_CooldownCounter->SetText(FText::AsNumber(CachedCooldownTimeRemaining, FormattingOptions));

	const float CooldownPercent = CachedCooldownTimeRemaining / CachedCooldownDuration;
	GetItemIcon()->GetDynamicMaterial()->SetScalarParameterValue(CooldownPercentParamName, CooldownPercent);
}


void UMobaInventoryItemSlotWidget::ClearCooldown()
{
	CooldownFinished();
}
