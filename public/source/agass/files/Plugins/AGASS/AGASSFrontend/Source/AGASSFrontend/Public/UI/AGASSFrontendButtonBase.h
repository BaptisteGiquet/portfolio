#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "AGASSFrontendButtonBase.generated.h"

class UCommonTextBlock;
class UBorder;
class UWidgetTree;
class IWidgetCompilerLog;

UCLASS()
class AGASSFRONTEND_API UAGASSFrontendButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UAGASSFrontendButtonBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "AGASS Frontend")
	void SetButtonText(const FText& InText);

	UFUNCTION(BlueprintCallable, Category = "AGASS Frontend")
	void SetUseSelectedVisual(bool bInUseSelectedVisual);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnWidgetRebuilt() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void SynchronizeProperties() override;
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
#if WITH_EDITOR
	virtual void ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS Frontend", meta = (AllowPrivateAccess = true))
	FText ButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS Frontend", meta = (AllowPrivateAccess = true))
	bool bUseSelectedVisual = false;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_Label;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Label;

private:
	void SynchronizeButtonText();
	void UpdateVisualState();

	bool bHasFocusPath = false;
};
