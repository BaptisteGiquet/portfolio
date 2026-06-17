#include "Settings/AGASSBenchmarkDeveloperSettings.h"

UAGASSBenchmarkDeveloperSettings::UAGASSBenchmarkDeveloperSettings()
{
	CategoryName = TEXT("Game");

	BenchmarkSchemaVersion = 1;
	BenchmarkWorkScale = 10;
	CPUMultiplier = 0.85f;
	GPUMultiplier = 0.80f;
	TargetFrameRateLimit = 60;
	FallbackOverallQualityLevel = 1;
	FallbackResolutionQuality = 70.0f;
}

const UAGASSBenchmarkDeveloperSettings* UAGASSBenchmarkDeveloperSettings::Get()
{
	return GetDefault<UAGASSBenchmarkDeveloperSettings>();
}

FName UAGASSBenchmarkDeveloperSettings::GetCategoryName() const
{
	return CategoryName;
}
