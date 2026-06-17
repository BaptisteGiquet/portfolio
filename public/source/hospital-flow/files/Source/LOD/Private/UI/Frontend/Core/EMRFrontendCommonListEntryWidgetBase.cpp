


#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"

#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "Components/ListView.h"
#include "UI/Frontend/Data/EMRListDataObjectBase.h"



void UEMRFrontendCommonListEntryWidgetBase::NativeOnListEntryWidgetHovered(bool bWasHovered)
{
	bool bIsSelected = false;
	UObject* ListItem = GetListItem();
	if (UListView* OwningListView = Cast<UListView>(GetOwningListView()))
	{
		if (ListItem && OwningListView->GetListItems().Contains(ListItem))
		{
			bIsSelected = OwningListView->IsItemSelected(ListItem);
		}
	}

	BP_OnListEntryWidgetHovered(bWasHovered, bIsSelected);
	
	if (bWasHovered)
	{
		BP_OnToggleEntryWidgetHighlightState(true);
	}
	else
	{
		BP_OnToggleEntryWidgetHighlightState((GetListItem() && IsListItemSelected())? true : false);
	}
}


void UEMRFrontendCommonListEntryWidgetBase::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();
	
	NativeOnListEntryWidgetHovered(false);
}


void UEMRFrontendCommonListEntryWidgetBase::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	OnOwningListDataObjectSet(CastChecked<UEMRListDataObjectBase>(ListItemObject));
}


FReply UEMRFrontendCommonListEntryWidgetBase::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	UCommonInputSubsystem* CommonInputSubsystem = GetInputSubsystem();
	if (CommonInputSubsystem && CommonInputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad)
	{
		if (UWidget* WidgetToFocus = BP_GetWidgetToFocusForGamepad())
		{
			if (TSharedPtr<SWidget> SlateWidgetToFocus = WidgetToFocus->GetCachedWidget())
			{
				return FReply::Handled().SetUserFocus(SlateWidgetToFocus.ToSharedRef());	
			}
		}
	}
	
	return Super::NativeOnFocusReceived(InGeometry,InFocusEvent);
}


void UEMRFrontendCommonListEntryWidgetBase::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
	
	BP_OnToggleEntryWidgetHighlightState(bIsSelected);
}


void UEMRFrontendCommonListEntryWidgetBase::OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject)
{
	if (CommonTextBlock_SettingDisplayName)
	{
		CommonTextBlock_SettingDisplayName->SetText(InOwningListDataObject->GetDataDisplayName());
	}
	
	if (!InOwningListDataObject->OnListDataModified.IsBoundToObject(this))
	{
		InOwningListDataObject->OnListDataModified.AddUObject(this, &ThisClass::OnOwningListDataObjectModified);
	}
	
	if (!InOwningListDataObject->OnDependencyDataModified.IsBoundToObject(this))
	{
		InOwningListDataObject->OnDependencyDataModified.AddUObject(this, &ThisClass::OnOwningDependencyDataObjectModified);
	}
	
	OnToggleEditableState(InOwningListDataObject->IsDataCurrentlyEditable());
	
	CachedOwningDataObject = InOwningListDataObject;
}


void UEMRFrontendCommonListEntryWidgetBase::OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason)
{
}


void UEMRFrontendCommonListEntryWidgetBase::OnOwningDependencyDataObjectModified(UEMRListDataObjectBase* OwningModifiedDependencyData, EOptionsListDataModifyReason ModifyReason)
{
	if (CachedOwningDataObject)
	{
		OnToggleEditableState(CachedOwningDataObject->IsDataCurrentlyEditable());	
	}
}


void UEMRFrontendCommonListEntryWidgetBase::OnToggleEditableState(bool bIsEditable)
{
	if (CommonTextBlock_SettingDisplayName)
	{
		CommonTextBlock_SettingDisplayName->SetIsEnabled(bIsEditable);
	}
}


void UEMRFrontendCommonListEntryWidgetBase::SelectThisEntryWidget()
{
	CastChecked<UListView>(GetOwningListView())->SetSelectedItem(GetListItem());
}
