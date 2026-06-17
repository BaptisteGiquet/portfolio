#pragma once

#include "CoreMinimal.h"
#include "GameSettingRegistryChangeTracker.h"
#include "UI/AGASSFrontendScreenWidget.h"
#include "Input/UIActionBindingHandle.h"
#include "AGASSOptionsScreenWidget.generated.h"

enum class EGameSettingChangeReason : uint8;

class UAGASSFrontendButtonBase;
class UAGASSGameSettingPanelWidget;
class UAGASSGameSettingRegistry;
class UGameSetting;
class UTextBlock;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSOptionsScreenWidget : public UAGASSFrontendScreenWidget
{
	GENERATED_BODY()

public:
	UAGASSOptionsScreenWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual FText GetTitleText() const override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual void BuildFallbackScreenContent(UVerticalBox* ContentBox) override;
	virtual void RefreshFromSessionState() override;
	virtual UWidget* ResolveInitialFocusTarget() const override;
#if WITH_EDITOR
	virtual void GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const override;
#endif

private:
	void InitializeRegistry();
	void RegisterTopLevelTabs();
	void BindCategoryButton(UAGASSFrontendButtonBase* Button, void (UAGASSOptionsScreenWidget::*Handler)());
	void ShowCategory(const FName& InCategoryId, bool bFocusSettingsPanel = false);
	void RefreshTopLevelTabs();
	void RefreshSummaryText();
	FText GetCategoryDisplayText(const FName& CategoryId) const;
	TArray<FName> GetTopLevelCategoryOrder() const;
	int32 GetCategoryIndex(const FName& CategoryId) const;
	bool IsGamepadInputActive() const;
	void FocusSettingsPanelIfPossible() const;
	void ShowAdjacentCategory(int32 Direction);
	void HandleNextTabAction();
	void HandlePreviousTabAction();

	void HandleRegistrySettingChanged(UGameSetting* Setting, EGameSettingChangeReason Reason);

	UFUNCTION()
	void HandleVideoTabClicked();
	UFUNCTION()
	void HandleAudioTabClicked();
	UFUNCTION()
	void HandleGameplayTabClicked();
	UFUNCTION()
	void HandleControlsTabClicked();
	UFUNCTION()
	void HandleInterfaceTabClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_TabVideo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_TabAudio;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_TabGameplay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_TabControls;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_TabInterface;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_OptionsSummary;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSGameSettingPanelWidget> Settings_Panel;

	UPROPERTY(Transient)
	TObjectPtr<UAGASSGameSettingRegistry> Registry;

	FGameSettingRegistryChangeTracker ChangeTracker;
	FDelegateHandle RegistrySettingChangedHandle;
	FName ActiveCategoryId = TEXT("VideoCollection");
	FUIActionBindingHandle NextTabActionHandle;
	FUIActionBindingHandle PreviousTabActionHandle;
};
