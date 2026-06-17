#include "Settings/AGASSSteamDeveloperSettings.h"

#include "Subsystems/AGASSGameplayEventSubsystem.h"

UAGASSSteamDeveloperSettings::UAGASSSteamDeveloperSettings()
{
	CategoryName = TEXT("Game");

	StoreStatsIntervalSeconds = 30.f;
	bLogSteamWriteOperations = true;

	FAGASSSteamStatDefinition& SessionsEnteredStat = StatDefinitions.AddDefaulted_GetRef();
	SessionsEnteredStat.SourceEventName = AGASSGameplayEventNames::SessionEntered();
	SessionsEnteredStat.SteamStatName = TEXT("AGASS_SESSIONS_ENTERED");

	FAGASSSteamStatDefinition& RunsCompletedStat = StatDefinitions.AddDefaulted_GetRef();
	RunsCompletedStat.SourceEventName = AGASSGameplayEventNames::RunCompleted();
	RunsCompletedStat.SteamStatName = TEXT("AGASS_RUNS_COMPLETED");

	FAGASSSteamStatDefinition& MoneyEarnedStat = StatDefinitions.AddDefaulted_GetRef();
	MoneyEarnedStat.SourceEventName = AGASSGameplayEventNames::MoneyEarned();
	MoneyEarnedStat.SteamStatName = TEXT("AGASS_TEAM_MONEY_EARNED");
	MoneyEarnedStat.ValueSource = EAGASSSteamEventValueSource::IntValue;

	FAGASSSteamStatDefinition& MoneySpentStat = StatDefinitions.AddDefaulted_GetRef();
	MoneySpentStat.SourceEventName = AGASSGameplayEventNames::MoneySpent();
	MoneySpentStat.SteamStatName = TEXT("AGASS_TEAM_MONEY_SPENT");
	MoneySpentStat.ValueSource = EAGASSSteamEventValueSource::IntValue;

	PlaytimeTracking.bAccumulateSessionMinutesToSteamStat = true;
	PlaytimeTracking.SteamStatName = TEXT("AGASS_TOWER_MINUTES_PLAYED");
	PlaytimeTracking.MinimumSecondsPerWrite = 60.f;
}

const UAGASSSteamDeveloperSettings* UAGASSSteamDeveloperSettings::Get()
{
	return GetDefault<UAGASSSteamDeveloperSettings>();
}

FName UAGASSSteamDeveloperSettings::GetCategoryName() const
{
	return CategoryName;
}
