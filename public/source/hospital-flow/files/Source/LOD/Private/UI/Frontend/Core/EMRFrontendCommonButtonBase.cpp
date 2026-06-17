


#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"


void UEMRFrontendCommonButtonBase::SetButtonText(FText InText)
{
	if (CommonTextBlock_ButtonText && !InText.IsEmpty())
	{
		CommonTextBlock_ButtonText->SetText(bUseUpperCaseForButtonText ? InText.ToUpper() : InText);
	}
}


void UEMRFrontendCommonButtonBase::SetButtonDisplayImage(const FSlateBrush& InBrush)
{
	if (CommonLazyImage_ButtonImage)
	{
		CommonLazyImage_ButtonImage->SetBrush(InBrush);
	}
}


void UEMRFrontendCommonButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	SetButtonText(ButtonDisplayText);
}


void UEMRFrontendCommonButtonBase::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();

	if (CommonTextBlock_ButtonText && GetCurrentTextStyleClass())
	{
		CommonTextBlock_ButtonText->SetStyle(GetCurrentTextStyleClass());
	}
}


void UEMRFrontendCommonButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();

	if (!ButtonDescriptionText.IsEmpty())
	{
		UEMRUIManagerSubsystem::Get(this)->OnButtonDescriptionTextUpdated.Broadcast(this, ButtonDescriptionText);
	}
}


void UEMRFrontendCommonButtonBase::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();

	if (!ButtonDescriptionText.IsEmpty())
	{
		UEMRUIManagerSubsystem::Get(this)->OnButtonDescriptionTextUpdated.Broadcast(this, FText::GetEmpty());
	}
}
