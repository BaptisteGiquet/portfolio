

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "EMRFrontendCommonButtonBase.generated.h"

class UCommonLazyImage;
class UCommonTextBlock;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendCommonButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void SetButtonText(FText InText);
	
	UFUNCTION(BlueprintCallable)
	UCommonTextBlock* GetButtonDisplayText() const { return CommonTextBlock_ButtonText; };
	
	UFUNCTION(BlueprintCallable)
	void SetButtonDisplayImage(const FSlateBrush& InBrush);
	
	UCommonLazyImage* GetButtonDisplayImage() const { return CommonLazyImage_ButtonImage; } 

	
private:
	virtual void NativePreConstruct() override;

	virtual void NativeOnCurrentTextStyleChanged() override;

	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	
	//*** Bound Widgets ***//
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_ButtonText;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UCommonLazyImage> CommonLazyImage_ButtonImage;
	//*** Bound Widgets ***//

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Button", meta = (AllowPrivateAccess = true))
	FText ButtonDisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Button", meta = (AllowPrivateAccess = true))
	FText ButtonDescriptionText;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Button", meta = (AllowPrivateAccess = true))
	bool bUseUpperCaseForButtonText = false;
};
