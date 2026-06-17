#include "Framework/EMRHubPlayerSlot.h"

#include "Framework/EMRHubGameState.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Characters/Player/EMRPlayerState.h"
#include "GAS/EMRTags.h"
#include "Interfaces/EMRSeatedAnimationInterface.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

AEMRHubPlayerSlot::AEMRHubPlayerSlot()
{
    bReplicates = true;
    bAlwaysRelevant = true;

    SeatAnimationTag = EMRTags::SeatAnimation::Hub;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SeatPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SeatPoint"));
    SeatPoint->SetupAttachment(SceneRoot);
}

void AEMRHubPlayerSlot::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        if (AEMRHubGameState* HubGameState = GetWorld() ? GetWorld()->GetGameState<AEMRHubGameState>() : nullptr)
        {
            HubGameState->RegisterHubSlot(this);
        }
    }
}

void AEMRHubPlayerSlot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (APlayerState* PlayerState = BoundPlayerState.Get())
    {
        PlayerState->OnPawnSet.RemoveDynamic(this, &ThisClass::HandleAssignedPawnSet);
    }

    BoundPlayerState.Reset();

    if (HasAuthority() && AssignedPlayerState)
    {
        UnseatPawn(AssignedPlayerState->GetPawn());
    }

    if (HasAuthority())
    {
        if (AEMRHubGameState* HubGameState = GetWorld() ? GetWorld()->GetGameState<AEMRHubGameState>() : nullptr)
        {
            HubGameState->UnregisterHubSlot(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

bool AEMRHubPlayerSlot::AssignPlayerState(APlayerState* NewPlayerState)
{
    if (!HasAuthority() || !NewPlayerState)
    {
        return false;
    }

    if (AssignedPlayerState == NewPlayerState)
    {
        return true;
    }

    if (AssignedPlayerState)
    {
        return false;
    }

    SetAssignedPlayerState(NewPlayerState);
    return true;
}

bool AEMRHubPlayerSlot::ClearPlayerState()
{
    if (!HasAuthority())
    {
        return false;
    }

    if (!AssignedPlayerState)
    {
        return true;
    }

    SetAssignedPlayerState(nullptr);
    return true;
}

bool AEMRHubPlayerSlot::IsOccupied() const
{
    return AssignedPlayerState != nullptr;
}

APlayerState* AEMRHubPlayerSlot::GetAssignedPlayerState() const
{
    return AssignedPlayerState;
}

AEMRPlayerState* AEMRHubPlayerSlot::GetAssignedEMRPlayerState() const
{
    return Cast<AEMRPlayerState>(AssignedPlayerState);
}

int32 AEMRHubPlayerSlot::GetSlotIndex() const
{
    return SlotIndex;
}

void AEMRHubPlayerSlot::HandleSeatAssigned_Implementation(APlayerState* NewPlayerState)
{
    if (!NewPlayerState)
    {
        return;
    }

    BoundPlayerState = NewPlayerState;
    NewPlayerState->OnPawnSet.AddDynamic(this, &ThisClass::HandleAssignedPawnSet);
    TrySeatAssignedPawn();
}

void AEMRHubPlayerSlot::HandleSeatCleared_Implementation(APlayerState* OldPlayerState)
{
    if (APlayerState* PlayerState = BoundPlayerState.Get())
    {
        PlayerState->OnPawnSet.RemoveDynamic(this, &ThisClass::HandleAssignedPawnSet);
    }

    BoundPlayerState.Reset();

    if (HasAuthority() && OldPlayerState)
    {
        UnseatPawn(OldPlayerState->GetPawn());
    }
}

void AEMRHubPlayerSlot::OnRep_AssignedPlayerState(APlayerState* PreviousPlayerState)
{
    NotifyAssignmentChanged(PreviousPlayerState, AssignedPlayerState);
}

void AEMRHubPlayerSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRHubPlayerSlot, AssignedPlayerState);
}

void AEMRHubPlayerSlot::SetAssignedPlayerState(APlayerState* NewPlayerState)
{
    if (AssignedPlayerState == NewPlayerState)
    {
        return;
    }

    APlayerState* PreviousPlayerState = AssignedPlayerState;
    AssignedPlayerState = NewPlayerState;
    OnRep_AssignedPlayerState(PreviousPlayerState);
}

void AEMRHubPlayerSlot::NotifyAssignmentChanged(APlayerState* PreviousPlayerState, APlayerState* NewPlayerState)
{
    if (PreviousPlayerState)
    {
        HandleSeatCleared(PreviousPlayerState);
    }

    if (NewPlayerState)
    {
        HandleSeatAssigned(NewPlayerState);
    }
}

void AEMRHubPlayerSlot::TrySeatAssignedPawn()
{
    if (!HasAuthority())
    {
        return;
    }

    APlayerState* PlayerState = BoundPlayerState.Get();
    if (!PlayerState)
    {
        return;
    }

    SeatPawn(PlayerState->GetPawn());
}

void AEMRHubPlayerSlot::SeatPawn(APawn* Pawn)
{
    if (!HasAuthority() || !Pawn)
    {
        return;
    }

    USceneComponent* AttachComponent = SeatAttachComponent ? SeatAttachComponent : SeatPoint;
    USceneComponent* SeatTransformComponent = SeatPoint ? SeatPoint : AttachComponent;
    if (!AttachComponent || !SeatTransformComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HubPlayerSlot] Missing seat attach component for slot %d"), SlotIndex);
        return;
    }

    const FTransform SeatTransform = SeatTransformComponent->GetComponentTransform();
    Pawn->SetActorTransform(SeatTransform, false, nullptr, ETeleportType::TeleportPhysics);
    Pawn->AttachToComponent(AttachComponent, FAttachmentTransformRules::KeepWorldTransform);

    if (AController* Controller = Pawn->GetController())
    {
        const FRotator SeatRotation = SeatTransform.GetRotation().Rotator();
        Controller->SetControlRotation(SeatRotation);

        if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
        {
            PlayerController->ClientSetRotation(SeatRotation, true);
        }
    }

    if (AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(Pawn))
    {
        if (Pawn->GetClass()->ImplementsInterface(UEMRSeatedAnimationInterface::StaticClass()))
        {
            IEMRSeatedAnimationInterface::Execute_SetSeatedAnimationTag(Pawn, SeatAnimationTag);
        }

        PlayerCharacter->RequestInstantSeatedCameraTransition();
        PlayerCharacter->SetIsSeated(true);

        if (UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement())
        {
            MovementComponent->DisableMovement();
        }
    }
}

void AEMRHubPlayerSlot::UnseatPawn(APawn* Pawn)
{
    if (!HasAuthority() || !Pawn)
    {
        return;
    }

    Pawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    if (AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(Pawn))
    {
        PlayerCharacter->RequestInstantSeatedCameraTransition();
        PlayerCharacter->SetIsSeated(false);

        if (Pawn->GetClass()->ImplementsInterface(UEMRSeatedAnimationInterface::StaticClass()))
        {
            IEMRSeatedAnimationInterface::Execute_SetSeatedAnimationTag(Pawn, FGameplayTag());
        }

        if (UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement())
        {
            MovementComponent->SetMovementMode(MOVE_Walking);
        }
    }
}

void AEMRHubPlayerSlot::HandleAssignedPawnSet(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn)
{
    if (!HasAuthority())
    {
        return;
    }

    if (PlayerState != AssignedPlayerState)
    {
        return;
    }

    if (OldPawn && OldPawn != NewPawn)
    {
        UnseatPawn(OldPawn);
    }

    SeatPawn(NewPawn);
}
