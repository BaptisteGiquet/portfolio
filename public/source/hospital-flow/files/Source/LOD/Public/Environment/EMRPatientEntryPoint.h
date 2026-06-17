#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRPatientEntryPoint.generated.h"

class UArrowComponent;

/**
 * Placement actor used as the fixed entry point for newly spawned patients.
 */
UCLASS()
class LOD_API AEMRPatientEntryPoint : public AActor
{
    GENERATED_BODY()

public:
    AEMRPatientEntryPoint();

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    FTransform GetSpawnTransform() const;

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<UArrowComponent> ArrowComponent;
};
