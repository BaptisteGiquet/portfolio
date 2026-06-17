#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMRCoffeeMachineActor.generated.h"

class AEMRCoffeePitcherItemActor;
class AEMRPlayerCharacter;
struct FEMRCoffeeUpgradeTuning;
class UAudioComponent;
class UEMRInteractableComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class USoundBase;

UCLASS()
class LOD_API AEMRCoffeeMachineActor : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMRCoffeeMachineActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    bool TryDockPitcher(AEMRPlayerCharacter* Player, AEMRCoffeePitcherItemActor* PitcherActor);
    bool TryStartBrew(AEMRPlayerCharacter* Player);
    void SetCoffeeMachineEnabledByUpgrade(bool bEnabled);
    void ApplyUpgradeTuning(const FEMRCoffeeUpgradeTuning& InTuning);

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee")
    AEMRCoffeePitcherItemActor* GetOwnedPitcher() const { return OwnedPitcher; }

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee")
    bool IsEnabledByUpgrade() const { return bCoffeeMachineEnabledByUpgrade; }

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee|Upgrade")
    int32 GetRequiredUpgradeStackCount() const { return FMath::Max(RequiredUpgradeStackCount, 1); }

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Coffee|Components")
    TObjectPtr<UStaticMeshComponent> MachineMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Coffee|Components")
    TObjectPtr<USceneComponent> PitcherAnchor = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Coffee|Components")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Coffee|Components")
    TObjectPtr<UAudioComponent> BrewLoopAudioComponent = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|Coffee")
    TSubclassOf<AEMRCoffeePitcherItemActor> PitcherClass;

    UPROPERTY(EditAnywhere, Category = "EMR|Coffee|Tuning", meta = (ClampMin = "1"))
    int32 MaxServingsPerFill = 5;

    UPROPERTY(EditAnywhere, Category = "EMR|Coffee|Tuning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PatienceRestorePercentOfMax = 0.5f;

    UPROPERTY(EditAnywhere, Category = "EMR|Coffee|Tuning", meta = (ClampMin = "0.1"))
    float BrewFillDurationSeconds = 3.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|Coffee|Audio", meta = (DisplayName = "Brew Loop Sound", ToolTip = "Looping sound played while the coffee machine is brewing."))
    TObjectPtr<USoundBase> BrewStartSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|Coffee", meta = (Categories = "EMR.RunUpgrade"))
    FGameplayTag RequiredUpgradeTag;

    UPROPERTY(EditAnywhere, Category = "EMR|Coffee|Upgrade", meta = (ClampMin = "1", DisplayName = "Unlock At Coffee Upgrade Count", ToolTip = "Machine becomes enabled when the run has this many Coffee Machine upgrade selections."))
    int32 RequiredUpgradeStackCount = 1;

private:
    void EnsurePitcherSpawned();
    void ApplyMachineEnabledState();
    void UpdateMachineVisibilityTraceResponse();
    void UpdateBrewLoopAudioState();
    bool ShouldHideMachineForPickup() const;

    UFUNCTION()
    void OnRep_CoffeeMachineEnabled();

    UPROPERTY(ReplicatedUsing = OnRep_CoffeeMachineEnabled)
    bool bCoffeeMachineEnabledByUpgrade = false;

    UPROPERTY(Replicated)
    TObjectPtr<AEMRCoffeePitcherItemActor> OwnedPitcher = nullptr;

    bool bMachineVisibilityBlocksTrace = false;
    bool bMachineCollisionQueryEnabled = false;
    bool bBrewLoopAudioPlaying = false;
};
