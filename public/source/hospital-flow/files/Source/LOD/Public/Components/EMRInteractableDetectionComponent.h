#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EMRInteractableDetectionComponent.generated.h"

class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableTargetChanged, AActor*, NewTarget);

/**
 * Client-side detection component that finds the best interactable actor in front of the local camera.
 */
UCLASS(ClassGroup = (EMR), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRInteractableDetectionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEMRInteractableDetectionComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Radius for the detection sweep. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction")
    float TraceRadius = 30.f;

    /** Maximum distance at which we consider interactables. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction")
    float TraceDistance = 900.f;

    /** Maximum view angle (degrees) from the camera forward vector to keep a candidate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction")
    float MaxInteractAngle = 25.f;

    /** Debug draw traces. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction")
    bool bDebugDraw = false;

    UPROPERTY(BlueprintAssignable, Category = "EMR|Interaction")
    FOnInteractableTargetChanged OnTargetChanged;

    UFUNCTION(BlueprintPure, Category = "EMR|Interaction")
    AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

private:
    bool CanTrace() const;
    bool GatherViewPoint(FVector& OutLocation, FVector& OutDirection) const;
    AActor* FindBestInteractable(const FVector& ViewLocation, const FVector& ViewDirection);
    void UpdateTarget(AActor* NewTarget);

    /** Cached pointer to the owner's camera (if any). */
    TWeakObjectPtr<UCameraComponent> CachedCameraComponent;

    /** Current focused interactable target. */
    TWeakObjectPtr<AActor> CurrentTarget;
};
