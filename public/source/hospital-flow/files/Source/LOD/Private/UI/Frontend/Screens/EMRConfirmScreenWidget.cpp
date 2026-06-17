


#include "UI/Frontend/Screens/EMRConfirmScreenWidget.h"

#include "CommonTextBlock.h"
#include "LocalizationLibrary.h"
#include "Components/DynamicEntryBox.h"
#include "ICommonInputModule.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"


UConfirmScreenInfoObject* UConfirmScreenInfoObject::CreateOkScreen(const FText& InScreenTitle, const FText& InScreenMessage)
{
	UConfirmScreenInfoObject* InfoObject = NewObject<UConfirmScreenInfoObject>();
	InfoObject->ScreenTitle = InScreenTitle;
	InfoObject->ScreenMessage = InScreenMessage;

	// When we click on Ok button, we close the confirm screen.
	FConfirmScreenButtonInfo OkButtonInfo;
	OkButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Closed;
	OkButtonInfo.ButtonTextToDisplay = GetLocalizedText(UIStringTableId, "UI.Confirm.Button.Ok");

	InfoObject->AvailableScreenButtons.Add(OkButtonInfo);

	return InfoObject;
}


UConfirmScreenInfoObject* UConfirmScreenInfoObject::CreateYesNoScreen(const FText& InScreenTitle, const FText& InScreenMessage)
{
	UConfirmScreenInfoObject* InfoObject = NewObject<UConfirmScreenInfoObject>();
	InfoObject->ScreenTitle = InScreenTitle;
	InfoObject->ScreenMessage = InScreenMessage;
	
	FConfirmScreenButtonInfo YesButtonInfo;
	YesButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Confirmed;
	YesButtonInfo.ButtonTextToDisplay = GetLocalizedText(UIStringTableId, "UI.Confirm.Button.Yes");

	FConfirmScreenButtonInfo NoButtonInfo;
	NoButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Canceled;
	NoButtonInfo.ButtonTextToDisplay = GetLocalizedText(UIStringTableId, "UI.Confirm.Button.No");

	InfoObject->AvailableScreenButtons.Add(YesButtonInfo);
	InfoObject->AvailableScreenButtons.Add(NoButtonInfo);

	return InfoObject;
}


UConfirmScreenInfoObject* UConfirmScreenInfoObject::CreateOkCancelScreen(const FText& InScreenTitle, const FText& InScreenMessage)
{
	UConfirmScreenInfoObject* InfoObject = NewObject<UConfirmScreenInfoObject>();
	InfoObject->ScreenTitle = InScreenTitle;
	InfoObject->ScreenMessage = InScreenMessage;
	
	FConfirmScreenButtonInfo OkButtonInfo;
	OkButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Confirmed;
	OkButtonInfo.ButtonTextToDisplay = GetLocalizedText(UIStringTableId, "UI.Confirm.Button.Ok");

	FConfirmScreenButtonInfo CancelButtonInfo;
	CancelButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Canceled;
	CancelButtonInfo.ButtonTextToDisplay = GetLocalizedText(UIStringTableId, "UI.Confirm.Button.Cancel");

	InfoObject->AvailableScreenButtons.Add(OkButtonInfo);
	InfoObject->AvailableScreenButtons.Add(CancelButtonInfo);

	return InfoObject;
}


void UEMRConfirmScreenWidget::InitConfirmScreen(UConfirmScreenInfoObject* InScreenInfoObject, TFunction<void(EConfirmScreenButtonType)> ClickedButtonCallback)
{
	check(InScreenInfoObject && CommonTextBlock_Title && CommonTextBlock_Message && DynamicEntryBox_Buttons);

	CommonTextBlock_Title->SetText(InScreenInfoObject->ScreenTitle);
	CommonTextBlock_Message->SetText(InScreenInfoObject->ScreenMessage);

	// Checking if EntryBox has old buttons created previously
	if (DynamicEntryBox_Buttons->GetNumEntries() != 0)
	{
		// Clearing old buttons the EntryBox has. The Widget type for the EntryBox is specified in the child widget blueprint
		DynamicEntryBox_Buttons->Reset<UEMRFrontendCommonButtonBase>(
			[](UEMRFrontendCommonButtonBase& ExistingButton)
			{
				ExistingButton.OnClicked().Clear();
			});
	}

	check(!InScreenInfoObject->AvailableScreenButtons.IsEmpty());

	for (const FConfirmScreenButtonInfo& AvailableButtonInfo : InScreenInfoObject->AvailableScreenButtons)
	{
		UEMRFrontendCommonButtonBase* AddedButton = DynamicEntryBox_Buttons->CreateEntry<UEMRFrontendCommonButtonBase>();
		if (!AddedButton) continue;

		AddedButton->SetButtonText(AvailableButtonInfo.ButtonTextToDisplay);
		AddedButton->OnClicked().AddLambda(
			[ClickedButtonCallback, AvailableButtonInfo, this]()
			{
				ClickedButtonCallback(AvailableButtonInfo.ConfirmScreenButtonType);

				DeactivateWidget();
			});
	}
}


UWidget* UEMRConfirmScreenWidget::NativeGetDesiredFocusTarget() const
{
	if (DynamicEntryBox_Buttons->GetNumEntries() != 0)
	{
		// Set focus on the last button. So if there are two buttons (Yes/No), the gamepad will focus on the No button.
		DynamicEntryBox_Buttons->GetAllEntries().Last()->SetFocus();
	}
	
	return Super::NativeGetDesiredFocusTarget();
}
