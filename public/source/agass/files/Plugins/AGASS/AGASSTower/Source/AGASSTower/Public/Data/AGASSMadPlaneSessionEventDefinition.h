#pragma once

#include "CoreMinimal.h"
#include "Data/AGASSSessionEventDefinition.h"
#include "AGASSMadPlaneSessionEventDefinition.generated.h"

class AAGASSMadPlaneAircraftActor;

UCLASS(BlueprintType)
class AGASSTOWER_API UAGASSMadPlaneSessionEventDefinition : public UAGASSSessionEventDefinition
{
	GENERATED_BODY()

public:
	virtual bool CanAutoActivate_Implementation(const UAGASSSessionEventManagerComponent* EventManager) const override;
	virtual void ConfigureEventActor_Implementation(AAGASSSessionEventActor* EventActor, UAGASSSessionEventManagerComponent* EventManager) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Classes")
	TSoftClassPtr<AAGASSMadPlaneAircraftActor> AircraftActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Gameplay", meta = (ClampMin = "0.0"))
	float MinimumTowerHeightMeters = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Timing", meta = (ClampMin = "0.0"))
	float LeadInSeconds = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Timing", meta = (ClampMin = "0.0"))
	float ApproachDurationSeconds = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Timing", meta = (ClampMin = "0.0"))
	float RedirectDurationSeconds = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Timing", meta = (ClampMin = "0.0"))
	float PostImpactPauseSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Flight", meta = (ClampMin = "0.0"))
	float FlightDistanceFromTower = 5000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Gameplay", meta = (ClampMin = "0.0"))
	float ImpactCollapseRadius = 450.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Gameplay", meta = (ClampMin = "0.0"))
	float CollapseImpulseStrength = 325.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Gameplay", meta = (ClampMin = "0.0"))
	float SignalHorizontalTolerance = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mad Plane|Gameplay", meta = (ClampMin = "0.0"))
	float SignalVerticalToleranceBelowTop = 220.f;
};
