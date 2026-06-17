#pragma once

#include "CoreMinimal.h"
#include "CommonInputTypeEnum.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRKeyRemapScreenWidget.generated.h"

class UCommonRichTextBlock;
class FKeyRemapScreenInputPreProcessor;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRKeyRemapScreenWidget : public UEMRFrontendCommonActivatableWidgetBase
{
	GENERATED_BODY()
	
public:
	void SetDesiredInputTypeToFilter(ECommonInputType InDesiredInputType);
	
	DECLARE_DELEGATE_OneParam(FOnKeyRemapScreenKeyPressedDelegate, const FKey& /*PressedKey*/);
	FOnKeyRemapScreenKeyPressedDelegate OnKeyRemapScreenKeyPressed;
	
	DECLARE_DELEGATE_OneParam(FOnKeyRemapScreenKeySelectCanceledDelegate, const FText& /*CanceledReason*/);
	FOnKeyRemapScreenKeySelectCanceledDelegate OnKeyRemapScreenKeySelectCanceled;
	
	
protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	
private:
	void OnValidKeyPressedDetected(const FKey& PressedKey);
	void OnKeySelectCanceled(const FText& CanceledReason);
	
	// Delay a tick to make sure the input key is captured properly before calling the PreDeactivateCallback and deactivating the widget.
	void RequestDeactivateWidget(TFunction<void()> PreDeactivateCallback);
	
	//***** Bound Widgets *****//
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonRichTextBlock> CommonRichTextBlock_RemapMessage;
	//***** Bound Widgets *****//
	
	TSharedPtr<FKeyRemapScreenInputPreProcessor> CachedInputPreProcessor;
	
	ECommonInputType CachedDesiredInputType;
};
