#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMRToiletSlotActor.generated.h"

class UArrowComponent;
class UBoxComponent;
class UDecalComponent;
class UEMRInteractableComponent;
class AEMRPatient;
class AEMRToiletCleaner;
class USphereComponent;
class USoundBase;

UCLASS()
class LOD_API AEMRToiletSlotActor : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMRToiletSlotActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    void AssignToPatient(AEMRPatient* Patient);
    void ClearAssignment();
    bool IsAvailable() const;
    void SetOccupied(bool bNewOccupied);

    AEMRPatient* GetAssignedPatient() const { return AssignedPatient.Get(); }

    bool IsDirty() const { return bIsDirty; }
    void SetDirty(bool bNewDirty);

    float GetCurrentDirtChance() const { return CurrentDirtChance; }
    void SetCurrentDirtChance(float NewChance) { CurrentDirtChance = NewChance; }

    FTransform GetApproachTransform() const;
    void PlayUseOutcomeSound(bool bBecameDirty);
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere, Category="EMR|Toilet")
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category="EMR|Toilet")
    TObjectPtr<UArrowComponent> ApproachArrow = nullptr;

    UPROPERTY(VisibleAnywhere, Category="EMR|Toilet")
    TObjectPtr<UArrowComponent> CleanerAnchor = nullptr;

    UPROPERTY(VisibleAnywhere, Category="EMR|Toilet")
    TObjectPtr<USphereComponent> CleanerAnchorTrigger = nullptr;

    UPROPERTY(VisibleAnywhere, Category="EMR|Toilet")
    TObjectPtr<UBoxComponent> UseTrigger = nullptr;

    UPROPERTY(VisibleAnywhere, Category="EMR|Toilet")
    TObjectPtr<UDecalComponent> DirtDecal = nullptr;

    UPROPERTY(VisibleAnywhere, Category="EMR|Toilet")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent = nullptr;

    UPROPERTY(EditInstanceOnly, Category="EMR|Toilet")
    TObjectPtr<AEMRToiletCleaner> ToiletCleaner = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet|Audio")
    TObjectPtr<USoundBase> ToiletUseCleanSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Toilet|Audio")
    TObjectPtr<USoundBase> ToiletUseDirtySound = nullptr;

    UPROPERTY(ReplicatedUsing=OnRep_Dirty, VisibleAnywhere, Category="EMR|Toilet")
    bool bIsDirty = false;

    UPROPERTY(ReplicatedUsing=OnRep_Occupied, VisibleAnywhere, Category="EMR|Toilet")
    bool bIsOccupied = false;

    UPROPERTY()
    TWeakObjectPtr<AEMRPatient> AssignedPatient;

    float CurrentDirtChance = 0.0f;

    UFUNCTION()
    void HandleUseTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnRep_Dirty();

    UFUNCTION()
    void OnRep_Occupied();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayToiletUseOutcomeSound(bool bBecameDirty);

    void UpdateDirtDecal();
    void UpdateInteractableState();
};
