#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMRLabAnalyzerLid.generated.h"

class UPrimitiveComponent;
class UStaticMeshComponent;
class UEMRInteractableComponent;
class AEMRLabAnalyzerMachine;

UCLASS()
class LOD_API AEMRLabAnalyzerLid : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMRLabAnalyzerLid();

    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    void SetOwningMachine(AEMRLabAnalyzerMachine* InOwningMachine);
    AEMRLabAnalyzerMachine* GetOwningMachine() const { return OwningMachine; }
    bool IsLidComponent(const UPrimitiveComponent* Component) const;
    UStaticMeshComponent* GetLidMeshComponent() const { return LidMesh; }

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UStaticMeshComponent> LidMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "EMR|LabAnalyzer|Lid")
    TObjectPtr<AEMRLabAnalyzerMachine> OwningMachine = nullptr;
};
