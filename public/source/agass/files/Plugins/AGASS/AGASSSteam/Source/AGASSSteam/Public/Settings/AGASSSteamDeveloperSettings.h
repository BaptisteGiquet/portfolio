#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AGASSSteamDeveloperSettings.generated.h"

UENUM()
enum class EAGASSSteamStatValueType : uint8
{
	Integer,
	Float
};

UENUM()
enum class EAGASSSteamStatAggregation : uint8
{
	Add,
	Maximum,
	Set
};

UENUM()
enum class EAGASSSteamEventValueSource : uint8
{
	UnitValue,
	IntValue,
	FloatValue,
	FlagValue
};

UENUM()
enum class EAGASSSteamAchievementTriggerType : uint8
{
	GameplayEventCountAtLeast,
	GameplayEventFlagTrue,
	StatValueAtLeast
};

USTRUCT()
struct AGASSSTEAM_API FAGASSSteamStatDefinition
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category="Stat")
	FName SourceEventName;

	UPROPERTY(Config, EditAnywhere, Category="Stat")
	FString SteamStatName;

	UPROPERTY(Config, EditAnywhere, Category="Stat")
	EAGASSSteamStatValueType ValueType = EAGASSSteamStatValueType::Integer;

	UPROPERTY(Config, EditAnywhere, Category="Stat")
	EAGASSSteamStatAggregation Aggregation = EAGASSSteamStatAggregation::Add;

	UPROPERTY(Config, EditAnywhere, Category="Stat")
	EAGASSSteamEventValueSource ValueSource = EAGASSSteamEventValueSource::UnitValue;

	UPROPERTY(Config, EditAnywhere, Category="Stat", meta=(ClampMin="0.0", UIMin="0.0"))
	float Multiplier = 1.f;
};

USTRUCT()
struct AGASSSTEAM_API FAGASSSteamAchievementDefinition
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category="Achievement")
	FString SteamAchievementId;

	UPROPERTY(Config, EditAnywhere, Category="Achievement")
	EAGASSSteamAchievementTriggerType TriggerType = EAGASSSteamAchievementTriggerType::StatValueAtLeast;

	UPROPERTY(Config, EditAnywhere, Category="Achievement")
	FName SourceEventName;

	UPROPERTY(Config, EditAnywhere, Category="Achievement")
	FString SourceStatName;

	UPROPERTY(Config, EditAnywhere, Category="Achievement", meta=(ClampMin="1", UIMin="1"))
	int32 RequiredEventCount = 1;

	UPROPERTY(Config, EditAnywhere, Category="Achievement", meta=(ClampMin="0.0", UIMin="0.0"))
	float RequiredStatValue = 1.f;
};

USTRUCT()
struct AGASSSTEAM_API FAGASSSteamPlaytimeTrackingSettings
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category="Playtime")
	bool bAccumulateSessionMinutesToSteamStat = true;

	UPROPERTY(Config, EditAnywhere, Category="Playtime")
	FString SteamStatName;

	UPROPERTY(Config, EditAnywhere, Category="Playtime", meta=(ClampMin="1.0", UIMin="1.0"))
	float MinimumSecondsPerWrite = 60.f;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AGASS Steam"))
class AGASSSTEAM_API UAGASSSteamDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAGASSSteamDeveloperSettings();

	static const UAGASSSteamDeveloperSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category="Stats", meta=(ClampMin="1.0", UIMin="1.0"))
	float StoreStatsIntervalSeconds;

	UPROPERTY(Config, EditAnywhere, Category="Stats")
	bool bLogSteamWriteOperations;

	UPROPERTY(Config, EditAnywhere, Category="Stats")
	TArray<FAGASSSteamStatDefinition> StatDefinitions;

	UPROPERTY(Config, EditAnywhere, Category="Achievements")
	TArray<FAGASSSteamAchievementDefinition> AchievementDefinitions;

	UPROPERTY(Config, EditAnywhere, Category="Playtime")
	FAGASSSteamPlaytimeTrackingSettings PlaytimeTracking;
};
