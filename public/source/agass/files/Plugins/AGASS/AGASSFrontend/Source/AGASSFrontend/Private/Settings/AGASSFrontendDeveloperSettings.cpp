#include "Settings/AGASSFrontendDeveloperSettings.h"

#include "Subsystems/AGASSUIPolicy.h"
#include "UI/AGASSCreditsScreenWidget.h"
#include "UI/AGASSFriendTimedRunEntryWidget.h"
#include "UI/AGASSFrontendPrimaryLayoutWidget.h"
#include "UI/AGASSGameMenuScreenWidget.h"
#include "UI/AGASSLoadingScreenWidget.h"
#include "UI/AGASSMapSelectionEntryWidget.h"
#include "UI/AGASSMainMenuScreenWidget.h"
#include "UI/AGASSModToggleEntryWidget.h"
#include "UI/AGASSModsAndMapsScreenWidget.h"
#include "UI/AGASSOptionsScreenWidget.h"
#include "UI/AGASSSessionBrowserEntryWidget.h"
#include "UI/AGASSSessionBrowserScreenWidget.h"
#include "UI/Settings/AGASSGameSettingKeyBind.h"
#include "UI/Settings/AGASSGameSettingPanelWidget.h"
#include "UI/Settings/AGASSGameSettingPressAnyKeyWidget.h"
#include "UI/Settings/AGASSGameSettingWidgets.h"
#include "UI/Settings/AGASSKeyAlreadyBoundWarningWidget.h"

UAGASSFrontendDeveloperSettings::UAGASSFrontendDeveloperSettings()
{
	CategoryName = TEXT("Game");

	DefaultUIPolicyClass = UAGASSUIPolicy::StaticClass();
	FrontendPrimaryLayoutClass = UAGASSFrontendPrimaryLayoutWidget::StaticClass();
	MainMenuScreenClass = UAGASSMainMenuScreenWidget::StaticClass();
	SessionBrowserScreenClass = UAGASSSessionBrowserScreenWidget::StaticClass();
	ModsAndMapsScreenClass = UAGASSModsAndMapsScreenWidget::StaticClass();
	OptionsScreenClass = UAGASSOptionsScreenWidget::StaticClass();
	GameMenuScreenClass = UAGASSGameMenuScreenWidget::StaticClass();
	CreditsScreenClass = UAGASSCreditsScreenWidget::StaticClass();
	LoadingScreenClass = UAGASSLoadingScreenWidget::StaticClass();
	SessionBrowserEntryWidgetClass = UAGASSSessionBrowserEntryWidget::StaticClass();
	MapSelectionEntryWidgetClass = UAGASSMapSelectionEntryWidget::StaticClass();
	ModToggleEntryWidgetClass = UAGASSModToggleEntryWidget::StaticClass();
	FriendTimedRunEntryWidgetClass = UAGASSFriendTimedRunEntryWidget::StaticClass();
	GameSettingPanelWidgetClass = UAGASSGameSettingPanelWidget::StaticClass();
	GameSettingScalarEntryWidgetClass = UAGASSGameSettingScalarEntryWidget::StaticClass();
	GameSettingDiscreteEntryWidgetClass = UAGASSGameSettingDiscreteEntryWidget::StaticClass();
	GameSettingActionEntryWidgetClass = UAGASSGameSettingActionEntryWidget::StaticClass();
	GameSettingKeyBindEntryWidgetClass = UAGASSGameSettingKeyBindEntryWidget::StaticClass();
	GameSettingSectionHeaderEntryWidgetClass = UAGASSGameSettingSectionHeaderEntryWidget::StaticClass();
	GameSettingNavigationEntryWidgetClass = UAGASSGameSettingNavigationEntryWidget::StaticClass();
	GameSettingPressAnyKeyWidgetClass = UAGASSGameSettingPressAnyKeyWidget::StaticClass();
	KeyAlreadyBoundWarningWidgetClass = UAGASSKeyAlreadyBoundWarningWidget::StaticClass();
}

const UAGASSFrontendDeveloperSettings* UAGASSFrontendDeveloperSettings::Get()
{
	return GetDefault<UAGASSFrontendDeveloperSettings>();
}

FName UAGASSFrontendDeveloperSettings::GetCategoryName() const
{
	return CategoryName;
}
