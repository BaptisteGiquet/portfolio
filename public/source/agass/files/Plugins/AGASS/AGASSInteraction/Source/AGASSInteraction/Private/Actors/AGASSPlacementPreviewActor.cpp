#include "Actors/AGASSPlacementPreviewActor.h"

#include "Actors/AGASSPlaceableItemActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAGASSPlacementPreviewActor::AAGASSPlacementPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(false);
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(30.f);
	SetMinNetUpdateFrequency(10.f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMeshComponent->SetupAttachment(SceneRoot);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetGenerateOverlapEvents(false);
	PreviewMeshComponent->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		FallbackPreviewMesh = CubeMesh.Object;
		PreviewMeshComponent->SetStaticMesh(CubeMesh.Object);
	}
}

void AAGASSPlacementPreviewActor::BeginPlay()
{
	Super::BeginPlay();

	ServerPreviewLocation = GetActorLocation();
	ServerPreviewRotation = GetActorRotation();
	bHasVisualTransform = true;
	BasePreviewMeshScale = PreviewMeshComponent != nullptr ? PreviewMeshComponent->GetRelativeScale3D() : FVector::OneVector;
	CacheAuthoredPreviewMaterials();

	RefreshVisualFromSourceItem();
	ApplyPlacementValidationVisuals(ServerPlacementValidationReason);
}

void AAGASSPlacementPreviewActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FTransform TargetTransform = GetActiveTargetTransform();
	if (!bHasVisualTransform)
	{
		ApplyVisualTransform(TargetTransform);
		bHasVisualTransform = true;
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FRotator CurrentRotation = GetActorRotation();
	const FVector TargetLocation = TargetTransform.GetLocation();
	const FRotator TargetRotation = TargetTransform.Rotator();
	const bool bUseOwnerLocalSmoothing = bUseLocalPredictedTransform && IsLocalPreviewOwner();
	const float EffectivePositionInterpolationSpeed = bUseOwnerLocalSmoothing ? OwnerLocalPositionInterpolationSpeed : PositionInterpolationSpeed;
	const float EffectiveRotationInterpolationSpeed = bUseOwnerLocalSmoothing ? OwnerLocalRotationInterpolationSpeed : RotationInterpolationSpeed;
	const float EffectiveSnapDistance = bUseOwnerLocalSmoothing ? OwnerLocalSnapDistance : SnapDistance;

	if (EffectiveSnapDistance > 0.f && FVector::DistSquared(CurrentLocation, TargetLocation) > FMath::Square(EffectiveSnapDistance))
	{
		ApplyVisualTransform(TargetTransform);
		return;
	}

	const FVector SmoothedLocation = EffectivePositionInterpolationSpeed > 0.f
		? FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaSeconds, EffectivePositionInterpolationSpeed)
		: TargetLocation;
	const FRotator SmoothedRotation = EffectiveRotationInterpolationSpeed > 0.f
		? FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, EffectiveRotationInterpolationSpeed)
		: TargetRotation;

	ApplyVisualTransform(FTransform(SmoothedRotation, SmoothedLocation));
}

void AAGASSPlacementPreviewActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, SourceItem);
	DOREPLIFETIME(ThisClass, ServerPreviewLocation);
	DOREPLIFETIME(ThisClass, ServerPreviewRotation);
	DOREPLIFETIME(ThisClass, ServerPlacementValidationReason);
}

void AAGASSPlacementPreviewActor::ApplyLocalPredictedTransform(const FTransform& NewTransform, const EAGASSPlacementValidationReason ValidationReason)
{
	bUseLocalPredictedTransform = true;
	LocalPredictedTransform = NewTransform;
	LocalPredictedValidationReason = ValidationReason;
	ApplyPlacementValidationVisuals(GetActiveValidationReason());
}

void AAGASSPlacementPreviewActor::ClearLocalPrediction()
{
	bUseLocalPredictedTransform = false;
	ApplyPlacementValidationVisuals(GetActiveValidationReason());
}

void AAGASSPlacementPreviewActor::SetReplicatedPreviewState(const FTransform& NewTransform, const EAGASSPlacementValidationReason ValidationReason)
{
	if (!HasAuthority())
	{
		return;
	}

	ServerPreviewLocation = NewTransform.GetLocation();
	ServerPreviewRotation = NewTransform.Rotator();
	ServerPlacementValidationReason = ValidationReason;

	if (!bUseLocalPredictedTransform || !IsLocalPreviewOwner())
	{
		ApplyVisualTransform(FTransform(ServerPreviewRotation, FVector(ServerPreviewLocation)));
		bHasVisualTransform = true;
	}

	ApplyPlacementValidationVisuals(GetActiveValidationReason());
	ForceNetUpdate();
}

void AAGASSPlacementPreviewActor::SetSourceItem(AAGASSPlaceableItemActor* NewSourceItem)
{
	if (!HasAuthority())
	{
		return;
	}

	SourceItem = NewSourceItem;
	RefreshVisualFromSourceItem();
	ForceNetUpdate();
}

void AAGASSPlacementPreviewActor::InitializeFromPlaceableItem(const AAGASSPlaceableItemActor* PlaceableItem)
{
	if (PlaceableItem == nullptr || PreviewMeshComponent == nullptr)
	{
		return;
	}

	CacheAuthoredPreviewMaterials();

	const UStaticMeshComponent* const SourceMeshComponent = PlaceableItem->GetItemMeshComponent();
	if (SourceMeshComponent == nullptr)
	{
		return;
	}

	UStaticMesh* const SourceStaticMesh = SourceMeshComponent->GetStaticMesh();
	if (SourceStaticMesh == nullptr)
	{
		return;
	}

	PreviewMeshComponent->SetStaticMesh(SourceStaticMesh);
	PreviewMeshComponent->SetRelativeScale3D(SourceMeshComponent->GetRelativeScale3D());

	PlaceableItem->ConfigureCarryPreviewMesh(*PreviewMeshComponent);
	ApplyPreviewBaseMaterials(*SourceMeshComponent);
	BasePreviewMeshScale = PreviewMeshComponent->GetRelativeScale3D();
	bSourceItemVisualsInitialized = true;
	bHasAppliedValidationReason = false;
	ApplyPlacementValidationVisuals(GetActiveValidationReason());
}

FTransform AAGASSPlacementPreviewActor::GetServerPreviewTransform() const
{
	return FTransform(ServerPreviewRotation, FVector(ServerPreviewLocation));
}

bool AAGASSPlacementPreviewActor::IsPlacementValid() const
{
	return GetPlacementValidationReason() == EAGASSPlacementValidationReason::Valid;
}

EAGASSPlacementValidationReason AAGASSPlacementPreviewActor::GetPlacementValidationReason() const
{
	return GetActiveValidationReason();
}

bool AAGASSPlacementPreviewActor::IsLocalPreviewOwner() const
{
	const APlayerController* const OwningController = Cast<APlayerController>(GetOwner());
	return OwningController != nullptr && OwningController->IsLocalController();
}

FTransform AAGASSPlacementPreviewActor::GetActiveTargetTransform() const
{
	if (bUseLocalPredictedTransform && IsLocalPreviewOwner())
	{
		return LocalPredictedTransform;
	}

	return FTransform(ServerPreviewRotation, FVector(ServerPreviewLocation));
}

EAGASSPlacementValidationReason AAGASSPlacementPreviewActor::GetActiveValidationReason() const
{
	if (bUseLocalPredictedTransform && IsLocalPreviewOwner())
	{
		if (ServerPlacementValidationReason != EAGASSPlacementValidationReason::Valid
			&& LocalPredictedValidationReason == EAGASSPlacementValidationReason::Valid)
		{
			return ServerPlacementValidationReason;
		}

		return LocalPredictedValidationReason;
	}

	return ServerPlacementValidationReason;
}

bool AAGASSPlacementPreviewActor::ShouldSyncPreviewStaticMeshFromSource() const
{
	if (PreviewMeshComponent == nullptr)
	{
		return false;
	}

	const UStaticMesh* const CurrentMesh = PreviewMeshComponent->GetStaticMesh();
	return CurrentMesh == nullptr || CurrentMesh == FallbackPreviewMesh;
}

void AAGASSPlacementPreviewActor::ApplyVisualTransform(const FTransform& NewTransform)
{
	SetActorLocationAndRotation(NewTransform.GetLocation(), NewTransform.Rotator(), false, nullptr, ETeleportType::None);
}

void AAGASSPlacementPreviewActor::RefreshVisualFromSourceItem()
{
	if (SourceItem != nullptr)
	{
		InitializeFromPlaceableItem(SourceItem);
	}
}

void AAGASSPlacementPreviewActor::CacheAuthoredPreviewMaterials()
{
	if (PreviewMeshComponent == nullptr || bHasCachedAuthoredPreviewMaterials)
	{
		return;
	}

	AuthoredPreviewMaterials.Reset();

	const int32 MaterialSlotCount = PreviewMeshComponent->GetNumMaterials();
	for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
	{
		AuthoredPreviewMaterials.Add(PreviewMeshComponent->GetMaterial(SlotIndex));
	}

	bHasCachedAuthoredPreviewMaterials = true;
}

void AAGASSPlacementPreviewActor::ApplyPreviewBaseMaterials(const UStaticMeshComponent& SourceMeshComponent)
{
	if (PreviewMeshComponent == nullptr)
	{
		return;
	}

	PreviewMeshComponent->EmptyOverrideMaterials();
	BasePreviewMaterials.Reset();

	const int32 MaterialSlotCount = PreviewMeshComponent->GetNumMaterials();
	if (AuthoredPreviewMaterials.Num() > 0)
	{
		BasePreviewMaterials.Reserve(MaterialSlotCount);
		for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
		{
			const int32 SourcePreviewMaterialIndex = FMath::Min(SlotIndex, AuthoredPreviewMaterials.Num() - 1);
			UMaterialInterface* const PreviewMaterial = AuthoredPreviewMaterials[SourcePreviewMaterialIndex];
			PreviewMeshComponent->SetMaterial(SlotIndex, PreviewMaterial);
			BasePreviewMaterials.Add(PreviewMaterial);
		}
		return;
	}

	BasePreviewMaterials.Reserve(MaterialSlotCount);
	for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
	{
		UMaterialInterface* const PreviewMaterial = SourceMeshComponent.GetMaterial(SlotIndex);
		PreviewMeshComponent->SetMaterial(SlotIndex, PreviewMaterial);
		BasePreviewMaterials.Add(PreviewMaterial);
	}
}

void AAGASSPlacementPreviewActor::ApplyPlacementValidationVisuals(const EAGASSPlacementValidationReason NewReason)
{
	if (PreviewMeshComponent == nullptr)
	{
		return;
	}

	if (bHasAppliedValidationReason && LastAppliedValidationReason == NewReason)
	{
		return;
	}

	bHasAppliedValidationReason = true;
	LastAppliedValidationReason = NewReason;

	const bool bIsValid = NewReason == EAGASSPlacementValidationReason::Valid;
	if (!bIsValid && LoadedInvalidOverlayMaterial == nullptr && !InvalidOverlayMaterial.IsNull())
	{
		LoadedInvalidOverlayMaterial = InvalidOverlayMaterial.LoadSynchronous();
	}

	UMaterialInterface* const ValidationMaterial = bIsValid ? nullptr : LoadedInvalidOverlayMaterial.Get();
	ApplyPreviewValidationBaseMaterial(ValidationMaterial);
	ApplyPreviewOverlayMaterial(ValidationMaterial);
	PreviewMeshComponent->SetRenderCustomDepth(!bIsValid);
	const FVector TargetPreviewScale =
		(!bIsValid && bScalePreviewWhenInvalid)
			? (BasePreviewMeshScale * FMath::Max(InvalidPreviewScaleMultiplier, 0.01f))
			: BasePreviewMeshScale;
	PreviewMeshComponent->SetRelativeScale3D(TargetPreviewScale);
	ReceivePlacementValidationChanged(NewReason);
}

void AAGASSPlacementPreviewActor::ApplyPreviewValidationBaseMaterial(UMaterialInterface* ValidationMaterial)
{
	if (PreviewMeshComponent == nullptr)
	{
		return;
	}

	const int32 MaterialSlotCount = PreviewMeshComponent->GetNumMaterials();
	if (MaterialSlotCount <= 0)
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
	{
		UMaterialInterface* MaterialToApply = ValidationMaterial;
		if (MaterialToApply == nullptr)
		{
			if (BasePreviewMaterials.Num() <= 0)
			{
				continue;
			}

			const int32 BaseMaterialIndex = FMath::Min(SlotIndex, BasePreviewMaterials.Num() - 1);
			MaterialToApply = BasePreviewMaterials[BaseMaterialIndex];
		}

		PreviewMeshComponent->SetMaterial(SlotIndex, MaterialToApply);
	}
}

void AAGASSPlacementPreviewActor::ApplyPreviewOverlayMaterial(UMaterialInterface* OverlayMaterial)
{
	if (PreviewMeshComponent == nullptr)
	{
		return;
	}

	PreviewMeshComponent->SetOverlayMaterial(OverlayMaterial);

	TArray<TObjectPtr<UMaterialInterface>>& MaterialSlotOverlays = PreviewMeshComponent->MaterialSlotsOverlayMaterial;
	const int32 MaterialSlotCount = PreviewMeshComponent->GetNumMaterials();

	if (OverlayMaterial == nullptr || MaterialSlotCount <= 0)
	{
		if (MaterialSlotOverlays.Num() > 0)
		{
			MaterialSlotOverlays.Reset();
			PreviewMeshComponent->PrecachePSOs();
			PreviewMeshComponent->MarkRenderStateDirty();
		}

		return;
	}

	bool bRequiresSlotOverlayUpdate = MaterialSlotOverlays.Num() != MaterialSlotCount;
	if (!bRequiresSlotOverlayUpdate)
	{
		for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
		{
			if (MaterialSlotOverlays[SlotIndex] != OverlayMaterial)
			{
				bRequiresSlotOverlayUpdate = true;
				break;
			}
		}
	}

	if (!bRequiresSlotOverlayUpdate)
	{
		return;
	}

	MaterialSlotOverlays.Init(OverlayMaterial, MaterialSlotCount);
	PreviewMeshComponent->PrecachePSOs();
	PreviewMeshComponent->MarkRenderStateDirty();
}

void AAGASSPlacementPreviewActor::OnRep_ServerPreviewState()
{
	if (SourceItem != nullptr && !bSourceItemVisualsInitialized)
	{
		RefreshVisualFromSourceItem();
	}

	if (!bUseLocalPredictedTransform || !IsLocalPreviewOwner())
	{
		if (!bHasVisualTransform)
		{
			ApplyVisualTransform(FTransform(ServerPreviewRotation, FVector(ServerPreviewLocation)));
			bHasVisualTransform = true;
		}
	}

	ApplyPlacementValidationVisuals(GetActiveValidationReason());
}

void AAGASSPlacementPreviewActor::OnRep_SourceItem()
{
	bSourceItemVisualsInitialized = false;
	bHasAppliedValidationReason = false;
	RefreshVisualFromSourceItem();
}
