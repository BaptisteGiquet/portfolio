#include "Settings/AGASSEconomyDeveloperSettings.h"

#include "UI/AGASSTeamMoneyTestWidget.h"

UAGASSEconomyDeveloperSettings::UAGASSEconomyDeveloperSettings()
{
	CategoryName = TEXT("Game");

	InitialSharedMoney = 300;
	DefaultShopCatalog = nullptr;
	bShowTeamMoneyTestWidget = true;
	TeamMoneyTestWidgetClass = UAGASSTeamMoneyTestWidget::StaticClass();
	TeamMoneyTestWidgetZOrder = 20;
}

const UAGASSEconomyDeveloperSettings* UAGASSEconomyDeveloperSettings::Get()
{
	return GetDefault<UAGASSEconomyDeveloperSettings>();
}

FName UAGASSEconomyDeveloperSettings::GetCategoryName() const
{
	return CategoryName;
}
