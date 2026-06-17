#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EMRWaitingSeatActor.generated.h"

class UArrowComponent;
class UEMRWaitingSeatComponent;
class USceneComponent;

/**
 * Actor wrapper used to place a waiting seat with a dedicated approach point on the navmesh.
 */
UCLASS()
class LOD_API AEMRWaitingSeatActor : public AActor
{
    GENERATED_BODY()

public:
    AEMRWaitingSeatActor();

    /** Waiting seat component tracked by the manager. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    TObjectPtr<UEMRWaitingSeatComponent> WaitingSeat;

    /** Arrow indicating the navigation target/approach point. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMR|WaitingRoom")
    TObjectPtr<UArrowComponent> ApproachArrow;

protected:
    /** Root scene used to author seat transforms cleanly in the editor. */
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;
};
