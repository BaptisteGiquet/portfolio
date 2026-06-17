#include "Interaction/EMROxygenMask.h"

#include "GAS/EMRTags.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Interaction/EMROxygenMachine.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

AEMROxygenMask::AEMROxygenMask()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AEMROxygenMask::BeginPlay()
{
    Super::BeginPlay();

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        Carryable->SetUseObjectEventTag(EMRTags::Abilities::Treatment::Oxygen);
        Carryable->SetPlaceObjectEventTag(FGameplayTag());
    }

    if (UEMRInteractableComponent* Interactable = FindInteractable())
    {
        Interactable->SetInteractionEventTag(EMRTags::Event::Item::Pick);
        if (Interactable->GetDisplayName().IsEmpty())
        {
            Interactable->SetDisplayName(FText::FromString(TEXT("Oxygen Mask")));
        }
    }

    if (HasAuthority() && OwningMachine)
    {
        AttachToMachineHome();
    }

    HandleMaskStateChanged();
}

void AEMROxygenMask::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority())
    {
        return;
    }

    UEMRCarryableComponent* Carryable = GetCarryableComponent();
    const bool bIsCarriedNow = Carryable && Carryable->IsCarried();
    if (bIsCarriedNow && MaskState != EEMROxygenMaskState::Carried)
    {
        MaskState = EEMROxygenMaskState::Carried;
        HandleMaskStateChanged();
        ForceNetUpdate();
    }

}

void AEMROxygenMask::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMROxygenMask, MaskState);
}

bool AEMROxygenMask::IsInteractableEnabled_Implementation() const
{
    if (!Super::IsInteractableEnabled_Implementation())
    {
        return false;
    }

    if (MaskState != EEMROxygenMaskState::AtMachine)
    {
        return false;
    }

    if (OwningMachine && OwningMachine->IsMoving())
    {
        return false;
    }

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        if (Carryable->IsCarried())
        {
            return false;
        }
    }

    return true;
}

void AEMROxygenMask::AttachToMachineHome()
{
    if (!HasAuthority() || !OwningMachine)
    {
        return;
    }

    USceneComponent* Anchor = OwningMachine->GetMaskHomeAnchor();
    if (!Anchor)
    {
        return;
    }

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        Carryable->SetLockedInPlace(true);
        Carryable->DropAtLocation(Anchor->GetComponentLocation());
    }

    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    AttachToComponent(Anchor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    SetActorRelativeTransform(FTransform::Identity);
    SetReplicateMovement(false);

    MaskState = EEMROxygenMaskState::AtMachine;
    HandleMaskStateChanged();
    ForceNetUpdate();
}

void AEMROxygenMask::AttachToPatient(USkeletalMeshComponent* PatientMesh, const FName& SocketName, const FTransform& RelativeOffset)
{
    if (!HasAuthority() || !PatientMesh || SocketName.IsNone())
    {
        return;
    }

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        Carryable->SetLockedInPlace(true);
        Carryable->DropAtLocation(PatientMesh->GetSocketLocation(SocketName));
    }

    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    AttachToComponent(PatientMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
    SetActorRelativeTransform(RelativeOffset);
    SetReplicateMovement(false);

    MaskState = EEMROxygenMaskState::AttachedToPatient;
    HandleMaskStateChanged();
    ForceNetUpdate();
}

void AEMROxygenMask::ReturnToMachine()
{
    AttachToMachineHome();
}

bool AEMROxygenMask::IsCarriedBy(const AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    return GetAttachParentActor() == Actor;
}

void AEMROxygenMask::HandleMaskStateChanged()
{
    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        const bool bShouldBeLocked = MaskState == EEMROxygenMaskState::AtMachine
            || MaskState == EEMROxygenMaskState::AttachedToPatient;
        Carryable->SetLockedInPlace(bShouldBeLocked);
    }

    if (UEMRInteractableComponent* Interactable = FindInteractable())
    {
        const bool bEnabled = (MaskState == EEMROxygenMaskState::AtMachine)
            && (!OwningMachine || !OwningMachine->IsMoving());
        Interactable->SetEnabled(bEnabled);
    }
}

void AEMROxygenMask::OnRep_MaskState()
{
    HandleMaskStateChanged();
}

UEMRInteractableComponent* AEMROxygenMask::FindInteractable() const
{
    return FindComponentByClass<UEMRInteractableComponent>();
}
