#pragma once

#include "CoreMinimal.h"
#include "UI/AGASSFrontendScreenWidget.h"
#include "AGASSMainMenuScreenWidget.generated.h"

class UAGASSFrontendButtonBase;
class UAGASSFriendTimedRunEntryWidget;
class UAGASSSteamPlatformSubsystem;
class UScrollBox;
class UTextBlock;
class UWidget;

UCLASS()
class AGASSFRONTEND_API UAGASSMainMenuScreenWidget : public UAGASSFrontendScreenWidget
{
	GENERATED_BODY()

protected:
	virtual FText GetTitleText() const override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void BuildFallbackScreenContent(UVerticalBox* ContentBox) override;
	virtual void RefreshFromSessionState() override;
	virtual UWidget* ResolveInitialFocusTarget() const override;
#if WITH_EDITOR
	virtual void GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const override;
#endif

private:
	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleJoinSessionClicked();

	UFUNCTION()
	void HandleModsAndMapsClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleCreditsClicked();

	UFUNCTION()
	void HandleQuitClicked();
	void HandleModsSelectionChanged();
	void HandleTimedRunFriendsUpdated(const FString& LeaderboardKey);
	void RefreshTimedRunSummary();
	UAGASSSteamPlatformSubsystem* GetSteamPlatformSubsystem() const;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Host;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_JoinSession;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_ModsAndMaps;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Options;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Credits;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UAGASSFrontendButtonBase> Button_Quit;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SelectedMapSummary;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ActiveModsSummary;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PersonalBest;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_FriendTimesStatus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> Scroll_FriendTimes;

	FDelegateHandle ModsSelectionChangedHandle;
	FDelegateHandle TimedRunFriendsUpdatedHandle;
};
