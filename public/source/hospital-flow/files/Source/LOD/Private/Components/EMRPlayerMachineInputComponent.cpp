#include "Components/EMRPlayerMachineInputComponent.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "Interaction/EMRUltrasoundMachine.h"
#include "Interaction/EMRXRayMachine.h"

UEMRPlayerMachineInputComponent::UEMRPlayerMachineInputComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEMRPlayerMachineInputComponent::SetXRayMoveLeft(bool bPressed, AEMRXRayMachine* ActiveMachine)
{
    bXRayMoveLeft = bPressed;
    UpdateXRayMoveAxis(ActiveMachine);
}

void UEMRPlayerMachineInputComponent::SetXRayMoveRight(bool bPressed, AEMRXRayMachine* ActiveMachine)
{
    bXRayMoveRight = bPressed;
    UpdateXRayMoveAxis(ActiveMachine);
}

void UEMRPlayerMachineInputComponent::SetXRayMoveForward(bool bPressed, AEMRXRayMachine* ActiveMachine)
{
    bXRayMoveForward = bPressed;
    UpdateXRayMoveAxis(ActiveMachine);
}

void UEMRPlayerMachineInputComponent::SetXRayMoveBackward(bool bPressed, AEMRXRayMachine* ActiveMachine)
{
    bXRayMoveBackward = bPressed;
    UpdateXRayMoveAxis(ActiveMachine);
}

void UEMRPlayerMachineInputComponent::UpdateXRayMoveAxis(AEMRXRayMachine* ActiveMachine)
{
    if (!ActiveMachine)
    {
        XRayMoveAxis = FVector2D::ZeroVector;
        return;
    }

    const float InputX = (bXRayMoveRight ? 1.0f : 0.0f) - (bXRayMoveLeft ? 1.0f : 0.0f);
    const float InputY = (bXRayMoveForward ? 1.0f : 0.0f) - (bXRayMoveBackward ? 1.0f : 0.0f);
    const FVector2D NewAxis(InputX, InputY);
    if (NewAxis.Equals(XRayMoveAxis, KINDA_SMALL_NUMBER))
    {
        return;
    }

    XRayMoveAxis = NewAxis;

    if (AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(GetOwner()))
    {
        PlayerCharacter->Server_SetXRayMoveInput(ActiveMachine, XRayMoveAxis.X, XRayMoveAxis.Y);
    }
}

void UEMRPlayerMachineInputComponent::ResetXRayMoveInput(AEMRXRayMachine* ActiveMachine)
{
    bXRayMoveLeft = false;
    bXRayMoveRight = false;
    bXRayMoveForward = false;
    bXRayMoveBackward = false;

    if (XRayMoveAxis.IsNearlyZero())
    {
        return;
    }

    XRayMoveAxis = FVector2D::ZeroVector;
    if (!ActiveMachine)
    {
        return;
    }

    if (AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(GetOwner()))
    {
        UE_LOG(LogTemp, Log, TEXT("[XRayInput] ResetMoveAxis"));
        PlayerCharacter->Server_SetXRayMoveInput(ActiveMachine, 0.0f, 0.0f);
    }
}

void UEMRPlayerMachineInputComponent::SetUltrasoundMoveLeft(bool bPressed, AEMRUltrasoundMachine* ActiveMachine)
{
    bUltrasoundMoveLeft = bPressed;
    UpdateUltrasoundMoveAxis(ActiveMachine);
}

void UEMRPlayerMachineInputComponent::SetUltrasoundMoveRight(bool bPressed, AEMRUltrasoundMachine* ActiveMachine)
{
    bUltrasoundMoveRight = bPressed;
    UpdateUltrasoundMoveAxis(ActiveMachine);
}

void UEMRPlayerMachineInputComponent::SetUltrasoundMoveForward(bool bPressed, AEMRUltrasoundMachine* ActiveMachine)
{
    bUltrasoundMoveForward = bPressed;
    UpdateUltrasoundMoveAxis(ActiveMachine);
}

void UEMRPlayerMachineInputComponent::SetUltrasoundMoveBackward(bool bPressed, AEMRUltrasoundMachine* ActiveMachine)
{
    bUltrasoundMoveBackward = bPressed;
    UpdateUltrasoundMoveAxis(ActiveMachine);
}

void UEMRPlayerMachineInputComponent::UpdateUltrasoundMoveAxis(AEMRUltrasoundMachine* ActiveMachine)
{
    if (!ActiveMachine)
    {
        UltrasoundMoveAxis = FVector2D::ZeroVector;
        return;
    }

    const float InputX = (bUltrasoundMoveRight ? 1.0f : 0.0f) - (bUltrasoundMoveLeft ? 1.0f : 0.0f);
    const float InputY = (bUltrasoundMoveForward ? 1.0f : 0.0f) - (bUltrasoundMoveBackward ? 1.0f : 0.0f);
    const FVector2D NewAxis(InputX, InputY);
    if (NewAxis.Equals(UltrasoundMoveAxis, KINDA_SMALL_NUMBER))
    {
        return;
    }

    UltrasoundMoveAxis = NewAxis;

    if (AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(GetOwner()))
    {
        PlayerCharacter->Server_SetUltrasoundMoveInput(ActiveMachine, UltrasoundMoveAxis.X, UltrasoundMoveAxis.Y);
    }
}

void UEMRPlayerMachineInputComponent::ResetUltrasoundMoveInput(AEMRUltrasoundMachine* ActiveMachine)
{
    bUltrasoundMoveLeft = false;
    bUltrasoundMoveRight = false;
    bUltrasoundMoveForward = false;
    bUltrasoundMoveBackward = false;

    if (UltrasoundMoveAxis.IsNearlyZero())
    {
        return;
    }

    UltrasoundMoveAxis = FVector2D::ZeroVector;
    if (!ActiveMachine)
    {
        return;
    }

    if (AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(GetOwner()))
    {
        PlayerCharacter->Server_SetUltrasoundMoveInput(ActiveMachine, 0.0f, 0.0f);
    }
}

void UEMRPlayerMachineInputComponent::HandleUltrasoundAdjustInput(float InputValue, AEMRUltrasoundMachine* ActiveMachine)
{
    if (!ActiveMachine)
    {
        UltrasoundSliderAdjustAxis = 0.0f;
        return;
    }

    const float ClampedInput = FMath::Clamp(InputValue, -1.0f, 1.0f);
    if (FMath::IsNearlyEqual(ClampedInput, UltrasoundSliderAdjustAxis, KINDA_SMALL_NUMBER))
    {
        return;
    }

    UltrasoundSliderAdjustAxis = ClampedInput;
    if (AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(GetOwner()))
    {
        PlayerCharacter->Server_SetUltrasoundSliderAdjust(ActiveMachine, UltrasoundSliderAdjustAxis);
    }
}

void UEMRPlayerMachineInputComponent::ResetUltrasoundAdjustInput(AEMRUltrasoundMachine* ActiveMachine)
{
    if (FMath::IsNearlyZero(UltrasoundSliderAdjustAxis))
    {
        return;
    }

    UltrasoundSliderAdjustAxis = 0.0f;
    if (!ActiveMachine)
    {
        return;
    }

    if (AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(GetOwner()))
    {
        PlayerCharacter->Server_SetUltrasoundSliderAdjust(ActiveMachine, 0.0f);
    }
}

