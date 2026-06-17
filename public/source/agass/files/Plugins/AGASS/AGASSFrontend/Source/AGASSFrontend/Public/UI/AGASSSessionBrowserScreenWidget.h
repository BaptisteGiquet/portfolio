#pragma once

#include "CoreMinimal.h"
#include "UI/AGASSFrontendScreenWidget.h"
#include "AGASSSessionBrowserScreenWidget.generated.h"

class UAGASSFrontendButtonBase;
class UCommonTextBlock;
class UEditableTextBox;
class UScrollBox;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSSessionBrowserScreenWidget : public UAGASSFrontendScreenWidget
{
	GENERATED_BODY()

public:
	UAGASSSessionBrowserScreenWidget(const FObjectInitializer& ObjectInitializer);
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
	void RebuildSessionList();
	void RefreshEntryPoints();
	void HandleSearchResultsChanged();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleJoinInviteCodeClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Refresh;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_JoinInviteCode;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> TextBox_InviteCode;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> Scroll_Results;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_ResultsState;

	FDelegateHandle SearchResultsChangedHandle;
};
