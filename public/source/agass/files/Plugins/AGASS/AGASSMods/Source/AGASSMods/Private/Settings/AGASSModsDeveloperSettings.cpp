#include "Settings/AGASSModsDeveloperSettings.h"

#include "Data/AGASSMapDefinition.h"

UAGASSModsDeveloperSettings::UAGASSModsDeveloperSettings()
{
	CategoryName = TEXT("Game");

	bScanProjectModsDirectory = true;
	bMountDiscoveredModsDuringRefresh = true;
	BaseGameMapDefinitions.Reset();
	DefaultSelectedMapId = TEXT("agass.base.tower");
	DefaultBaseMapDefinition = nullptr;
	DefaultBaseGameMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/AGASS/Maps/TowerMap.TowerMap")));
	DefaultBaseMapId = TEXT("agass.base.tower");
	DefaultBaseMapDisplayName = NSLOCTEXT("AGASSMods", "DefaultBaseMapDisplayName", "TowerMap");
	DefaultBaseMapDescription = NSLOCTEXT("AGASSMods", "DefaultBaseMapDescription", "The default handcrafted AGASS gameplay session map.");
	DefaultBaseMapVersion = TEXT("1.0.0");
	DefaultBaseMapCompatibilityVersion = TEXT("base-1");
}

const UAGASSModsDeveloperSettings* UAGASSModsDeveloperSettings::Get()
{
	return GetDefault<UAGASSModsDeveloperSettings>();
}

FName UAGASSModsDeveloperSettings::GetCategoryName() const
{
	return CategoryName;
}
