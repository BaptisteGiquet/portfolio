#pragma once

#include "CoreMinimal.h"
#include "Shop/EMRItemActor.h"
#include "EMROxygenMask.generated.h"

class AEMROxygenMachine;
class UEMRInteractableComponent;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EEMROxygenMaskState : uint8
{
    AtMachine,
    Carried,
    AttachedToPatient
};

UCLASS()
class LOD_API AEMROxygenMask : public AEMRItemActor
{
    GENERATED_BODY()

public:
    AEMROxygenMask();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    void AttachToMachineHome();
    void AttachToPatient(USkeletalMeshComponent* PatientMesh, const FName& SocketName, const FTransform& RelativeOffset);
    void ReturnToMachine();

    bool IsMaskCarried() const { return MaskState == EEMROxygenMaskState::Carried; }
    bool IsMaskAttached() const { return MaskState == EEMROxygenMaskState::AttachedToPatient; }
    bool IsCarriedBy(const AActor* Actor) const;

    void SetOwningMachine(AEMROxygenMachine* Machine) { OwningMachine = Machine; }

    // Runtime owner derived from the machine's MaskActor binding (single source of truth).
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "EMR|Oxygen")
    TObjectPtr<AEMROxygenMachine> OwningMachine = nullptr;

protected:
    void HandleMaskStateChanged();

    UFUNCTION()
    void OnRep_MaskState();

    UPROPERTY(ReplicatedUsing = OnRep_MaskState)
    EEMROxygenMaskState MaskState = EEMROxygenMaskState::AtMachine;

private:
    UEMRInteractableComponent* FindInteractable() const;
};
