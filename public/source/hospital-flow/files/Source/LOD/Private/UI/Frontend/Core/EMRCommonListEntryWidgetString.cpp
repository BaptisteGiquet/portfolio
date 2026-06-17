


#include "UI/Frontend/Core/EMRCommonListEntryWidgetString.h"

#include "CommonInputSubsystem.h"
#include "UI/Frontend/Core/EMRFrontendCommonRotator.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/Data/EMRListDataObjectString.h"

void UEMRCommonListEntryWidgetString::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	CommonButton_PreviousOption->OnClicked().AddUObject(this, &ThisClass::OnPreviousOptionButtonClicked);
	CommonButton_NextOption->OnClicked().AddUObject(this, &ThisClass::OnNextOptionButtonClicked);
	
	CommonRotator_AvailableOptions->OnClicked().AddLambda([this](){ SelectThisEntryWidget(); });
	CommonRotator_AvailableOptions->OnRotatedEvent.AddUObject(this, &ThisClass::OnRotatorValueChanged);
}


void UEMRCommonListEntryWidgetString::OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject)
{
	Super::OnOwningListDataObjectSet(InOwningListDataObject);
	
	CachedOwningStringDataObject = CastChecked<UEMRListDataObjectString>(InOwningListDataObject);
	
	CommonRotator_AvailableOptions->PopulateTextLabels(CachedOwningStringDataObject->GetAvailableOptionsTextArray());
	CommonRotator_AvailableOptions->SetSelectedOptionByText(CachedOwningStringDataObject->GetCurrentDisplayText());
}


void UEMRCommonListEntryWidgetString::OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason)
{
	if (CachedOwningStringDataObject)
	{
		CommonRotator_AvailableOptions->PopulateTextLabels(CachedOwningStringDataObject->GetAvailableOptionsTextArray());
		CommonRotator_AvailableOptions->SetSelectedOptionByText(CachedOwningStringDataObject->GetCurrentDisplayText());
	}
}


void UEMRCommonListEntryWidgetString::OnToggleEditableState(bool bIsEditable)
{
	Super::OnToggleEditableState(bIsEditable);
	
	CommonButton_PreviousOption->SetIsEnabled(bIsEditable);
	CommonButton_NextOption->SetIsEnabled(bIsEditable);
	CommonRotator_AvailableOptions->SetIsEnabled(bIsEditable);
}


void UEMRCommonListEntryWidgetString::OnPreviousOptionButtonClicked()
{
	if (CachedOwningStringDataObject)
	{
		CachedOwningStringDataObject->BackToPreviousOption();
	}	
	
	SelectThisEntryWidget();
}


void UEMRCommonListEntryWidgetString::OnNextOptionButtonClicked()
{
	if (CachedOwningStringDataObject)
	{
		CachedOwningStringDataObject->AdvanceToNextOption();
	}	
	
	SelectThisEntryWidget();
}


void UEMRCommonListEntryWidgetString::OnRotatorValueChanged(int32 Value, bool bUserInitiated)
{
	if (!CachedOwningStringDataObject) return;
	
	UCommonInputSubsystem* CommonInputSubsystem = GetInputSubsystem();
	if (!CommonInputSubsystem || !bUserInitiated) return;
	
	if (CommonInputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad)
	{
		CachedOwningStringDataObject->OnRotatorInitiatedValueChanged(CommonRotator_AvailableOptions->GetSelectedText());
	}
}
