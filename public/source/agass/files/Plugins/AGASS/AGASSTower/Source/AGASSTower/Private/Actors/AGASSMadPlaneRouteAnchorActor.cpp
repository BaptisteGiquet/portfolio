#include "Actors/AGASSMadPlaneRouteAnchorActor.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

AAGASSMadPlaneRouteAnchorActor::AAGASSMadPlaneRouteAnchorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ApproachStartPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("ApproachStartPoint"));
	ApproachStartPoint->SetupAttachment(SceneRoot);
	ApproachStartPoint->SetRelativeLocation(FVector(-5000.f, 0.f, 1600.f));
	ApproachStartPoint->ArrowColor = FColor::Yellow;
	ApproachStartPoint->ArrowSize = 1.75f;
	ApproachStartPoint->SetHiddenInGame(true);

	RedirectExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("RedirectExitPoint"));
	RedirectExitPoint->SetupAttachment(SceneRoot);
	RedirectExitPoint->SetRelativeLocation(FVector(5500.f, 0.f, 2300.f));
	RedirectExitPoint->ArrowColor = FColor::Green;
	RedirectExitPoint->ArrowSize = 1.75f;
	RedirectExitPoint->SetHiddenInGame(true);
}

FTransform AAGASSMadPlaneRouteAnchorActor::GetApproachStartTransform() const
{
	return ApproachStartPoint != nullptr ? ApproachStartPoint->GetComponentTransform() : GetActorTransform();
}

FTransform AAGASSMadPlaneRouteAnchorActor::GetRedirectExitTransform() const
{
	return RedirectExitPoint != nullptr ? RedirectExitPoint->GetComponentTransform() : GetActorTransform();
}
