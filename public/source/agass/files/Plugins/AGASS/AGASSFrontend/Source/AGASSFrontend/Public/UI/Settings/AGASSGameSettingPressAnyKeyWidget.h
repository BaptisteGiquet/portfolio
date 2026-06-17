#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "UI/Settings/AGASSGameSettingKeyBind.h"
#include "AGASSGameSettingPressAnyKeyWidget.generated.h"

class UCommonTextBlock;
class UWidgetTree;
class FAGASSPressAnyKeyInputProcessor;

UCLASS(Blueprintable)
class AGASSFRONTEND_API UAGASSGameSettingPressAnyKeyWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UAGASSGameSettingPressAnyKeyWidget(const FObjectInitializer& ObjectInitializer);

	DECLARE_EVENT_OneParam(UAGASSGameSettingPressAnyKeyWidget, FOnKeySelected, FKey);
	FOnKeySelected OnKeySelected;

	DECLARE_EVENT(UAGASSGameSettingPressAnyKeyWidget, FOnKeySelectionCanceled);
	FOnKeySelectionCanceled OnKeySelectionCanceled;

	void Configure(EAGASSRebindInputType InInputType, bool bInAllowAxisKeys);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	friend class FAGASSPressAnyKeyInputProcessor;

	void EnsureWidgetTree();
	void RefreshPromptText();
	void HandleCapturedKey(FKey InKey);
	void HandleSelectionCanceled();
	void Dismiss(TFunction<void()> PostDismissCallback);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Prompt;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Details;

	EAGASSRebindInputType InputType = EAGASSRebindInputType::MouseAndKeyboard;
	bool bAllowAxisKeys = false;
	bool bSelectionComplete = false;
	TSharedPtr<FAGASSPressAnyKeyInputProcessor> InputProcessor;
};
