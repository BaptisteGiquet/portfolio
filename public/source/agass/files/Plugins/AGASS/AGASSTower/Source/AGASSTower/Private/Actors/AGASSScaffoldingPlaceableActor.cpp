#include "Actors/AGASSScaffoldingPlaceableActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/AGASSScaffoldingBehaviorData.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

namespace
{
void AGASSDrawScaffoldingPlacementDebug(
	const UWorld* const World,
	const bool bEnabled,
	const float Duration,
	const FVector& DesiredOrigin,
	const FVector& TraceStart,
	const FVector& TraceEnd,
	const bool bHit,
	const FVector& HitLocation,
	const FColor& TraceColor,
	const FColor& HitColor)
{
	if (!bEnabled || World == nullptr)
	{
		return;
	}

	DrawDebugSphere(World, DesiredOrigin, 14.f, 12, FColor::Cyan, false, Duration, 0, 1.5f);
	DrawDebugLine(World, TraceStart, TraceEnd, TraceColor, false, Duration, 0, 1.5f);
	DrawDebugSphere(World, TraceStart, 8.f, 12, FColor::Yellow, false, Duration, 0, 1.f);
	DrawDebugSphere(World, TraceEnd, 8.f, 12, FColor::Orange, false, Duration, 0, 1.f);
	if (bHit)
	{
		DrawDebugSphere(World, HitLocation, 18.f, 14, HitColor, false, Duration, 0, 2.f);
	}
}
}

AAGASSScaffoldingPlaceableActor::AAGASSScaffoldingPlaceableActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.SetTickFunctionEnable(false);

	StackedMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StackedMesh"));
	StackedMeshComponent->SetupAttachment(GetRootComponent());
	StackedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StackedMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	StackedMeshComponent->SetGenerateOverlapEvents(false);
	StackedMeshComponent->SetCanEverAffectNavigation(false);
	StackedMeshComponent->SetSimulatePhysics(false);
	StackedMeshComponent->SetEnableGravity(false);
	StackedMeshComponent->SetVisibility(false, true);
	StackedMeshComponent->SetHiddenInGame(true);
	StackedMeshComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	StackTargetVolumeComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("StackTargetVolume"));
	StackTargetVolumeComponent->SetupAttachment(GetRootComponent());
	StackTargetVolumeComponent->SetRelativeLocation(FVector(0.f, 0.f, 340.f));
	StackTargetVolumeComponent->SetBoxExtent(FVector(180.f, 180.f, 90.f));
	StackTargetVolumeComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	StackTargetVolumeComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	StackTargetVolumeComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	StackTargetVolumeComponent->SetGenerateOverlapEvents(false);
	StackTargetVolumeComponent->SetCanEverAffectNavigation(false);
	StackTargetVolumeComponent->ShapeColor = FColor::Transparent;
	StackTargetVolumeComponent->bDrawOnlyIfSelected = true;
	StackTargetVolumeComponent->SetLineThickness(0.f);
	StackTargetVolumeComponent->SetVisibility(false, true);
	StackTargetVolumeComponent->SetHiddenInGame(true);

	StackSupportPointComponent = CreateDefaultSubobject<USceneComponent>(TEXT("StackSupportPoint"));
	StackSupportPointComponent->SetupAttachment(GetRootComponent());
	StackSupportPointComponent->SetRelativeLocation(FVector(0.f, 0.f, 250.f));

	RefreshScaffoldingBehavior();
}

void AAGASSScaffoldingPlaceableActor::BeginPlay()
{
	Super::BeginPlay();

	if (UStaticMeshComponent* const MeshComponent = GetMutableItemMeshComponent())
	{
		MeshComponent->OnComponentWake.AddDynamic(this, &ThisClass::HandleScaffoldingMeshWake);
		MeshComponent->OnComponentSleep.AddDynamic(this, &ThisClass::HandleScaffoldingMeshSleep);
	}

	if (StackTargetVolumeComponent != nullptr)
	{
		StackTargetVolumeComponent->SetVisibility(false, true);
		StackTargetVolumeComponent->SetHiddenInGame(true, true);
	}

	if (StackedMeshComponent != nullptr)
	{
		StackedMeshComponent->SetVisibility(false, true);
		StackedMeshComponent->SetHiddenInGame(true, true);
	}

	UpdatePawnCollisionForPhysicsState();
}

void AAGASSScaffoldingPlaceableActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AAGASSScaffoldingPlaceableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		InvalidateSupportedScaffolding();
		SetSupportingScaffolding(nullptr);
	}

	Super::EndPlay(EndPlayReason);
}

void AAGASSScaffoldingPlaceableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bUseStackedVisualMesh);
	DOREPLIFETIME(ThisClass, bIsFallingUnsupported);
}

UPrimitiveComponent* AAGASSScaffoldingPlaceableActor::GetMovementBaseComponent() const
{
	return GetActiveWalkableMeshComponent();
}

bool AAGASSScaffoldingPlaceableActor::CanBeClaimedBy(const AController* Controller) const
{
	return Super::CanBeClaimedBy(Controller);
}

bool AAGASSScaffoldingPlaceableActor::SupportsPlacementPitchRotation() const
{
	return false;
}

FTransform AAGASSScaffoldingPlaceableActor::AdjustDesiredPreviewTransform(const FTransform& DesiredTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const
{
	FTransform ResolvedTransform = DesiredTransform;
	if (TryResolveSupportedPlacementTransform(DesiredTransform, PlacementOwner, IgnoredPlacementActor, ResolvedTransform))
	{
		return ResolvedTransform;
	}

	return DesiredTransform;
}

void AAGASSScaffoldingPlaceableActor::ConfigureCarryPreviewMesh(UStaticMeshComponent& PreviewMeshComponent) const
{
	Super::ConfigureCarryPreviewMesh(PreviewMeshComponent);

	if (UStaticMesh* const GroundVisualMesh = ResolveGroundVisualMesh())
	{
		PreviewMeshComponent.SetStaticMesh(GroundVisualMesh);
	}

	PreviewMeshComponent.SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	PreviewMeshComponent.SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	PreviewMeshComponent.CanCharacterStepUpOn = ECanBeCharacterBase::ECB_Yes;
}

void AAGASSScaffoldingPlaceableActor::CollapseForSessionEvent(const FVector& WorldImpulse)
{
	if (!HasAuthority())
	{
		return;
	}

	BeginUnsupportedFall();

	if (UStaticMeshComponent* const MeshComponent = GetMutableItemMeshComponent())
	{
		if (!WorldImpulse.IsNearlyZero())
		{
			MeshComponent->AddImpulse(WorldImpulse, NAME_None, true);
		}
		else
		{
			MeshComponent->WakeRigidBody();
		}
	}
}

void AAGASSScaffoldingPlaceableActor::GatherPlacementOverlapIgnoredActors(
	const FTransform& CandidateTransform,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor,
	TArray<const AActor*>& OutIgnoredActors) const
{
	if (const AAGASSScaffoldingPlaceableActor* const SupportScaffolding = FindSupportingScaffoldingAtOrigin(CandidateTransform.GetLocation(), PlacementOwner, IgnoredPlacementActor))
	{
		OutIgnoredActors.Add(SupportScaffolding);
	}

	if (const UWorld* const World = GetWorld())
	{
		for (TActorIterator<ACharacter> CharacterIterator(World); CharacterIterator; ++CharacterIterator)
		{
			const ACharacter* const Character = *CharacterIterator;
			const UPrimitiveComponent* const MovementBase = Character != nullptr ? Character->GetMovementBase() : nullptr;
			const AActor* const MovementBaseOwner = MovementBase != nullptr ? MovementBase->GetOwner() : nullptr;
			if (MovementBaseOwner == this || MovementBaseOwner == IgnoredPlacementActor)
			{
				OutIgnoredActors.Add(Character);
			}
		}
	}
}

bool AAGASSScaffoldingPlaceableActor::IsPlacementTransformAllowedByActor(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const
{
	FTransform ResolvedTransform = CandidateTransform;
	if (!TryResolveSupportedPlacementTransform(CandidateTransform, PlacementOwner, IgnoredPlacementActor, ResolvedTransform))
	{
		return false;
	}

	return ResolvedTransform.Equals(CandidateTransform, 0.5f);
}

bool AAGASSScaffoldingPlaceableActor::ShouldSimulatePhysicsWhenVisible() const
{
	return false;
}

void AAGASSScaffoldingPlaceableActor::HandleItemDataChanged()
{
	Super::HandleItemDataChanged();
	RefreshScaffoldingBehavior();
	ApplyScaffoldingVisualMesh();
	UpdatePawnCollisionForPhysicsState();
}

void AAGASSScaffoldingPlaceableActor::HandleCarryStateChanged(const bool bNowCarried)
{
	Super::HandleCarryStateChanged(bNowCarried);

	if (bNowCarried)
	{
		if (HasAuthority())
		{
			InvalidateSupportedScaffolding();
			SetSupportingScaffolding(nullptr);
			bIsFallingUnsupported = false;
			UnsupportedFallVerticalSpeed = 0.f;
			SetActorTickEnabled(false);
		}

		SetUseStackedVisualMesh(false);
		UpdatePawnCollisionForPhysicsState();
		return;
	}

	if (!IsHeldHidden())
	{
		bIsFallingUnsupported = false;
		UnsupportedFallVerticalSpeed = 0.f;
		SetActorTickEnabled(false);
		RefreshPlacedVisualState();
	}

	UpdatePawnCollisionForPhysicsState();
}

void AAGASSScaffoldingPlaceableActor::HandlePlacedAtTransform(const FTransform& ApprovedTransform)
{
	Super::HandlePlacedAtTransform(ApprovedTransform);
	bIsFallingUnsupported = false;
	UnsupportedFallVerticalSpeed = 0.f;
	SetActorTickEnabled(false);
	SetSupportingScaffolding(FindSupportingScaffoldingAtOrigin(ApprovedTransform.GetLocation(), nullptr, nullptr));
	SetUseStackedVisualMesh(SupportingScaffolding.IsValid());
	UpdatePawnCollisionForPhysicsState();
}

void AAGASSScaffoldingPlaceableActor::RefreshPlacedVisualState()
{
	if (bIsFallingUnsupported)
	{
		return;
	}

	const AAGASSScaffoldingPlaceableActor* const SupportScaffolding = FindSupportingScaffoldingAtOrigin(GetActorLocation(), nullptr, nullptr);
	SetSupportingScaffolding(const_cast<AAGASSScaffoldingPlaceableActor*>(SupportScaffolding));
	SetUseStackedVisualMesh(SupportScaffolding != nullptr);
}

void AAGASSScaffoldingPlaceableActor::OnRep_UseStackedVisualMesh()
{
	ApplyScaffoldingVisualMesh();
	UpdatePawnCollisionForPhysicsState();
}

void AAGASSScaffoldingPlaceableActor::OnRep_IsFallingUnsupported()
{
	if (UStaticMeshComponent* const MeshComponent = GetMutableItemMeshComponent())
	{
		MeshComponent->SetSimulatePhysics(bIsFallingUnsupported);
		MeshComponent->SetEnableGravity(bIsFallingUnsupported);
		if (!bIsFallingUnsupported)
		{
			MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
			MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
	}

	UpdatePawnCollisionForPhysicsState();

	if (!bIsFallingUnsupported)
	{
		ApplyScaffoldingVisualMesh();
	}
}

void AAGASSScaffoldingPlaceableActor::HandleScaffoldingMeshWake(UPrimitiveComponent* WakingComponent, const FName BoneName)
{
	UpdatePawnCollisionForPhysicsState();
}

void AAGASSScaffoldingPlaceableActor::HandleScaffoldingMeshSleep(UPrimitiveComponent* SleepingComponent, const FName BoneName)
{
	if (HasAuthority() && !IsHeldHidden())
	{
		if (bIsFallingUnsupported)
		{
			AAGASSScaffoldingPlaceableActor* ResolvedSupportScaffolding = nullptr;
			if (CanStabilizeUnsupportedRestingPose(ResolvedSupportScaffolding))
			{
				if (UStaticMeshComponent* const MeshComponent = GetMutableItemMeshComponent())
				{
					if (MeshComponent->IsSimulatingPhysics())
					{
						MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
						MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
					}

					MeshComponent->SetSimulatePhysics(false);
					MeshComponent->SetEnableGravity(false);
				}

				bIsFallingUnsupported = false;
				UnsupportedFallVerticalSpeed = 0.f;
				SetSupportingScaffolding(ResolvedSupportScaffolding);
				SetUseStackedVisualMesh(ResolvedSupportScaffolding != nullptr);
				ForceNetUpdate();
			}
			else if (UStaticMeshComponent* const MeshComponent = GetMutableItemMeshComponent())
			{
				MeshComponent->WakeRigidBody();
			}
		}
		else
		{
			RefreshPlacedVisualState();
		}
	}

	UpdatePawnCollisionForPhysicsState();
}

bool AAGASSScaffoldingPlaceableActor::TryResolveSupportedPlacementTransform(
	const FTransform& DesiredTransform,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor,
	FTransform& OutResolvedTransform) const
{
	if (TryResolveStackedPlacementTransform(DesiredTransform, PlacementOwner, IgnoredPlacementActor, OutResolvedTransform))
	{
		return true;
	}

	return TryResolveGroundPlacementTransform(DesiredTransform, PlacementOwner, IgnoredPlacementActor, OutResolvedTransform);
}

bool AAGASSScaffoldingPlaceableActor::TryResolveStackedPlacementTransform(
	const FTransform& DesiredTransform,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor,
	FTransform& OutResolvedTransform) const
{
	AAGASSScaffoldingPlaceableActor* SupportScaffolding = nullptr;
	FVector SupportHitLocation = FVector::ZeroVector;
	if (!FindTargetedSupportScaffolding(DesiredTransform, PlacementOwner, IgnoredPlacementActor, SupportScaffolding, SupportHitLocation)
		|| SupportScaffolding == nullptr
		|| SupportScaffolding->IsHeldHidden())
	{
		return false;
	}

	FTransform ResolvedTransform = DesiredTransform;
	ResolvedTransform.SetRotation(SupportScaffolding->GetActorQuat());
	ResolvedTransform.SetLocation(FVector(
		SupportScaffolding->GetActorLocation().X,
		SupportScaffolding->GetActorLocation().Y,
		GetStackTopWorldZ(*SupportScaffolding)));

	if (HasStableScaffoldingOccupyingOrigin(ResolvedTransform.GetLocation(), SupportScaffolding))
	{
		return false;
	}

	OutResolvedTransform = ResolvedTransform;
	return true;
}

bool AAGASSScaffoldingPlaceableActor::TryResolveGroundPlacementTransform(
	const FTransform& DesiredTransform,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor,
	FTransform& OutResolvedTransform) const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const FVector TraceStart = DesiredTransform.GetLocation();
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, GetGroundPlacementTraceDistance());

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSScaffoldingGroundPlacement), false);
	QueryParams.AddIgnoredActor(this);

	if (PlacementOwner != nullptr)
	{
		QueryParams.AddIgnoredActor(PlacementOwner);
	}

	if (IgnoredPlacementActor != nullptr)
	{
		QueryParams.AddIgnoredActor(IgnoredPlacementActor);
	}

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const bool bValidHit = bHit
		&& HitResult.bBlockingHit
		&& HitResult.Component.Get() != nullptr
		&& (HitResult.GetActor() == nullptr || Cast<AAGASSPlaceableItemActor>(HitResult.GetActor()) == nullptr);
	AGASSDrawScaffoldingPlacementDebug(
		World,
		ShouldDrawPlacementDebug(),
		GetPlacementDebugDrawDuration(),
		DesiredTransform.GetLocation(),
		TraceStart,
		TraceEnd,
		bHit,
		HitResult.ImpactPoint,
		bValidHit ? FColor::Green : FColor::Red,
		bValidHit ? FColor::Green : FColor::Red);
	if (!bValidHit)
	{
		return false;
	}

	FTransform ResolvedTransform = DesiredTransform;
	ResolvedTransform.SetLocation(HitResult.ImpactPoint);
	OutResolvedTransform = ResolvedTransform;
	return true;
}

bool AAGASSScaffoldingPlaceableActor::FindTargetedSupportScaffolding(
	const FTransform& DesiredTransform,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor,
	AAGASSScaffoldingPlaceableActor*& OutSupportScaffolding,
	FVector& OutSupportHitLocation) const
{
	OutSupportScaffolding = nullptr;
	OutSupportHitLocation = FVector::ZeroVector;

	const UWorld* const World = GetWorld();
	const AController* const Controller = PlacementOwner != nullptr ? PlacementOwner->GetController() : nullptr;
	if (World == nullptr || Controller == nullptr)
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const float TraceDistance = FMath::Max(FVector::Dist(ViewLocation, DesiredTransform.GetLocation()) + GetStackDetectionTraceSlack(), 0.f);
	const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * TraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSScaffoldingStackPlacement), false);
	QueryParams.AddIgnoredActor(this);

	if (PlacementOwner != nullptr)
	{
		QueryParams.AddIgnoredActor(PlacementOwner);
	}

	if (IgnoredPlacementActor != nullptr)
	{
		QueryParams.AddIgnoredActor(IgnoredPlacementActor);
	}

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, ECC_Visibility, QueryParams))
	{
		return false;
	}

	AAGASSScaffoldingPlaceableActor* const HitSupportScaffolding = Cast<AAGASSScaffoldingPlaceableActor>(HitResult.GetActor());
	OutSupportScaffolding = IsStableSupportScaffolding(HitSupportScaffolding) ? HitSupportScaffolding : nullptr;
	OutSupportHitLocation = HitResult.ImpactPoint;
	return OutSupportScaffolding != nullptr;
}

bool AAGASSScaffoldingPlaceableActor::IsStableSupportScaffolding(const AAGASSScaffoldingPlaceableActor* CandidateSupport) const
{
	if (CandidateSupport == nullptr || CandidateSupport == this || CandidateSupport->IsHeldHidden() || CandidateSupport->bIsFallingUnsupported)
	{
		return false;
	}

	const UStaticMeshComponent* const SupportMeshComponent = CandidateSupport->GetItemMeshComponent();
	return SupportMeshComponent == nullptr || !SupportMeshComponent->IsSimulatingPhysics();
}

bool AAGASSScaffoldingPlaceableActor::HasStableScaffoldingOccupyingOrigin(const FVector& CandidateOrigin, const AAGASSScaffoldingPlaceableActor* IgnoredScaffolding) const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	constexpr float HorizontalTolerance = 5.f;
	constexpr float VerticalTolerance = 5.f;

	for (TActorIterator<AAGASSScaffoldingPlaceableActor> It(World); It; ++It)
	{
		const AAGASSScaffoldingPlaceableActor* const CandidateScaffolding = *It;
		if (CandidateScaffolding == nullptr
			|| CandidateScaffolding == this
			|| CandidateScaffolding == IgnoredScaffolding
			|| !IsStableSupportScaffolding(CandidateScaffolding))
		{
			continue;
		}

		const FVector ExistingOrigin = CandidateScaffolding->GetActorLocation();
		if (FMath::Abs(CandidateOrigin.X - ExistingOrigin.X) <= HorizontalTolerance
			&& FMath::Abs(CandidateOrigin.Y - ExistingOrigin.Y) <= HorizontalTolerance
			&& FMath::Abs(CandidateOrigin.Z - ExistingOrigin.Z) <= VerticalTolerance)
		{
			return true;
		}
	}

	return false;
}

AAGASSScaffoldingPlaceableActor* AAGASSScaffoldingPlaceableActor::FindSupportingScaffoldingByStackAlignment(const FVector& CandidateOrigin) const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	constexpr float HorizontalTolerance = 5.f;
	constexpr float VerticalTolerance = 5.f;

	for (TActorIterator<AAGASSScaffoldingPlaceableActor> It(World); It; ++It)
	{
		AAGASSScaffoldingPlaceableActor* const CandidateSupport = *It;
		if (!IsStableSupportScaffolding(CandidateSupport))
		{
			continue;
		}

		const FVector SupportOrigin = CandidateSupport->GetActorLocation();
		if (FMath::Abs(CandidateOrigin.X - SupportOrigin.X) > HorizontalTolerance
			|| FMath::Abs(CandidateOrigin.Y - SupportOrigin.Y) > HorizontalTolerance)
		{
			continue;
		}

		const float SupportTopZ = GetStackTopWorldZ(*CandidateSupport);
		if (FMath::Abs(CandidateOrigin.Z - SupportTopZ) <= VerticalTolerance)
		{
			return CandidateSupport;
		}
	}

	return nullptr;
}

AAGASSScaffoldingPlaceableActor* AAGASSScaffoldingPlaceableActor::FindSupportingScaffoldingAtOrigin(
	const FVector& CandidateOrigin,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor) const
{
	if (AAGASSScaffoldingPlaceableActor* const AlignedSupport = FindSupportingScaffoldingByStackAlignment(CandidateOrigin))
	{
		return AlignedSupport;
	}

	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	constexpr float SupportProbeUpDistance = 5.f;
	constexpr float SupportProbeDownDistance = 50.f;
	const FVector TraceStart = CandidateOrigin + FVector(0.f, 0.f, SupportProbeUpDistance);
	const FVector TraceEnd = CandidateOrigin - FVector(0.f, 0.f, SupportProbeDownDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSScaffoldingSupportProbe), false);
	QueryParams.AddIgnoredActor(this);

	if (PlacementOwner != nullptr)
	{
		QueryParams.AddIgnoredActor(PlacementOwner);
	}

	if (IgnoredPlacementActor != nullptr)
	{
		QueryParams.AddIgnoredActor(IgnoredPlacementActor);
	}

	FHitResult HitResult;
	if (!World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return nullptr;
	}

	AAGASSScaffoldingPlaceableActor* const SupportScaffolding = Cast<AAGASSScaffoldingPlaceableActor>(HitResult.GetActor());
	return IsStableSupportScaffolding(SupportScaffolding) ? SupportScaffolding : nullptr;
}

UStaticMeshComponent* AAGASSScaffoldingPlaceableActor::GetActiveWalkableMeshComponent() const
{
	if (!IsHeldHidden()
		&& !bIsFallingUnsupported
		&& bUseStackedVisualMesh
		&& StackedMeshComponent != nullptr
		&& StackedMeshComponent->GetStaticMesh() != nullptr)
	{
		return StackedMeshComponent;
	}

	return GetMutableItemMeshComponent();
}

float AAGASSScaffoldingPlaceableActor::GetStackTopWorldZ(const AAGASSScaffoldingPlaceableActor& SupportScaffolding) const
{
	if (!SupportScaffolding.bUseStackedVisualMesh && SupportScaffolding.StackSupportPointComponent != nullptr)
	{
		return SupportScaffolding.StackSupportPointComponent->GetComponentLocation().Z;
	}

	const UStaticMeshComponent* SupportMeshComponent = nullptr;
	if (SupportScaffolding.bUseStackedVisualMesh
		&& SupportScaffolding.StackedMeshComponent != nullptr
		&& SupportScaffolding.StackedMeshComponent->GetStaticMesh() != nullptr)
	{
		SupportMeshComponent = SupportScaffolding.StackedMeshComponent;
	}
	else
	{
		SupportMeshComponent = SupportScaffolding.GetItemMeshComponent();
	}

	if (SupportMeshComponent == nullptr)
	{
		return SupportScaffolding.GetActorLocation().Z;
	}

	const FBoxSphereBounds& SupportBounds = SupportMeshComponent->Bounds;
	return SupportBounds.Origin.Z + SupportBounds.BoxExtent.Z;
}

bool AAGASSScaffoldingPlaceableActor::CanStabilizeUnsupportedRestingPose(AAGASSScaffoldingPlaceableActor*& OutSupportScaffolding) const
{
	OutSupportScaffolding = nullptr;

	const UStaticMeshComponent* const MeshComponent = GetItemMeshComponent();
	const UWorld* const World = GetWorld();
	if (MeshComponent == nullptr || World == nullptr)
	{
		return false;
	}

	const FBoxSphereBounds& Bounds = MeshComponent->Bounds;
	const float BottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
	const float HalfSampleX = Bounds.BoxExtent.X * 0.5f;
	const float HalfSampleY = Bounds.BoxExtent.Y * 0.5f;

	const FVector2D SampleOffsets[] =
	{
		FVector2D(0.f, 0.f),
		FVector2D(HalfSampleX, 0.f),
		FVector2D(-HalfSampleX, 0.f),
		FVector2D(0.f, HalfSampleY),
		FVector2D(0.f, -HalfSampleY),
		FVector2D(HalfSampleX, HalfSampleY),
		FVector2D(HalfSampleX, -HalfSampleY),
		FVector2D(-HalfSampleX, HalfSampleY),
		FVector2D(-HalfSampleX, -HalfSampleY)
	};

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSScaffoldingRestingPoseValidation), false);
	QueryParams.AddIgnoredActor(this);

	for (const FVector2D& SampleOffset : SampleOffsets)
	{
		const FVector TraceStart(Bounds.Origin.X + SampleOffset.X, Bounds.Origin.Y + SampleOffset.Y, BottomZ + 10.f);
		const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, 40.f);

		TArray<FHitResult> HitResults;
		if (!World->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
		{
			continue;
		}

		for (const FHitResult& HitResult : HitResults)
		{
			if (!HitResult.bBlockingHit || HitResult.Component.Get() == nullptr)
			{
				continue;
			}

			if (AAGASSScaffoldingPlaceableActor* const StableSupportScaffolding = Cast<AAGASSScaffoldingPlaceableActor>(HitResult.GetActor()))
			{
				if (IsStableSupportScaffolding(StableSupportScaffolding))
				{
					OutSupportScaffolding = StableSupportScaffolding;
					return true;
				}

				continue;
			}

			if (Cast<AAGASSPlaceableItemActor>(HitResult.GetActor()) != nullptr)
			{
				continue;
			}

			return true;
		}
	}

	return false;
}

bool AAGASSScaffoldingPlaceableActor::TryResolveUnsupportedFallLanding(
	const FVector& TraceStart,
	const FVector& TraceEnd,
	FVector& OutResolvedOrigin,
	AAGASSScaffoldingPlaceableActor*& OutSupportScaffolding) const
{
	OutSupportScaffolding = nullptr;

	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSScaffoldingUnsupportedFallLanding), false);
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	if (!World->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return false;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		if (!HitResult.bBlockingHit || HitResult.Component.Get() == nullptr)
		{
			continue;
		}

		if (AAGASSScaffoldingPlaceableActor* const StableSupportScaffolding = Cast<AAGASSScaffoldingPlaceableActor>(HitResult.GetActor()))
		{
			if (IsStableSupportScaffolding(StableSupportScaffolding))
			{
				OutSupportScaffolding = StableSupportScaffolding;
				OutResolvedOrigin = FVector(
					StableSupportScaffolding->GetActorLocation().X,
					StableSupportScaffolding->GetActorLocation().Y,
					GetStackTopWorldZ(*StableSupportScaffolding));
				return true;
			}

			continue;
		}

		if (Cast<AAGASSPlaceableItemActor>(HitResult.GetActor()) != nullptr)
		{
			continue;
		}

		OutResolvedOrigin = HitResult.ImpactPoint;
		return true;
	}

	return false;
}

void AAGASSScaffoldingPlaceableActor::ApplyScaffoldingVisualMesh()
{
	UStaticMeshComponent* const GroundMeshComponent = GetMutableItemMeshComponent();
	if (GroundMeshComponent == nullptr)
	{
		return;
	}

	if (GroundMeshComponent->IsSimulatingPhysics())
	{
		return;
	}

	UStaticMesh* const GroundVisualMesh = ResolveGroundVisualMesh();
	UStaticMesh* const StackedVisualMesh = ResolveStackedVisualMesh();

	if (StackedMeshComponent != nullptr && StackedMeshComponent->GetStaticMesh() != StackedVisualMesh)
	{
		StackedMeshComponent->SetStaticMesh(StackedVisualMesh);
	}

	UpdatePawnCollisionForPhysicsState();
}

void AAGASSScaffoldingPlaceableActor::SetUseStackedVisualMesh(const bool bNewUseStackedVisualMesh)
{
	if (bUseStackedVisualMesh == bNewUseStackedVisualMesh)
	{
		ApplyScaffoldingVisualMesh();
		return;
	}

	bUseStackedVisualMesh = bNewUseStackedVisualMesh;
	ApplyScaffoldingVisualMesh();

	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

UStaticMesh* AAGASSScaffoldingPlaceableActor::ResolveGroundVisualMesh() const
{
	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		if (UStaticMesh* const GroundVisualMesh = Definition->WorldStaticMesh.LoadSynchronous())
		{
			return GroundVisualMesh;
		}
	}

	if (const UStaticMeshComponent* const SourceMeshComponent = GetItemMeshComponent())
	{
		return SourceMeshComponent->GetStaticMesh();
	}

	return nullptr;
}

UStaticMesh* AAGASSScaffoldingPlaceableActor::ResolveStackedVisualMesh() const
{
	if (ScaffoldingBehavior != nullptr)
	{
		if (UStaticMesh* const StackedVisualMesh = ScaffoldingBehavior->StackedWorldStaticMesh.LoadSynchronous())
		{
			return StackedVisualMesh;
		}
	}

	return ResolveGroundVisualMesh();
}

float AAGASSScaffoldingPlaceableActor::GetGroundPlacementTraceDistance() const
{
	return ScaffoldingBehavior != nullptr ? FMath::Max(ScaffoldingBehavior->GroundPlacementTraceDistance, 0.f) : 1500.f;
}

float AAGASSScaffoldingPlaceableActor::GetStackDetectionTraceSlack() const
{
	return ScaffoldingBehavior != nullptr ? FMath::Max(ScaffoldingBehavior->StackDetectionTraceSlack, 0.f) : 100.f;
}

bool AAGASSScaffoldingPlaceableActor::ShouldDrawPlacementDebug() const
{
	return ScaffoldingBehavior != nullptr && ScaffoldingBehavior->bDrawPlacementDebug;
}

float AAGASSScaffoldingPlaceableActor::GetPlacementDebugDrawDuration() const
{
	return ScaffoldingBehavior != nullptr ? FMath::Max(ScaffoldingBehavior->PlacementDebugDrawDuration, 0.f) : 5.f;
}

void AAGASSScaffoldingPlaceableActor::RefreshScaffoldingBehavior()
{
	ScaffoldingBehavior = Cast<UAGASSScaffoldingBehaviorData>(GetBehaviorTuning());
}

void AAGASSScaffoldingPlaceableActor::SetSupportingScaffolding(AAGASSScaffoldingPlaceableActor* NewSupportScaffolding)
{
	if (!HasAuthority())
	{
		SupportingScaffolding = NewSupportScaffolding;
		return;
	}

	AAGASSScaffoldingPlaceableActor* const PreviousSupportScaffolding = SupportingScaffolding.Get();
	if (PreviousSupportScaffolding == NewSupportScaffolding)
	{
		return;
	}

	if (PreviousSupportScaffolding != nullptr)
	{
		PreviousSupportScaffolding->RemoveSupportedScaffolding(this);
	}

	SupportingScaffolding = NewSupportScaffolding;
	if (NewSupportScaffolding != nullptr)
	{
		NewSupportScaffolding->AddSupportedScaffolding(this);
	}
}

void AAGASSScaffoldingPlaceableActor::AddSupportedScaffolding(AAGASSScaffoldingPlaceableActor* SupportedScaffolding)
{
	if (!HasAuthority() || SupportedScaffolding == nullptr || SupportedScaffolding == this)
	{
		return;
	}

	SupportedScaffoldingActors.RemoveAll(
		[](const TWeakObjectPtr<AAGASSScaffoldingPlaceableActor>& Candidate)
		{
			return !Candidate.IsValid();
		});

	const bool bAlreadyTracked = SupportedScaffoldingActors.ContainsByPredicate(
		[SupportedScaffolding](const TWeakObjectPtr<AAGASSScaffoldingPlaceableActor>& Candidate)
		{
			return Candidate.Get() == SupportedScaffolding;
		});
	if (!bAlreadyTracked)
	{
		SupportedScaffoldingActors.Add(SupportedScaffolding);
	}
}

void AAGASSScaffoldingPlaceableActor::RemoveSupportedScaffolding(AAGASSScaffoldingPlaceableActor* SupportedScaffolding)
{
	SupportedScaffoldingActors.RemoveAll(
		[SupportedScaffolding](const TWeakObjectPtr<AAGASSScaffoldingPlaceableActor>& Candidate)
		{
			return !Candidate.IsValid() || Candidate.Get() == SupportedScaffolding;
		});
}

void AAGASSScaffoldingPlaceableActor::InvalidateSupportedScaffolding()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<TWeakObjectPtr<AAGASSScaffoldingPlaceableActor>> SupportedScaffoldingSnapshot = SupportedScaffoldingActors;
	SupportedScaffoldingActors.Reset();

	for (const TWeakObjectPtr<AAGASSScaffoldingPlaceableActor>& CandidateDependent : SupportedScaffoldingSnapshot)
	{
		if (AAGASSScaffoldingPlaceableActor* const SupportedScaffolding = CandidateDependent.Get())
		{
			SupportedScaffolding->BeginUnsupportedFall();
		}
	}
}

void AAGASSScaffoldingPlaceableActor::BeginUnsupportedFall()
{
	if (!HasAuthority() || IsHeldHidden() || bIsFallingUnsupported)
	{
		return;
	}

	UStaticMeshComponent* const MeshComponent = GetMutableItemMeshComponent();
	if (MeshComponent == nullptr)
	{
		return;
	}

	SetSupportingScaffolding(nullptr);
	InvalidateSupportedScaffolding();
	bIsFallingUnsupported = true;
	SetUseStackedVisualMesh(false);
	UnsupportedFallVerticalSpeed = 0.f;
	SetActorTickEnabled(false);

	SetNetDormancy(DORM_Awake);
	FlushNetDormancy();
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetEnableGravity(true);
	MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	MeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	MeshComponent->WakeRigidBody();
	UpdatePawnCollisionForPhysicsState();
	ForceNetUpdate();
}

void AAGASSScaffoldingPlaceableActor::UpdatePawnCollisionForPhysicsState()
{
	UStaticMeshComponent* const GroundMeshComponent = GetMutableItemMeshComponent();
	if (GroundMeshComponent == nullptr)
	{
		return;
	}

	const bool bUseStackedComponent =
		!IsHeldHidden()
		&& !bIsFallingUnsupported
		&& bUseStackedVisualMesh
		&& StackedMeshComponent != nullptr
		&& StackedMeshComponent->GetStaticMesh() != nullptr;

	const bool bShowGroundMesh = !IsHeldHidden() && !bUseStackedComponent;
	GroundMeshComponent->SetVisibility(bShowGroundMesh, true);
	GroundMeshComponent->SetHiddenInGame(!bShowGroundMesh, true);

	if (bShowGroundMesh)
	{
		GroundMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GroundMeshComponent->CanCharacterStepUpOn = bIsFallingUnsupported ? ECanBeCharacterBase::ECB_No : ECanBeCharacterBase::ECB_Yes;
		GroundMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
	else
	{
		GroundMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GroundMeshComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		GroundMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	if (StackedMeshComponent == nullptr)
	{
		return;
	}

	StackedMeshComponent->SetVisibility(bUseStackedComponent, true);
	StackedMeshComponent->SetHiddenInGame(!bUseStackedComponent, true);

	if (bUseStackedComponent)
	{
		StackedMeshComponent->SetCollisionObjectType(GroundMeshComponent->GetCollisionObjectType());
		StackedMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		StackedMeshComponent->SetCollisionResponseToChannels(GroundMeshComponent->GetCollisionResponseToChannels());
		StackedMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		StackedMeshComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_Yes;
	}
	else
	{
		StackedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		StackedMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		StackedMeshComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	}
}
