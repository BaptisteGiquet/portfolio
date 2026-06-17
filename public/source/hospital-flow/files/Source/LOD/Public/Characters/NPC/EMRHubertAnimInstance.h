#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Characters/NPC/EMRHubertCharacter.h"
#include "EMRHubertAnimInstance.generated.h"

class AEMRHubertCharacter;
class UCharacterMovementComponent;

UCLASS()
class LOD_API UEMRHubertAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UEMRHubertAnimInstance();

    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    void SetHubertState(EEMRHubertState InState);

    UFUNCTION(BlueprintPure, Category = "EMR|Hubert|Animation")
    EEMRHubertState GetHubertState() const { return HubertState; }

protected:
    void RefreshStateFlags();
    void TryResolveOwningCharacter();

protected:
    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    EEMRHubertState HubertState = EEMRHubertState::HubDriver;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    float GroundSpeed = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsMoving = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsFalling = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsHubDriver = true;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsNightPatrol = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsWindowWatch = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsChaseEscaper = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsGrabEscaper = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsCarryEscaper = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsThrowEscaper = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    bool bIsReturnEscaper = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    TObjectPtr<AEMRHubertCharacter> HubertCharacter = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|Hubert|Animation")
    TObjectPtr<UCharacterMovementComponent> CharacterMovementComponent = nullptr;
};
