#pragma once

#include "CoreMinimal.h"
#include "UI/AGASSPrimaryGameLayout.h"
#include "AGASSFrontendPrimaryLayoutWidget.generated.h"

class UCommonActivatableWidgetStack;
class UCommonBoundActionBar;
class UCommonTextBlock;
class UImage;
class UWidgetTree;
class IWidgetCompilerLog;

UCLASS()
class AGASSFRONTEND_API UAGASSFrontendPrimaryLayoutWidget : public UAGASSPrimaryGameLayout
{
	GENERATED_BODY()

public:
	UAGASSFrontendPrimaryLayoutWidget(const FObjectInitializer& ObjectInitializer);
	void SetFrontendShellVisible(bool bVisible);

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
#if WITH_EDITOR
	virtual void ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog) const override;
#endif

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameLayer_Stack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameMenu_Stack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> Menu_Stack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> Modal_Stack;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonBoundActionBar> BottomActionBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_Backdrop;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_TopBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Wordmark;
};
