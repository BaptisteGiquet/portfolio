

#include "MobaInventoryWidget.h"

#include "MobaInventoryContextMenuWidget.h"
#include "MobaInventoryItemSlotWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Inventory/MobaInventoryComponent.h"

void UMobaInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	const APawn* OwnerPawn = GetOwningPlayerPawn();
	if (OwnerPawn)
	{
		InventoryComponent = OwnerPawn->GetComponentByClass<UMobaInventoryComponent>();
		if (InventoryComponent)
		{
			InventoryComponent->OnItemAdded.AddUObject(this, &ThisClass::ItemAdded);
			InventoryComponent->OnItemRemoved.AddUObject(this, &ThisClass::ItemRemoved);
			InventoryComponent->OnItemStackCountChanged.AddUObject(this, &ThisClass::ItemStackCountChanged);
			InventoryComponent->OnItemAbilityCommitted.AddUObject(this, &ThisClass::ItemAbilityCommitted);
			
			const int32 Capacity = InventoryComponent->GetCapacity();

			WrapBox_ItemList->ClearChildren();
			for (int32 WidgetSlotNumber = 0; WidgetSlotNumber < Capacity; ++WidgetSlotNumber)
			{
				UMobaInventoryItemSlotWidget* NewEmptyWidget = CreateWidget<UMobaInventoryItemSlotWidget>(GetOwningPlayer(), InventoryItemWidgetClass);
				if (NewEmptyWidget)
				{
					NewEmptyWidget->SetSlotNumber(WidgetSlotNumber);
					
					UWrapBoxSlot* NewItemSlot = WrapBox_ItemList->AddChildToWrapBox(NewEmptyWidget);
					NewItemSlot->SetPadding(FMargin(2.f));
					InventoryItemWidgets.Add(NewEmptyWidget);

					NewEmptyWidget->OnInventoryItemDropped.AddUObject(this, &ThisClass::HandleItemDragDrop);
					NewEmptyWidget->OnLeftButtonClicked.AddUObject(InventoryComponent, &UMobaInventoryComponent::TryActivateItem);
					NewEmptyWidget->OnRightButtonClicked.AddUObject(this, &ThisClass::ToggleContextMenu);
				}
			}

			SpawnContextMenu();
		}
	}
}


void UMobaInventoryWidget::NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);

	if (!NewWidgetPath.ContainsWidget(ContextMenuWidget->GetCachedWidget().Get()))
	{
		ClearContextMenu();
	}
}


void UMobaInventoryWidget::ItemAdded(const UMobaInventoryItem* InventoryItem)
{
	if (!InventoryItem) { return; }

	UMobaInventoryItemSlotWidget* NextAvailableSlot = GetNextAvailableSlot();
	if (NextAvailableSlot)
	{
		NextAvailableSlot->UpdateInventoryItem(InventoryItem);
		PopulatedItemEntryWidgets.Add(InventoryItem->GetInventoryItemHandle(), NextAvailableSlot);

		if (InventoryComponent)
		{
			InventoryComponent->ItemSlotChanged(InventoryItem->GetInventoryItemHandle(), NextAvailableSlot->GetSlotNumber());
		}
	}
}


void UMobaInventoryWidget::ItemRemoved(const FInventoryItemHandle& ItemHandle)
{
	UMobaInventoryItemSlotWidget** FoundWidget = PopulatedItemEntryWidgets.Find(ItemHandle);
	if (FoundWidget && *FoundWidget)
	{
		(*FoundWidget)->EmptySlot();
		PopulatedItemEntryWidgets.Remove(ItemHandle);
	}
}


void UMobaInventoryWidget::ItemStackCountChanged(const FInventoryItemHandle& InventoryItemHandle, int32 NewStackCount)
{
	UMobaInventoryItemSlotWidget** FoundWidget = PopulatedItemEntryWidgets.Find(InventoryItemHandle);
	if (FoundWidget)
	{
		(*FoundWidget)->UpdateStackCount();
	}
}


void UMobaInventoryWidget::ItemAbilityCommitted(const FInventoryItemHandle& InventoryItemHandle, float CooldownTimeRemaining, float CooldownDuration)
{
	UMobaInventoryItemSlotWidget** FoundWidget = PopulatedItemEntryWidgets.Find(InventoryItemHandle);
	if (FoundWidget && *FoundWidget)
	{
		(*FoundWidget)->StartCooldown(CooldownTimeRemaining, CooldownDuration);
	}
}


UMobaInventoryItemSlotWidget* UMobaInventoryWidget::GetNextAvailableSlot() const
{
	for (UMobaInventoryItemSlotWidget* Widget : InventoryItemWidgets)
	{
		if (Widget->IsSlotEmpty())
		{
			return Widget;
		}
	}

	return nullptr;
}


void UMobaInventoryWidget::HandleItemDragDrop(UMobaInventoryItemSlotWidget* DestinationWidget, UMobaInventoryItemSlotWidget* SourceWidget)
{
	const UMobaInventoryItem* SourceItem = SourceWidget->GetInventoryItem();
	const UMobaInventoryItem* DestinationItem = DestinationWidget->GetInventoryItem();

	DestinationWidget->UpdateInventoryItem(SourceItem);
	SourceWidget->UpdateInventoryItem(DestinationItem);

	PopulatedItemEntryWidgets[DestinationWidget->GetInventoryItemHandle()] = DestinationWidget;

	if (InventoryComponent)
	{
		InventoryComponent->ItemSlotChanged(DestinationWidget->GetInventoryItemHandle(), DestinationWidget->GetSlotNumber());
	}

	if (!SourceWidget->IsSlotEmpty())
	{
		PopulatedItemEntryWidgets[SourceWidget->GetInventoryItemHandle()] = SourceWidget;
		
		if (InventoryComponent)
		{
			InventoryComponent->ItemSlotChanged(SourceWidget->GetInventoryItemHandle(), SourceWidget->GetSlotNumber());
		}
	}
}


void UMobaInventoryWidget::SpawnContextMenu()
{
	if (!ContextMenuWidgetClass)  { return; }
	
	ContextMenuWidget = CreateWidget<UMobaInventoryContextMenuWidget>(this, ContextMenuWidgetClass);
	if (!ContextMenuWidget) { return; }

	ContextMenuWidget->GetUseButtonClickedEvent().AddDynamic(this, &ThisClass::UseFocusedItem);
	ContextMenuWidget->GetSellButtonClickedEvent().AddDynamic(this, &ThisClass::SellFocusedItem);
	ContextMenuWidget->AddToViewport(1);
	SetContextMenuVisible(false);
}


void UMobaInventoryWidget::UseFocusedItem()
{
	InventoryComponent->TryActivateItem(CurrentFocusedItemHandle);
	SetContextMenuVisible(false);
}


void UMobaInventoryWidget::SellFocusedItem()
{
	InventoryComponent->TrySellingItem(CurrentFocusedItemHandle);
	SetContextMenuVisible(false);
}


void UMobaInventoryWidget::SetContextMenuVisible(const bool bIsContextMenuVisible)
{
	if (!ContextMenuWidget) { return; }

	if (bIsContextMenuVisible)
	{
		ContextMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		ContextMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}


void UMobaInventoryWidget::ToggleContextMenu(const FInventoryItemHandle& ItemHandle)
{
	if (CurrentFocusedItemHandle == ItemHandle)
	{
		// When ContextMenu is already open
		ClearContextMenu();
		return;
	}
	
	CurrentFocusedItemHandle = ItemHandle;
	UMobaInventoryItemSlotWidget** ItemSlotWidgetPtr = PopulatedItemEntryWidgets.Find(ItemHandle);
	if (!ItemSlotWidgetPtr) { return; }

	UMobaInventoryItemSlotWidget* ItemSlotWidget = *ItemSlotWidgetPtr;
	if (!ItemSlotWidget) { return; }

	SetContextMenuVisible(true);
	
	const FVector2D ItemWidgetAbsPosition = ItemSlotWidget->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D{1.f, 0.5f});
	FVector2D ItemWidgetPixelPosition = FVector2D::ZeroVector;
	FVector2D ItemWidgetViewportPosition = FVector2D::ZeroVector;

	// Convert ScreenPosition to Pixel/Viewport positions
	USlateBlueprintLibrary::AbsoluteToViewport(this, ItemWidgetAbsPosition, ItemWidgetPixelPosition, ItemWidgetViewportPosition);

	
	// Make sure the Context Menu doesn't go over the bottom screen
	const APlayerController* OwningPlayerController = GetOwningPlayer();
	if (OwningPlayerController)
	{
		int32 ViewportSizeX = 0;
		int32 ViewportSizeY = 0;
		OwningPlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

		const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
		
		const int32 OvershootInY = ItemWidgetPixelPosition.Y + ContextMenuWidget->GetDesiredSize().Y * ViewportScale - ViewportSizeY;
		if (OvershootInY > 0)
		{
			ItemWidgetPixelPosition.Y -= OvershootInY;
		}
	}
	
	ContextMenuWidget->SetPositionInViewport(ItemWidgetPixelPosition);
}


void UMobaInventoryWidget::ClearContextMenu()
{
	if (ContextMenuWidget)
	{
		ContextMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	CurrentFocusedItemHandle = FInventoryItemHandle::GetInvalidHandle();
}
