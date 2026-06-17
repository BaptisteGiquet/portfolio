#include "Settings/AGASSOnlineDeveloperSettings.h"

UAGASSOnlineDeveloperSettings::UAGASSOnlineDeveloperSettings()
{
	CategoryName = TEXT("Game");

	FrontendMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/AGASS/Maps/FrontendMap.FrontendMap")));
	TowerMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/AGASS/Maps/TowerMap.TowerMap")));
	MaxPublicConnections = 4;
	SessionSearchMaxResults = 32;
	bPreferLanSessions = true;
	bUseLobbiesIfAvailable = true;
	BuildCompatibilityKey = TEXT("agass-phase-07");
	InviteCodeLength = 6;
}

const UAGASSOnlineDeveloperSettings* UAGASSOnlineDeveloperSettings::Get()
{
	return GetDefault<UAGASSOnlineDeveloperSettings>();
}

FName UAGASSOnlineDeveloperSettings::GetCategoryName() const
{
	return CategoryName;
}
