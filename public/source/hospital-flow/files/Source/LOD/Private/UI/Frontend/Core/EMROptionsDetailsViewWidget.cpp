


#include "UI/Frontend/Core/EMROptionsDetailsViewWidget.h"

#include "CommonLazyImage.h"
#include "CommonRichTextBlock.h"
#include "CommonTextBlock.h"
#include "UI/Frontend/Data/EMRListDataObjectBase.h"


void UEMROptionsDetailsViewWidget::UpdateDetailsViewInfo(UEMRListDataObjectBase* InDataObject, const FString& InEntryWidgetClassName)
{
	if (!InDataObject) return;
	
	CommonTextBlock_Title->SetText(InDataObject->GetDataDisplayName());
	if (!InDataObject->GetSoftDescriptionImage().IsNull())
	{
		CommonLazyImage_DescriptionImage->SetBrushFromLazyTexture(InDataObject->GetSoftDescriptionImage());
		CommonLazyImage_DescriptionImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		CommonLazyImage_DescriptionImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	CommonRichTextBlock_Description->SetText(InDataObject->GetDescriptionRichText());
	
	const FString DynamicDetails = FString::Printf(
		TEXT("Data Object Class: <Bold>%s</>\n\nEntry Widget Class: <Bold>%s</>"),
		*InDataObject->GetClass()->GetName(),
		*InEntryWidgetClassName);
	
	CommonRichTextBlock_DynamicDetails->SetText(FText::FromString(DynamicDetails));
	CommonRichTextBlock_DisabledReason->SetText(InDataObject->IsDataCurrentlyEditable()? FText::GetEmpty() : InDataObject->GetDisabledRichText());
}


void UEMROptionsDetailsViewWidget::ClearDetailsViewInfo()
{
	CommonTextBlock_Title->SetText(FText::GetEmpty());
	CommonLazyImage_DescriptionImage->SetVisibility(ESlateVisibility::Collapsed);
	CommonRichTextBlock_Description->SetText(FText::GetEmpty());
	CommonRichTextBlock_DynamicDetails->SetText(FText::GetEmpty());
	CommonRichTextBlock_DisabledReason->SetText(FText::GetEmpty());
}


void UEMROptionsDetailsViewWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	ClearDetailsViewInfo();
}
