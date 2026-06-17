#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AGASSBenchmarkDeveloperSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AGASS Benchmark"))
class AGASSCOMMON_API UAGASSBenchmarkDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAGASSBenchmarkDeveloperSettings();

	static const UAGASSBenchmarkDeveloperSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category="Benchmark", meta=(ClampMin="1", UIMin="1"))
	int32 BenchmarkSchemaVersion;

	UPROPERTY(Config, EditAnywhere, Category="Benchmark", meta=(ClampMin="1", UIMin="1"))
	int32 BenchmarkWorkScale;

	UPROPERTY(Config, EditAnywhere, Category="Benchmark", meta=(ClampMin="0.10", UIMin="0.10"))
	float CPUMultiplier;

	UPROPERTY(Config, EditAnywhere, Category="Benchmark", meta=(ClampMin="0.10", UIMin="0.10"))
	float GPUMultiplier;

	UPROPERTY(Config, EditAnywhere, Category="Benchmark", meta=(ClampMin="30", ClampMax="240", UIMin="30", UIMax="240"))
	int32 TargetFrameRateLimit;

	UPROPERTY(Config, EditAnywhere, Category="Fallback", meta=(ClampMin="0", ClampMax="4", UIMin="0", UIMax="4"))
	int32 FallbackOverallQualityLevel;

	UPROPERTY(Config, EditAnywhere, Category="Fallback", meta=(ClampMin="25.0", ClampMax="100.0", UIMin="25.0", UIMax="100.0"))
	float FallbackResolutionQuality;
};
