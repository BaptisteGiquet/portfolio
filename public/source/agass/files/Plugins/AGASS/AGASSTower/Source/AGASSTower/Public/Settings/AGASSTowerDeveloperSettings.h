#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AGASSTowerDeveloperSettings.generated.h"

class AAGASSPlaceableItemActor;
class AAGASSObjectiveActor;
class UAGASSRespawnTuningData;
class UUserWidget;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AGASS Tower"))
class AGASSTOWER_API UAGASSTowerDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAGASSTowerDeveloperSettings();

	static const UAGASSTowerDeveloperSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, Category="Bootstrap")
	TSoftClassPtr<AAGASSPlaceableItemActor> DefaultBootstrapItemClass;

	UPROPERTY(Config, EditAnywhere, Category="Objective")
	bool bAutoSpawnDefaultObjectiveWhenMissing = true;

	UPROPERTY(Config, EditAnywhere, Category="Objective")
	TSoftClassPtr<AAGASSObjectiveActor> DefaultObjectiveActorClass;

	UPROPERTY(Config, EditAnywhere, Category="Objective")
	FTransform DefaultObjectiveSpawnTransform = FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 1250.f));

	UPROPERTY(Config, EditAnywhere, Category="Respawn")
	TSoftObjectPtr<UAGASSRespawnTuningData> DefaultRespawnTuning;

	UPROPERTY(Config, EditAnywhere, Category="Events")
	bool bEnableSessionEvents = true;

	UPROPERTY(Config, EditAnywhere, Category="Events", meta=(ClampMin="0.1"))
	float SessionEventEvaluationIntervalSeconds = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Placement", meta=(ClampMin="0.0", DisplayName="Default Blocking Overlap Inset", ToolTip="Fallback overlap forgiveness for placeable items that do not provide an item-definition placement inset. Higher values shrink the blocking validation box and allow a small amount of interpenetration during placement."))
	float DefaultPlacementValidationInset = 4.f;

	UPROPERTY(Config, EditAnywhere, Category="Placement", meta=(ClampMin="0.0", DisplayName="Default Placement Touch Tolerance", ToolTip="Fallback touch-support forgiveness for placeable items that do not provide behavior tuning. Higher values make touch-gated placement more forgiving."))
	float DefaultPlacementTouchTolerance = 4.f;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftClassPtr<UUserWidget> EndOfRunWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="UI", meta=(ClampMin="0"))
	int32 EndOfRunWidgetZOrder = 200;

	UPROPERTY(Config, EditAnywhere, Category="UI")
	TSoftClassPtr<UUserWidget> TimedRunHUDWidgetClass;

	UPROPERTY(Config, EditAnywhere, Category="UI", meta=(ClampMin="0"))
	int32 TimedRunHUDWidgetZOrder = 100;
};
