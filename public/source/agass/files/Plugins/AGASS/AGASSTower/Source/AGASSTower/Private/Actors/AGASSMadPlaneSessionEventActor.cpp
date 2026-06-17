#include "Actors/AGASSMadPlaneSessionEventActor.h"

#include "Actors/AGASSMadPlaneAircraftActor.h"
#include "Actors/AGASSPlaceableItemActor.h"
#include "Data/AGASSMadPlaneSessionEventDefinition.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "SessionEvents/AGASSTowerStructureAnalysis.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSMadPlaneEvent, Log, All);

namespace
{
FRotator MakePlaneLookAtRotation(const FVector& From, const FVector& To)
{
	const FVector Direction = (To - From).GetSafeNormal();
	return Direction.IsNearlyZero() ? FRotator::ZeroRotator : Direction.Rotation();
}
}

AAGASSMadPlaneSessionEventActor::AAGASSMadPlaneSessionEventActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.SetTickFunctionEnable(false);

	ConfiguredAircraftActorClass = AAGASSMadPlaneAircraftActor::StaticClass();
}

void AAGASSMadPlaneSessionEventActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	switch (MadPlanePhase)
	{
	case EAGASSMadPlaneEventPhase::LeadIn:
		if (HasPhaseElapsed(LeadInSeconds))
		{
			StartFlightSegment(
				EAGASSMadPlaneEventPhase::Approaching,
				ApproachStartTransform.GetLocation(),
				BuildImpactPoint(),
				ApproachDurationSeconds);
		}
		break;

	case EAGASSMadPlaneEventPhase::Approaching:
		if (AircraftActor == nullptr)
		{
			RequestFinishEvent();
			return;
		}

		if (UpdateFlightSegment())
		{
			PerformImpactCollapse();
		}
		break;

	case EAGASSMadPlaneEventPhase::Diverted:
	case EAGASSMadPlaneEventPhase::Leaving:
		if (AircraftActor == nullptr)
		{
			RequestFinishEvent();
			return;
		}

		if (UpdateFlightSegment())
		{
			AdvancePhase(EAGASSMadPlaneEventPhase::Finished);
			RequestFinishEvent();
		}
		break;

	case EAGASSMadPlaneEventPhase::Impacted:
		if (HasPhaseElapsed(PostImpactPauseSeconds))
		{
			StartLeaveFlight(false);
		}
		break;

	default:
		break;
	}
}

void AAGASSMadPlaneSessionEventActor::HandleEventActivated_Implementation()
{
	Super::HandleEventActivated_Implementation();

	bPlaneDiverted = false;
	AircraftActor = nullptr;
	TargetTowerItems.Reset();
	TargetTowerCenter = FVector::ZeroVector;
	TargetTowerTopLocation = FVector::ZeroVector;
	TargetTowerHeightMeters = 0.f;
	ApproachStartTransform = FTransform::Identity;
	DivertExitTransform = FTransform::Identity;
	FlightSegmentStartTransform = FTransform::Identity;
	FlightSegmentEndTransform = FTransform::Identity;
	CurrentFlightDurationSeconds = 0.f;

	if (!ResolveTargetTower() || !ResolveFlightPath() || !SpawnAircraftActor())
	{
		RequestFinishEvent();
		return;
	}

	SetActorTickEnabled(true);
	AdvancePhase(EAGASSMadPlaneEventPhase::LeadIn);
}

void AAGASSMadPlaneSessionEventActor::HandleEventEnded_Implementation()
{
	SetActorTickEnabled(false);
	TargetTowerItems.Reset();

	if (AircraftActor != nullptr)
	{
		AircraftActor->Destroy();
		AircraftActor = nullptr;
	}

	Super::HandleEventEnded_Implementation();
}

void AAGASSMadPlaneSessionEventActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MadPlanePhase);
	DOREPLIFETIME(ThisClass, AircraftActor);
	DOREPLIFETIME(ThisClass, TargetTowerCenter);
	DOREPLIFETIME(ThisClass, TargetTowerTopLocation);
	DOREPLIFETIME(ThisClass, TargetTowerHeightMeters);
	DOREPLIFETIME(ThisClass, bPlaneDiverted);
}

void AAGASSMadPlaneSessionEventActor::ApplyDefinitionSettings(const UAGASSMadPlaneSessionEventDefinition& Definition)
{
	ConfiguredAircraftActorClass = Definition.AircraftActorClass;
	MinimumTargetTowerHeightMeters = FMath::Max(Definition.MinimumTowerHeightMeters, 0.f);
	LeadInSeconds = FMath::Max(Definition.LeadInSeconds, 0.f);
	ApproachDurationSeconds = FMath::Max(Definition.ApproachDurationSeconds, 0.f);
	FlightDistanceFromTower = FMath::Max(Definition.FlightDistanceFromTower, 1.f);
	RedirectDurationSeconds = FMath::Max(Definition.RedirectDurationSeconds, 0.f);
	PostImpactPauseSeconds = FMath::Max(Definition.PostImpactPauseSeconds, 0.f);
	ImpactCollapseRadius = FMath::Max(Definition.ImpactCollapseRadius, 0.f);
	CollapseImpulseStrength = FMath::Max(Definition.CollapseImpulseStrength, 0.f);
	SignalHorizontalTolerance = FMath::Max(Definition.SignalHorizontalTolerance, 0.f);
	SignalVerticalToleranceBelowTop = FMath::Max(Definition.SignalVerticalToleranceBelowTop, 0.f);
}

bool AAGASSMadPlaneSessionEventActor::TryAcceptFumigeneSignal(AController* UsingController)
{
	if (!HasAuthority()
		|| UsingController == nullptr
		|| bPlaneDiverted
		|| (MadPlanePhase != EAGASSMadPlaneEventPhase::LeadIn && MadPlanePhase != EAGASSMadPlaneEventPhase::Approaching)
		|| !IsControllerAtValidSignalPosition(*UsingController))
	{
		return false;
	}

	bPlaneDiverted = true;
	const APawn* const UsingPawn = UsingController->GetPawn();
	const FVector SignalLocation = UsingPawn != nullptr ? UsingPawn->GetActorLocation() : FVector(TargetTowerTopLocation);

	MulticastNotifyMadPlaneDiverted(SignalLocation);
	StartLeaveFlight(true);
	ForceNetUpdate();
	return true;
}

void AAGASSMadPlaneSessionEventActor::OnRep_MadPlanePhase()
{
	ReceiveMadPlanePhaseChanged(MadPlanePhase);

	if (!TargetTowerCenter.IsNearlyZero() || !TargetTowerTopLocation.IsNearlyZero())
	{
		ReceiveMadPlaneTargetResolved(TargetTowerCenter, TargetTowerTopLocation, TargetTowerHeightMeters);
	}
}

void AAGASSMadPlaneSessionEventActor::MulticastNotifyMadPlaneDiverted_Implementation(const FVector_NetQuantize SignalLocation)
{
	ReceiveMadPlaneDiverted(SignalLocation);
}

void AAGASSMadPlaneSessionEventActor::MulticastNotifyMadPlaneImpact_Implementation(
	const int32 CollapsedItemCount,
	const FVector_NetQuantize ImpactLocation)
{
	ReceiveMadPlaneImpact(CollapsedItemCount, ImpactLocation);
}

bool AAGASSMadPlaneSessionEventActor::ResolveTargetTower()
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FAGASSTowerStructureSnapshot Snapshot;
	if (!ResolveTallestEligibleTower(
		*World,
		FMath::Max(MinimumTargetTowerHeightMeters, 0.f) * 100.f,
		Snapshot))
	{
		UE_LOG(
			LogAGASSMadPlaneEvent,
			Warning,
			TEXT("MadPlane '%s' could not resolve a qualifying tower at activation time."),
			*GetNameSafe(this));
		return false;
	}

	TargetTowerItems = Snapshot.Items;
	TargetTowerCenter = Snapshot.TowerCenter;
	TargetTowerTopLocation = Snapshot.TowerTopLocation;
	TargetTowerHeightMeters = Snapshot.HeightCentimeters / 100.f;
	ReceiveMadPlaneTargetResolved(TargetTowerCenter, TargetTowerTopLocation, TargetTowerHeightMeters);
	ForceNetUpdate();
	return true;
}

bool AAGASSMadPlaneSessionEventActor::ResolveFlightPath()
{
	const FVector ImpactPoint = BuildImpactPoint();
	if (ImpactPoint.IsNearlyZero())
	{
		return false;
	}

	const float SpawnDistance = FMath::Max(FlightDistanceFromTower, 1.f);
	const float RandomYawDegrees = FMath::FRandRange(0.f, 360.f);
	const FVector TowerOutwardDirection = FRotator(0.f, RandomYawDegrees, 0.f).Vector().GetSafeNormal2D();
	if (TowerOutwardDirection.IsNearlyZero())
	{
		return false;
	}

	FVector ApproachLocation = ImpactPoint + (TowerOutwardDirection * SpawnDistance);
	ApproachLocation.Z = ImpactPoint.Z;
	ApproachStartTransform = FTransform(MakePlaneLookAtRotation(ApproachLocation, ImpactPoint), ApproachLocation);

	const float DivertYawOffset = FMath::RandBool() ? 90.f : -90.f;
	const FVector DivertDirection = TowerOutwardDirection.RotateAngleAxis(DivertYawOffset, FVector::UpVector).GetSafeNormal();
	FVector DivertExitLocation = ImpactPoint + (DivertDirection * SpawnDistance);
	DivertExitLocation.Z = ImpactPoint.Z;
	DivertExitTransform = FTransform(MakePlaneLookAtRotation(ImpactPoint, DivertExitLocation), DivertExitLocation);
	return true;
}

bool AAGASSMadPlaneSessionEventActor::SpawnAircraftActor()
{
	if (AircraftActor != nullptr)
	{
		AircraftActor->ApplyReplicatedPlaneTransform(ApproachStartTransform);
		return true;
	}

	UClass* ResolvedAircraftClass = ConfiguredAircraftActorClass.LoadSynchronous();
	if (ResolvedAircraftClass == nullptr)
	{
		ResolvedAircraftClass = AAGASSMadPlaneAircraftActor::StaticClass();
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AircraftActor = World->SpawnActor<AAGASSMadPlaneAircraftActor>(
		ResolvedAircraftClass,
		ApproachStartTransform,
		SpawnParameters);
	if (AircraftActor != nullptr)
	{
		AircraftActor->ApplyReplicatedPlaneTransform(ApproachStartTransform);
	}

	return AircraftActor != nullptr;
}

bool AAGASSMadPlaneSessionEventActor::IsControllerAtValidSignalPosition(const AController& UsingController) const
{
	const APawn* const UsingPawn = UsingController.GetPawn();
	if (UsingPawn == nullptr)
	{
		return false;
	}

	const FVector PawnLocation = UsingPawn->GetActorLocation();
	const FVector TargetTop = FVector(TargetTowerTopLocation);
	return FVector::DistSquared2D(PawnLocation, TargetTop) <= FMath::Square(FMath::Max(SignalHorizontalTolerance, 0.f))
		&& PawnLocation.Z >= (TargetTop.Z - FMath::Max(SignalVerticalToleranceBelowTop, 0.f));
}

FVector AAGASSMadPlaneSessionEventActor::BuildImpactPoint() const
{
	return !TargetTowerTopLocation.IsNearlyZero() ? FVector(TargetTowerTopLocation) : FVector(TargetTowerCenter);
}

FVector AAGASSMadPlaneSessionEventActor::BuildPassThroughExitPoint() const
{
	const FVector ImpactPoint = BuildImpactPoint();
	const FVector InboundDirection = (ImpactPoint - ApproachStartTransform.GetLocation()).GetSafeNormal2D();
	if (InboundDirection.IsNearlyZero())
	{
		return ImpactPoint;
	}

	FVector PassThroughExitLocation = ImpactPoint + (InboundDirection * FMath::Max(FlightDistanceFromTower, 1.f));
	PassThroughExitLocation.Z = ImpactPoint.Z;
	return PassThroughExitLocation;
}

void AAGASSMadPlaneSessionEventActor::ResolveAffectedItemsForImpact(TArray<AAGASSPlaceableItemActor*>& OutAffectedItems) const
{
	OutAffectedItems.Reset();

	FAGASSTowerStructureSnapshot Snapshot;
	Snapshot.Items = TargetTowerItems;
	Snapshot.TowerCenter = TargetTowerCenter;
	Snapshot.TowerTopLocation = TargetTowerTopLocation;

	TArray<TObjectPtr<AAGASSPlaceableItemActor>> AffectedItems;
	ResolveImpactSelection(Snapshot, BuildImpactPoint(), ImpactCollapseRadius, AffectedItems);

	for (AAGASSPlaceableItemActor* const AffectedItem : AffectedItems)
	{
		if (AffectedItem != nullptr)
		{
			OutAffectedItems.Add(AffectedItem);
		}
	}
}

void AAGASSMadPlaneSessionEventActor::PerformImpactCollapse()
{
	const FVector ImpactPoint = BuildImpactPoint();
	const FVector PlaneDirection = (ImpactPoint - ApproachStartTransform.GetLocation()).GetSafeNormal();
	TArray<AAGASSPlaceableItemActor*> AffectedItems;
	ResolveAffectedItemsForImpact(AffectedItems);

	int32 CollapsedItemCount = 0;
	for (AAGASSPlaceableItemActor* const Item : AffectedItems)
	{
		if (Item == nullptr || Item->IsHeldHidden() || !Item->IsPlacementCommitted())
		{
			continue;
		}

		FVector ImpulseDirection = (Item->GetActorLocation() - ImpactPoint).GetSafeNormal();
		if (ImpulseDirection.IsNearlyZero())
		{
			ImpulseDirection = PlaneDirection;
		}
		else
		{
			ImpulseDirection = (ImpulseDirection + (PlaneDirection * 0.35f)).GetSafeNormal();
		}

		ImpulseDirection.Z = FMath::Max(ImpulseDirection.Z, 0.15f);
		ImpulseDirection.Normalize();
		Item->CollapseForSessionEvent(ImpulseDirection * CollapseImpulseStrength);
		++CollapsedItemCount;
	}

	MulticastNotifyMadPlaneImpact(CollapsedItemCount, ImpactPoint);
	AdvancePhase(EAGASSMadPlaneEventPhase::Impacted);
}

void AAGASSMadPlaneSessionEventActor::AdvancePhase(const EAGASSMadPlaneEventPhase NewPhase)
{
	if (MadPlanePhase == NewPhase)
	{
		return;
	}

	MadPlanePhase = NewPhase;
	PhaseStartServerTimeSeconds = GetServerTimeSeconds();
	ReceiveMadPlanePhaseChanged(NewPhase);
	ForceNetUpdate();
}

void AAGASSMadPlaneSessionEventActor::StartFlightSegment(
	const EAGASSMadPlaneEventPhase NewPhase,
	const FVector& StartLocation,
	const FVector& EndLocation,
	const float DurationSeconds)
{
	const FRotator FlightRotation = MakePlaneLookAtRotation(StartLocation, EndLocation);
	FlightSegmentStartTransform = FTransform(FlightRotation, StartLocation);
	FlightSegmentEndTransform = FTransform(FlightRotation, EndLocation);
	CurrentFlightDurationSeconds = FMath::Max(DurationSeconds, 0.f);

	if (AircraftActor != nullptr)
	{
		AircraftActor->ApplyReplicatedPlaneTransform(FlightSegmentStartTransform);
	}

	AdvancePhase(NewPhase);
}

void AAGASSMadPlaneSessionEventActor::StartLeaveFlight(const bool bWasDiverted)
{
	const FVector StartLocation = AircraftActor != nullptr
		? AircraftActor->GetActorLocation()
		: BuildImpactPoint();
	StartFlightSegment(
		bWasDiverted ? EAGASSMadPlaneEventPhase::Diverted : EAGASSMadPlaneEventPhase::Leaving,
		StartLocation,
		bWasDiverted ? DivertExitTransform.GetLocation() : BuildPassThroughExitPoint(),
		RedirectDurationSeconds);
}

bool AAGASSMadPlaneSessionEventActor::UpdateFlightSegment() const
{
	if (AircraftActor == nullptr)
	{
		return true;
	}

	if (CurrentFlightDurationSeconds <= 0.f)
	{
		AircraftActor->ApplyReplicatedPlaneTransform(FlightSegmentEndTransform);
		return true;
	}

	const float Alpha = FMath::Clamp(
		(GetServerTimeSeconds() - PhaseStartServerTimeSeconds) / CurrentFlightDurationSeconds,
		0.f,
		1.f);

	FTransform BlendedTransform;
	BlendedTransform.Blend(FlightSegmentStartTransform, FlightSegmentEndTransform, Alpha);
	AircraftActor->ApplyReplicatedPlaneTransform(BlendedTransform);
	return Alpha >= 1.f;
}

float AAGASSMadPlaneSessionEventActor::GetServerTimeSeconds() const
{
	if (const AGameStateBase* const GameState = GetWorld() != nullptr ? GetWorld()->GetGameState() : nullptr)
	{
		return GameState->GetServerWorldTimeSeconds();
	}

	return GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
}

bool AAGASSMadPlaneSessionEventActor::HasPhaseElapsed(const float DurationSeconds) const
{
	return DurationSeconds <= 0.f || (GetServerTimeSeconds() - PhaseStartServerTimeSeconds) >= DurationSeconds;
}
