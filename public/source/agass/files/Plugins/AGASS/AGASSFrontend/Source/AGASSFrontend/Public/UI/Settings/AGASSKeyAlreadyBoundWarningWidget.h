#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "AGASSKeyAlreadyBoundWarningWidget.generated.h"

class UAGASSFrontendButtonBase;
class UCommonTextBlock;
struct FGeometry;
struct FPointerEvent;

UCLASS(Blueprintable)
class AGASSFRONTEND_API UAGASSKeyAlreadyBoundWarningWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UAGASSKeyAlreadyBoundWarningWidget(const FObjectInitializer& ObjectInitializer);

	DECLARE_EVENT(UAGASSKeyAlreadyBoundWarningWidget, FOnConfirmed);
	FOnConfirmed OnConfirmed;

	DECLARE_EVENT(UAGASSKeyAlreadyBoundWarningWidget, FOnCanceled);
	FOnCanceled OnCanceled;

	void SetWarningText(const FText& InWarningText);

protected:
	virtual void NativeOnActivated() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void EnsureWidgetTree();
	void HandleConfirmClicked();
	void HandleCancelClicked();
	void Dismiss(TFunction<void()> PostDismissCallback);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Warning;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Confirm;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Cancel;

	FText WarningText;
	bool bDismissed = false;
};
