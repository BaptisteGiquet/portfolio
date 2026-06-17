#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AGASSMadPlaneRouteAnchorActor.generated.h"

class USceneComponent;

UCLASS()
class AGASSTOWER_API AAGASSMadPlaneRouteAnchorActor : public AActor
{
	GENERATED_BODY()

public:
	AAGASSMadPlaneRouteAnchorActor();

	FTransform GetApproachStartTransform() const;
	FTransform GetRedirectExitTransform() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "AGASS|Mad Plane")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(
		VisibleAnywhere,
		Category = "AGASS|Mad Plane",
		meta = (DisplayName = "Approach Start Point", ToolTip = "Where the plane starts when MadPlane begins."))
	TObjectPtr<class UArrowComponent> ApproachStartPoint;

	UPROPERTY(
		VisibleAnywhere,
		Category = "AGASS|Mad Plane",
		meta = (DisplayName = "Redirect Exit Point", ToolTip = "Where the plane flies toward after it is diverted or after it passes through the tower."))
	TObjectPtr<class UArrowComponent> RedirectExitPoint;
};
