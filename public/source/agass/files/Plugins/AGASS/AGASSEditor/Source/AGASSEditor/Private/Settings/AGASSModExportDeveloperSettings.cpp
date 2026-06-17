#include "Settings/AGASSModExportDeveloperSettings.h"

UAGASSModExportDeveloperSettings::UAGASSModExportDeveloperSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("AGASS Mod Export");

	BaseReleaseRoot.Path = TEXT("Releases");
	ExportOutputDirectory.Path = TEXT("Saved/ModExports");
	TemporaryStagingDirectory.Path = TEXT("Saved/ModExportStaging");
}

const UAGASSModExportDeveloperSettings* UAGASSModExportDeveloperSettings::Get()
{
	return GetDefault<UAGASSModExportDeveloperSettings>();
}

FName UAGASSModExportDeveloperSettings::GetCategoryName() const
{
	return CategoryName;
}

FName UAGASSModExportDeveloperSettings::GetSectionName() const
{
	return SectionName;
}
