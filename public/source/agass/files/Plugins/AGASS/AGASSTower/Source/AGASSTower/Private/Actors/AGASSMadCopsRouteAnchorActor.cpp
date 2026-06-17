#include "Actors/AGASSMadCopsRouteAnchorActor.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

AAGASSMadCopsRouteAnchorActor::AAGASSMadCopsRouteAnchorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CarApproachStartPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("CarApproachStartPoint"));
	CarApproachStartPoint->SetupAttachment(SceneRoot);
	CarApproachStartPoint->SetRelativeLocation(FVector(-2200.f, 0.f, 0.f));
	CarApproachStartPoint->ArrowColor = FColor::Yellow;
	CarApproachStartPoint->ArrowSize = 1.75f;
	CarApproachStartPoint->SetHiddenInGame(true);

	CarParkingPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("CarParkingPoint"));
	CarParkingPoint->SetupAttachment(SceneRoot);
	CarParkingPoint->SetRelativeLocation(FVector(-850.f, 0.f, 0.f));
	CarParkingPoint->ArrowColor = FColor(64, 160, 255);
	CarParkingPoint->ArrowSize = 1.5f;
	CarParkingPoint->SetHiddenInGame(true);

	OfficerScanPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("OfficerScanPoint"));
	OfficerScanPoint->SetupAttachment(SceneRoot);
	OfficerScanPoint->SetRelativeLocation(FVector(-350.f, 0.f, 0.f));
	OfficerScanPoint->ArrowColor = FColor::Red;
	OfficerScanPoint->ArrowSize = 1.5f;
	OfficerScanPoint->SetHiddenInGame(true);

	CarDeparturePoint = CreateDefaultSubobject<UArrowComponent>(TEXT("CarDeparturePoint"));
	CarDeparturePoint->SetupAttachment(SceneRoot);
	CarDeparturePoint->SetRelativeLocation(FVector(2400.f, 0.f, 0.f));
	CarDeparturePoint->ArrowColor = FColor::Green;
	CarDeparturePoint->ArrowSize = 1.75f;
	CarDeparturePoint->SetHiddenInGame(true);
}

FTransform AAGASSMadCopsRouteAnchorActor::GetCarApproachStartTransform() const
{
	return CarApproachStartPoint != nullptr ? CarApproachStartPoint->GetComponentTransform() : GetActorTransform();
}

FTransform AAGASSMadCopsRouteAnchorActor::GetCarParkingTransform() const
{
	return CarParkingPoint != nullptr ? CarParkingPoint->GetComponentTransform() : GetActorTransform();
}

FTransform AAGASSMadCopsRouteAnchorActor::GetOfficerScanTransform() const
{
	return OfficerScanPoint != nullptr ? OfficerScanPoint->GetComponentTransform() : GetActorTransform();
}

FTransform AAGASSMadCopsRouteAnchorActor::GetCarDepartureTransform() const
{
	return CarDeparturePoint != nullptr ? CarDeparturePoint->GetComponentTransform() : GetActorTransform();
}
