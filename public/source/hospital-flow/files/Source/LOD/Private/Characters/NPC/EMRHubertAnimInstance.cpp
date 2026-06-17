#include "Characters/NPC/EMRHubertAnimInstance.h"

#include "Characters/NPC/EMRHubertCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"


namespace
{
const TCHAR* HubertStateToString_Anim(const EEMRHubertState State)
{
    switch (State)
    {
    case EEMRHubertState::HubDriver:
        return TEXT("HubDriver");
    case EEMRHubertState::NightPatrol:
        return TEXT("NightPatrol");
    case EEMRHubertState::WindowWatch:
        return TEXT("WindowWatch");
    case EEMRHubertState::ChaseEscaper:
        return TEXT("ChaseEscaper");
    case EEMRHubertState::GrabEscaper:
        return TEXT("GrabEscaper");
    case EEMRHubertState::CarryEscaper:
        return TEXT("CarryEscaper");
    case EEMRHubertState::ThrowEscaper:
        return TEXT("ThrowEscaper");
    case EEMRHubertState::ReturnEscaper:
        return TEXT("ReturnEscaper");
    default:
        return TEXT("Unknown");
    }
}
}


UEMRHubertAnimInstance::UEMRHubertAnimInstance()
{
    RefreshStateFlags();
}


void UEMRHubertAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    TryResolveOwningCharacter();
}


void UEMRHubertAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    (void)DeltaSeconds;

    if (!HubertCharacter || !CharacterMovementComponent)
    {
        TryResolveOwningCharacter();
    }

    if (CharacterMovementComponent)
    {
        GroundSpeed = CharacterMovementComponent->Velocity.Size2D();
        bIsMoving = GroundSpeed > 3.0f;
        bIsFalling = CharacterMovementComponent->IsFalling();
    }
    else
    {
        GroundSpeed = 0.0f;
        bIsMoving = false;
        bIsFalling = false;
    }

    if (HubertCharacter)
    {
        SetHubertState(HubertCharacter->GetHubertState());
    }
}


void UEMRHubertAnimInstance::SetHubertState(const EEMRHubertState InState)
{
    if (HubertState == InState)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] AnimInstance SetHubertState animInstance=%s owner=%s from=%s to=%s"),
        *GetNameSafe(this),
        *GetNameSafe(HubertCharacter),
        HubertStateToString_Anim(HubertState),
        HubertStateToString_Anim(InState));
    HubertState = InState;
    RefreshStateFlags();
}


void UEMRHubertAnimInstance::RefreshStateFlags()
{
    bIsHubDriver = HubertState == EEMRHubertState::HubDriver;
    bIsNightPatrol = HubertState == EEMRHubertState::NightPatrol;
    bIsWindowWatch = HubertState == EEMRHubertState::WindowWatch;
    bIsChaseEscaper = HubertState == EEMRHubertState::ChaseEscaper;
    bIsGrabEscaper = HubertState == EEMRHubertState::GrabEscaper;
    bIsCarryEscaper = HubertState == EEMRHubertState::CarryEscaper;
    bIsThrowEscaper = HubertState == EEMRHubertState::ThrowEscaper;
    bIsReturnEscaper = HubertState == EEMRHubertState::ReturnEscaper;
}


void UEMRHubertAnimInstance::TryResolveOwningCharacter()
{
    HubertCharacter = Cast<AEMRHubertCharacter>(TryGetPawnOwner());
    CharacterMovementComponent = HubertCharacter ? HubertCharacter->GetCharacterMovement() : nullptr;
    if (HubertCharacter)
    {
        SetHubertState(HubertCharacter->GetHubertState());
    }
}
