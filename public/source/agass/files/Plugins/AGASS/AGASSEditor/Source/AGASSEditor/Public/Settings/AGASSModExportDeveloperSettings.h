#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AGASSModExportDeveloperSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "AGASS Mod Export"))
class AGASSEDITOR_API UAGASSModExportDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAGASSModExportDeveloperSettings();

	static const UAGASSModExportDeveloperSettings* Get();

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

	UPROPERTY(Config, EditAnywhere, Category = "Release")
	FString BaseReleaseVersion;

	UPROPERTY(Config, EditAnywhere, Category = "Release")
	FDirectoryPath BaseReleaseRoot;

	UPROPERTY(Config, EditAnywhere, Category = "Output")
	FDirectoryPath ExportOutputDirectory;

	UPROPERTY(Config, EditAnywhere, Category = "Output")
	FDirectoryPath TemporaryStagingDirectory;
};
