


#include "UI/Frontend/Screens/MainMenu/EMROptionsScreenWidget.h"

#include "ICommonInputModule.h"
#include "LocalizationLibrary.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/Frontend/EMRGameUserSettings.h"
#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"
#include "UI/Frontend/Core/EMRFrontendCommonListView.h"
#include "UI/Frontend/Utils/DebugHelper.h"
#include "UI/Frontend/Data/EMRListDataObjectCollection.h"
#include "UI/Frontend/Data/EMROptionsDataRegistry.h"
#include "UI/Frontend/Core/EMRFrontendCommonTabListWidgetBase.h"
#include "UI/Frontend/Core/EMROptionsDetailsViewWidget.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"


void UEMROptionsScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ResetAction.IsNull()) return;
	
	ResetActionHandle = RegisterUIActionBinding(FBindUIActionArgs(ResetAction,
		true,
		FSimpleDelegate::CreateUObject(this, &ThisClass::OnResetBoundActionTriggered)));

	BackActionHandle = RegisterUIActionBinding(FBindUIActionArgs(ICommonInputModule::GetSettings().GetDefaultBackAction(),
		true,
		FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)));

	TabListWidget_OptionsTabs->OnTabSelected.AddUniqueDynamic(this, &ThisClass::OnOptionsTabSelected);
	
	CommonListView_OptionsList->OnItemIsHoveredChanged().AddUObject(this, &ThisClass::OnListViewItemHoveredChanged);
	CommonListView_OptionsList->OnItemSelectionChanged().AddUObject(this, &ThisClass::OnListViewItemSelectionChanged);
}


void UEMROptionsScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	for (UEMRListDataObjectCollection* TabCollection : GetOrCreateDataRegistry()->GetRegisteredOptionsTabCollections())
	{
		if (!TabCollection) continue;

		const FName TabID = TabCollection->GetDataID();

		// if there is already a tab button with this ID, skip it.
		if (TabListWidget_OptionsTabs->GetTabButtonBaseByID(TabID) != nullptr)
		{
			continue;
		}

		TabListWidget_OptionsTabs->RequestRegisterTab(TabID, TabCollection->GetDataDisplayName());
	}
}


void UEMROptionsScreenWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	
	if (UEMRGameUserSettings* Settings = UEMRGameUserSettings::Get())
	{
		Settings->SaveSettings();
	}
}


UWidget* UEMROptionsScreenWidget::NativeGetDesiredFocusTarget() const
{
	if (UObject* SelectedObject = CommonListView_OptionsList->GetSelectedItem())
	{
		if (UUserWidget* SelectedEntryWidget = CommonListView_OptionsList->GetEntryWidgetFromItem(SelectedObject))
		{
			return SelectedEntryWidget;
		}
		
		return Super::NativeGetDesiredFocusTarget();
	}
	
	return Super::NativeGetDesiredFocusTarget();
}


UEMROptionsDataRegistry* UEMROptionsScreenWidget::GetOrCreateDataRegistry()
{
	if (!CreatedOwningDataRegistry)
	{
		CreatedOwningDataRegistry = NewObject<UEMROptionsDataRegistry>();
		CreatedOwningDataRegistry->InitOptionsDataRegistry(GetOwningLocalPlayer());
	}

	checkf(CreatedOwningDataRegistry, TEXT("Data registry for Options screen is not valid"))

	return CreatedOwningDataRegistry;
}


void UEMROptionsScreenWidget::OnResetBoundActionTriggered()
{
	if (ResettableDataArray.IsEmpty()) return;
	
	UEMRUIManagerSubsystem::Get(this)->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::YesNo,
		GetLocalizedText(UIStringTableId, "UI.Confirm.Title.ResetSettings"), 
		GetLocalizedText(UIStringTableId, "UI.Confirm.Message.ResetSettings"),
		[this](EConfirmScreenButtonType ClickedButtonType)
		{
			if (ClickedButtonType != EConfirmScreenButtonType::Confirmed)
			{
				return;
			}
			
			bIsResettingData = true;
			
			bool bHasDataFailedToReset = false;
			
			for (UEMRListDataObjectBase* DataToReset : ResettableDataArray)
			{
				if (!DataToReset) continue;
				
				if (DataToReset->TryResetBackToDefaultValue())
				{
					Debug::Print(DataToReset->GetDataDisplayName().ToString() + TEXT(" was reset"));
				}
				else
				{
					Debug::Print(DataToReset->GetDataDisplayName().ToString() + TEXT(" failed to reset"));
					bHasDataFailedToReset = true;
				}
			}
			
			if (!bHasDataFailedToReset)
			{
				ResettableDataArray.Empty();
				RemoveActionBinding(ResetActionHandle);	
			}
			
			bIsResettingData = false;
		});
}


void UEMROptionsScreenWidget::OnBackBoundActionTriggered()
{
	DeactivateWidget();
}


void UEMROptionsScreenWidget::OnOptionsTabSelected(FName TabID)
{
	DetailsView_ListEntryInfo->ClearDetailsViewInfo();
	
	TArray<UEMRListDataObjectBase*> FoundListSourceItems = GetOrCreateDataRegistry()->GetListSourceItemsBySelectedTabID(TabID);
	
	CommonListView_OptionsList->SetListItems(FoundListSourceItems);
	CommonListView_OptionsList->RequestRefresh();
	
	if (CommonListView_OptionsList->GetNumItems() != 0)
	{
		CommonListView_OptionsList->NavigateToIndex(0);
		CommonListView_OptionsList->SetSelectedIndex(0);
	}
	
	ResettableDataArray.Empty();
	
	for (UEMRListDataObjectBase* FoundListSourceItem : FoundListSourceItems)
	{
		if (!FoundListSourceItem) continue;
		
		
		if (!FoundListSourceItem->OnListDataModified.IsBoundToObject(this))
		{
			FoundListSourceItem->OnListDataModified.AddUObject(this, &ThisClass::OnListViewListDataModified);	
		}
		
		
		if (FoundListSourceItem->CanResetBackToDefaultValue())
		{
			ResettableDataArray.AddUnique(FoundListSourceItem);
		}
	}
	
	if (ResettableDataArray.IsEmpty())
	{
		RemoveActionBinding(ResetActionHandle);
	}
	else
	{
		if (!GetActionBindings().Contains(ResetActionHandle))
		{
			AddActionBinding(ResetActionHandle);
		}
	}
}


void UEMROptionsScreenWidget::OnListViewItemHoveredChanged(UObject* InHoveredItem, bool bWasHovered)
{
	if (!InHoveredItem) return;
	
	UEMRFrontendCommonListEntryWidgetBase* HoveredEntryWidget = CommonListView_OptionsList->GetEntryWidgetFromItem<UEMRFrontendCommonListEntryWidgetBase>(InHoveredItem);
	check(HoveredEntryWidget);
	
	HoveredEntryWidget->NativeOnListEntryWidgetHovered(bWasHovered);
	
	if (bWasHovered)
	{
		DetailsView_ListEntryInfo->UpdateDetailsViewInfo(CastChecked<UEMRListDataObjectBase>(InHoveredItem), TryGetEntryWidgetClassName(InHoveredItem));
	}
	else
	{
		if (UEMRListDataObjectBase* SelectedItem = CommonListView_OptionsList->GetSelectedItem<UEMRListDataObjectBase>())
		{
			DetailsView_ListEntryInfo->UpdateDetailsViewInfo(SelectedItem, TryGetEntryWidgetClassName(SelectedItem));
		}
	}
}


void UEMROptionsScreenWidget::OnListViewItemSelectionChanged(UObject* InSelectedItem)
{
	if (!InSelectedItem) return;
	
	DetailsView_ListEntryInfo->UpdateDetailsViewInfo(CastChecked<UEMRListDataObjectBase>(InSelectedItem), TryGetEntryWidgetClassName(InSelectedItem));
}


FString UEMROptionsScreenWidget::TryGetEntryWidgetClassName(UObject* InOwningListItem) const
{
	UUserWidget* FoundEntryWidget = CommonListView_OptionsList->GetEntryWidgetFromItem(InOwningListItem);
	if (FoundEntryWidget)
	{
		return FoundEntryWidget->GetClass()->GetName();
	}
	else
	{
		return FString(TEXT("Entry widget not valid"));
	}
}


void UEMROptionsScreenWidget::OnListViewListDataModified(UEMRListDataObjectBase* ModifiedData, EOptionsListDataModifyReason ModifyReason)
{
	if (!ModifiedData || bIsResettingData ) return;
	
	if (ModifiedData->CanResetBackToDefaultValue())
	{
		ResettableDataArray.AddUnique(ModifiedData);
		
		if (!GetActionBindings().Contains(ResetActionHandle))
		{
			AddActionBinding(ResetActionHandle);
		}
	}
	else
	{
		if (ResettableDataArray.Contains(ModifiedData))
		{
			ResettableDataArray.Remove(ModifiedData);
		}
	}
	
	if (ResettableDataArray.IsEmpty())
	{
		RemoveActionBinding(ResetActionHandle);
	}
}
