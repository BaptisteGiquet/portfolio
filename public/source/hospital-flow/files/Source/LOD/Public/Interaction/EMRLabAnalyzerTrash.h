#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMRLabAnalyzerTrash.generated.h"

class UStaticMeshComponent;
class UEMRInteractableComponent;

UCLASS()
class LOD_API AEMRLabAnalyzerTrash : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMRLabAnalyzerTrash();

    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UStaticMeshComponent> TrashMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent = nullptr;
};
