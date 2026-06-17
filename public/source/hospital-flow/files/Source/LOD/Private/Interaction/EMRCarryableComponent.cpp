
#include "Interaction/EMRCarryableComponent.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "LOD/EMRCollisionChannels.h"
#include "Net/UnrealNetwork.h"
#include "Engine/EngineTypes.h"
#include "Shop/EMRItemActor.h"

UEMRCarryableComponent::UEMRCarryableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}


void UEMRCarryableComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UEMRCarryableComponent, bIsCarried);
    DOREPLIFETIME(UEMRCarryableComponent, CurrentCarrierActor);
    DOREPLIFETIME(UEMRCarryableComponent, CurrentCarrierSocketName);
    DOREPLIFETIME(UEMRCarryableComponent, bLockedInPlace);
}


void UEMRCarryableComponent::SetSimulatedComponent(UPrimitiveComponent* InComponent)
{
    SimulatedComponent = InComponent;

    ApplyCarryState();
}


USkeletalMeshComponent* UEMRCarryableComponent::FindCarrierMesh(const AActor& CarrierActor, const FName& SocketName)
{
    if (const ACharacter* CarrierCharacter = Cast<ACharacter>(&CarrierActor))
    {
        if (USkeletalMeshComponent* CharacterMesh = CarrierCharacter->GetMesh())
        {
            if (SocketName.IsNone() || CharacterMesh->DoesSocketExist(SocketName))
            {
                return CharacterMesh;
            }
        }
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshes;
    CarrierActor.GetComponents(SkeletalMeshes);
    for (USkeletalMeshComponent* MeshComponent : SkeletalMeshes)
    {
        if (!MeshComponent)
        {
            continue;
        }

        if (SocketName.IsNone() || MeshComponent->DoesSocketExist(SocketName))
        {
            return MeshComponent;
        }
    }

    return nullptr;
}


void UEMRCarryableComponent::AttachToHand(USkeletalMeshComponent* HandMesh, const FName& SocketName)
{
    AttachToMesh(HandMesh, SocketName);
}


void UEMRCarryableComponent::AttachToMesh(USkeletalMeshComponent* TargetMesh, const FName& SocketName)
{
    if (!GetOwner() || !TargetMesh)
    {
        return;
    }

    if (!TargetMesh->DoesSocketExist(SocketName))
    {
        return;
    }

    CurrentCarrierActor = TargetMesh->GetOwner();
    CurrentCarrierSocketName = SocketName;

    bIsCarried = true;
    ApplyCarryState();
    UpdateCollisionResponses();
    if (SimulatedComponent)
    {
        SimulatedComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    const bool bAttached = OwnerActor->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);

    OwnerActor->SetReplicateMovement(false);

    if (bAttached)
    {
        NotifyCarrierLocalItemInteractionSound(true);
    }

    UE_LOG(LogTemp, Verbose, TEXT("[Carryable] AttachToMesh result: %s"), bAttached ? TEXT("Success") : TEXT("Fail"));
    
    ApplyLocalCarrierMeshVisibility();

    if (AEMRPlayerCharacter* CarrierCharacter = Cast<AEMRPlayerCharacter>(CurrentCarrierActor.Get()))
    {
        CarrierCharacter->NotifyCarriedItemStateChanged();
    }
}


void UEMRCarryableComponent::DropAtLocation(const FVector& TargetLocation)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    if (SimulatedComponent)
    {
        SimulatedComponent->SetSimulatePhysics(false);
        SimulatedComponent->SetEnableGravity(false);
        SimulatedComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    bIsCarried = false;
    NotifyCarrierLocalItemInteractionSound(false);
    RestoreLocalCarrierMeshVisibility();

    if (AEMRPlayerCharacter* CarrierCharacter = Cast<AEMRPlayerCharacter>(CurrentCarrierActor.Get()))
    {
        CarrierCharacter->NotifyCarriedItemStateChanged();
    }

    OwnerActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    OwnerActor->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
    OwnerActor->SetReplicateMovement(true);

    ApplyCarryState();
    UpdateCollisionResponses();

    CurrentCarrierActor = nullptr;
    CurrentCarrierSocketName = NAME_None;

    OwnerActor->ForceNetUpdate();
}

void UEMRCarryableComponent::NotifyCarrierLocalItemInteractionSound(bool bPickup) const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority())
    {
        return;
    }

    AEMRPlayerCharacter* CarrierCharacter = Cast<AEMRPlayerCharacter>(CurrentCarrierActor.Get());
    if (!CarrierCharacter)
    {
        return;
    }

    AEMRItemActor* ItemActor = Cast<AEMRItemActor>(OwnerActor);
    if (!ItemActor)
    {
        return;
    }

    USoundBase* InteractionSound = bPickup
    ? ItemActor->GetPickupInteractionSound()
    : ItemActor->GetPutDownInteractionSound();
    if (!InteractionSound)
    {
        return;
    }

    CarrierCharacter->PlayWorldSoundForAllPlayers(InteractionSound, ItemActor->GetActorLocation());
}

void UEMRCarryableComponent::SetLockedInPlace(bool bLocked)
{
    if (bLockedInPlace == bLocked)
    {
        return;
    }

    bLockedInPlace = bLocked;
    ApplyCarryState();
    UpdateCollisionResponses();
    }


bool UEMRCarryableComponent::BuildPlaceObjectEventData(const AActor* InstigatorActor, FGameplayEventData& OutEventData) const
{
    if (!InstigatorActor || PlaceObjectEventTag.IsValid() == false)
    {
        return false;
    }

    OutEventData = FGameplayEventData();
    OutEventData.EventTag = PlaceObjectEventTag;
    OutEventData.Instigator = InstigatorActor;
    OutEventData.Target = GetOwner();
    OutEventData.OptionalObject = GetOwner();

    return true;
}


bool UEMRCarryableComponent::BuildUseObjectEventData(const AActor* InstigatorActor, FGameplayEventData& OutEventData, const AActor* TargetActor) const
{
    if (!InstigatorActor || UseObjectEventTag.IsValid() == false)
    {
        return false;
    }

	OutEventData = FGameplayEventData();
    OutEventData.EventTag = UseObjectEventTag;
    OutEventData.Instigator = InstigatorActor;
    OutEventData.Target = TargetActor ? TargetActor : GetOwner();
    OutEventData.OptionalObject = GetOwner();

    return true;
}


void UEMRCarryableComponent::ApplyCarryState()
{
    if (SimulatedComponent)
    {
	     const bool bShouldSimulate = !bIsCarried && !bLockedInPlace;
	     SimulatedComponent->SetSimulatePhysics(bShouldSimulate);
	     SimulatedComponent->SetEnableGravity(bShouldSimulate);
    }
}

void UEMRCarryableComponent::UpdateCollisionResponses() const
{
    if (!SimulatedComponent)
    {
        return;
    }

    if (bIsCarried)
    {
        SimulatedComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SimulatedComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
        return;
    }

    if (bLockedInPlace)
    {
        SimulatedComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        SimulatedComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
        SimulatedComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        return;
    }

    SimulatedComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    SimulatedComponent->SetCollisionResponseToAllChannels(ECR_Block);
    SimulatedComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    SimulatedComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}


void UEMRCarryableComponent::OnRep_Carried()
{
    ApplyCarryState();
    RestoreLocalCarrierMeshVisibility();

    if (!bIsCarried)
    {
        AEMRPlayerCharacter* PreviousCarrierCharacter = Cast<AEMRPlayerCharacter>(CurrentCarrierActor.Get());
        if (AActor* OwnerActor = GetOwner())
        {
            if (!bLockedInPlace)
            {
                OwnerActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
                OwnerActor->SetReplicateMovement(true);
            }
        }
        CurrentCarrierActor = nullptr;
        CurrentCarrierSocketName = NAME_None;
        UpdateCollisionResponses();
        if (PreviousCarrierCharacter)
        {
            PreviousCarrierCharacter->NotifyCarriedItemStateChanged();
        }
        return;
    }

    if (!CurrentCarrierActor || CurrentCarrierSocketName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Carryable] OnRep_Carried missing carrier info"));
        return;
    }

    USkeletalMeshComponent* CarrierMesh = FindCarrierMesh(*CurrentCarrierActor, CurrentCarrierSocketName);

    if (!CarrierMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Carryable] OnRep_Carried missing carrier mesh"));
        return;
    }

    if (!CarrierMesh->DoesSocketExist(CurrentCarrierSocketName))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Carryable] OnRep_Carried missing socket %s"), *CurrentCarrierSocketName.ToString());
        return;
    }

	if (SimulatedComponent)
	{
		SimulatedComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}
	
    if (AActor* OwnerActor = GetOwner())
    {
        const bool bAttached = OwnerActor->AttachToComponent(CarrierMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentCarrierSocketName);
        OwnerActor->SetReplicateMovement(false);
    }

    ApplyLocalCarrierMeshVisibility();
    if (AEMRPlayerCharacter* CarrierCharacter = Cast<AEMRPlayerCharacter>(CurrentCarrierActor.Get()))
    {
        CarrierCharacter->NotifyCarriedItemStateChanged();
    }
}

void UEMRCarryableComponent::OnRep_LockedInPlace()
{
    ApplyCarryState();
    UpdateCollisionResponses();
    }

void UEMRCarryableComponent::ApplyLocalCarrierMeshVisibility()
{
    RestoreLocalCarrierMeshVisibility();

    if (!bIsCarried || !CurrentCarrierActor)
    {
        return;
    }

    const APawn* CarrierPawn = Cast<APawn>(CurrentCarrierActor.Get());
    if (!CarrierPawn || !CarrierPawn->IsLocallyControlled())
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    TArray<UMeshComponent*> MeshComponents;
    OwnerActor->GetComponents(MeshComponents);
    for (UMeshComponent* MeshComponent : MeshComponents)
    {
        if (!MeshComponent || MeshComponent->IsA<UWidgetComponent>())
        {
            continue;
        }

        LocallyHiddenMeshComponents.Add(MeshComponent);
        LocallyHiddenMeshPreviousHiddenState.Add(MeshComponent->bHiddenInGame ? 1 : 0);
        MeshComponent->SetHiddenInGame(true);
    }
}

void UEMRCarryableComponent::RestoreLocalCarrierMeshVisibility()
{
    const int32 StateCount = FMath::Min(LocallyHiddenMeshComponents.Num(), LocallyHiddenMeshPreviousHiddenState.Num());
    for (int32 Index = 0; Index < StateCount; ++Index)
    {
        UMeshComponent* MeshComponent = LocallyHiddenMeshComponents[Index].Get();
        if (!MeshComponent)
        {
            continue;
        }

        MeshComponent->SetHiddenInGame(LocallyHiddenMeshPreviousHiddenState[Index] != 0);
    }

    LocallyHiddenMeshComponents.Reset();
    LocallyHiddenMeshPreviousHiddenState.Reset();
}
