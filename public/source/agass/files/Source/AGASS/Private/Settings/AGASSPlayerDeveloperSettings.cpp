#include "Settings/AGASSPlayerDeveloperSettings.h"

#include "AGASSCharacter.h"
#include "AGASSPlayerController.h"

UAGASSPlayerDeveloperSettings::UAGASSPlayerDeveloperSettings()
{
	CategoryName = TEXT("Game");

	PlayerControllerClass = AAGASSPlayerController::StaticClass();
	TowerPawnClass = AAGASSCharacter::StaticClass();
}

const UAGASSPlayerDeveloperSettings* UAGASSPlayerDeveloperSettings::Get()
{
	return GetDefault<UAGASSPlayerDeveloperSettings>();
}

FName UAGASSPlayerDeveloperSettings::GetCategoryName() const
{
	return CategoryName;
}
