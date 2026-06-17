#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "EMRTriageValidationErrorWidget.generated.h"

class UCommonTextBlock;
class UEMRFrontendCommonButtonBase;

DECLARE_MULTICAST_DELEGATE(FOnTriageValidationErrorDismissRequested);

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRTriageValidationErrorWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void ShowMessage(const FText& InMessage);
	void ClearMessage();

	FOnTriageValidationErrorDismissRequested OnDismissRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void HandleDismissClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Message = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Dismiss = nullptr;
};
