
#include "Shop/EMRItemActor.h"

#include "Shop/EMRItemData.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRFixedPlacementComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "GAS/EMRTags.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "LOD/EMRCollisionChannels.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"


AEMRItemActor::AEMRItemActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);
	
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
    SetRootComponent(StaticMeshComponent);
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    StaticMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    StaticMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    FilledOverlayMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FilledOverlayMeshComponent"));
    FilledOverlayMeshComponent->SetupAttachment(StaticMeshComponent);
    FilledOverlayMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FilledOverlayMeshComponent->SetVisibility(false, true);
    FilledOverlayMeshComponent->SetHiddenInGame(true);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
    InteractableComponent->SetDisplayName(FText::FromString(TEXT("Item")));
    InteractableComponent->SetInteractionEventTag(EMRTags::Event::Item::Pick);
    CarryableComponent = CreateDefaultSubobject<UEMRCarryableComponent>(TEXT("CarryableComponent"));
}


void AEMRItemActor::BeginPlay()
{
    Super::BeginPlay();

    ApplyClientSafePawnCollision();

    if (StaticMeshComponent)
    {
        CarryableComponent->SetSimulatedComponent(StaticMeshComponent);
    }

    ApplyItemDataToActor();
}


void AEMRItemActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    ApplyClientSafePawnCollision();
    ApplyItemDataToActor();
}


void AEMRItemActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRItemActor, ItemOwner);
    DOREPLIFETIME(AEMRItemActor, Quantity);
    DOREPLIFETIME(AEMRItemActor, Durability);
    DOREPLIFETIME(AEMRItemActor, ItemData);
}


FGameplayEventData AEMRItemActor::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    FGameplayEventData EventData = InteractableComponent->BuildInteractionEventData(InterActor);

    if (ItemData)
    {
        EventData.OptionalObject = const_cast<UEMRItemData*>(ItemData.Get());
    }
    else
    {
        EventData.OptionalObject = const_cast<AEMRItemActor*>(this);
    }
	
    EventData.Instigator = InterActor;
    EventData.Target = const_cast<AEMRItemActor*>(this);
    EventData.EventMagnitude = Quantity;
    return EventData;
}

FText AEMRItemActor::GetInteractableDisplayName_Implementation() const
{
    if (ItemData)
    {
        return ItemData->GetItemName();
    }

    if (InteractableComponent)
    {
        return InteractableComponent->GetDisplayName();
    }

    return FText::FromString(GetName());
}

bool AEMRItemActor::IsInteractableEnabled_Implementation() const
{
    return InteractableComponent ? InteractableComponent->IsEnabled() : true;
}


void AEMRItemActor::SetItemData(const UEMRItemData* InItemData)
{
    ItemData = InItemData;

    if (InteractableComponent && ItemData)
    {
        InteractableComponent->SetDisplayName(ItemData->GetItemName());
    }

    ApplyItemDataToActor();
}


void AEMRItemActor::SetQuantity(const int32 NewQuantity)
{
	UE_LOG(LogTemp, Warning, TEXT("SetQuantity: %d"), NewQuantity)
    Quantity = NewQuantity;
}


void AEMRItemActor::SetDurability(const float NewDurability)
{
    Durability = FMath::Clamp(NewDurability, 0.f, 1.f);
}


void AEMRItemActor::SetItemOwner(AActor* NewOwner)
{
    ItemOwner = NewOwner;
}


void AEMRItemActor::OnRep_ItemData()
{
    ApplyClientSafePawnCollision();
    ApplyItemDataToActor();
}

FName AEMRItemActor::GetHandSocketName() const
{
    return ItemData ? ItemData->GetCarrySocketName() : NAME_None;
}

USoundBase* AEMRItemActor::GetPickupInteractionSound() const
{
    return ItemData ? ItemData->GetInteractionSound() : nullptr;
}

USoundBase* AEMRItemActor::GetPutDownInteractionSound() const
{
    return ItemData ? ItemData->GetInteractionSound() : nullptr;
}

void AEMRItemActor::ApplyFilledOverlayToFirstPersonPreview(
    UStaticMeshComponent* PreviewOverlayMeshComponent,
    UMaterialInstanceDynamic*& InOutPreviewOverlayMID,
    UStaticMesh*& InOutCachedPreviewOverlayMesh,
    UMaterialInterface*& InOutCachedPreviewOverlayMaterial) const
{
    if (!PreviewOverlayMeshComponent)
    {
        return;
    }

    UStaticMesh* OverlayMesh = nullptr;
    UMaterialInterface* OverlayMaterial = nullptr;
    FTransform OverlayRelativeTransform = FTransform::Identity;
    FName FillParameterName = NAME_None;
    float OverlayFillAmount = 0.0f;
    if (!GetFilledOverlayPreviewData(
        OverlayMesh,
        OverlayMaterial,
        OverlayRelativeTransform,
        FillParameterName,
        OverlayFillAmount))
    {
        ClearFilledOverlayFirstPersonPreview(
            PreviewOverlayMeshComponent,
            InOutPreviewOverlayMID,
            InOutCachedPreviewOverlayMesh,
            InOutCachedPreviewOverlayMaterial);
        return;
    }

    const bool bMeshChanged = InOutCachedPreviewOverlayMesh != OverlayMesh;
    if (bMeshChanged)
    {
        PreviewOverlayMeshComponent->SetStaticMesh(OverlayMesh);
        InOutCachedPreviewOverlayMesh = OverlayMesh;
    }

    const bool bMaterialChanged = InOutCachedPreviewOverlayMaterial != OverlayMaterial;
    if (bMaterialChanged)
    {
        InOutCachedPreviewOverlayMaterial = OverlayMaterial;
        PreviewOverlayMeshComponent->SetMaterial(0, OverlayMaterial);
    }

    if (bMeshChanged || bMaterialChanged || !InOutPreviewOverlayMID)
    {
        InOutPreviewOverlayMID = PreviewOverlayMeshComponent->CreateDynamicMaterialInstance(0);
    }

    if (InOutPreviewOverlayMID && FillParameterName != NAME_None)
    {
        InOutPreviewOverlayMID->SetScalarParameterValue(
            FillParameterName,
            FMath::Clamp(OverlayFillAmount, 0.0f, 1.0f));
    }

    PreviewOverlayMeshComponent->SetRelativeTransform(OverlayRelativeTransform);
    PreviewOverlayMeshComponent->SetHiddenInGame(false);
    PreviewOverlayMeshComponent->SetVisibility(true, true);
}

void AEMRItemActor::ClearFilledOverlayFirstPersonPreview(
    UStaticMeshComponent* PreviewOverlayMeshComponent,
    UMaterialInstanceDynamic*& InOutPreviewOverlayMID,
    UStaticMesh*& InOutCachedPreviewOverlayMesh,
    UMaterialInterface*& InOutCachedPreviewOverlayMaterial)
{
    if (!PreviewOverlayMeshComponent)
    {
        return;
    }

    PreviewOverlayMeshComponent->SetStaticMesh(nullptr);
    PreviewOverlayMeshComponent->SetRelativeTransform(FTransform::Identity);
    PreviewOverlayMeshComponent->SetHiddenInGame(true);
    PreviewOverlayMeshComponent->SetVisibility(false, true);
    InOutPreviewOverlayMID = nullptr;
    InOutCachedPreviewOverlayMesh = nullptr;
    InOutCachedPreviewOverlayMaterial = nullptr;
}

bool AEMRItemActor::GetFilledOverlayPreviewData(
    UStaticMesh*& OutOverlayMesh,
    UMaterialInterface*& OutOverlayMaterial,
    FTransform& OutOverlayRelativeTransform,
    FName& OutFillParameterName,
    float& OutNormalizedFillAmount) const
{
    OutOverlayMesh = nullptr;
    OutOverlayMaterial = nullptr;
    OutOverlayRelativeTransform = FTransform::Identity;
    OutFillParameterName = NAME_None;
    OutNormalizedFillAmount = 0.0f;

    if (!ItemData || !ItemData->ShouldUseFilledOverlayVisual())
    {
        return false;
    }

    UStaticMesh* OverlayMesh = ItemData->GetFilledOverlayMesh();
    if (!OverlayMesh)
    {
        return false;
    }

    OutOverlayMesh = OverlayMesh;
    OutOverlayMaterial = ItemData->GetFilledOverlayMaterial();
    OutOverlayRelativeTransform = ItemData->GetFilledOverlayRelativeTransform();
    OutFillParameterName = ItemData->GetFilledOverlayParameterName();
    OutNormalizedFillAmount = FilledOverlayAmount;
    return true;
}


bool AEMRItemActor::Pickup(USkeletalMeshComponent* HandMesh)
{
	if (!HasAuthority() || !CarryableComponent || !HandMesh)
	{
				return false;
	}

	if (CarryableComponent->IsCarried())
	{
				return false;
	}

	const FName CarrySocketName = GetHandSocketName();
	if (CarrySocketName.IsNone())
	{
				return false;
	}
	
	CarryableComponent->AttachToHand(HandMesh, CarrySocketName);

	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	
	return true;
}


void AEMRItemActor::DropAtLocation(const FVector& TargetLocation)
{
    if (!CarryableComponent)
    {
    	return;
    }

    CarryableComponent->DropAtLocation(TargetLocation);
    SetItemOwner(nullptr);
}


void AEMRItemActor::ApplyItemDataToMesh()
{
    if (!ItemData)
    {
        return;
    }

    if (StaticMeshComponent)
    {
        if (UStaticMesh* NewMesh = ItemData->GetWorldItemMesh())
        {
            StaticMeshComponent->SetStaticMesh(NewMesh);
        }
    }
}

void AEMRItemActor::ApplyClientSafePawnCollision()
{
    if (!StaticMeshComponent)
    {
        return;
    }

    StaticMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    StaticMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}


void AEMRItemActor::ApplyItemDataToActor()
{
    if (CarryableComponent && ItemData)
    {
        CarryableComponent->SetUseObjectEventTag(ItemData->GetUseItemTag());
    }

    ApplyItemDataToMesh();
    ApplyFilledOverlayVisual();
}

void AEMRItemActor::ApplyFilledOverlayVisual()
{
    if (!FilledOverlayMeshComponent)
    {
        return;
    }

    FilledOverlayMID = nullptr;
    FilledOverlayAmount = 0.0f;

    if (!ItemData || !ItemData->ShouldUseFilledOverlayVisual())
    {
        FilledOverlayMeshComponent->SetStaticMesh(nullptr);
        FilledOverlayMeshComponent->SetRelativeTransform(FTransform::Identity);
        FilledOverlayMeshComponent->SetVisibility(false, true);
        FilledOverlayMeshComponent->SetHiddenInGame(true);
        return;
    }

    UStaticMesh* OverlayMesh = ItemData->GetFilledOverlayMesh();
    if (!OverlayMesh)
    {
        FilledOverlayMeshComponent->SetStaticMesh(nullptr);
        FilledOverlayMeshComponent->SetRelativeTransform(FTransform::Identity);
        FilledOverlayMeshComponent->SetVisibility(false, true);
        FilledOverlayMeshComponent->SetHiddenInGame(true);
        return;
    }

    FilledOverlayMeshComponent->SetStaticMesh(OverlayMesh);
    FilledOverlayMeshComponent->SetRelativeTransform(ItemData->GetFilledOverlayRelativeTransform());
    FilledOverlayMeshComponent->SetVisibility(true, true);
    FilledOverlayMeshComponent->SetHiddenInGame(false);

    if (UMaterialInterface* OverlayMaterial = ItemData->GetFilledOverlayMaterial())
    {
        FilledOverlayMeshComponent->SetMaterial(0, OverlayMaterial);
    }

    FilledOverlayMID = FilledOverlayMeshComponent->CreateDynamicMaterialInstance(0);
    const FName FillParam = ItemData->GetFilledOverlayParameterName();
    FilledOverlayAmount = FMath::Clamp(ItemData->GetFilledOverlayAmount(), 0.0f, 1.0f);
    if (FilledOverlayMID && FillParam != NAME_None)
    {
        FilledOverlayMID->SetScalarParameterValue(FillParam, FilledOverlayAmount);
    }
}

void AEMRItemActor::SetFilledOverlayAmount(const float NormalizedFillAmount)
{
    FilledOverlayAmount = FMath::Clamp(NormalizedFillAmount, 0.0f, 1.0f);

    if (!ItemData || !FilledOverlayMID)
    {
        return;
    }

    const FName FillParam = ItemData->GetFilledOverlayParameterName();
    if (FillParam == NAME_None)
    {
        return;
    }

    FilledOverlayMID->SetScalarParameterValue(FillParam, FilledOverlayAmount);
}
