#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "EMRMagicBoxActor.generated.h"

class AEMRPatient;
class ATargetPoint;
class UAudioComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class USoundBase;
struct FHitResult;

UCLASS()
class LOD_API AEMRMagicBoxActor : public AActor
{
    GENERATED_BODY()

public:
    AEMRMagicBoxActor();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    bool IsAvailable() const;
    void AssignToPatient(AEMRPatient* Patient);
    void ClearAssignment();
    void SetOccupied(bool bNewOccupied);
    void SetMagicBoxEnabledByUpgrade(bool bEnabled);
    FTransform GetApproachTransform() const;
    FTransform GetInsideTransform() const;
    AActor* GetApproachNavigationTargetActor() const;
    AActor* GetInsideNavigationTargetActor() const;
    float GetDoorMoveDurationSeconds() const;
    float GetEntryDelayAfterDoorOpenSeconds() const { return FMath::Max(EntryDelayAfterDoorOpenSeconds, 0.0f); }
    float GetExitDelayAfterDoorOpenSeconds() const { return FMath::Max(ExitDelayAfterDoorOpenSeconds, 0.0f); }
    int32 GetRequiredUpgradeStackCount() const { return FMath::Max(RequiredUpgradeStackCount, 1); }
    FGameplayTag GetRequiredUpgradeTag() const { return RequiredUpgradeTag; }

    void SetDoorsOpen(bool bOpen);
    void NotifyTreatmentStarted(AEMRPatient* Patient, float DurationSeconds);
    void NotifyTreatmentFinished(AEMRPatient* Patient);
    void NotifyTreatmentAborted(AEMRPatient* Patient);

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|MagicBox")
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "EMR|MagicBox|Flow")
    TObjectPtr<ATargetPoint> ApproachTargetPoint = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "EMR|MagicBox|Flow")
    TObjectPtr<ATargetPoint> InsideTargetPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|MagicBox")
    TObjectPtr<UStaticMeshComponent> LeftDoorMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|MagicBox")
    TObjectPtr<UStaticMeshComponent> RightDoorMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|MagicBox")
    TObjectPtr<UBoxComponent> EntryTrigger = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|MagicBox")
    TObjectPtr<UAudioComponent> TreatmentLoopAudioComponent = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox", meta = (Categories = "EMR.RunUpgrade"))
    FGameplayTag RequiredUpgradeTag;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Upgrade", meta = (ClampMin = "1", DisplayName = "Unlock At Magic Box Upgrade Count"))
    int32 RequiredUpgradeStackCount = 1;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Door")
    FRotator LeftDoorClosedRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Door")
    FRotator LeftDoorOpenRotation = FRotator(0.0f, -90.0f, 0.0f);

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Door")
    FRotator RightDoorClosedRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Door")
    FRotator RightDoorOpenRotation = FRotator(0.0f, 90.0f, 0.0f);

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Door", meta = (ClampMin = "0.01"))
    float DoorMoveDurationSeconds = 0.6f;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Door", meta = (ClampMin = "0.0"))
    float EntryDelayAfterDoorOpenSeconds = 0.15f;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Door", meta = (ClampMin = "0.0"))
    float ExitDelayAfterDoorOpenSeconds = 0.15f;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Audio", meta = (DisplayName = "Door Open Sound"))
    TObjectPtr<USoundBase> DoorOpenSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Audio", meta = (DisplayName = "Door Close Sound"))
    TObjectPtr<USoundBase> DoorCloseSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Audio", meta = (DisplayName = "Treatment Started Sound"))
    TObjectPtr<USoundBase> TreatmentStartedSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Audio", meta = (DisplayName = "Treatment Finished Sound"))
    TObjectPtr<USoundBase> TreatmentFinishedSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Audio", meta = (DisplayName = "Treatment Loop Sound"))
    TObjectPtr<USoundBase> TreatmentLoopSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Audio", meta = (ClampMin = "0.0"))
    float TreatmentLoopFadeInSeconds = 0.08f;

    UPROPERTY(EditAnywhere, Category = "EMR|MagicBox|Audio", meta = (ClampMin = "0.0"))
    float TreatmentLoopFadeOutSeconds = 0.15f;

private:
    UPROPERTY(ReplicatedUsing = OnRep_MagicBoxEnabled, VisibleAnywhere, Category = "EMR|MagicBox")
    bool bMagicBoxEnabledByUpgrade = false;

    UPROPERTY(ReplicatedUsing = OnRep_Occupied, VisibleAnywhere, Category = "EMR|MagicBox")
    bool bIsOccupied = false;

    UPROPERTY(ReplicatedUsing = OnRep_DoorsOpen, VisibleAnywhere, Category = "EMR|MagicBox")
    bool bDoorsOpen = false;

    UPROPERTY()
    TWeakObjectPtr<AEMRPatient> AssignedPatient;

    UPROPERTY(ReplicatedUsing = OnRep_TreatmentLoopAudioPlaying)
    bool bTreatmentLoopAudioPlaying = false;
    float DoorOpenAlpha = 0.0f;
    float DoorTargetAlpha = 0.0f;
    bool bDoorMotionActive = false;
    float LoopAudioRestartRetryElapsedSeconds = 0.0f;

    UFUNCTION()
    void HandleEntryTriggerOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnRep_MagicBoxEnabled();

    UFUNCTION()
    void OnRep_Occupied();

    UFUNCTION()
    void OnRep_DoorsOpen();

    UFUNCTION()
    void OnRep_TreatmentLoopAudioPlaying();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnMagicBoxTreatmentStarted(AEMRPatient* Patient, float DurationSeconds);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnMagicBoxTreatmentFinished(AEMRPatient* Patient);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnMagicBoxTreatmentAborted(AEMRPatient* Patient);

    void ApplyRuntimeState();
    void SetTreatmentLoopAudioPlaying(bool bNewPlaying);
    void UpdateLoopAudioState();
    void PlayLocalSound(USoundBase* Sound) const;
    void RefreshDoorPose(float DeltaSeconds);
    void ApplyDoorPose() const;
    void ApplyDoorTarget(bool bOpen, bool bPlaySound);
};
