#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AGASSPlayerDeveloperSettings.generated.h"

class AAGASSCharacter;
class AAGASSPlayerController;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AGASS Player"))
class AGASS_API UAGASSPlayerDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAGASSPlayerDeveloperSettings();

	static const UAGASSPlayerDeveloperSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category="Classes")
	TSoftClassPtr<AAGASSPlayerController> PlayerControllerClass;

	UPROPERTY(Config, EditAnywhere, Category="Classes")
	TSoftClassPtr<AAGASSCharacter> TowerPawnClass;
};
