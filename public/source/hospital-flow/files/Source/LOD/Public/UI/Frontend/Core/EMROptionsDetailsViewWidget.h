

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EMROptionsDetailsViewWidget.generated.h"

class UEMRListDataObjectBase;
class UCommonTextBlock;
class UCommonLazyImage;
class UCommonRichTextBlock;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMROptionsDetailsViewWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void UpdateDetailsViewInfo(UEMRListDataObjectBase* InDataObject, const FString& InEntryWidgetClassName = FString());
	void ClearDetailsViewInfo();
	
protected:
	virtual void NativeOnInitialized() override;
	
private:
	//***** Bound Widgets *****//
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Title;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonLazyImage> CommonLazyImage_DescriptionImage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonRichTextBlock> CommonRichTextBlock_Description;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonRichTextBlock> CommonRichTextBlock_DynamicDetails;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonRichTextBlock> CommonRichTextBlock_DisabledReason;
	//***** Bound Widgets *****//
};
