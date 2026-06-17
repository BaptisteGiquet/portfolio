#pragma once

#include "CoreMinimal.h"
#include "UI/AGASSFrontendScreenWidget.h"
#include "AGASSGameMenuScreenWidget.generated.h"

class UAGASSFrontendButtonBase;
class UCommonTextBlock;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSGameMenuScreenWidget : public UAGASSFrontendScreenWidget
{
	GENERATED_BODY()

public:
	UAGASSGameMenuScreenWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual FText GetTitleText() const override;
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual void BuildFallbackScreenContent(UVerticalBox* ContentBox) override;
	virtual void RefreshFromSessionState() override;
	virtual UWidget* ResolveInitialFocusTarget() const override;
#if WITH_EDITOR
	virtual void GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const override;
#endif

private:
	UFUNCTION()
	void HandleSteamInviteClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	UFUNCTION()
	void HandleRegenerateInviteCodeClicked();

	void RefreshInviteCodeText();
	void RefreshActionAvailability();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_SteamInvite;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Options;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_MainMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_RegenerateInviteCode;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_InviteCodeValue;
};
