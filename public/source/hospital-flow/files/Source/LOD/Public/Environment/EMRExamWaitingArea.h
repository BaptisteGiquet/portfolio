#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRExamWaitingArea.generated.h"

class UBoxComponent;
class UEMRExamWaitingManagerComponent;

/**
 * Volume wrapper used to mark the waiting space for exam queues.
 */
UCLASS()
class LOD_API AEMRExamWaitingArea : public AActor
{
    GENERATED_BODY()

public:
    AEMRExamWaitingArea();

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamWaiting")
    UBoxComponent* GetAreaVolume() const { return AreaVolume; }

    UFUNCTION(BlueprintCallable, Category = "EMR|ExamWaiting")
    UEMRExamWaitingManagerComponent* GetManagerComponent() const { return ExamWaitingManager; }

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<UBoxComponent> AreaVolume;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<UEMRExamWaitingManagerComponent> ExamWaitingManager;
};
