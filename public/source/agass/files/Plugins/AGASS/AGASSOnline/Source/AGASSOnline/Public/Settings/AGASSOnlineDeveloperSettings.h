#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AGASSOnlineDeveloperSettings.generated.h"

class UWorld;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AGASS Online"))
class AGASSONLINE_API UAGASSOnlineDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAGASSOnlineDeveloperSettings();

	static const UAGASSOnlineDeveloperSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category="Maps")
	TSoftObjectPtr<UWorld> FrontendMap;

	UPROPERTY(Config, EditAnywhere, Category="Legacy", meta=(DisplayName="Legacy Tower Map", ToolTip="Deprecated compatibility fallback. Gameplay map selection now comes from AGASS Mods map definitions."))
	TSoftObjectPtr<UWorld> TowerMap;

	UPROPERTY(Config, EditAnywhere, Category="Sessions", meta=(ClampMin="1", UIMin="1"))
	int32 MaxPublicConnections;

	UPROPERTY(Config, EditAnywhere, Category="Sessions", meta=(ClampMin="1", UIMin="1"))
	int32 SessionSearchMaxResults;

	UPROPERTY(Config, EditAnywhere, Category="Sessions")
	bool bPreferLanSessions;

	UPROPERTY(Config, EditAnywhere, Category="Sessions")
	bool bUseLobbiesIfAvailable;

	UPROPERTY(Config, EditAnywhere, Category="Sessions")
	FString BuildCompatibilityKey;

	UPROPERTY(Config, EditAnywhere, Category="Invite Code", meta=(ClampMin="4", ClampMax="10", UIMin="4", UIMax="10"))
	int32 InviteCodeLength;
};
