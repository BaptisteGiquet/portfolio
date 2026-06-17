#include "Actors/AGASSMadCopsSessionEventActor.h"

#include "AIController.h"
#include "Actors/AGASSMadCopsCarActor.h"
#include "Actors/AGASSMadCopsOfficerCharacter.h"
#include "Actors/AGASSMadCopsRouteAnchorActor.h"
#include "Actors/AGASSPlaceableItemActor.h"
#include "Components/AGASSSessionEventManagerComponent.h"
#include "Data/AGASSMadCopsSessionEventDefinition.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Interfaces/AGASSSharedTeamMoneyInterface.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSMadCopsEvent, Log, All);

namespace
{
FRotator MakeLookAtRotation(const FVector& From, const FVector& To)
{
	const FVector Direction = (To - From).GetSafeNormal2D();
	return Direction.IsNearlyZero() ? FRotator::ZeroRotator : Direction.Rotation();
}
}

AAGASSMadCopsSessionEventActor::AAGASSMadCopsSessionEventActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.SetTickFunctionEnable(false);

	ConfiguredOfficerCharacterClass = AAGASSMadCopsOfficerCharacter::StaticClass();
	ConfiguredPoliceCarActorClass = AAGASSMadCopsCarActor::StaticClass();
}

void AAGASSMadCopsSessionEventActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	switch (MadCopsPhase)
	{
	case EAGASSMadCopsEventPhase::SirenLeadIn:
		if (HasPhaseElapsed(SirenLeadInSeconds))
		{
			AdvancePhase(EAGASSMadCopsEventPhase::CarArriving);
		}
		break;

	case EAGASSMadCopsEventPhase::CarArriving:
		if (PoliceCarActor == nullptr)
		{
			RequestFinishEvent();
			return;
		}

		if (CarArrivalDurationSeconds <= 0.f)
		{
			PoliceCarActor->ApplyReplicatedCarTransform(RouteParkingTransform);
		}
		else
		{
			const float Alpha = FMath::Clamp((GetServerTimeSeconds() - PhaseStartServerTimeSeconds) / CarArrivalDurationSeconds, 0.f, 1.f);
			UpdateCarTransform(RouteEntryTransform, RouteParkingTransform, Alpha);
			if (Alpha < 1.f)
			{
				break;
			}
		}

		if (!SpawnOfficerCharacter())
		{
			BeginLeaveFlow();
			return;
		}

		BeginOfficerScan();
		break;

	case EAGASSMadCopsEventPhase::OfficerScanning:
		if (bDrawDebugTowerDetection)
		{
			DrawDebugScanVisualization();
		}

		if (OfficerCharacter == nullptr)
		{
			BeginLeaveFlow();
			return;
		}

		if (!bReachedScanLocation)
		{
			if (!bOfficerMoveActive && !BeginOfficerMoveTo(RouteScanTransform.GetLocation(), OfficerNavigationAcceptanceRadius))
			{
				BeginLeaveFlow();
				return;
			}

			if (HasOfficerReachedMoveGoal())
			{
				ClearOfficerMoveGoal();
				bReachedScanLocation = true;
				UE_LOG(
					LogAGASSMadCopsEvent,
					Log,
					TEXT("Mad Cops '%s' reached scan location. OfficerLocation=%s Goal=%s"),
					*GetNameSafe(this),
					OfficerCharacter != nullptr ? *OfficerCharacter->GetActorLocation().ToCompactString() : TEXT("<none>"),
					*RouteScanTransform.GetLocation().ToCompactString());
				PhaseStartServerTimeSeconds = GetServerTimeSeconds();
			}
			else if ((GetServerTimeSeconds() - OfficerMoveStartServerTimeSeconds) > OfficerMoveTimeoutSeconds)
			{
				const FVector OfficerLocation = OfficerCharacter != nullptr ? OfficerCharacter->GetActorLocation() : FVector::ZeroVector;
				const float DistanceToGoal2D = FVector::Dist2D(OfficerLocation, CurrentOfficerMoveGoalLocation);
				const AAIController* const AIController = OfficerCharacter != nullptr ? Cast<AAIController>(OfficerCharacter->GetController()) : nullptr;
				UE_LOG(
					LogAGASSMadCopsEvent,
					Warning,
					TEXT("Mad Cops '%s' timed out moving to scan point. OfficerLocation=%s Goal=%s Distance2D=%.1f MoveStatus=%d"),
					*GetNameSafe(this),
					*OfficerLocation.ToCompactString(),
					*CurrentOfficerMoveGoalLocation.ToCompactString(),
					DistanceToGoal2D,
					AIController != nullptr ? static_cast<int32>(AIController->GetMoveStatus()) : -1);
				BeginLeaveFlow();
			}

			break;
		}

		if (!HasPhaseElapsed(ScanDurationSeconds))
		{
			break;
		}

		{
			FAGASSMadCopsTowerTarget NextTowerTarget;
			if (ResolveNextTowerTarget(NextTowerTarget))
			{
				BeginMoveToTower(NextTowerTarget);
			}
			else
			{
				BeginLeaveFlow();
			}
		}
		break;

	case EAGASSMadCopsEventPhase::OfficerMovingToStack:
		if (OfficerCharacter == nullptr)
		{
			BeginLeaveFlow();
			return;
		}

		if (HasOfficerReachedMoveGoal())
		{
			ClearOfficerMoveGoal();
			AdvancePhase(EAGASSMadCopsEventPhase::OfficerDestroyingStack);
		}
		else if ((GetServerTimeSeconds() - OfficerMoveStartServerTimeSeconds) > OfficerMoveTimeoutSeconds)
		{
			BeginLeaveFlow();
		}
		break;

	case EAGASSMadCopsEventPhase::OfficerDestroyingStack:
		if (!HasPhaseElapsed(DestroyPauseSeconds))
		{
			break;
		}

		DestroyActiveTower();
		if (bBribeAccepted)
		{
			BeginLeaveFlow();
			return;
		}

		{
			FAGASSMadCopsTowerTarget NextTowerTarget;
			if (ResolveNextTowerTarget(NextTowerTarget))
			{
				BeginMoveToTower(NextTowerTarget);
			}
			else
			{
				BeginLeaveFlow();
			}
		}
		break;

	case EAGASSMadCopsEventPhase::OfficerReturningToCar:
		if (!HasPhaseElapsed(ReturnToCarDelaySeconds))
		{
			break;
		}

		if (OfficerCharacter == nullptr)
		{
			AdvancePhase(EAGASSMadCopsEventPhase::Leaving);
			break;
		}

		if (!bOfficerMoveActive && !BeginOfficerMoveTo(RouteParkingTransform.GetLocation(), OfficerNavigationAcceptanceRadius))
		{
			OfficerCharacter->Destroy();
			OfficerCharacter = nullptr;
			AdvancePhase(EAGASSMadCopsEventPhase::Leaving);
			break;
		}

		if (HasOfficerReachedMoveGoal() || (GetServerTimeSeconds() - OfficerMoveStartServerTimeSeconds) > OfficerMoveTimeoutSeconds)
		{
			ClearOfficerMoveGoal();
			OfficerCharacter->Destroy();
			OfficerCharacter = nullptr;
			AdvancePhase(EAGASSMadCopsEventPhase::Leaving);
		}
		break;

	case EAGASSMadCopsEventPhase::Leaving:
		if (PoliceCarActor == nullptr)
		{
			RequestFinishEvent();
			return;
		}

		if (CarDepartureDurationSeconds <= 0.f)
		{
			PoliceCarActor->ApplyReplicatedCarTransform(RouteExitTransform);
		}
		else
		{
			const float Alpha = FMath::Clamp((GetServerTimeSeconds() - PhaseStartServerTimeSeconds) / CarDepartureDurationSeconds, 0.f, 1.f);
			UpdateCarTransform(RouteParkingTransform, RouteExitTransform, Alpha);
			if (Alpha < 1.f)
			{
				break;
			}
		}

		if (PoliceCarActor != nullptr)
		{
			PoliceCarActor->Destroy();
			PoliceCarActor = nullptr;
		}

		AdvancePhase(EAGASSMadCopsEventPhase::Finished);
		RequestFinishEvent();
		break;

	default:
		break;
	}
}

void AAGASSMadCopsSessionEventActor::HandleEventActivated_Implementation()
{
	Super::HandleEventActivated_Implementation();

	bBribeAccepted = false;
	OfficerCharacter = nullptr;
	PoliceCarActor = nullptr;
	ActiveTowerTarget.Items.Reset();
	ClearOfficerMoveGoal();
	bReachedScanLocation = false;

	if (AAGASSMadCopsRouteAnchorActor* const RouteAnchor = ResolvePrimaryRouteAnchor())
	{
		RouteEntryTransform = RouteAnchor->GetCarApproachStartTransform();
		RouteParkingTransform = RouteAnchor->GetCarParkingTransform();
		RouteScanTransform = RouteAnchor->GetOfficerScanTransform();
		RouteExitTransform = RouteAnchor->GetCarDepartureTransform();

		UE_LOG(
			LogAGASSMadCopsEvent,
			Log,
			TEXT("Activating Mad Cops '%s' with route anchor '%s'. Entry=%s Parking=%s Scan=%s Exit=%s ScanRadius=%.1f"),
			*GetNameSafe(this),
			*GetNameSafe(RouteAnchor),
			*RouteEntryTransform.GetLocation().ToCompactString(),
			*RouteParkingTransform.GetLocation().ToCompactString(),
			*RouteScanTransform.GetLocation().ToCompactString(),
			*RouteExitTransform.GetLocation().ToCompactString(),
			ScanRadius);
	}
	else
	{
		UE_LOG(
			LogAGASSMadCopsEvent,
			Warning,
			TEXT("Mad Cops event '%s' could not activate because no AGASSMadCopsRouteAnchorActor exists in world '%s'."),
			*GetNameSafe(this),
			*GetNameSafe(GetWorld()));
		RequestFinishEvent();
		return;
	}

	CurrentBribeCost = ComputeBribeCost();
	UE_LOG(
		LogAGASSMadCopsEvent,
		Log,
		TEXT("Mad Cops '%s' computed bribe cost=%d."),
		*GetNameSafe(this),
		CurrentBribeCost);

	if (!SpawnPoliceCar())
	{
		RequestFinishEvent();
		return;
	}

	SetActorTickEnabled(true);
	AdvancePhase(EAGASSMadCopsEventPhase::SirenLeadIn);
}

void AAGASSMadCopsSessionEventActor::HandleEventEnded_Implementation()
{
	SetActorTickEnabled(false);
	ClearOfficerMoveGoal();
	ActiveTowerTarget.Items.Reset();

	if (OfficerCharacter != nullptr)
	{
		if (AAIController* const AIController = Cast<AAIController>(OfficerCharacter->GetController()))
		{
			AIController->StopMovement();
		}

		OfficerCharacter->Destroy();
		OfficerCharacter = nullptr;
	}

	if (PoliceCarActor != nullptr)
	{
		PoliceCarActor->Destroy();
		PoliceCarActor = nullptr;
	}

	Super::HandleEventEnded_Implementation();
}

void AAGASSMadCopsSessionEventActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MadCopsPhase);
	DOREPLIFETIME(ThisClass, CurrentBribeCost);
	DOREPLIFETIME(ThisClass, bBribeAccepted);
	DOREPLIFETIME(ThisClass, OfficerCharacter);
	DOREPLIFETIME(ThisClass, PoliceCarActor);
}

void AAGASSMadCopsSessionEventActor::ApplyDefinitionSettings(const UAGASSMadCopsSessionEventDefinition& Definition)
{
	ConfiguredOfficerCharacterClass = Definition.OfficerCharacterClass;
	ConfiguredPoliceCarActorClass = Definition.PoliceCarActorClass;
	BaseBribeCost = FMath::Max(Definition.BaseBribeCost, 0);
	BribeCostIncreasePerActivation = FMath::Max(Definition.BribeCostIncreasePerActivation, 0);
	ScanRadius = FMath::Max(Definition.ScanRadius, 0.f);
	OfficerTowerStandOffDistance = FMath::Max(Definition.OfficerTowerStandOffDistance, 0.f);
	OfficerNavigationAcceptanceRadius = FMath::Max(Definition.OfficerNavigationAcceptanceRadius, 0.f);
	OfficerMoveTimeoutSeconds = FMath::Max(Definition.OfficerMoveTimeoutSeconds, 0.f);
	CollapseImpulseStrength = FMath::Max(Definition.CollapseImpulseStrength, 0.f);
	SirenLeadInSeconds = FMath::Max(Definition.SirenLeadInSeconds, 0.f);
	CarArrivalDurationSeconds = FMath::Max(Definition.CarArrivalDurationSeconds, 0.f);
	ScanDurationSeconds = FMath::Max(Definition.ScanDurationSeconds, 0.f);
	DestroyPauseSeconds = FMath::Max(Definition.DestroyPauseSeconds, 0.f);
	ReturnToCarDelaySeconds = FMath::Max(Definition.ReturnToCarDelaySeconds, 0.f);
	CarDepartureDurationSeconds = FMath::Max(Definition.CarDepartureDurationSeconds, 0.f);
	OfficerExitLocalOffset = Definition.OfficerExitLocalOffset;
	GroundProjectionTraceHeight = FMath::Max(Definition.GroundProjectionTraceHeight, 0.f);
	bDrawDebugTowerDetection = Definition.bDrawDebugTowerDetection;
}

EAGASSMadCopsEventPhase AAGASSMadCopsSessionEventActor::GetMadCopsPhase() const
{
	return MadCopsPhase;
}

int32 AAGASSMadCopsSessionEventActor::GetCurrentBribeCost() const
{
	return CurrentBribeCost;
}

bool AAGASSMadCopsSessionEventActor::CanAcceptBribe() const
{
	return !bBribeAccepted
		&& (MadCopsPhase == EAGASSMadCopsEventPhase::OfficerScanning || MadCopsPhase == EAGASSMadCopsEventPhase::OfficerMovingToStack);
}

bool AAGASSMadCopsSessionEventActor::TryAcceptBribe(AController* InteractingController)
{
	if (!HasAuthority() || !CanAcceptBribe())
	{
		return false;
	}

	const TScriptInterface<IAGASSSharedTeamMoneyInterface> SharedMoneyProvider = ResolveSharedMoneyProvider();
	UObject* const SharedMoneyObject = SharedMoneyProvider.GetObject();
	if (SharedMoneyObject == nullptr
		|| !IAGASSSharedTeamMoneyInterface::Execute_TrySpendAGASSSharedMoney(SharedMoneyObject, CurrentBribeCost))
	{
		UE_LOG(
			LogAGASSMadCopsEvent,
			Log,
			TEXT("Mad Cops '%s' rejected bribe attempt from '%s'. Shared provider valid=%d Cost=%d"),
			*GetNameSafe(this),
			*GetNameSafe(InteractingController),
			SharedMoneyObject != nullptr ? 1 : 0,
			CurrentBribeCost);
		return false;
	}

	UE_LOG(
		LogAGASSMadCopsEvent,
		Log,
		TEXT("Mad Cops '%s' accepted bribe from '%s' for %d."),
		*GetNameSafe(this),
		*GetNameSafe(InteractingController),
		CurrentBribeCost);

	bBribeAccepted = true;
	MulticastNotifyBribeAccepted(CurrentBribeCost);
	BeginLeaveFlow();
	ForceNetUpdate();
	return true;
}

void AAGASSMadCopsSessionEventActor::OnRep_MadCopsPhase()
{
	ReceiveMadCopsPhaseChanged(MadCopsPhase);
}

void AAGASSMadCopsSessionEventActor::MulticastNotifyBribeAccepted_Implementation(const int32 PaidAmount)
{
	ReceiveMadCopsBribeAccepted(PaidAmount);
}

void AAGASSMadCopsSessionEventActor::MulticastNotifyStackDestroyed_Implementation(
	const int32 DestroyedItemCount,
	const FVector_NetQuantize ClusterCenter)
{
	ReceiveMadCopsStackDestroyed(DestroyedItemCount, ClusterCenter);
}

bool AAGASSMadCopsSessionEventActor::TryProjectLocationToGround(FVector& InOutLocation) const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSMadCopsGroundProjection), false);
	QueryParams.AddIgnoredActor(this);

	for (TActorIterator<AAGASSPlaceableItemActor> It(World); It; ++It)
	{
		QueryParams.AddIgnoredActor(*It);
	}

	const FVector TraceStart = InOutLocation + FVector(0.f, 0.f, GroundProjectionTraceHeight);
	const FVector TraceEnd = InOutLocation - FVector(0.f, 0.f, GroundProjectionTraceHeight);

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams) || !HitResult.bBlockingHit)
	{
		return false;
	}

	InOutLocation = HitResult.ImpactPoint;
	return true;
}

AAGASSMadCopsRouteAnchorActor* AAGASSMadCopsSessionEventActor::ResolvePrimaryRouteAnchor() const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	AAGASSMadCopsRouteAnchorActor* PrimaryAnchor = nullptr;
	int32 AnchorCount = 0;

	for (TActorIterator<AAGASSMadCopsRouteAnchorActor> It(World); It; ++It)
	{
		AAGASSMadCopsRouteAnchorActor* const Candidate = *It;
		if (Candidate == nullptr)
		{
			continue;
		}

		++AnchorCount;
		if (PrimaryAnchor == nullptr)
		{
			PrimaryAnchor = Candidate;
		}
	}

	if (AnchorCount > 1)
	{
		UE_LOG(
			LogAGASSMadCopsEvent,
			Warning,
			TEXT("Mad Cops found %d route anchors in world '%s'. Using the first discovered anchor '%s'."),
			AnchorCount,
			*GetNameSafe(World),
			*GetNameSafe(PrimaryAnchor));
	}

	return PrimaryAnchor;
}

bool AAGASSMadCopsSessionEventActor::SpawnPoliceCar()
{
	if (PoliceCarActor != nullptr)
	{
		PoliceCarActor->ApplyReplicatedCarTransform(RouteEntryTransform);
		return true;
	}

	UClass* ResolvedCarClass = ConfiguredPoliceCarActorClass.LoadSynchronous();
	if (ResolvedCarClass == nullptr)
	{
		ResolvedCarClass = AAGASSMadCopsCarActor::StaticClass();
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PoliceCarActor = World->SpawnActor<AAGASSMadCopsCarActor>(ResolvedCarClass, RouteEntryTransform, SpawnParameters);
	return PoliceCarActor != nullptr;
}

bool AAGASSMadCopsSessionEventActor::SpawnOfficerCharacter()
{
	if (OfficerCharacter != nullptr)
	{
		return true;
	}

	UClass* ResolvedOfficerClass = ConfiguredOfficerCharacterClass.LoadSynchronous();
	if (ResolvedOfficerClass == nullptr)
	{
		ResolvedOfficerClass = AAGASSMadCopsOfficerCharacter::StaticClass();
	}

	FVector OfficerSpawnLocation = RouteParkingTransform.TransformPosition(OfficerExitLocalOffset);
	TryProjectLocationToGround(OfficerSpawnLocation);

	const FTransform OfficerSpawnTransform(
		MakeLookAtRotation(OfficerSpawnLocation, RouteScanTransform.GetLocation()),
		OfficerSpawnLocation);

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = nullptr;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	OfficerCharacter = World->SpawnActor<AAGASSMadCopsOfficerCharacter>(ResolvedOfficerClass, OfficerSpawnTransform, SpawnParameters);
	if (OfficerCharacter == nullptr)
	{
		return false;
	}

	OfficerCharacter->SetOwningMadCopsEvent(this);
	OfficerCharacter->SpawnDefaultController();
	return true;
}

bool AAGASSMadCopsSessionEventActor::BeginOfficerMoveTo(const FVector& GoalLocation, const float AcceptanceRadius)
{
	if (OfficerCharacter == nullptr)
	{
		return false;
	}

	CurrentOfficerMoveGoalLocation = GoalLocation;
	CurrentOfficerMoveAcceptanceRadius = FMath::Max(AcceptanceRadius, 0.f);
	OfficerMoveStartServerTimeSeconds = GetServerTimeSeconds();
	bOfficerMoveActive = true;

	if (HasOfficerReachedMoveGoal())
	{
		return true;
	}

	if (OfficerCharacter->GetController() == nullptr)
	{
		OfficerCharacter->SpawnDefaultController();
	}

	AAIController* const AIController = Cast<AAIController>(OfficerCharacter->GetController());
	if (AIController == nullptr)
	{
		return false;
	}

	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		GoalLocation,
		CurrentOfficerMoveAcceptanceRadius,
		true,
		true,
		true,
		true,
		nullptr,
		true);

	UE_LOG(
		LogAGASSMadCopsEvent,
		Log,
		TEXT("Officer move request for '%s'. Goal=%s AcceptanceRadius=%.1f Result=%d"),
		*GetNameSafe(this),
		*GoalLocation.ToCompactString(),
		CurrentOfficerMoveAcceptanceRadius,
		static_cast<int32>(MoveResult));

	return MoveResult != EPathFollowingRequestResult::Failed;
}

bool AAGASSMadCopsSessionEventActor::HasOfficerReachedMoveGoal() const
{
	if (!bOfficerMoveActive || OfficerCharacter == nullptr)
	{
		return false;
	}

	float EffectiveAcceptanceRadius = FMath::Max(CurrentOfficerMoveAcceptanceRadius, 0.f);
	EffectiveAcceptanceRadius += OfficerCharacter->GetSimpleCollisionRadius();

	const float DistanceSquared2D = FVector::DistSquared2D(OfficerCharacter->GetActorLocation(), CurrentOfficerMoveGoalLocation);
	if (DistanceSquared2D <= FMath::Square(EffectiveAcceptanceRadius))
	{
		return true;
	}

	const AAIController* const AIController = Cast<AAIController>(OfficerCharacter->GetController());
	return AIController != nullptr
		&& AIController->GetMoveStatus() == EPathFollowingStatus::Idle
		&& DistanceSquared2D <= FMath::Square(EffectiveAcceptanceRadius + 150.f);
}

void AAGASSMadCopsSessionEventActor::ClearOfficerMoveGoal()
{
	bOfficerMoveActive = false;
	CurrentOfficerMoveGoalLocation = FVector::ZeroVector;
	CurrentOfficerMoveAcceptanceRadius = 0.f;
	OfficerMoveStartServerTimeSeconds = 0.f;
}

bool AAGASSMadCopsSessionEventActor::IsCandidatePlacedTowerItem(const AAGASSPlaceableItemActor* Candidate) const
{
	return Candidate != nullptr
		&& !Candidate->IsHeldHidden()
		&& Candidate->IsPlacementCommitted();
}

bool AAGASSMadCopsSessionEventActor::IsCandidateVisibleFromScan(const AAGASSPlaceableItemActor* Candidate, const FVector& ScanOrigin) const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr || Candidate == nullptr)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSMadCopsVisibility), false);
	QueryParams.AddIgnoredActor(this);

	if (OfficerCharacter != nullptr)
	{
		QueryParams.AddIgnoredActor(OfficerCharacter);
	}

	if (PoliceCarActor != nullptr)
	{
		QueryParams.AddIgnoredActor(PoliceCarActor);
	}

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, ScanOrigin, Candidate->GetActorLocation(), ECC_Visibility, QueryParams))
	{
		return true;
	}

	return HitResult.GetActor() == Candidate;
}

AAGASSPlaceableItemActor* AAGASSMadCopsSessionEventActor::ResolveTowerRoot(AAGASSPlaceableItemActor* Candidate) const
{
	AAGASSPlaceableItemActor* Current = Candidate;
	TSet<const AAGASSPlaceableItemActor*> VisitedItems;

	while (Current != nullptr && IsCandidatePlacedTowerItem(Current) && !VisitedItems.Contains(Current))
	{
		VisitedItems.Add(Current);

		AAGASSPlaceableItemActor* const SupportingPlaceable = Current->GetSupportingTowerPlaceable();
		if (!IsCandidatePlacedTowerItem(SupportingPlaceable))
		{
			break;
		}

		Current = SupportingPlaceable;
	}

	return Current;
}

bool AAGASSMadCopsSessionEventActor::BuildTowerTargetFromRoot(
	AAGASSPlaceableItemActor* TowerRoot,
	const FVector& ScanOrigin,
	const float ScanRadiusSquared,
	FAGASSMadCopsTowerTarget& OutTowerTarget) const
{
	OutTowerTarget = FAGASSMadCopsTowerTarget();
	if (!IsCandidatePlacedTowerItem(TowerRoot))
	{
		return false;
	}

	const FVector ScanCenter = RouteScanTransform.GetLocation();
	TArray<AAGASSPlaceableItemActor*> PendingItems;
	TSet<const AAGASSPlaceableItemActor*> VisitedItems;
	PendingItems.Add(TowerRoot);

	FVector LocationAccumulator = FVector::ZeroVector;
	float BestVisibleDistanceSquared = TNumericLimits<float>::Max();
	bool bHasVisibleTowerItemInScan = false;

	while (!PendingItems.IsEmpty())
	{
		AAGASSPlaceableItemActor* const CurrentItem = PendingItems.Pop(EAllowShrinking::No);
		if (CurrentItem == nullptr || VisitedItems.Contains(CurrentItem) || !IsCandidatePlacedTowerItem(CurrentItem))
		{
			continue;
		}

		VisitedItems.Add(CurrentItem);
		OutTowerTarget.Items.Add(CurrentItem);
		LocationAccumulator += CurrentItem->GetActorLocation();

		if (FVector::DistSquared(CurrentItem->GetActorLocation(), ScanCenter) <= ScanRadiusSquared
			&& IsCandidateVisibleFromScan(CurrentItem, ScanOrigin))
		{
			bHasVisibleTowerItemInScan = true;
			BestVisibleDistanceSquared = FMath::Min(
				BestVisibleDistanceSquared,
				FVector::DistSquared2D(ScanOrigin, CurrentItem->GetActorLocation()));
		}

		if (AAGASSPlaceableItemActor* const SupportingPlaceable = CurrentItem->GetSupportingTowerPlaceable())
		{
			PendingItems.Add(SupportingPlaceable);
		}

		TArray<AAGASSPlaceableItemActor*> SupportedPlaceables;
		CurrentItem->GetSupportedTowerPlaceables(SupportedPlaceables);
		PendingItems.Append(SupportedPlaceables);
	}

	if (OutTowerTarget.Items.Num() < 2 || !bHasVisibleTowerItemInScan)
	{
		UE_LOG(
			LogAGASSMadCopsEvent,
			Log,
			TEXT("Rejected tower root '%s'. ItemCount=%d VisibleInScan=%d"),
			*GetNameSafe(TowerRoot),
			OutTowerTarget.Items.Num(),
			bHasVisibleTowerItemInScan ? 1 : 0);
		OutTowerTarget.Items.Reset();
		return false;
	}

	OutTowerTarget.TowerCenter = LocationAccumulator / static_cast<float>(OutTowerTarget.Items.Num());
	OutTowerTarget.DistanceSqToOfficer = BestVisibleDistanceSquared;
	UE_LOG(
		LogAGASSMadCopsEvent,
		Log,
		TEXT("Built tower target from root '%s'. ItemCount=%d Center=%s BestVisibleDistanceSq=%.1f"),
		*GetNameSafe(TowerRoot),
		OutTowerTarget.Items.Num(),
		*OutTowerTarget.TowerCenter.ToCompactString(),
		OutTowerTarget.DistanceSqToOfficer);
	return true;
}

bool AAGASSMadCopsSessionEventActor::ResolveNextTowerTarget(FAGASSMadCopsTowerTarget& OutTowerTarget) const
{
	OutTowerTarget = FAGASSMadCopsTowerTarget();

	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const float ScanRadiusSquared = FMath::Square(FMath::Max(ScanRadius, 0.f));
	const FVector ScanCenter = RouteScanTransform.GetLocation();
	const FVector ScanOrigin = OfficerCharacter != nullptr ? OfficerCharacter->GetActorLocation() : ScanCenter;
	TSet<const AAGASSPlaceableItemActor*> ProcessedRoots;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	int32 CandidateCountInRadius = 0;
	int32 CandidateCountInStacks = 0;
	int32 CandidateCountVisible = 0;

	UE_LOG(
		LogAGASSMadCopsEvent,
		Log,
		TEXT("Resolving next tower target for '%s'. ScanCenter=%s ScanOrigin=%s ScanRadius=%.1f"),
		*GetNameSafe(this),
		*ScanCenter.ToCompactString(),
		*ScanOrigin.ToCompactString(),
		ScanRadius);

	for (TActorIterator<AAGASSPlaceableItemActor> It(World); It; ++It)
	{
		AAGASSPlaceableItemActor* const Candidate = *It;
		if (!IsCandidatePlacedTowerItem(Candidate))
		{
			continue;
		}

		if (FVector::DistSquared(Candidate->GetActorLocation(), ScanCenter) > ScanRadiusSquared)
		{
			continue;
		}

		++CandidateCountInRadius;

		TArray<AAGASSPlaceableItemActor*> SupportedPlaceables;
		Candidate->GetSupportedTowerPlaceables(SupportedPlaceables);
		const bool bIsInTowerStack = Candidate->IsPartOfTowerStack();
		const bool bIsVisible = IsCandidateVisibleFromScan(Candidate, ScanOrigin);

		UE_LOG(
			LogAGASSMadCopsEvent,
			Log,
			TEXT("Scan candidate '%s'. Location=%s InTowerStack=%d Visible=%d Supporting='%s' SupportedChildren=%d"),
			*GetNameSafe(Candidate),
			*Candidate->GetActorLocation().ToCompactString(),
			bIsInTowerStack ? 1 : 0,
			bIsVisible ? 1 : 0,
			*GetNameSafe(Candidate->GetSupportingTowerPlaceable()),
			SupportedPlaceables.Num());

		if (!bIsInTowerStack)
		{
			continue;
		}

		++CandidateCountInStacks;

		if (!bIsVisible)
		{
			continue;
		}

		++CandidateCountVisible;

		AAGASSPlaceableItemActor* const TowerRoot = ResolveTowerRoot(Candidate);
		if (TowerRoot == nullptr || ProcessedRoots.Contains(TowerRoot))
		{
			UE_LOG(
				LogAGASSMadCopsEvent,
				Log,
				TEXT("Skipping candidate '%s'. TowerRoot='%s' AlreadyProcessed=%d"),
				*GetNameSafe(Candidate),
				*GetNameSafe(TowerRoot),
				TowerRoot != nullptr && ProcessedRoots.Contains(TowerRoot) ? 1 : 0);
			continue;
		}

		ProcessedRoots.Add(TowerRoot);

		FAGASSMadCopsTowerTarget CandidateTowerTarget;
		if (!BuildTowerTargetFromRoot(TowerRoot, ScanOrigin, ScanRadiusSquared, CandidateTowerTarget))
		{
			continue;
		}

		if (CandidateTowerTarget.DistanceSqToOfficer < BestDistanceSquared)
		{
			BestDistanceSquared = CandidateTowerTarget.DistanceSqToOfficer;
			OutTowerTarget = CandidateTowerTarget;
		}
	}

	if (bDrawDebugTowerDetection && !OutTowerTarget.Items.IsEmpty())
	{
		DrawDebugTowerTarget(OutTowerTarget);
	}

	UE_LOG(
		LogAGASSMadCopsEvent,
		Log,
		TEXT("ResolveNextTowerTarget summary for '%s'. CandidatesInRadius=%d CandidatesInStacks=%d VisibleCandidates=%d ChosenItemCount=%d"),
		*GetNameSafe(this),
		CandidateCountInRadius,
		CandidateCountInStacks,
		CandidateCountVisible,
		OutTowerTarget.Items.Num());

	return !OutTowerTarget.Items.IsEmpty();
}

FVector AAGASSMadCopsSessionEventActor::ResolveOfficerApproachLocation(const FAGASSMadCopsTowerTarget& TowerTarget) const
{
	const FVector ReferenceLocation = OfficerCharacter != nullptr ? OfficerCharacter->GetActorLocation() : RouteScanTransform.GetLocation();
	FVector ApproachDirection = (TowerTarget.TowerCenter - ReferenceLocation).GetSafeNormal2D();
	if (ApproachDirection.IsNearlyZero())
	{
		ApproachDirection = (TowerTarget.TowerCenter - RouteScanTransform.GetLocation()).GetSafeNormal2D();
	}
	if (ApproachDirection.IsNearlyZero())
	{
		ApproachDirection = FVector::ForwardVector;
	}

	FVector ApproachLocation = TowerTarget.TowerCenter - (ApproachDirection * OfficerTowerStandOffDistance);
	TryProjectLocationToGround(ApproachLocation);
	return ApproachLocation;
}

void AAGASSMadCopsSessionEventActor::DrawDebugScanVisualization() const
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const FVector ScanCenter = RouteScanTransform.GetLocation();
	DrawDebugSphere(World, ScanCenter, ScanRadius, 32, FColor::Cyan, false, 0.f, 0, 2.f);
	DrawDebugSphere(World, ScanCenter, 45.f, 12, FColor::Blue, false, 0.f, 0, 2.5f);
}

void AAGASSMadCopsSessionEventActor::DrawDebugTowerTarget(const FAGASSMadCopsTowerTarget& TowerTarget) const
{
	UWorld* const World = GetWorld();
	if (World == nullptr || TowerTarget.Items.IsEmpty())
	{
		return;
	}

	for (const AAGASSPlaceableItemActor* const Item : TowerTarget.Items)
	{
		if (Item == nullptr)
		{
			continue;
		}

		DrawDebugSphere(World, Item->GetActorLocation(), 45.f, 12, FColor::Yellow, false, 5.f, 0, 2.f);
	}

	DrawDebugSphere(World, TowerTarget.TowerCenter, 65.f, 16, FColor::Orange, false, 5.f, 0, 2.5f);
	DrawDebugLine(World, RouteScanTransform.GetLocation(), TowerTarget.TowerCenter, FColor::Green, false, 5.f, 0, 2.f);
}

void AAGASSMadCopsSessionEventActor::AdvancePhase(const EAGASSMadCopsEventPhase NewPhase)
{
	if (MadCopsPhase == NewPhase)
	{
		return;
	}

	MadCopsPhase = NewPhase;
	PhaseStartServerTimeSeconds = GetServerTimeSeconds();
	UE_LOG(
		LogAGASSMadCopsEvent,
		Log,
		TEXT("Mad Cops '%s' advanced to phase %s."),
		*GetNameSafe(this),
		*StaticEnum<EAGASSMadCopsEventPhase>()->GetValueAsString(NewPhase));
	ReceiveMadCopsPhaseChanged(NewPhase);
	ForceNetUpdate();
}

void AAGASSMadCopsSessionEventActor::BeginOfficerScan()
{
	ClearOfficerMoveGoal();
	bReachedScanLocation = false;
	UE_LOG(
		LogAGASSMadCopsEvent,
		Log,
		TEXT("Mad Cops '%s' beginning scan. ScanLocation=%s ScanDuration=%.2f"),
		*GetNameSafe(this),
		*RouteScanTransform.GetLocation().ToCompactString(),
		ScanDurationSeconds);
	AdvancePhase(EAGASSMadCopsEventPhase::OfficerScanning);
}

void AAGASSMadCopsSessionEventActor::BeginMoveToTower(const FAGASSMadCopsTowerTarget& NewTowerTarget)
{
	ActiveTowerTarget = NewTowerTarget;
	ClearOfficerMoveGoal();

	if (OfficerCharacter != nullptr)
	{
		if (AAIController* const AIController = Cast<AAIController>(OfficerCharacter->GetController()))
		{
			AIController->StopMovement();
		}
	}

	AdvancePhase(EAGASSMadCopsEventPhase::OfficerMovingToStack);
	const FVector ApproachLocation = ResolveOfficerApproachLocation(NewTowerTarget);
	if (!BeginOfficerMoveTo(ApproachLocation, OfficerNavigationAcceptanceRadius))
	{
		BeginLeaveFlow();
	}
}

void AAGASSMadCopsSessionEventActor::DestroyActiveTower()
{
	if (ActiveTowerTarget.Items.IsEmpty())
	{
		return;
	}

	const FVector OfficerLocation = OfficerCharacter != nullptr ? OfficerCharacter->GetActorLocation() : ActiveTowerTarget.TowerCenter;
	int32 DestroyedItemCount = 0;

	for (AAGASSPlaceableItemActor* const Item : ActiveTowerTarget.Items)
	{
		if (!IsCandidatePlacedTowerItem(Item))
		{
			continue;
		}

		FVector ImpulseDirection = (Item->GetActorLocation() - OfficerLocation).GetSafeNormal();
		if (ImpulseDirection.IsNearlyZero())
		{
			ImpulseDirection = FVector::UpVector;
		}

		ImpulseDirection.Z = FMath::Max(ImpulseDirection.Z, 0.2f);
		ImpulseDirection.Normalize();
		Item->CollapseForSessionEvent(ImpulseDirection * CollapseImpulseStrength);
		++DestroyedItemCount;
	}

	if (DestroyedItemCount > 0)
	{
		UE_LOG(
			LogAGASSMadCopsEvent,
			Log,
			TEXT("Mad Cops '%s' destroyed tower. DestroyedItemCount=%d TowerCenter=%s"),
			*GetNameSafe(this),
			DestroyedItemCount,
			*ActiveTowerTarget.TowerCenter.ToCompactString());
		MulticastNotifyStackDestroyed(DestroyedItemCount, ActiveTowerTarget.TowerCenter);
	}
	else
	{
		UE_LOG(
			LogAGASSMadCopsEvent,
			Log,
			TEXT("Mad Cops '%s' reached destroy phase but no active tower items were still valid."),
			*GetNameSafe(this));
	}

	ActiveTowerTarget.Items.Reset();
}

void AAGASSMadCopsSessionEventActor::BeginLeaveFlow()
{
	if (MadCopsPhase == EAGASSMadCopsEventPhase::OfficerReturningToCar
		|| MadCopsPhase == EAGASSMadCopsEventPhase::Leaving
		|| MadCopsPhase == EAGASSMadCopsEventPhase::Finished)
	{
		return;
	}

	UE_LOG(
		LogAGASSMadCopsEvent,
		Log,
		TEXT("Mad Cops '%s' beginning leave flow from phase %s."),
		*GetNameSafe(this),
		*StaticEnum<EAGASSMadCopsEventPhase>()->GetValueAsString(MadCopsPhase));

	ActiveTowerTarget.Items.Reset();
	ClearOfficerMoveGoal();

	if (OfficerCharacter != nullptr)
	{
		if (AAIController* const AIController = Cast<AAIController>(OfficerCharacter->GetController()))
		{
			AIController->StopMovement();
		}

		AdvancePhase(EAGASSMadCopsEventPhase::OfficerReturningToCar);
		return;
	}

	AdvancePhase(EAGASSMadCopsEventPhase::Leaving);
}

void AAGASSMadCopsSessionEventActor::UpdateCarTransform(
	const FTransform& StartTransform,
	const FTransform& EndTransform,
	const float Alpha) const
{
	if (PoliceCarActor == nullptr)
	{
		return;
	}

	FTransform BlendedTransform;
	BlendedTransform.Blend(StartTransform, EndTransform, FMath::Clamp(Alpha, 0.f, 1.f));
	PoliceCarActor->ApplyReplicatedCarTransform(BlendedTransform);
}

int32 AAGASSMadCopsSessionEventActor::ComputeBribeCost() const
{
	const UAGASSSessionEventManagerComponent* const EventManager = OwningEventManager.Get();
	const int32 ActivationOrdinal = EventManager != nullptr
		? const_cast<UAGASSSessionEventManagerComponent*>(EventManager)->RegisterAndGetActivationOrdinal(GetEventId())
		: 1;
	return BaseBribeCost + (FMath::Max(ActivationOrdinal - 1, 0) * BribeCostIncreasePerActivation);
}

TScriptInterface<IAGASSSharedTeamMoneyInterface> AAGASSMadCopsSessionEventActor::ResolveSharedMoneyProvider() const
{
	TScriptInterface<IAGASSSharedTeamMoneyInterface> SharedMoneyProvider;

	AGameStateBase* const GameState = GetWorld() != nullptr ? GetWorld()->GetGameState() : nullptr;
	if (GameState != nullptr && GameState->GetClass()->ImplementsInterface(UAGASSSharedTeamMoneyInterface::StaticClass()))
	{
		SharedMoneyProvider.SetObject(GameState);
		SharedMoneyProvider.SetInterface(Cast<IAGASSSharedTeamMoneyInterface>(GameState));
	}

	return SharedMoneyProvider;
}

float AAGASSMadCopsSessionEventActor::GetServerTimeSeconds() const
{
	if (const AGameStateBase* const GameState = GetWorld() != nullptr ? GetWorld()->GetGameState() : nullptr)
	{
		return GameState->GetServerWorldTimeSeconds();
	}

	return GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
}

bool AAGASSMadCopsSessionEventActor::HasPhaseElapsed(const float DurationSeconds) const
{
	return DurationSeconds <= 0.f || (GetServerTimeSeconds() - PhaseStartServerTimeSeconds) >= DurationSeconds;
}
