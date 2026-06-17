#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRWaitingRoomArea.generated.h"

class UBoxComponent;
class UEMRWaitingRoomManagerComponent;

/**
 * Volume wrapper used to mark the waiting room space for patients.
 */
UCLASS()
class LOD_API AEMRWaitingRoomArea : public AActor
{
    GENERATED_BODY()

public:
    AEMRWaitingRoomArea();

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    UBoxComponent* GetAreaVolume() const { return AreaVolume; }

    UFUNCTION(BlueprintCallable, Category = "EMR|WaitingRoom")
    UEMRWaitingRoomManagerComponent* GetManagerComponent() const { return WaitingRoomManager; }

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<UBoxComponent> AreaVolume;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Components")
    TObjectPtr<UEMRWaitingRoomManagerComponent> WaitingRoomManager;
};
