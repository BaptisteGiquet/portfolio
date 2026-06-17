#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRPatientPoolAnchor.generated.h"

class UArrowComponent;

/**
 * Placement actor used to author the hidden pooled patient storage location.
 */
UCLASS()
class LOD_API AEMRPatientPoolAnchor : public AActor
{
    GENERATED_BODY()

public:
    AEMRPatientPoolAnchor();

    UFUNCTION(BlueprintPure, Category = "EMR|Spawn")
    FVector GetPoolLocation() const;

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<UArrowComponent> ArrowComponent;
};
