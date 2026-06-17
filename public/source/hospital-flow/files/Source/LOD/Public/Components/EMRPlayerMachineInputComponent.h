#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EMRPlayerMachineInputComponent.generated.h"

class AEMRXRayMachine;
class AEMRUltrasoundMachine;

UCLASS(ClassGroup = (EMR), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRPlayerMachineInputComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEMRPlayerMachineInputComponent();

    void SetXRayMoveLeft(bool bPressed, AEMRXRayMachine* ActiveMachine);
    void SetXRayMoveRight(bool bPressed, AEMRXRayMachine* ActiveMachine);
    void SetXRayMoveForward(bool bPressed, AEMRXRayMachine* ActiveMachine);
    void SetXRayMoveBackward(bool bPressed, AEMRXRayMachine* ActiveMachine);
    void ResetXRayMoveInput(AEMRXRayMachine* ActiveMachine);

    void SetUltrasoundMoveLeft(bool bPressed, AEMRUltrasoundMachine* ActiveMachine);
    void SetUltrasoundMoveRight(bool bPressed, AEMRUltrasoundMachine* ActiveMachine);
    void SetUltrasoundMoveForward(bool bPressed, AEMRUltrasoundMachine* ActiveMachine);
    void SetUltrasoundMoveBackward(bool bPressed, AEMRUltrasoundMachine* ActiveMachine);
    void ResetUltrasoundMoveInput(AEMRUltrasoundMachine* ActiveMachine);

    void HandleUltrasoundAdjustInput(float InputValue, AEMRUltrasoundMachine* ActiveMachine);
    void ResetUltrasoundAdjustInput(AEMRUltrasoundMachine* ActiveMachine);

private:
    void UpdateXRayMoveAxis(AEMRXRayMachine* ActiveMachine);
    void UpdateUltrasoundMoveAxis(AEMRUltrasoundMachine* ActiveMachine);

    bool bXRayMoveLeft = false;
    bool bXRayMoveRight = false;
    bool bXRayMoveForward = false;
    bool bXRayMoveBackward = false;
    FVector2D XRayMoveAxis = FVector2D::ZeroVector;

    bool bUltrasoundMoveLeft = false;
    bool bUltrasoundMoveRight = false;
    bool bUltrasoundMoveForward = false;
    bool bUltrasoundMoveBackward = false;
    FVector2D UltrasoundMoveAxis = FVector2D::ZeroVector;
    float UltrasoundSliderAdjustAxis = 0.0f;
};

