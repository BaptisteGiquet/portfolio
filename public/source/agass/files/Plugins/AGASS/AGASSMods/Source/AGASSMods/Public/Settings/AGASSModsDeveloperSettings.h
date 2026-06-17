#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "AGASSModsDeveloperSettings.generated.h"

class UAGASSMapDefinition;
class UWorld;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "AGASS Mods"))
class AGASSMODS_API UAGASSModsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAGASSModsDeveloperSettings();

	static const UAGASSModsDeveloperSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category = "Discovery")
	bool bScanProjectModsDirectory = true;

	UPROPERTY(Config, EditAnywhere, Category = "Discovery")
	bool bMountDiscoveredModsDuringRefresh = true;

	UPROPERTY(Config, EditAnywhere, Category = "Discovery")
	TArray<FDirectoryPath> AdditionalLocalModPluginRoots;

	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	TArray<FDirectoryPath> WorkshopStagingPluginRoots;

	UPROPERTY(Config, EditAnywhere, Category = "Workshop")
	TArray<FDirectoryPath> WorkshopInstallPluginRoots;

	UPROPERTY(Config, EditAnywhere, Category = "Base Maps")
	TArray<TSoftObjectPtr<UAGASSMapDefinition>> BaseGameMapDefinitions;

	UPROPERTY(Config, EditAnywhere, Category = "Base Maps")
	FString DefaultSelectedMapId;

	UPROPERTY(Config, EditAnywhere, Category = "Legacy Base Map")
	TSoftObjectPtr<UAGASSMapDefinition> DefaultBaseMapDefinition;

	UPROPERTY(Config, EditAnywhere, Category = "Legacy Base Map")
	TSoftObjectPtr<UWorld> DefaultBaseGameMap;

	UPROPERTY(Config, EditAnywhere, Category = "Legacy Base Map")
	FString DefaultBaseMapId;

	UPROPERTY(Config, EditAnywhere, Category = "Legacy Base Map")
	FText DefaultBaseMapDisplayName;

	UPROPERTY(Config, EditAnywhere, Category = "Legacy Base Map")
	FText DefaultBaseMapDescription;

	UPROPERTY(Config, EditAnywhere, Category = "Legacy Base Map")
	FString DefaultBaseMapVersion;

	UPROPERTY(Config, EditAnywhere, Category = "Legacy Base Map")
	FString DefaultBaseMapCompatibilityVersion;
};
