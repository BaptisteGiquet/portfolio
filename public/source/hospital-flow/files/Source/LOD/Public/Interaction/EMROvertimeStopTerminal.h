#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMROvertimeStopTerminal.generated.h"

class UStaticMeshComponent;
class UEMRInteractableComponent;
class USoundBase;
class UPrimitiveComponent;
class UAudioComponent;

/**
 * Simple interactable actor that can only be used during NightShift overtime to stop the shift.
 */
UCLASS()
class LOD_API AEMROvertimeStopTerminal : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMROvertimeStopTerminal();
    bool TryConsumeTerminalInteraction();
    bool IsButtonComponent(const UPrimitiveComponent* Component) const;
    bool IsConfirmButtonInteractionEnabled() const;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaSeconds) override;

    // IEMRInteractableInterface
    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ApplyInteractionEnabledState(bool bEnable);
    void RefreshInteractionAvailability();
    bool IsNightShiftInOvertime() const;
    void HandleNightShiftOvertimeStarted();
    void ApplyCoverRotation(bool bOpen);
    void UpdateCoverRotation(float DeltaSeconds);
    void CacheButtonLocations();
    void ApplyButtonPressedState(bool bPressed);
    void UpdateButtonLocation(float DeltaSeconds);
    void PlayTerminalSound(USoundBase* Sound);
    void PlayOvertimeStopAlertSound();
    void StopOvertimeStopAlertSound();
    void HandlePendingTravelChanged();
    void UpdateTickEnabled();
    bool ConsumeTerminalInteraction();

    UFUNCTION()
    void OnRep_InteractionEnabled();

    UFUNCTION()
    void OnRep_InteractionConsumed();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayTerminalSound(USoundBase* Sound);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayOvertimeStopAlertSound(USoundBase* Sound);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EMR|OvertimeStopTerminal|Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> TerminalMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EMR|OvertimeStopTerminal|Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> CoverMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EMR|OvertimeStopTerminal|Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> ButtonMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EMR|OvertimeStopTerminal|Components", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UEMRInteractableComponent> InteractableComponent;

    UPROPERTY(ReplicatedUsing=OnRep_InteractionEnabled)
    bool bInteractionEnabled = false;

    UPROPERTY(ReplicatedUsing=OnRep_InteractionConsumed)
    bool bInteractionConsumed = false;

    UPROPERTY(EditAnywhere, Category="EMR|OvertimeStopTerminal|Cover")
    FRotator CoverOpenRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category="EMR|OvertimeStopTerminal|Cover")
    FRotator CoverClosedRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category="EMR|OvertimeStopTerminal|Cover", meta = (ClampMin = "0.0"))
    float CoverInterpSpeed = 6.0f;

    UPROPERTY(EditDefaultsOnly, Category="EMR|OvertimeStopTerminal|Audio")
    TObjectPtr<USoundBase> LidOpenSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="EMR|OvertimeStopTerminal|Audio")
    TObjectPtr<USoundBase> InteractionSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="EMR|OvertimeStopTerminal|Audio")
    TObjectPtr<USoundBase> OvertimeStopAlertSound = nullptr;

    UPROPERTY(EditAnywhere, Category="EMR|OvertimeStopTerminal|Button")
    FVector ButtonPressedOffset = FVector(0.0f, 0.0f, -1.5f);

    UPROPERTY(EditAnywhere, Category="EMR|OvertimeStopTerminal|Button", meta = (ClampMin = "0.0"))
    float ButtonInterpSpeed = 10.0f;

    UPROPERTY(Transient)
    FQuat TargetCoverRotationQuat = FQuat::Identity;

    UPROPERTY(Transient)
    bool bIsCoverAnimating = false;

    UPROPERTY(Transient)
    FVector ButtonRestRelativeLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    FVector ButtonPressedRelativeLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    FVector TargetButtonRelativeLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    bool bButtonLocationsCached = false;

    UPROPERTY(Transient)
    bool bIsButtonAnimating = false;

    FDelegateHandle OvertimeStartedHandle;
    FDelegateHandle PendingTravelChangedHandle;

    UPROPERTY(Transient)
    TWeakObjectPtr<UAudioComponent> ActiveOvertimeStopAlertAudio;

};
