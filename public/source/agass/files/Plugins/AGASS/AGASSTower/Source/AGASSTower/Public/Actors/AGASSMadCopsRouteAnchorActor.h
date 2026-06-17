#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGASSMadCopsRouteAnchorActor.generated.h"

class USceneComponent;

UCLASS()
class AGASSTOWER_API AAGASSMadCopsRouteAnchorActor : public AActor
{
	GENERATED_BODY()

public:
	AAGASSMadCopsRouteAnchorActor();

	FTransform GetCarApproachStartTransform() const;
	FTransform GetCarParkingTransform() const;
	FTransform GetOfficerScanTransform() const;
	FTransform GetCarDepartureTransform() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Cops")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Cops", meta = (DisplayName = "Car Approach Start Point", ToolTip = "Where the police car starts when the event begins."))
	TObjectPtr<class UArrowComponent> CarApproachStartPoint;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Cops", meta = (DisplayName = "Car Parking Point", ToolTip = "Where the police car stops so the officer can get out."))
	TObjectPtr<class UArrowComponent> CarParkingPoint;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Cops", meta = (DisplayName = "Officer Scan Point", ToolTip = "Where the officer walks to before scanning the area for towers."))
	TObjectPtr<class UArrowComponent> OfficerScanPoint;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Cops", meta = (DisplayName = "Car Departure Point", ToolTip = "Where the police car drives toward when leaving the area."))
	TObjectPtr<class UArrowComponent> CarDeparturePoint;
};
