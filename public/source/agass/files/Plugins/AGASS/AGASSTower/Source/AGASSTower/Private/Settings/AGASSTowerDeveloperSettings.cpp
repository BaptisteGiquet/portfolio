#include "Settings/AGASSTowerDeveloperSettings.h"

#include "Actors/AGASSObjectiveActor.h"
#include "Actors/AGASSPlaceableItemActor.h"
#include "Data/AGASSRespawnTuningData.h"
#include "UI/AGASSEndOfRunWidget.h"
#include "UI/AGASSTimedRunHUDWidget.h"

UAGASSTowerDeveloperSettings::UAGASSTowerDeveloperSettings()
{
	CategoryName = TEXT("Game");

	DefaultBootstrapItemClass = AAGASSPlaceableItemActor::StaticClass();
	bAutoSpawnDefaultObjectiveWhenMissing = true;
	DefaultObjectiveActorClass = AAGASSObjectiveActor::StaticClass();
	DefaultObjectiveSpawnTransform = FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 1250.f));
	DefaultRespawnTuning = nullptr;
	bEnableSessionEvents = true;
	SessionEventEvaluationIntervalSeconds = 1.0f;
	DefaultPlacementValidationInset = 4.f;
	DefaultPlacementTouchTolerance = 4.f;
	EndOfRunWidgetClass = UAGASSEndOfRunWidget::StaticClass();
	EndOfRunWidgetZOrder = 200;
	TimedRunHUDWidgetClass = UAGASSTimedRunHUDWidget::StaticClass();
	TimedRunHUDWidgetZOrder = 100;
}

const UAGASSTowerDeveloperSettings* UAGASSTowerDeveloperSettings::Get()
{
	return GetDefault<UAGASSTowerDeveloperSettings>();
}

FName UAGASSTowerDeveloperSettings::GetCategoryName() const
{
	return CategoryName;
}
