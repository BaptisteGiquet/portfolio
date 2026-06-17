#pragma once

#include "CoreMinimal.h"
#include "Interfaces/EMRAnchoredCarryItemInterface.h"
#include "Shop/EMRItemActor.h"
#include "EMRToiletCleaner.generated.h"

class AActor;
class UEMRFixedPlacementComponent;
class UPrimitiveComponent;

UCLASS()
class LOD_API AEMRToiletCleaner : public AEMRItemActor, public IEMRAnchoredCarryItemInterface
{
    GENERATED_BODY()

public:
    AEMRToiletCleaner();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Toilet")
    void SetAnchorTransform(const FTransform& InTransform);

    UFUNCTION(BlueprintCallable, Category = "EMR|Toilet")
    void ReturnToAnchor();

    UFUNCTION(BlueprintCallable, Category = "EMR|Toilet")
    void SetReturnTraceComponent(UPrimitiveComponent* InComponent);

    bool TryReturnToAnchorFromTrace(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance);
    bool IsCarriedBy(const AActor* Actor) const;

    virtual bool EMRReturnToAnchorFromTrace_Implementation(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance) override;
    virtual void EMRReturnToAnchor_Implementation() override;
    virtual bool EMRIsCarriedBy_Implementation(const AActor* Actor) const override;

private:
    void UpdateAnchorTraceCollision(bool bEnableBlocking);

    UPROPERTY(VisibleAnywhere, Category = "EMR|Toilet")
    TObjectPtr<UEMRFixedPlacementComponent> FixedPlacementComponent = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<UPrimitiveComponent> ReturnTraceComponent;

    bool bWasCarried = false;
};
