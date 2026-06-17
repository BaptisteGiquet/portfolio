#pragma once

#include "CoreMinimal.h"
#include "UI/AGASSFrontendScreenWidget.h"
#include "AGASSModsAndMapsScreenWidget.generated.h"

class UAGASSFrontendButtonBase;
class UScrollBox;
class UTextBlock;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSModsAndMapsScreenWidget : public UAGASSFrontendScreenWidget
{
	GENERATED_BODY()

public:
	UAGASSModsAndMapsScreenWidget(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	virtual FText GetTitleText() const override;
	virtual void BuildFallbackScreenContent(UVerticalBox* ContentBox) override;
	virtual void RefreshFromSessionState() override;
	virtual UWidget* ResolveInitialFocusTarget() const override;
#if WITH_EDITOR
	virtual void GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const override;
#endif

private:
	void RebuildLists();
	void HandleModsRegistryChanged();
	void HandleModsSelectionChanged();

	UFUNCTION()
	void HandleRefreshClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_RefreshRegistry;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CurrentSelection;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CurrentMapValue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CurrentSourceValue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CurrentModsValue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> Scroll_Maps;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> Scroll_Mods;

	FDelegateHandle ModsRegistryChangedHandle;
	FDelegateHandle ModsSelectionChangedHandle;
};
