#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRTreatmentWaitingArea.generated.h"

class UBoxComponent;
class UEMRTreatmentWaitingManagerComponent;

/**
 * Volume wrapper used to mark the waiting space for treatment queues.
 */
UCLASS()
class LOD_API AEMRTreatmentWaitingArea : public AActor
{
    GENERATED_BODY()

public:
    AEMRTreatmentWaitingArea();

    UFUNCTION(BlueprintCallable, Category = "EMR|TreatmentWaiting")
    UBoxComponent* GetAreaVolume() const { return AreaVolume; }

    UFUNCTION(BlueprintCallable, Category = "EMR|TreatmentWaiting")
    UEMRTreatmentWaitingManagerComponent* GetManagerComponent() const { return TreatmentWaitingManager; }

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<UBoxComponent> AreaVolume;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<UEMRTreatmentWaitingManagerComponent> TreatmentWaitingManager;
};
