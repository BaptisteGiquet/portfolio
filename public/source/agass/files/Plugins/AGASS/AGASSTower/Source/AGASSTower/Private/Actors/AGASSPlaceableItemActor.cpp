#include "Actors/AGASSPlaceableItemActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/AGASSItemDefinition.h"
#include "Data/AGASSPlaceableBehaviorData.h"
#include "Engine/CollisionProfile.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Settings/AGASSTowerDeveloperSettings.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSPlaceableSupport, Log, All);

AAGASSPlaceableItemActor::AAGASSPlaceableItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.f);
	SetMinNetUpdateFrequency(10.f);
	NetDormancy = DORM_Awake;

	ItemMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMeshComponent);

	ItemMeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	ItemMeshComponent->SetGenerateOverlapEvents(false);
	ItemMeshComponent->SetCanEverAffectNavigation(false);
	ItemMeshComponent->SetSimulatePhysics(true);
	ItemMeshComponent->SetUseCCD(true);
	ItemMeshComponent->SetIsReplicated(true);
	ItemMeshComponent->BodyInstance.bGenerateWakeEvents = true;

	SpawnFallCollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnFallCollision"));
	SpawnFallCollisionComponent->SetupAttachment(ItemMeshComponent);
	SpawnFallCollisionComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	SpawnFallCollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnFallCollisionComponent->SetGenerateOverlapEvents(false);
	SpawnFallCollisionComponent->SetCanEverAffectNavigation(false);
	SpawnFallCollisionComponent->SetSimulatePhysics(false);
	SpawnFallCollisionComponent->SetVisibility(false, true);
	SpawnFallCollisionComponent->SetHiddenInGame(true);
	SpawnFallCollisionComponent->bDrawOnlyIfSelected = true;
	SpawnFallCollisionComponent->ShapeColor = FColor::Transparent;
	SpawnFallCollisionComponent->SetBoxExtent(FVector(25.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		ItemMeshComponent->SetStaticMesh(CubeMesh.Object);
	}
}

void AAGASSPlaceableItemActor::BeginPlay()
{
	Super::BeginPlay();

	if (ItemMeshComponent != nullptr)
	{
		ItemMeshComponent->OnComponentWake.AddDynamic(this, &ThisClass::HandleMeshComponentWake);
		ItemMeshComponent->OnComponentSleep.AddDynamic(this, &ThisClass::HandleMeshComponentSleep);
		ItemMeshComponent->OnComponentHit.AddDynamic(this, &ThisClass::HandleMeshComponentHit);
	}

	ApplyResolvedItemVisuals();
	ApplyResolvedPhysicsTuning();
	HandleItemDataChanged();
	ApplyHeldHiddenState();

	if (HasAuthority() && !IsCarried() && ItemMeshComponent != nullptr && ItemMeshComponent->IsSimulatingPhysics() && !ItemMeshComponent->RigidBodyIsAwake())
	{
		ScheduleSettledDormancy();
	}
}

void AAGASSPlaceableItemActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearSettledDormancyTimer();

	if (HasAuthority())
	{
		InvalidateSupportedTowerPlaceables();
		SetSupportingTowerPlaceable(nullptr);
	}

	Super::EndPlay(EndPlayReason);
}

void AAGASSPlaceableItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, RuntimeItemData);
	DOREPLIFETIME(ThisClass, bPlacementCommitted);
}

EAGASSCarriedItemType AAGASSPlaceableItemActor::GetCarriedItemType() const
{
	return EAGASSCarriedItemType::Placeable;
}

bool AAGASSPlaceableItemActor::Claim(AController* Controller)
{
	ClearSettledDormancyTimer();
	bPlacementCommittedBeforeClaim = bPlacementCommitted;
	const bool bClaimed = BeginCarry(Controller, false);
	if (bClaimed)
	{
		SetPlacementCommitted(false);
		ResumeActiveReplication();
		ApplyHeldHiddenState();
	}

	return bClaimed;
}

bool AAGASSPlaceableItemActor::ClaimForImmediateCarry(AController* Controller)
{
	ClearSettledDormancyTimer();
	bPlacementCommittedBeforeClaim = bPlacementCommitted;
	const bool bClaimed = BeginCarry(Controller, true);
	if (bClaimed)
	{
		SetPlacementCommitted(false);
		ResumeActiveReplication();
		ApplyHeldHiddenState();
	}

	return bClaimed;
}

void AAGASSPlaceableItemActor::ReleaseClaim(const bool bRestoreTransform)
{
	if (!HasAuthority() || !IsCarried())
	{
		return;
	}

	ClearSettledDormancyTimer();
	if (bRestoreTransform)
	{
		SetPlacementCommitted(bPlacementCommittedBeforeClaim);
	}
	else
	{
		SetPlacementCommitted(false);
	}

	EndCarry(bRestoreTransform, nullptr);
	ResumeActiveReplication();
	ApplyHeldHiddenState();
}

void AAGASSPlaceableItemActor::ReleaseFromCarry(const FTransform& ReleaseTransform)
{
	if (!HasAuthority() || !IsCarried())
	{
		return;
	}

	ClearSettledDormancyTimer();
	SetPlacementCommitted(false);
	EndCarry(false, &ReleaseTransform);
	ResumeActiveReplication();
	ApplyHeldHiddenState();
}

bool AAGASSPlaceableItemActor::PlaceAtTransform(const FTransform& ApprovedTransform)
{
	if (!HasAuthority() || !IsCarried())
	{
		return false;
	}

	ClearSettledDormancyTimer();
	SetPlacementCommitted(RequiresTouchingPlacementSupport());
	bPlacementCommittedBeforeClaim = bPlacementCommitted;
	EndCarry(false, &ApprovedTransform);
	ResumeActiveReplication();
	ApplyHeldHiddenState();
	ResolveFinalPlacedEncroachment();
	HandlePlacedAtTransform(GetActorTransform());
	return true;
}

bool AAGASSPlaceableItemActor::CanToggleHeldRotationMode() const
{
	return true;
}

bool AAGASSPlaceableItemActor::CanAdjustHeldPreviewDistance() const
{
	return true;
}

float AAGASSPlaceableItemActor::GetPreferredHeldPreviewDistance() const
{
	return 0.f;
}

FVector AAGASSPlaceableItemActor::GetPreviewHalfExtent() const
{
	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		if (!Definition->PlacementValidationHalfExtent.IsNearlyZero())
		{
			return Definition->PlacementValidationHalfExtent;
		}
	}

	return GetMeshDerivedHalfExtent();
}

float AAGASSPlaceableItemActor::GetPlacementRotationStepDegrees() const
{
	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		return FMath::Max(Definition->PlacementRotationStepDegrees, 1.f);
	}

	return 5.f;
}

bool AAGASSPlaceableItemActor::RequiresTouchingPlacementSupport() const
{
	return RequiresTouchingPlacementSupportByDefault();
}

EAGASSPlacementValidationReason AAGASSPlaceableItemActor::EvaluatePlacementTransform(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPreviewActor) const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr || ItemMeshComponent == nullptr)
	{
		return EAGASSPlacementValidationReason::ItemRuleFailed;
	}

	FVector PlacementHalfExtent = GetPreviewHalfExtent() - FVector(GetPlacementValidationInset());
	PlacementHalfExtent.X = FMath::Max(PlacementHalfExtent.X, 1.f);
	PlacementHalfExtent.Y = FMath::Max(PlacementHalfExtent.Y, 1.f);
	PlacementHalfExtent.Z = FMath::Max(PlacementHalfExtent.Z, 1.f);
	const FVector PlacementCenter = CandidateTransform.TransformPosition(GetMeshDerivedLocalBoundsCenter());

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSPlacementOverlap), false);
	QueryParams.AddIgnoredActor(this);

	if (PlacementOwner != nullptr)
	{
		QueryParams.AddIgnoredActor(PlacementOwner);
	}

	if (IgnoredPreviewActor != nullptr)
	{
		QueryParams.AddIgnoredActor(IgnoredPreviewActor);
	}

	TArray<const AActor*> AdditionalIgnoredActors;
	GatherPlacementOverlapIgnoredActors(CandidateTransform, PlacementOwner, IgnoredPreviewActor, AdditionalIgnoredActors);
	for (const AActor* IgnoredActor : AdditionalIgnoredActors)
	{
		if (IgnoredActor != nullptr)
		{
			QueryParams.AddIgnoredActor(IgnoredActor);
		}
	}

	const ECollisionChannel CollisionChannel = ItemMeshComponent->GetCollisionObjectType();
	const FCollisionResponseParams CollisionResponseParams(ItemMeshComponent->GetCollisionResponseToChannels());

	if (World->OverlapBlockingTestByChannel(
		PlacementCenter,
		CandidateTransform.GetRotation(),
		CollisionChannel,
		FCollisionShape::MakeBox(PlacementHalfExtent),
		QueryParams,
		CollisionResponseParams))
	{
		return EAGASSPlacementValidationReason::BlockingOverlap;
	}

	if (!IsPlacementTransformAllowedByActor(CandidateTransform, PlacementOwner, IgnoredPreviewActor))
	{
		return EAGASSPlacementValidationReason::ItemRuleFailed;
	}

	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		if (!Definition->AllowsPlacementAtTransform(this, PlacementOwner, CandidateTransform))
		{
			return EAGASSPlacementValidationReason::ItemRuleFailed;
		}
	}

	if (RequiresTouchingPlacementSupport() && !HasTouchingPlacementSupport(CandidateTransform, PlacementOwner, IgnoredPreviewActor))
	{
		return EAGASSPlacementValidationReason::ItemRuleFailed;
	}

	return EAGASSPlacementValidationReason::Valid;
}

bool AAGASSPlaceableItemActor::TryResolveTouchingPlacementSnapTransform(
	const FTransform& CandidateTransform,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor,
	FTransform& OutSnappedTransform) const
{
	OutSnappedTransform = CandidateTransform;

	if (!RequiresTouchingPlacementSupport())
	{
		return false;
	}

	const UWorld* const World = GetWorld();
	if (World == nullptr || ItemMeshComponent == nullptr)
	{
		return false;
	}

	const float SnapDistance = GetPlacementTouchTolerance();
	if (SnapDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	FVector PlacementHalfExtent = GetPreviewHalfExtent();
	PlacementHalfExtent.X = FMath::Max(PlacementHalfExtent.X, 1.f);
	PlacementHalfExtent.Y = FMath::Max(PlacementHalfExtent.Y, 1.f);
	PlacementHalfExtent.Z = FMath::Max(PlacementHalfExtent.Z, 1.f);

	const FVector PlacementCenter = CandidateTransform.TransformPosition(GetMeshDerivedLocalBoundsCenter());
	const FVector SweepEnd = PlacementCenter - FVector(0.f, 0.f, SnapDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSPlacementSnapSupport), false);
	QueryParams.AddIgnoredActor(this);

	if (PlacementOwner != nullptr)
	{
		QueryParams.AddIgnoredActor(PlacementOwner);
	}

	if (IgnoredPlacementActor != nullptr)
	{
		QueryParams.AddIgnoredActor(IgnoredPlacementActor);
	}

	TArray<const AActor*> AdditionalIgnoredActors;
	GatherPlacementOverlapIgnoredActors(CandidateTransform, PlacementOwner, IgnoredPlacementActor, AdditionalIgnoredActors);
	for (const AActor* IgnoredActor : AdditionalIgnoredActors)
	{
		if (IgnoredActor != nullptr)
		{
			QueryParams.AddIgnoredActor(IgnoredActor);
		}
	}

	const ECollisionChannel CollisionChannel = ItemMeshComponent->GetCollisionObjectType();
	const FCollisionResponseParams CollisionResponseParams(ItemMeshComponent->GetCollisionResponseToChannels());

	TArray<FHitResult> HitResults;
	if (!World->SweepMultiByChannel(
		HitResults,
		PlacementCenter,
		SweepEnd,
		CandidateTransform.GetRotation(),
		CollisionChannel,
		FCollisionShape::MakeBox(PlacementHalfExtent),
		QueryParams,
		CollisionResponseParams))
	{
		return false;
	}

	float ClosestSnapDistance = TNumericLimits<float>::Max();

	for (const FHitResult& HitResult : HitResults)
	{
		if (!HitResult.bBlockingHit || HitResult.Component.Get() == nullptr)
		{
			continue;
		}

		const AActor* const HitActor = HitResult.GetActor();
		if (HitActor != nullptr)
		{
			if (HitActor == this || HitActor == PlacementOwner || HitActor == IgnoredPlacementActor || HitActor->IsA<APawn>())
			{
				continue;
			}

			if (const AAGASSPlaceableItemActor* const HitPlaceable = Cast<AAGASSPlaceableItemActor>(HitActor))
			{
				if (HitPlaceable->IsHeldHidden())
				{
					continue;
				}
			}
		}

		ClosestSnapDistance = FMath::Min(ClosestSnapDistance, FMath::Max(HitResult.Distance, 0.f));
	}

	if (!FMath::IsFinite(ClosestSnapDistance) || ClosestSnapDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	FTransform SnappedTransform = CandidateTransform;
	SnappedTransform.AddToTranslation(FVector(0.f, 0.f, -ClosestSnapDistance));

	if (EvaluatePlacementTransform(SnappedTransform, PlacementOwner, IgnoredPlacementActor) != EAGASSPlacementValidationReason::Valid)
	{
		return false;
	}

	OutSnappedTransform = SnappedTransform;
	return true;
}

void AAGASSPlaceableItemActor::SetRuntimeItemData(const FAGASSPlaceableItemRuntimeData& NewRuntimeItemData)
{
	if (!HasAuthority())
	{
		return;
	}

	FAGASSPlaceableItemRuntimeData SanitizedRuntimeItemData = NewRuntimeItemData;
	SanitizedRuntimeItemData.bUseRuntimeData = true;
	SanitizedRuntimeItemData.SellValue = FMath::Max(SanitizedRuntimeItemData.SellValue, 0);
	if (SanitizedRuntimeItemData.bOverrideSimulatedMass)
	{
		SanitizedRuntimeItemData.SimulatedMassKg = FMath::Max(SanitizedRuntimeItemData.SimulatedMassKg, 0.001f);
	}

	if (ItemDefinition == nullptr && RuntimeItemData == SanitizedRuntimeItemData)
	{
		return;
	}

	ItemDefinition = nullptr;
	RuntimeItemData = SanitizedRuntimeItemData;
	ApplyResolvedItemVisuals();
	ApplyResolvedPhysicsTuning();
	HandleItemDataChanged();
	ForceNetUpdate();
}

UAGASSPlaceableBehaviorData* AAGASSPlaceableItemActor::GetBehaviorTuning() const
{
	return ResolveBehaviorTuning();
}

int32 AAGASSPlaceableItemActor::GetPurchaseCost() const
{
	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		return FMath::Max(Definition->PurchaseCost, 0);
	}

	return 0;
}

int32 AAGASSPlaceableItemActor::GetSellValue() const
{
	if (RuntimeItemData.IsConfigured())
	{
		return FMath::Max(RuntimeItemData.SellValue, 0);
	}

	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		return FMath::Max(Definition->SellValue, 0);
	}

	return 0;
}

bool AAGASSPlaceableItemActor::IsHeldHidden() const
{
	return IsCarried();
}

bool AAGASSPlaceableItemActor::IsPlacementCommitted() const
{
	return bPlacementCommitted;
}

void AAGASSPlaceableItemActor::BeginSpawnFallCollisionSequence()
{
	if (!HasAuthority() || IsCarried() || ItemMeshComponent == nullptr || SpawnFallCollisionComponent == nullptr)
	{
		return;
	}

	RefreshSpawnFallCollisionBounds();
	SpawnFallCollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	bSpawnFallCollisionActive = true;
}

AAGASSPlaceableItemActor* AAGASSPlaceableItemActor::GetSupportingTowerPlaceable() const
{
	return SupportingTowerPlaceable.Get();
}

void AAGASSPlaceableItemActor::GetSupportedTowerPlaceables(TArray<AAGASSPlaceableItemActor*>& OutSupportedPlaceables) const
{
	OutSupportedPlaceables.Reset();

	for (const TWeakObjectPtr<AAGASSPlaceableItemActor>& SupportedPlaceable : SupportedTowerPlaceables)
	{
		AAGASSPlaceableItemActor* const SupportedActor = SupportedPlaceable.Get();
		if (SupportedActor != nullptr && SupportedActor != this)
		{
			OutSupportedPlaceables.AddUnique(SupportedActor);
		}
	}
}

bool AAGASSPlaceableItemActor::IsPartOfTowerStack() const
{
	if (SupportingTowerPlaceable.IsValid())
	{
		return true;
	}

	for (const TWeakObjectPtr<AAGASSPlaceableItemActor>& SupportedPlaceable : SupportedTowerPlaceables)
	{
		if (SupportedPlaceable.IsValid())
		{
			return true;
		}
	}

	return false;
}

void AAGASSPlaceableItemActor::CollapseForSessionEvent(const FVector& WorldImpulse)
{
	if (!HasAuthority() || IsCarried() || ItemMeshComponent == nullptr)
	{
		return;
	}

	TArray<AAGASSPlaceableItemActor*> SupportedPlaceables;
	GetSupportedTowerPlaceables(SupportedPlaceables);
	UE_LOG(
		LogAGASSPlaceableSupport,
		Log,
		TEXT("CollapseForSessionEvent on '%s'. Location=%s Committed=%d Supporting='%s' SupportedChildren=%d"),
		*GetNameSafe(this),
		*GetActorLocation().ToCompactString(),
		bPlacementCommitted ? 1 : 0,
		*GetNameSafe(GetSupportingTowerPlaceable()),
		SupportedPlaceables.Num());

	ClearSettledDormancyTimer();
	InvalidateSupportedTowerPlaceables();
	SetSupportingTowerPlaceable(nullptr);
	SetPlacementCommitted(false);
	EndSpawnFallCollisionSequence();
	ResumeActiveReplication();
	ApplyHeldHiddenState();

	ItemMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMeshComponent->SetSimulatePhysics(true);
	ItemMeshComponent->SetEnableGravity(true);
	ItemMeshComponent->SetUseCCD(true);
	ItemMeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	ItemMeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	if (!WorldImpulse.IsNearlyZero())
	{
		ItemMeshComponent->AddImpulse(WorldImpulse, NAME_None, true);
	}
	else
	{
		ItemMeshComponent->WakeRigidBody();
	}

	ForceNetUpdate();
}

bool AAGASSPlaceableItemActor::HasTouchingPlacementSupport(
	const FTransform& CandidateTransform,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor) const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr || ItemMeshComponent == nullptr)
	{
		return false;
	}

	FVector PlacementHalfExtent = GetPreviewHalfExtent();
	PlacementHalfExtent.X = FMath::Max(PlacementHalfExtent.X, 1.f);
	PlacementHalfExtent.Y = FMath::Max(PlacementHalfExtent.Y, 1.f);
	PlacementHalfExtent.Z = FMath::Max(PlacementHalfExtent.Z, 1.f);
	PlacementHalfExtent += FVector(FMath::Max(GetPlacementTouchTolerance(), 0.f));

	const FVector PlacementCenter = CandidateTransform.TransformPosition(GetMeshDerivedLocalBoundsCenter());

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSPlacementTouchSupport), false);
	QueryParams.AddIgnoredActor(this);
	if (PlacementOwner != nullptr)
	{
		QueryParams.AddIgnoredActor(PlacementOwner);
	}

	if (IgnoredPlacementActor != nullptr)
	{
		QueryParams.AddIgnoredActor(IgnoredPlacementActor);
	}

	TArray<const AActor*> AdditionalIgnoredActors;
	GatherPlacementOverlapIgnoredActors(CandidateTransform, PlacementOwner, IgnoredPlacementActor, AdditionalIgnoredActors);
	for (const AActor* IgnoredActor : AdditionalIgnoredActors)
	{
		if (IgnoredActor != nullptr)
		{
			QueryParams.AddIgnoredActor(IgnoredActor);
		}
	}

	const ECollisionChannel CollisionChannel = ItemMeshComponent->GetCollisionObjectType();
	const FCollisionResponseParams CollisionResponseParams(ItemMeshComponent->GetCollisionResponseToChannels());

	TArray<FOverlapResult> OverlapResults;
	if (!World->OverlapMultiByChannel(
		OverlapResults,
		PlacementCenter,
		CandidateTransform.GetRotation(),
		CollisionChannel,
		FCollisionShape::MakeBox(PlacementHalfExtent),
		QueryParams,
		CollisionResponseParams))
	{
		return false;
	}

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		const UPrimitiveComponent* const OverlapComponent = OverlapResult.Component.Get();
		if (OverlapComponent == nullptr)
		{
			continue;
		}

		const AActor* const OverlapActor = OverlapResult.GetActor();
		if (OverlapActor == nullptr)
		{
			return true;
		}

		if (OverlapActor == this || OverlapActor == PlacementOwner || OverlapActor == IgnoredPlacementActor || OverlapActor->IsA<APawn>())
		{
			continue;
		}

		if (const AAGASSPlaceableItemActor* const OverlapPlaceable = Cast<AAGASSPlaceableItemActor>(OverlapActor))
		{
			if (OverlapPlaceable->IsHeldHidden())
			{
				continue;
			}
		}

		return true;
	}

	return false;
}

float AAGASSPlaceableItemActor::GetPlacementTouchTolerance() const
{
	if (const UAGASSPlaceableBehaviorData* const BehaviorTuning = ResolveBehaviorTuning())
	{
		return FMath::Max(BehaviorTuning->PlacementTouchTolerance, 0.f);
	}

	if (const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get())
	{
		return FMath::Max(TowerSettings->DefaultPlacementTouchTolerance, 0.f);
	}

	return 4.f;
}

bool AAGASSPlaceableItemActor::SupportsPlacementPitchRotation() const
{
	return true;
}

bool AAGASSPlaceableItemActor::ShouldProvideCarryPreviewCollision() const
{
	return false;
}

FTransform AAGASSPlaceableItemActor::AdjustDesiredPreviewTransform(const FTransform& DesiredTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const
{
	return DesiredTransform;
}

void AAGASSPlaceableItemActor::ConfigureCarryPreviewMesh(UStaticMeshComponent& PreviewMeshComponent) const
{
	if (const UStaticMeshComponent* const SourceMeshComponent = GetItemMeshComponent())
	{
		PreviewMeshComponent.SetCollisionProfileName(SourceMeshComponent->GetCollisionProfileName());
		PreviewMeshComponent.SetCollisionObjectType(SourceMeshComponent->GetCollisionObjectType());
		PreviewMeshComponent.CanCharacterStepUpOn = SourceMeshComponent->CanCharacterStepUpOn;
	}

	PreviewMeshComponent.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PreviewMeshComponent.SetGenerateOverlapEvents(false);
	PreviewMeshComponent.SetCanEverAffectNavigation(false);
	PreviewMeshComponent.SetSimulatePhysics(false);
	PreviewMeshComponent.SetEnableGravity(false);
}

UPrimitiveComponent* AAGASSPlaceableItemActor::GetMovementBaseComponent() const
{
	return ItemMeshComponent;
}

void AAGASSPlaceableItemActor::TransferCharacterMovementBases(UPrimitiveComponent* FromBaseComponent, UPrimitiveComponent* ToBaseComponent) const
{
	UWorld* const World = GetWorld();
	if (World == nullptr || FromBaseComponent == nullptr || FromBaseComponent == ToBaseComponent)
	{
		return;
	}

	for (TActorIterator<ACharacter> CharacterIterator(World); CharacterIterator; ++CharacterIterator)
	{
		ACharacter* const Character = *CharacterIterator;
		if (Character == nullptr || Character->GetMovementBase() != FromBaseComponent)
		{
			continue;
		}

		if (UCharacterMovementComponent* const MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->SetBase(ToBaseComponent);
		}
	}
}

void AAGASSPlaceableItemActor::OnRep_RuntimeItemData()
{
	ApplyResolvedItemVisuals();
	ApplyResolvedPhysicsTuning();
	HandleItemDataChanged();
}

void AAGASSPlaceableItemActor::OnRep_PlacementCommitted()
{
	ApplyHeldHiddenState();
}

void AAGASSPlaceableItemActor::HandleMeshComponentWake(UPrimitiveComponent* WakingComponent, const FName BoneName)
{
	if (!HasAuthority() || IsCarried())
	{
		return;
	}

	if (ItemMeshComponent != nullptr)
	{
		ItemMeshComponent->SetUseCCD(true);
	}

	ClearSettledDormancyTimer();
	ResumeActiveReplication();
	ForceNetUpdate();
}

void AAGASSPlaceableItemActor::HandleMeshComponentSleep(UPrimitiveComponent* SleepingComponent, const FName BoneName)
{
	if (ItemMeshComponent != nullptr)
	{
		ItemMeshComponent->SetUseCCD(false);
	}

	if (!HasAuthority() || IsCarried() || !ShouldUseSettledDormancy())
	{
		return;
	}

	ScheduleSettledDormancy();
}

void AAGASSPlaceableItemActor::HandleMeshComponentHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority()
		|| IsCarried()
		|| HitComponent == nullptr
		|| HitComponent != ItemMeshComponent
		|| GroundImpactSounds.IsEmpty())
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const double CurrentTimeSeconds = World->GetTimeSeconds();
	if (CurrentTimeSeconds - LastGroundImpactSoundTimeSeconds < 0.15)
	{
		return;
	}

	const float ImpactStrength = NormalImpulse.Size();
	const float CurrentSpeed = ItemMeshComponent != nullptr ? ItemMeshComponent->GetPhysicsLinearVelocity().Size() : 0.f;
	if (ImpactStrength < 15000.f && CurrentSpeed < 200.f)
	{
		return;
	}

	const int32 SoundIndex = ResolveGroundImpactSoundIndex();
	if (SoundIndex == INDEX_NONE)
	{
		return;
	}

	LastGroundImpactSoundTimeSeconds = CurrentTimeSeconds;
	const FVector SoundLocation = Hit.ImpactPoint.IsNearlyZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
	MulticastPlayGroundImpactSound(SoundLocation, SoundIndex);
}

void AAGASSPlaceableItemActor::ApplyHeldHiddenState()
{
	SetActorHiddenInGame(IsCarried());
	SetActorEnableCollision(!IsCarried());

	if (ItemMeshComponent == nullptr)
	{
		return;
	}

	ItemMeshComponent->SetCollisionEnabled(IsCarried() ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	ItemMeshComponent->SetVisibility(!IsCarried(), true);
	ItemMeshComponent->SetHiddenInGame(IsCarried(), true);
	ApplyVisiblePhysicsState(false);
}

void AAGASSPlaceableItemActor::ApplyResolvedItemVisuals()
{
	if (ItemMeshComponent == nullptr)
	{
		return;
	}

	if (UStaticMesh* const WorldStaticMesh = ResolveWorldStaticMesh())
	{
		ItemMeshComponent->SetStaticMesh(WorldStaticMesh);
	}

	RefreshSpawnFallCollisionBounds();
}

void AAGASSPlaceableItemActor::ApplyResolvedPhysicsTuning()
{
	if (ItemMeshComponent == nullptr)
	{
		return;
	}

	if (RuntimeItemData.IsConfigured())
	{
		if (RuntimeItemData.bOverrideSimulatedMass)
		{
			ItemMeshComponent->SetMassOverrideInKg(NAME_None, FMath::Max(RuntimeItemData.SimulatedMassKg, 0.001f), true);
			return;
		}

		ItemMeshComponent->SetMassOverrideInKg(NAME_None, 1.f, false);
		return;
	}

	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		if (Definition->bOverrideSimulatedMass)
		{
			ItemMeshComponent->SetMassOverrideInKg(NAME_None, FMath::Max(Definition->SimulatedMassKg, 0.001f), true);
			return;
		}
	}

	ItemMeshComponent->SetMassOverrideInKg(NAME_None, 1.f, false);
}

void AAGASSPlaceableItemActor::ApplyVisiblePhysicsState(const bool bForceWakeIfSimulating)
{
	if (ItemMeshComponent == nullptr)
	{
		return;
	}

	const bool bShouldSimulatePhysics =
		!IsCarried()
		&& !bPlacementCommitted
		&& ShouldSimulatePhysicsWhenVisible();

	if (ItemMeshComponent->IsSimulatingPhysics())
	{
		ItemMeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
		ItemMeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	ItemMeshComponent->SetSimulatePhysics(bShouldSimulatePhysics);
	ItemMeshComponent->SetEnableGravity(bShouldSimulatePhysics);

	if (bShouldSimulatePhysics)
	{
		ItemMeshComponent->SetUseCCD(true);
		ItemMeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
		ItemMeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		if (bForceWakeIfSimulating || !ItemMeshComponent->RigidBodyIsAwake())
		{
			ItemMeshComponent->WakeRigidBody();
		}
	}
	else
	{
		EndSpawnFallCollisionSequence();
		ItemMeshComponent->SetUseCCD(false);
	}
}

void AAGASSPlaceableItemActor::RefreshSpawnFallCollisionBounds()
{
	if (SpawnFallCollisionComponent == nullptr)
	{
		return;
	}

	const FVector MeshHalfExtent = GetMeshDerivedHalfExtent();
	const FVector BoxHalfExtent(
		FMath::Max(MeshHalfExtent.X, 1.f),
		FMath::Max(MeshHalfExtent.Y, 1.f),
		FMath::Max(MeshHalfExtent.Z, 1.f));

	SpawnFallCollisionComponent->SetRelativeLocation(GetMeshDerivedLocalBoundsCenter());
	SpawnFallCollisionComponent->SetBoxExtent(BoxHalfExtent, true);
}

void AAGASSPlaceableItemActor::EndSpawnFallCollisionSequence()
{
	if (SpawnFallCollisionComponent == nullptr)
	{
		bSpawnFallCollisionActive = false;
		return;
	}

	SpawnFallCollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bSpawnFallCollisionActive = false;
}

void AAGASSPlaceableItemActor::ResolveFinalPlacedEncroachment()
{
	if (!HasAuthority() || IsCarried() || ItemMeshComponent == nullptr)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (!ItemMeshComponent->IsQueryCollisionEnabled())
	{
		return;
	}

	FVector AdjustedLocation = GetActorLocation();
	const FRotator CurrentRotation = GetActorRotation();
	if (!World->FindTeleportSpot(this, AdjustedLocation, CurrentRotation))
	{
		return;
	}

	if (AdjustedLocation.Equals(GetActorLocation(), KINDA_SMALL_NUMBER))
	{
		return;
	}

	SetActorLocation(AdjustedLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AAGASSPlaceableItemActor::ClearSettledDormancyTimer()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SettledDormancyTimerHandle);
	}
}

void AAGASSPlaceableItemActor::ScheduleSettledDormancy()
{
	if (!HasAuthority() || !ShouldUseSettledDormancy())
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float DormancyDelaySeconds = GetSettledDormancyDelaySeconds();
	if (DormancyDelaySeconds <= 0.f)
	{
		CommitSettledDormancy();
		return;
	}

	World->GetTimerManager().SetTimer(SettledDormancyTimerHandle, this, &ThisClass::CommitSettledDormancy, DormancyDelaySeconds, false);
}

void AAGASSPlaceableItemActor::CommitSettledDormancy()
{
	ClearSettledDormancyTimer();

	if (!HasAuthority() || IsCarried() || !ShouldUseSettledDormancy() || ItemMeshComponent == nullptr)
	{
		return;
	}

	if (bPlacementCommitted)
	{
		ForceNetUpdate();
		SetNetDormancy(DORM_DormantAll);
		return;
	}

	const bool bPhysicsStillAwake = ItemMeshComponent->IsSimulatingPhysics() && ItemMeshComponent->RigidBodyIsAwake();
	if (!ItemMeshComponent->IsSimulatingPhysics() || bPhysicsStillAwake)
	{
		return;
	}

	ForceNetUpdate();
	SetNetDormancy(DORM_DormantAll);
}

void AAGASSPlaceableItemActor::ResumeActiveReplication()
{
	SetNetDormancy(DORM_Awake);
	FlushNetDormancy();
}

UStaticMesh* AAGASSPlaceableItemActor::ResolveWorldStaticMesh() const
{
	if (RuntimeItemData.HasConfiguredMesh())
	{
		return RuntimeItemData.WorldStaticMesh.LoadSynchronous();
	}

	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		return Definition->WorldStaticMesh.LoadSynchronous();
	}

	return nullptr;
}

UAGASSPlaceableBehaviorData* AAGASSPlaceableItemActor::ResolveBehaviorTuning() const
{
	if (RuntimeItemData.IsConfigured() && !RuntimeItemData.BehaviorTuning.IsNull())
	{
		return RuntimeItemData.BehaviorTuning.LoadSynchronous();
	}

	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		return Definition->BehaviorTuning.LoadSynchronous();
	}

	return nullptr;
}

FVector AAGASSPlaceableItemActor::GetMeshDerivedHalfExtent() const
{
	if (ItemMeshComponent == nullptr)
	{
		return FVector(25.f);
	}

	FVector BoundsMin = FVector::ZeroVector;
	FVector BoundsMax = FVector::ZeroVector;
	ItemMeshComponent->GetLocalBounds(BoundsMin, BoundsMax);

	FVector HalfExtent = (BoundsMax - BoundsMin) * 0.5f;
	HalfExtent *= ItemMeshComponent->GetComponentScale().GetAbs();
	return HalfExtent.IsNearlyZero() ? FVector(25.f) : HalfExtent;
}

FVector AAGASSPlaceableItemActor::GetMeshDerivedLocalBoundsCenter() const
{
	if (ItemMeshComponent == nullptr)
	{
		return FVector::ZeroVector;
	}

	FVector BoundsMin = FVector::ZeroVector;
	FVector BoundsMax = FVector::ZeroVector;
	ItemMeshComponent->GetLocalBounds(BoundsMin, BoundsMax);
	return (BoundsMin + BoundsMax) * 0.5f;
}

void AAGASSPlaceableItemActor::GatherPlacementOverlapIgnoredActors(
	const FTransform& CandidateTransform,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor,
	TArray<const AActor*>& OutIgnoredActors) const
{
}

float AAGASSPlaceableItemActor::GetPlacementValidationInset() const
{
	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		return FMath::Max(Definition->PlacementValidationInset, 0.f);
	}

	if (const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get())
	{
		return FMath::Max(TowerSettings->DefaultPlacementValidationInset, 0.f);
	}

	return 4.f;
}

bool AAGASSPlaceableItemActor::ShouldUseSettledDormancy() const
{
	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		return Definition->bEnterDormancyWhenSettled;
	}

	return true;
}

float AAGASSPlaceableItemActor::GetSettledDormancyDelaySeconds() const
{
	if (const UAGASSItemDefinition* const Definition = GetItemDefinition())
	{
		return FMath::Max(Definition->SettledDormancyDelaySeconds, 0.f);
	}

	return 0.25f;
}

int32 AAGASSPlaceableItemActor::ResolveGroundImpactSoundIndex() const
{
	TArray<int32> ValidIndices;
	ValidIndices.Reserve(GroundImpactSounds.Num());

	for (int32 SoundIndex = 0; SoundIndex < GroundImpactSounds.Num(); ++SoundIndex)
	{
		if (GroundImpactSounds[SoundIndex] != nullptr)
		{
			ValidIndices.Add(SoundIndex);
		}
	}

	return ValidIndices.IsEmpty() ? INDEX_NONE : ValidIndices[FMath::RandHelper(ValidIndices.Num())];
}

void AAGASSPlaceableItemActor::MulticastPlayGroundImpactSound_Implementation(const FVector_NetQuantize SoundLocation, const int32 SoundIndex)
{
	if (!GroundImpactSounds.IsValidIndex(SoundIndex) || GroundImpactSounds[SoundIndex] == nullptr)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, GroundImpactSounds[SoundIndex], SoundLocation);
}

void AAGASSPlaceableItemActor::SetPlacementCommitted(const bool bNewCommitted)
{
	bPlacementCommitted = bNewCommitted;
}

AAGASSPlaceableItemActor* AAGASSPlaceableItemActor::FindSupportingTowerPlaceableAtTransform(
	const FTransform& CandidateTransform,
	const APawn* PlacementOwner,
	const AActor* IgnoredPlacementActor) const
{
	const UWorld* const World = GetWorld();
	if (World == nullptr || ItemMeshComponent == nullptr)
	{
		return nullptr;
	}

	const float SnapDistance = FMath::Max(GetPlacementTouchTolerance(), 1.f);
	FVector PlacementHalfExtent = GetPreviewHalfExtent();
	PlacementHalfExtent.X = FMath::Max(PlacementHalfExtent.X, 1.f);
	PlacementHalfExtent.Y = FMath::Max(PlacementHalfExtent.Y, 1.f);
	PlacementHalfExtent.Z = FMath::Max(PlacementHalfExtent.Z, 1.f);

	const FVector PlacementCenter = CandidateTransform.TransformPosition(GetMeshDerivedLocalBoundsCenter());
	const FVector SweepEnd = PlacementCenter - FVector(0.f, 0.f, SnapDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AGASSPlacementSupportLink), false);
	QueryParams.AddIgnoredActor(this);

	if (PlacementOwner != nullptr)
	{
		QueryParams.AddIgnoredActor(PlacementOwner);
	}

	if (IgnoredPlacementActor != nullptr)
	{
		QueryParams.AddIgnoredActor(IgnoredPlacementActor);
	}

	TArray<const AActor*> AdditionalIgnoredActors;
	GatherPlacementOverlapIgnoredActors(CandidateTransform, PlacementOwner, IgnoredPlacementActor, AdditionalIgnoredActors);
	for (const AActor* IgnoredActor : AdditionalIgnoredActors)
	{
		if (IgnoredActor != nullptr)
		{
			QueryParams.AddIgnoredActor(IgnoredActor);
		}
	}

	const ECollisionChannel CollisionChannel = ItemMeshComponent->GetCollisionObjectType();
	const FCollisionResponseParams CollisionResponseParams(ItemMeshComponent->GetCollisionResponseToChannels());

	TArray<FHitResult> HitResults;
	if (!World->SweepMultiByChannel(
		HitResults,
		PlacementCenter,
		SweepEnd,
		CandidateTransform.GetRotation(),
		CollisionChannel,
		FCollisionShape::MakeBox(PlacementHalfExtent),
		QueryParams,
		CollisionResponseParams))
	{
		return nullptr;
	}

	AAGASSPlaceableItemActor* BestSupportPlaceable = nullptr;
	float BestSupportDistance = TNumericLimits<float>::Max();

	for (const FHitResult& HitResult : HitResults)
	{
		if (!HitResult.bBlockingHit || HitResult.Component.Get() == nullptr || HitResult.ImpactNormal.Z <= 0.25f)
		{
			continue;
		}

		AAGASSPlaceableItemActor* const HitPlaceable = Cast<AAGASSPlaceableItemActor>(HitResult.GetActor());
		if (HitPlaceable == nullptr
			|| HitPlaceable == this
			|| HitPlaceable->IsHeldHidden()
			|| !HitPlaceable->IsPlacementCommitted())
		{
			continue;
		}

		const float SupportDistance = FMath::Max(HitResult.Distance, 0.f);
		if (SupportDistance < BestSupportDistance)
		{
			BestSupportDistance = SupportDistance;
			BestSupportPlaceable = HitPlaceable;
		}
	}

	if (BestSupportPlaceable != nullptr)
	{
		UE_LOG(
			LogAGASSPlaceableSupport,
			Log,
			TEXT("Resolved support link for '%s' at %s -> '%s' (distance=%.2f)."),
			*GetNameSafe(this),
			*CandidateTransform.GetLocation().ToCompactString(),
			*GetNameSafe(BestSupportPlaceable),
			BestSupportDistance);
	}
	else
	{
		UE_LOG(
			LogAGASSPlaceableSupport,
			Log,
			TEXT("No support link found for '%s' at %s."),
			*GetNameSafe(this),
			*CandidateTransform.GetLocation().ToCompactString());
	}

	return BestSupportPlaceable;
}

void AAGASSPlaceableItemActor::SetSupportingTowerPlaceable(AAGASSPlaceableItemActor* NewSupportPlaceable)
{
	if (NewSupportPlaceable == this)
	{
		NewSupportPlaceable = nullptr;
	}

	AAGASSPlaceableItemActor* const CurrentSupportPlaceable = SupportingTowerPlaceable.Get();
	if (CurrentSupportPlaceable == NewSupportPlaceable)
	{
		return;
	}

	UE_LOG(
		LogAGASSPlaceableSupport,
		Log,
		TEXT("Updating support link for '%s': '%s' -> '%s'."),
		*GetNameSafe(this),
		*GetNameSafe(CurrentSupportPlaceable),
		*GetNameSafe(NewSupportPlaceable));

	if (CurrentSupportPlaceable != nullptr)
	{
		CurrentSupportPlaceable->RemoveSupportedTowerPlaceable(this);
	}

	SupportingTowerPlaceable = NewSupportPlaceable;

	if (NewSupportPlaceable != nullptr)
	{
		NewSupportPlaceable->AddSupportedTowerPlaceable(this);
	}
}

void AAGASSPlaceableItemActor::AddSupportedTowerPlaceable(AAGASSPlaceableItemActor* SupportedPlaceable)
{
	SupportedTowerPlaceables.RemoveAll(
		[SupportedPlaceable](const TWeakObjectPtr<AAGASSPlaceableItemActor>& ExistingPlaceable)
		{
			return !ExistingPlaceable.IsValid() || ExistingPlaceable.Get() == SupportedPlaceable;
		});

	if (SupportedPlaceable != nullptr && SupportedPlaceable != this)
	{
		SupportedTowerPlaceables.Add(SupportedPlaceable);
	}
}

void AAGASSPlaceableItemActor::RemoveSupportedTowerPlaceable(AAGASSPlaceableItemActor* SupportedPlaceable)
{
	SupportedTowerPlaceables.RemoveAll(
		[SupportedPlaceable](const TWeakObjectPtr<AAGASSPlaceableItemActor>& ExistingPlaceable)
		{
			return !ExistingPlaceable.IsValid() || ExistingPlaceable.Get() == SupportedPlaceable;
		});
}

void AAGASSPlaceableItemActor::InvalidateSupportedTowerPlaceables()
{
	TArray<AAGASSPlaceableItemActor*> CurrentSupportedPlaceables;
	GetSupportedTowerPlaceables(CurrentSupportedPlaceables);

	if (!CurrentSupportedPlaceables.IsEmpty())
	{
		UE_LOG(
			LogAGASSPlaceableSupport,
			Log,
			TEXT("Invalidating %d supported tower child link(s) for '%s'."),
			CurrentSupportedPlaceables.Num(),
			*GetNameSafe(this));
	}

	SupportedTowerPlaceables.Reset();

	for (AAGASSPlaceableItemActor* const SupportedPlaceable : CurrentSupportedPlaceables)
	{
		if (SupportedPlaceable == nullptr)
		{
			continue;
		}

		if (SupportedPlaceable->SupportingTowerPlaceable.Get() == this)
		{
			SupportedPlaceable->SupportingTowerPlaceable = nullptr;
		}

		SupportedPlaceable->InvalidateSupportedTowerPlaceables();
	}
}

bool AAGASSPlaceableItemActor::CanBeClaimedByController(const AController* Controller) const
{
	return true;
}

bool AAGASSPlaceableItemActor::IsPlacementTransformAllowedByActor(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const
{
	return true;
}

bool AAGASSPlaceableItemActor::ShouldSimulatePhysicsWhenVisible() const
{
	return true;
}

bool AAGASSPlaceableItemActor::RequiresTouchingPlacementSupportByDefault() const
{
	return true;
}

void AAGASSPlaceableItemActor::HandleItemDefinitionChanged()
{
	ApplyResolvedItemVisuals();
	ApplyResolvedPhysicsTuning();
	HandleItemDataChanged();
}

void AAGASSPlaceableItemActor::HandleItemDataChanged()
{
	ApplyHeldHiddenState();
}

void AAGASSPlaceableItemActor::HandleCarryStateChanged(const bool bNowCarried)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bNowCarried)
	{
		EndSpawnFallCollisionSequence();
	}

	if (bNowCarried || !bPlacementCommitted)
	{
		UE_LOG(
			LogAGASSPlaceableSupport,
			Log,
			TEXT("Clearing support links for '%s' because bNowCarried=%d bPlacementCommitted=%d."),
			*GetNameSafe(this),
			bNowCarried ? 1 : 0,
			bPlacementCommitted ? 1 : 0);

		InvalidateSupportedTowerPlaceables();
		SetSupportingTowerPlaceable(nullptr);
		return;
	}

	UE_LOG(
		LogAGASSPlaceableSupport,
		Log,
		TEXT("Refreshing support link after claim state change for '%s' at %s."),
		*GetNameSafe(this),
		*GetActorLocation().ToCompactString());

	SetSupportingTowerPlaceable(FindSupportingTowerPlaceableAtTransform(GetActorTransform(), nullptr, nullptr));
}

void AAGASSPlaceableItemActor::HandlePlacedAtTransform(const FTransform& ApprovedTransform)
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(
		LogAGASSPlaceableSupport,
		Log,
		TEXT("HandlePlacedAtTransform for '%s'. ApprovedTransform=%s Committed=%d"),
		*GetNameSafe(this),
		*ApprovedTransform.GetLocation().ToCompactString(),
		bPlacementCommitted ? 1 : 0);

	if (!bPlacementCommitted)
	{
		InvalidateSupportedTowerPlaceables();
		SetSupportingTowerPlaceable(nullptr);
		return;
	}

	SetSupportingTowerPlaceable(FindSupportingTowerPlaceableAtTransform(ApprovedTransform, nullptr, nullptr));
}
