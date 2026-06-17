#include "Actors/AGASSUsableItemActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/AGASSItemDefinition.h"
#include "Data/AGASSUsableItemDefinition.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/AGASSCarriedUsablePresentationInterface.h"
#include "Animation/AnimMontage.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSUsableItem, Log, All);

AAGASSUsableItemActor::AAGASSUsableItemActor()
{
	ItemMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMeshComponent);

	ItemMeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	ItemMeshComponent->SetGenerateOverlapEvents(false);
	ItemMeshComponent->SetCanEverAffectNavigation(false);
	ItemMeshComponent->SetSimulatePhysics(true);
	ItemMeshComponent->SetIsReplicated(true);

	FirstPersonItemMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonItemMesh"));
	FirstPersonItemMeshComponent->SetupAttachment(GetRootComponent());
	FirstPersonItemMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	FirstPersonItemMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonItemMeshComponent->SetGenerateOverlapEvents(false);
	FirstPersonItemMeshComponent->SetCanEverAffectNavigation(false);
	FirstPersonItemMeshComponent->SetSimulatePhysics(false);
	FirstPersonItemMeshComponent->SetOnlyOwnerSee(true);
	FirstPersonItemMeshComponent->SetCastShadow(false);
	FirstPersonItemMeshComponent->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonItemMeshComponent->SetVisibility(false, true);
	FirstPersonItemMeshComponent->SetHiddenInGame(true, true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		ItemMeshComponent->SetStaticMesh(CubeMesh.Object);
		FirstPersonItemMeshComponent->SetStaticMesh(CubeMesh.Object);
	}
}

void AAGASSUsableItemActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyResolvedItemVisuals();
	ApplyCarryState();
}

EAGASSCarriedItemType AAGASSUsableItemActor::GetCarriedItemType() const
{
	return EAGASSCarriedItemType::Usable;
}

bool AAGASSUsableItemActor::TryUse(AController* UsingController)
{
	return false;
}

bool AAGASSUsableItemActor::ConsumeOnSuccessfulUse() const
{
	if (const UAGASSUsableItemDefinition* const UsableDefinition = GetUsableItemDefinition())
	{
		return UsableDefinition->bConsumeOnSuccessfulUse;
	}

	return bConsumeOnSuccessfulUse;
}

const UAGASSUsableItemDefinition* AAGASSUsableItemActor::GetUsableItemDefinition() const
{
	return Cast<UAGASSUsableItemDefinition>(GetItemDefinition());
}

FName AAGASSUsableItemActor::GetCarryAttachSocketName() const
{
	if (const UAGASSUsableItemDefinition* const UsableDefinition = GetUsableItemDefinition())
	{
		return !UsableDefinition->CarryAttachSocketName.IsNone()
			? UsableDefinition->CarryAttachSocketName
			: LegacyCarryAttachSocketName;
	}

	return LegacyCarryAttachSocketName;
}

FTransform AAGASSUsableItemActor::GetCarryAttachRelativeTransform() const
{
	if (const UAGASSUsableItemDefinition* const UsableDefinition = GetUsableItemDefinition())
	{
		return UsableDefinition->CarryAttachRelativeTransform;
	}

	return LegacyCarryAttachRelativeTransform;
}

FName AAGASSUsableItemActor::GetCarryAnimationId() const
{
	if (const UAGASSUsableItemDefinition* const UsableDefinition = GetUsableItemDefinition())
	{
		return !UsableDefinition->CarryAnimationId.IsNone()
			? UsableDefinition->CarryAnimationId
			: LegacyCarryAnimationId;
	}

	return LegacyCarryAnimationId;
}

UAnimMontage* AAGASSUsableItemActor::GetUseMontage() const
{
	if (const UAGASSUsableItemDefinition* const UsableDefinition = GetUsableItemDefinition())
	{
		return UsableDefinition->UseMontage.LoadSynchronous();
	}

	return LegacyUseMontage.LoadSynchronous();
}

void AAGASSUsableItemActor::PlayUseMontageForController(AController* UsingController, const float PlayRate) const
{
	if (UsingController == nullptr)
	{
		return;
	}

	APawn* const CarrierPawn = UsingController->GetPawn();
	if (CarrierPawn == nullptr)
	{
		return;
	}

	if (IAGASSCarriedUsablePresentationInterface* const PresentationInterface =
			Cast<IAGASSCarriedUsablePresentationInterface>(CarrierPawn))
	{
		PresentationInterface->PlayAGASSCarriedUsableUseMontage(GetUseMontage(), PlayRate);
	}
}

void AAGASSUsableItemActor::HandleItemDefinitionChanged()
{
	ApplyResolvedItemVisuals();
}

void AAGASSUsableItemActor::HandleCarryStarted(AController* ClaimingController)
{
	CurrentCarrierPawn = ClaimingController != nullptr ? ClaimingController->GetPawn() : nullptr;
	ApplyCarryState();
	AttachToCarrierPawn(CurrentCarrierPawn.Get());
	AttachFirstPersonPresentationToCarrierPawn(CurrentCarrierPawn.Get());

	if (IAGASSCarriedUsablePresentationInterface* const PresentationInterface =
			Cast<IAGASSCarriedUsablePresentationInterface>(CurrentCarrierPawn.Get()))
	{
		PresentationInterface->SetAGASSCarriedUsablePresentation(this, GetCarryAnimationId());
	}
}

void AAGASSUsableItemActor::HandleCarryEnded()
{
	if (IAGASSCarriedUsablePresentationInterface* const PresentationInterface =
			Cast<IAGASSCarriedUsablePresentationInterface>(CurrentCarrierPawn.Get()))
	{
		PresentationInterface->ClearAGASSCarriedUsablePresentation(this);
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ResetFirstPersonPresentationAttachment();
	if (ItemMeshComponent != nullptr)
	{
		ItemMeshComponent->SetOwnerNoSee(false);
	}

	if (FirstPersonItemMeshComponent != nullptr)
	{
		FirstPersonItemMeshComponent->SetHiddenInGame(true, true);
		FirstPersonItemMeshComponent->SetVisibility(false, true);
	}

	HandleFirstPersonCarryPresentationUpdated();
	CurrentCarrierPawn.Reset();
}

void AAGASSUsableItemActor::HandleCarryStateChanged(const bool bNowCarried)
{
	if (bNowCarried)
	{
		if (!CurrentCarrierPawn.IsValid() && HoldingPlayerState != nullptr)
		{
			CurrentCarrierPawn = HoldingPlayerState->GetPawn();
		}

		ApplyCarryState();
		AttachToCarrierPawn(CurrentCarrierPawn.Get());
		AttachFirstPersonPresentationToCarrierPawn(CurrentCarrierPawn.Get());
		return;
	}

	if (GetAttachParentActor() != nullptr)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	ResetFirstPersonPresentationAttachment();
	CurrentCarrierPawn.Reset();
	ApplyCarryState();
}

void AAGASSUsableItemActor::ApplyResolvedItemVisuals()
{
	if (ItemMeshComponent == nullptr)
	{
		return;
	}

	if (const UAGASSItemDefinition* const ResolvedItemDefinition = GetItemDefinition())
	{
		if (UStaticMesh* const WorldStaticMesh = ResolvedItemDefinition->WorldStaticMesh.LoadSynchronous())
		{
			ItemMeshComponent->SetStaticMesh(WorldStaticMesh);
			FirstPersonItemMeshComponent->SetStaticMesh(WorldStaticMesh);
		}
	}

	for (int32 MaterialIndex = 0; MaterialIndex < ItemMeshComponent->GetNumMaterials(); ++MaterialIndex)
	{
		FirstPersonItemMeshComponent->SetMaterial(MaterialIndex, ItemMeshComponent->GetMaterial(MaterialIndex));
	}
}

void AAGASSUsableItemActor::ApplyCarryState()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(!IsCarried());

	if (ItemMeshComponent == nullptr)
	{
		return;
	}

	const bool bUseFirstPersonCarryPresentation = IsUsingFirstPersonCarryPresentation();
	ItemMeshComponent->SetCollisionEnabled(IsCarried() ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	ItemMeshComponent->SetVisibility(true, true);
	ItemMeshComponent->SetHiddenInGame(false, true);
	ItemMeshComponent->SetOwnerNoSee(bUseFirstPersonCarryPresentation);
	ItemMeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	ItemMeshComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	ItemMeshComponent->SetSimulatePhysics(!IsCarried());
	ItemMeshComponent->SetEnableGravity(!IsCarried());

	if (FirstPersonItemMeshComponent != nullptr)
	{
		FirstPersonItemMeshComponent->SetHiddenInGame(!bUseFirstPersonCarryPresentation, true);
		FirstPersonItemMeshComponent->SetVisibility(bUseFirstPersonCarryPresentation, true);
	}

	if (!IsCarried())
	{
		ItemMeshComponent->WakeRigidBody();
	}

	HandleFirstPersonCarryPresentationUpdated();
}

void AAGASSUsableItemActor::AttachToCarrierPawn(APawn* CarrierPawn)
{
	if (CarrierPawn == nullptr)
	{
		return;
	}

	USceneComponent* AttachComponent = nullptr;
	if (IAGASSCarriedUsablePresentationInterface* const PresentationInterface =
			Cast<IAGASSCarriedUsablePresentationInterface>(CarrierPawn))
	{
		AttachComponent = PresentationInterface->GetAGASSCarriedUsableAttachMesh();
	}

	if (AttachComponent == nullptr)
	{
		AttachComponent = CarrierPawn->GetRootComponent();
	}

	if (AttachComponent == nullptr)
	{
		return;
	}

	FName AttachSocketName = GetCarryAttachSocketName();
	if (const USkeletalMeshComponent* const SkeletalMeshComponent = Cast<USkeletalMeshComponent>(AttachComponent))
	{
		const bool bSocketExists = AttachSocketName.IsNone()
			|| SkeletalMeshComponent->DoesSocketExist(AttachSocketName)
			|| SkeletalMeshComponent->GetBoneIndex(AttachSocketName) != INDEX_NONE;
		if (!bSocketExists)
		{
			UE_LOG(
				LogAGASSUsableItem,
				Warning,
				TEXT("Usable item '%s' could not find socket or bone '%s' on '%s'. Falling back to the mesh root."),
				*GetNameSafe(this),
				*AttachSocketName.ToString(),
				*GetNameSafe(SkeletalMeshComponent));
			AttachSocketName = NAME_None;
		}
	}
	else if (!AttachSocketName.IsNone())
	{
		UE_LOG(
			LogAGASSUsableItem,
			Warning,
			TEXT("Usable item '%s' requested socket '%s' on non-skeletal component '%s'. Falling back to the component root."),
			*GetNameSafe(this),
			*AttachSocketName.ToString(),
			*GetNameSafe(AttachComponent));
		AttachSocketName = NAME_None;
	}

	AttachToComponent(AttachComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);
	SetActorRelativeTransform(GetCarryAttachRelativeTransform());
}

void AAGASSUsableItemActor::AttachFirstPersonPresentationToCarrierPawn(APawn* CarrierPawn)
{
	if (FirstPersonItemMeshComponent == nullptr)
	{
		return;
	}

	if (!CanUseFirstPersonCarryPresentation(CarrierPawn))
	{
		ResetFirstPersonPresentationAttachment();
		HandleFirstPersonCarryPresentationUpdated();
		return;
	}

	USkeletalMeshComponent* AttachComponent = nullptr;
	if (IAGASSCarriedUsablePresentationInterface* const PresentationInterface =
			Cast<IAGASSCarriedUsablePresentationInterface>(CarrierPawn))
	{
		AttachComponent = PresentationInterface->GetAGASSLocalViewCarriedUsableAttachMesh();
	}

	if (AttachComponent == nullptr)
	{
		ResetFirstPersonPresentationAttachment();
		HandleFirstPersonCarryPresentationUpdated();
		return;
	}

	FName AttachSocketName = GetCarryAttachSocketName();
	const bool bSocketExists = AttachSocketName.IsNone()
		|| AttachComponent->DoesSocketExist(AttachSocketName)
		|| AttachComponent->GetBoneIndex(AttachSocketName) != INDEX_NONE;
	if (!bSocketExists)
	{
		UE_LOG(
			LogAGASSUsableItem,
			Warning,
			TEXT("First-person usable presentation for '%s' could not find socket or bone '%s' on '%s'. Falling back to the mesh root."),
			*GetNameSafe(this),
			*AttachSocketName.ToString(),
			*GetNameSafe(AttachComponent));
		AttachSocketName = NAME_None;
	}

	FirstPersonItemMeshComponent->AttachToComponent(
		AttachComponent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName);
	FirstPersonItemMeshComponent->SetRelativeTransform(GetCarryAttachRelativeTransform());
	HandleFirstPersonCarryPresentationUpdated();
}

void AAGASSUsableItemActor::ResetFirstPersonPresentationAttachment()
{
	if (FirstPersonItemMeshComponent == nullptr)
	{
		return;
	}

	FirstPersonItemMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	FirstPersonItemMeshComponent->AttachToComponent(
		GetRootComponent(),
		FAttachmentTransformRules::KeepRelativeTransform);
	FirstPersonItemMeshComponent->SetRelativeTransform(FTransform::Identity);
}

bool AAGASSUsableItemActor::CanUseFirstPersonCarryPresentation(APawn* CarrierPawn) const
{
	if (!IsCarried() || CarrierPawn == nullptr || !CarrierPawn->IsLocallyControlled())
	{
		return false;
	}

	if (const IAGASSCarriedUsablePresentationInterface* const PresentationInterface =
			Cast<IAGASSCarriedUsablePresentationInterface>(CarrierPawn))
	{
		return PresentationInterface->GetAGASSLocalViewCarriedUsableAttachMesh() != nullptr;
	}

	return false;
}

bool AAGASSUsableItemActor::IsUsingFirstPersonCarryPresentation() const
{
	return CanUseFirstPersonCarryPresentation(CurrentCarrierPawn.Get());
}

void AAGASSUsableItemActor::HandleFirstPersonCarryPresentationUpdated()
{
}
