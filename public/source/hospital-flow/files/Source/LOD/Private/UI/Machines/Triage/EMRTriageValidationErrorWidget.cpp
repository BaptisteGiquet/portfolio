#include "UI/Machines/Triage/EMRTriageValidationErrorWidget.h"

#include "CommonTextBlock.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"

void UEMRTriageValidationErrorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CommonButton_Dismiss)
	{
		CommonButton_Dismiss->OnClicked().RemoveAll(this);
		CommonButton_Dismiss->OnClicked().AddUObject(this, &ThisClass::HandleDismissClicked);
	}

	ClearMessage();
}

void UEMRTriageValidationErrorWidget::NativeDestruct()
{
	if (CommonButton_Dismiss)
	{
		CommonButton_Dismiss->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UEMRTriageValidationErrorWidget::ShowMessage(const FText& InMessage)
{
	if (InMessage.IsEmpty())
	{
		ClearMessage();
		return;
	}

	if (CommonTextBlock_Message)
	{
		CommonTextBlock_Message->SetText(InMessage);
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UEMRTriageValidationErrorWidget::ClearMessage()
{
	if (CommonTextBlock_Message)
	{
		CommonTextBlock_Message->SetText(FText::GetEmpty());
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UEMRTriageValidationErrorWidget::HandleDismissClicked()
{
	ClearMessage();
	OnDismissRequested.Broadcast();
}
