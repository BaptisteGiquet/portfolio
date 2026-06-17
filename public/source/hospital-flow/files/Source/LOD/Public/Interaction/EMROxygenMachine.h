#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMROxygenMachine.generated.h"

class AEMROxygenMask;
class AEMRPatient;
class AEMRPlayerCharacter;
class UAudioComponent;
class UCableComponent;
class UEMRInteractableComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USphereComponent;
class USoundBase;
class UStaticMeshComponent;
struct FHitResult;

UCLASS()
class LOD_API AEMROxygenMachine : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMROxygenMachine();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnRep_ReplicatedMovement() override;

    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    void ToggleMove(AEMRPlayerCharacter* Player);
    void TryStartMove(AEMRPlayerCharacter* Player);
    void StopMove(AEMRPlayerCharacter* Player);
    void BeginLocalMoveDriver(AEMRPlayerCharacter* Player);
    void EndLocalMoveDriver(AEMRPlayerCharacter* Player);

    bool TryAttachMaskToPatient(AEMRPlayerCharacter* Player, AEMRPatient* Patient, const FHitResult& HitResult);
    bool TryReturnMaskFromTrace(AEMRPlayerCharacter* Player, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance);
    bool IsMaskWithinDistanceLimit(const FVector& MaskWorldLocation) const;
    bool IsMaskPlacementSocketAimed(const AEMRPatient* Patient, const FHitResult& HitResult) const;
    void SetOxygenMachineEnabledByUpgrade(bool bEnabled);

    bool IsMoving() const { return CurrentMover != nullptr; }
    bool IsMaskInUse() const;
    bool IsEnabledByUpgrade() const { return bOxygenMachineEnabledByUpgrade; }
    int32 GetRequiredUpgradeStackCount() const { return FMath::Max(RequiredUpgradeStackCount, 0); }
    FGameplayTag GetRequiredUpgradeTag() const { return RequiredUpgradeTag; }

    AEMRPlayerCharacter* GetCurrentMover() const { return CurrentMover; }
    USceneComponent* GetMaskHomeAnchor() const { return MaskHomeAnchor; }
    const USphereComponent* GetMaskReturnTraceComponent() const { return MaskReturnTraceSphere.Get(); }
    float GetMoveSpeedMultiplier() const { return MoveSpeedMultiplier; }

protected:
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "EMR|Oxygen|Components")
    TObjectPtr<UStaticMeshComponent> MachineMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Oxygen|Components")
    TObjectPtr<USceneComponent> MaskHomeAnchor = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Oxygen|Components")
    TObjectPtr<UCableComponent> MaskCable = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Oxygen|Components")
    TObjectPtr<USphereComponent> MaskReturnTraceSphere = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Oxygen|Components")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Oxygen|Audio")
    TObjectPtr<UAudioComponent> TreatmentLoopAudioComponent = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "EMR|Oxygen")
    TObjectPtr<AEMROxygenMask> MaskActor = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Patient")
    FName PatientMeshComponentName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Patient")
    FName PatientMeshComponentTag = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Mask")
    FName PatientMaskSocketName = TEXT("oxygen_mask_socket");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Mask")
    FName PatientMaskInteractSocketName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Mask")
    FTransform MaskAttachOffset = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Mask")
    FName MaskCableSocketName = TEXT("mask_cable_socket");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Mask", meta = (ClampMin = "0.0"))
    float MaskPlacementToleranceRadius = 8.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|Oxygen|Upgrade", meta = (Categories = "EMR.RunUpgrade"))
    FGameplayTag RequiredUpgradeTag;

    UPROPERTY(EditAnywhere, Category = "EMR|Oxygen|Upgrade", meta = (ClampMin = "0", DisplayName = "Unlock At Oxygen Upgrade Count"))
    int32 RequiredUpgradeStackCount = 1;

    UPROPERTY(EditAnywhere, Category = "EMR|Oxygen|Mask", meta = (ClampMin = "0.0"))
    float MaxMaskDistanceFromMachine = 350.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|Oxygen|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MoveSpeedMultiplier = 0.5f;

    UPROPERTY(EditAnywhere, Category = "EMR|Oxygen|Movement")
    FVector MoveFollowOffset = FVector(85.0f, 45.0f, 0.0f);

    UPROPERTY(EditAnywhere, Category = "EMR|Oxygen|Movement", meta = (ClampMin = "0.0"))
    float MoveFollowCatchupSpeed = 14.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|Oxygen|Movement", meta = (ClampMin = "0.0"))
    float MoveMaxStepPerSecond = 350.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|Oxygen|Movement", meta = (ClampMin = "0.0"))
    float MoveSlideFriction = 0.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|Oxygen|Movement|Collision")
    FName MoveCollisionProfileOverride = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Audio")
    TObjectPtr<USoundBase> TreatmentLoopSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Audio", meta = (ClampMin = "0.0"))
    float TreatmentLoopFadeInSeconds = 0.08f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Audio", meta = (ClampMin = "0.0"))
    float TreatmentLoopFadeOutSeconds = 0.15f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Audio")
    TObjectPtr<USoundBase> TreatmentCompletedSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Oxygen|Audio")
    TObjectPtr<USoundBase> MaskPlacementBlockedSound = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentMover)
    TObjectPtr<AEMRPlayerCharacter> CurrentMover = nullptr;

    UPROPERTY(Replicated)
    TObjectPtr<AEMRPatient> CurrentPatient = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_TreatmentActive)
    bool bTreatmentActive = false;

    UPROPERTY(ReplicatedUsing = OnRep_OxygenMachineEnabledByUpgrade)
    bool bOxygenMachineEnabledByUpgrade = true;

private:
    UFUNCTION()
    void OnRep_CurrentMover();

    UFUNCTION()
    void OnRep_TreatmentActive();

    UFUNCTION()
    void OnRep_OxygenMachineEnabledByUpgrade();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayTreatmentCompletedSound();

    void AttachToMover(AEMRPlayerCharacter* Player);
    void DetachFromMover();
    void TickMoverFollow(float DeltaSeconds);
    FVector ComputeMoverFollowTarget(const AEMRPlayerCharacter* Player) const;
    void ApplySweepMoveWithSlide(const FVector& DesiredDelta);
    void SetMoverCollisionIgnore(AEMRPlayerCharacter* Player, bool bIgnore);
    AEMROxygenMask* GetOwnedMaskActor() const;
    void ResolveMaskActorBinding();

    void UpdateMaskCable();
    void ApplyMachineEnabledState();
    void RefreshInteractableState();
    void SetTreatmentActive(bool bNewActive);
    void RefreshTreatmentLoopAudioPlayback();
    void StartTreatmentTimer();
    void ClearTreatmentTimer();
    void HandleTreatmentComplete();
    void PlayBlockedMaskPlacementFeedback(AEMRPlayerCharacter* Player) const;
    void CancelTreatment();
    void SetCurrentPatientPatienceDrainSuppressed(bool bSuppressed);
    void ClearPatienceDrainSuppression();
    bool IsMaskPlacementValid(const USkeletalMeshComponent* PatientMesh, const FHitResult& HitResult) const;
    bool DoesPatientNeedOxygen(const AEMRPatient* Patient) const;
    bool IsPatientLeaving(const AEMRPatient* Patient) const;
    USkeletalMeshComponent* ResolvePatientMeshFor(const AEMRPatient* Patient) const;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPlayerCharacter> LocalMoveDriver;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPlayerCharacter> LastIgnoredMover;

    UPROPERTY(Transient)
    FVector CachedMoverRelativeOffset = FVector::ZeroVector;

    UPROPERTY(Transient)
    bool bHasCachedMoverRelativeOffset = false;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPatient> PatienceDrainSuppressedPatient;

    FTimerHandle TreatmentTimerHandle;
    float CachedMoverMaxWalkSpeed = 0.0f;
};
