#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AGASSFrontendDeveloperSettings.generated.h"

class UAGASSUIPolicy;
class UAGASSFrontendScreenWidget;
class UAGASSFrontendPrimaryLayoutWidget;
class UAGASSGameSettingActionEntryWidget;
class UAGASSGameSettingKeyBindEntryWidget;
class UAGASSGameSettingDiscreteEntryWidget;
class UAGASSGameSettingNavigationEntryWidget;
class UAGASSGameSettingPanelWidget;
class UAGASSGameSettingPressAnyKeyWidget;
class UAGASSGameSettingSectionHeaderEntryWidget;
class UAGASSGameSettingScalarEntryWidget;
class UAGASSKeyAlreadyBoundWarningWidget;
class UAGASSSessionBrowserEntryWidget;
class UAGASSFriendTimedRunEntryWidget;
class UAGASSMapSelectionEntryWidget;
class UAGASSModToggleEntryWidget;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AGASS Frontend"))
class AGASSFRONTEND_API UAGASSFrontendDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAGASSFrontendDeveloperSettings();

	static const UAGASSFrontendDeveloperSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category="Policy")
	TSoftClassPtr<UAGASSUIPolicy> DefaultUIPolicyClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSFrontendPrimaryLayoutWidget> FrontendPrimaryLayoutClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSFrontendScreenWidget> MainMenuScreenClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSFrontendScreenWidget> SessionBrowserScreenClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSFrontendScreenWidget> ModsAndMapsScreenClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSFrontendScreenWidget> OptionsScreenClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSFrontendScreenWidget> GameMenuScreenClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSFrontendScreenWidget> CreditsScreenClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSFrontendScreenWidget> LoadingScreenClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSSessionBrowserEntryWidget> SessionBrowserEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSMapSelectionEntryWidget> MapSelectionEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSModToggleEntryWidget> ModToggleEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Screens")
	TSoftClassPtr<UAGASSFriendTimedRunEntryWidget> FriendTimedRunEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Settings")
	TSoftClassPtr<UAGASSGameSettingPanelWidget> GameSettingPanelWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Settings")
	TSoftClassPtr<UAGASSGameSettingScalarEntryWidget> GameSettingScalarEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Settings")
	TSoftClassPtr<UAGASSGameSettingDiscreteEntryWidget> GameSettingDiscreteEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Settings")
	TSoftClassPtr<UAGASSGameSettingActionEntryWidget> GameSettingActionEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Settings")
	TSoftClassPtr<UAGASSGameSettingKeyBindEntryWidget> GameSettingKeyBindEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Settings")
	TSoftClassPtr<UAGASSGameSettingSectionHeaderEntryWidget> GameSettingSectionHeaderEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Settings")
	TSoftClassPtr<UAGASSGameSettingNavigationEntryWidget> GameSettingNavigationEntryWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Settings")
	TSoftClassPtr<UAGASSGameSettingPressAnyKeyWidget> GameSettingPressAnyKeyWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="Settings")
	TSoftClassPtr<UAGASSKeyAlreadyBoundWarningWidget> KeyAlreadyBoundWarningWidgetClass;
};
