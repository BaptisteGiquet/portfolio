#include "UI/Frontend/Core/EMRCommonListEntryWidgetKeyRemap.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "LocalizationLibrary.h"
#include "GAS/EMRTags.h"
#include "Styling/SlateBrush.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/Data/EMRListDataObjectKeyRemap.h"
#include "UI/Frontend/Screens/EMRKeyRemapScreenWidget.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Utils/DebugHelper.h"
#include "UI/Frontend/Utils/EMRFrontendFunctionLibrary.h"


void UEMRCommonListEntryWidgetKeyRemap::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	CommonButton_RemapKey->OnClicked().AddUObject(this, &ThisClass::OnRemapKeyButtonClicked);
	CommonButton_ResetKeyBinding->OnClicked().AddUObject(this, &ThisClass::OnResetKeyBindingButtonClicked);
}


void UEMRCommonListEntryWidgetKeyRemap::OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject)
{
	Super::OnOwningListDataObjectSet(InOwningListDataObject);
	
	CachedOwningKeyRemapDataObject = CastChecked<UEMRListDataObjectKeyRemap>(InOwningListDataObject);
	if (!CachedOwningKeyRemapDataObject) return;
	
	if (CachedOwningKeyRemapDataObject->GetDesiredInputTypeFromCurrentKey() == ECommonInputType::Gamepad)
	{
		CommonButton_RemapKey->GetButtonDisplayText()->SetVisibility(ESlateVisibility::Hidden);
		CommonButton_RemapKey->SetButtonDisplayImage(CachedOwningKeyRemapDataObject->GetIconFromCurrentKey());
	}
	else
	{
		CommonButton_RemapKey->GetButtonDisplayImage()->SetVisibility(ESlateVisibility::Hidden);
		CommonButton_RemapKey->SetButtonText(CachedOwningKeyRemapDataObject->GetDisplayNameFromCurrentKey());
	}
}


void UEMRCommonListEntryWidgetKeyRemap::OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason)
{
	Super::OnOwningListDataObjectModified(OwningModifiedListData, ModifyReason);
	
	if (CachedOwningKeyRemapDataObject)
	{
		CommonButton_RemapKey->SetButtonDisplayImage(CachedOwningKeyRemapDataObject->GetIconFromCurrentKey());	
		if (CachedOwningKeyRemapDataObject->GetDesiredInputTypeFromCurrentKey() == ECommonInputType::Gamepad)
		{
			CommonButton_RemapKey->SetButtonDisplayImage(CachedOwningKeyRemapDataObject->GetIconFromCurrentKey());
		}
		else
		{
			CommonButton_RemapKey->SetButtonText(CachedOwningKeyRemapDataObject->GetDisplayNameFromCurrentKey());	
		}
	}
}


void UEMRCommonListEntryWidgetKeyRemap::OnRemapKeyButtonClicked()
{
	SelectThisEntryWidget();
	
	UEMRUIManagerSubsystem::Get(this)->PushSoftWidgetToStackAsync(
		EMRTags::UI::WidgetStack::Modal,
		UEMRFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(EMRTags::UI::Widgets::KeyRemapScreen),
		[this](EAsyncPushWidgetState PushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
		{
			if (PushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				UEMRKeyRemapScreenWidget* CreatedKeyRemapScreen = CastChecked<UEMRKeyRemapScreenWidget>(PushedWidget);
				CreatedKeyRemapScreen->OnKeyRemapScreenKeyPressed.BindUObject(this, &ThisClass::OnKeyToRemapPressed);
				CreatedKeyRemapScreen->OnKeyRemapScreenKeySelectCanceled.BindUObject(this, &ThisClass::OnKeyRemapCanceled);
				if (CachedOwningKeyRemapDataObject)
				{
					CreatedKeyRemapScreen->SetDesiredInputTypeToFilter(CachedOwningKeyRemapDataObject->GetDesiredInputTypeFromCurrentKey());	
				}
			}
		});
}


void UEMRCommonListEntryWidgetKeyRemap::OnResetKeyBindingButtonClicked()
{
	SelectThisEntryWidget();
	
	if (!CachedOwningKeyRemapDataObject) return;
	
	// Check if current key is already the default key.
	if (!CachedOwningKeyRemapDataObject->CanResetBackToDefaultValue())
	{
		UEMRUIManagerSubsystem::Get(this)->PushConfirmScreenToModalStackAsync(
			EConfirmScreenType::Ok,
			GetLocalizedText(UIStringTableId, "UI.Confirm.Title.ResetKeyMapping"),
			GetLocalizedText(UIStringTableId, "UI.Confirm.Message.ResetKeyMapping.Error"),
[](EConfirmScreenButtonType ClickedButton){});
		
		return;
	}
	
	// Reset the key binding back to default
	UEMRUIManagerSubsystem::Get(this)->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::YesNo,
				GetLocalizedText(UIStringTableId, "UI.Confirm.Title.ResetKeyMapping"),
				GetLocalizedText(UIStringTableId, "UI.Confirm.Message.ResetKeyMapping"),
				[this](EConfirmScreenButtonType ClickedButton)
				{
					if (ClickedButton == EConfirmScreenButtonType::Confirmed)
					{
						CachedOwningKeyRemapDataObject->TryResetBackToDefaultValue();
					}
				});
}


void UEMRCommonListEntryWidgetKeyRemap::OnKeyToRemapPressed(const FKey& PressedKey)
{
	if (CachedOwningKeyRemapDataObject)
	{
		CachedOwningKeyRemapDataObject->BindNewInputKey(PressedKey);
	}
}


void UEMRCommonListEntryWidgetKeyRemap::OnKeyRemapCanceled(const FText& CanceledReason)
{
	UEMRUIManagerSubsystem::Get(this)->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::Ok,
		GetLocalizedText(UIStringTableId, "UI.Confirm.Title.Remap"),
		CanceledReason,
		[](EConfirmScreenButtonType ClickedButton){});
}
