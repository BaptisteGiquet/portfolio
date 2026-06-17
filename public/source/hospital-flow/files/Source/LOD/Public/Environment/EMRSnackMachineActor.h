#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "EMRSnackMachineActor.generated.h"

class AEMRPatient;
class UArrowComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class USoundBase;
struct FHitResult;

UCLASS()
class LOD_API AEMRSnackMachineActor : public AActor
{
    GENERATED_BODY()

public:
    AEMRSnackMachineActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    bool IsAvailable() const;
    void AssignToPatient(AEMRPatient* Patient);
    void ClearAssignment();
    void SetOccupied(bool bNewOccupied);
    void SetSnackMachineEnabledByUpgrade(bool bEnabled);
    FTransform GetApproachTransform() const;
    int32 GetRequiredUpgradeStackCount() const { return FMath::Max(RequiredUpgradeStackCount, 1); }
    FGameplayTag GetRequiredUpgradeTag() const { return RequiredUpgradeTag; }
    USoundBase* GetSnackUseStartGlobalSound() const { return SnackUseStartGlobalSound; }
    void PlayUseStartSound();

private:
    UPROPERTY(VisibleAnywhere, Category = "EMR|SnackMachine")
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|SnackMachine")
    TObjectPtr<UArrowComponent> ApproachArrow = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|SnackMachine")
    TObjectPtr<UBoxComponent> UseTrigger = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|SnackMachine", meta = (Categories = "EMR.RunUpgrade"))
    FGameplayTag RequiredUpgradeTag;

    UPROPERTY(EditAnywhere, Category = "EMR|SnackMachine|Upgrade", meta = (ClampMin = "1", DisplayName = "Unlock At Snack Upgrade Count"))
    int32 RequiredUpgradeStackCount = 1;

    UPROPERTY(EditAnywhere, Category = "EMR|SnackMachine|Audio")
    TObjectPtr<USoundBase> SnackUseStartGlobalSound = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_SnackMachineEnabled, VisibleAnywhere, Category = "EMR|SnackMachine")
    bool bSnackMachineEnabledByUpgrade = false;

    UPROPERTY(ReplicatedUsing = OnRep_Occupied, VisibleAnywhere, Category = "EMR|SnackMachine")
    bool bIsOccupied = false;

    UPROPERTY()
    TWeakObjectPtr<AEMRPatient> AssignedPatient;

    UFUNCTION()
    void HandleUseTriggerOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnRep_SnackMachineEnabled();

    UFUNCTION()
    void OnRep_Occupied();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlaySnackUseStartSound();

    void ApplyMachineRuntimeState();
};
