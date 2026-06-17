#pragma once

#include "CoreMinimal.h"
#include "Interfaces/EMRAnchoredCarryItemInterface.h"
#include "Shop/EMRItemActor.h"
#include "EMRCoffeePitcherItemActor.generated.h"

class AActor;
class AEMRCoffeeMachineActor;
class UEMRFixedPlacementComponent;
class UEMRInteractableComponent;
class UPrimitiveComponent;
class USoundBase;

UCLASS()
class LOD_API AEMRCoffeePitcherItemActor : public AEMRItemActor, public IEMRAnchoredCarryItemInterface
{
    GENERATED_BODY()

public:
    AEMRCoffeePitcherItemActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Coffee")
    void ConfigureCoffeePayload(int32 InMaxServings, float InPatienceRestorePercentOfMax);

    UFUNCTION(BlueprintCallable, Category = "EMR|Coffee")
    bool ConsumeServingAndGetRestoreAmount(float MaxPatience, float& OutRestoreAmount);

    UFUNCTION(BlueprintCallable, Category = "EMR|Coffee")
    void FillToMaxServings();

    UFUNCTION(BlueprintCallable, Category = "EMR|Coffee")
    bool StartBrewFill(float InDurationSeconds);

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee")
    bool IsEmpty() const { return RemainingServings <= 0; }

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee")
    bool IsFull() const { return RemainingServings >= MaxServings; }

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee")
    int32 GetRemainingServings() const { return RemainingServings; }

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee")
    int32 GetMaxServings() const { return MaxServings; }

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee")
    float GetPatienceRestorePercentOfMax() const { return PatienceRestorePercentOfMax; }

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee")
    bool IsBrewing() const { return bIsBrewing; }

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee|Audio")
    USoundBase* GetPatientUseSound() const { return PatientUseSound; }

    UFUNCTION(BlueprintPure, Category = "EMR|Coffee")
    bool IsCurrentlyCarried() const;

    UFUNCTION(BlueprintCallable, Category = "EMR|Coffee")
    void SetOwningMachine(AEMRCoffeeMachineActor* InOwningMachine);

    UFUNCTION(BlueprintCallable, Category = "EMR|Coffee")
    void SetAnchorTransform(const FTransform& InTransform);

    UFUNCTION(BlueprintCallable, Category = "EMR|Coffee")
    void SetReturnTraceComponent(UPrimitiveComponent* InComponent);

    UFUNCTION(BlueprintCallable, Category = "EMR|Coffee")
    void ReturnToAnchor();

    bool TryReturnToAnchorFromTrace(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance);
    bool IsCarriedBy(const AActor* Actor) const;

    virtual bool EMRReturnToAnchorFromTrace_Implementation(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance) override;
    virtual void EMRReturnToAnchor_Implementation() override;
    virtual bool EMRIsCarriedBy_Implementation(const AActor* Actor) const override;

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Coffee")
    TObjectPtr<UEMRFixedPlacementComponent> FixedPlacementComponent = nullptr;

private:
    void UpdateAnchorTraceCollision(bool bEnableBlocking);
    void UpdateItemVisibilityTraceCollision();
    void RefreshFillVisual();
    void SetPickupEnabled(bool bEnabled);
    void StartVisualFillTransition(float TargetFillAmount, float DurationSeconds);
    float GetCurrentTargetFillAmount() const;
    float GetSynchronizedServerTimeSeconds() const;

    UFUNCTION()
    void OnRep_RemainingServings();

    UFUNCTION()
    void OnRep_BrewingState();

    UPROPERTY(ReplicatedUsing = OnRep_RemainingServings, VisibleAnywhere, Category = "EMR|Coffee", meta = (ClampMin = "0"))
    int32 RemainingServings = 0;

    UPROPERTY(Replicated, EditAnywhere, Category = "EMR|Coffee", meta = (ClampMin = "1"))
    int32 MaxServings = 5;

    UPROPERTY(Replicated, EditAnywhere, Category = "EMR|Coffee", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PatienceRestorePercentOfMax = 0.5f;

    UPROPERTY(ReplicatedUsing = OnRep_BrewingState, VisibleAnywhere, Category = "EMR|Coffee")
    bool bIsBrewing = false;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "EMR|Coffee", meta = (ClampMin = "0.0"))
    float BrewStartServerTimeSeconds = 0.0f;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "EMR|Coffee", meta = (ClampMin = "0.0"))
    float BrewDurationSeconds = 0.0f;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "EMR|Coffee", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BrewStartFillAmount = 0.0f;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "EMR|Coffee", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BrewTargetFillAmount = 1.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|Coffee|Tuning", meta = (ClampMin = "0.01"))
    float UseDepleteVisualDurationSeconds = 0.2f;

    UPROPERTY(EditAnywhere, Category = "EMR|Coffee|Audio")
    TObjectPtr<USoundBase> PatientUseSound = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRCoffeeMachineActor> OwningMachine;

    UPROPERTY(Transient)
    TWeakObjectPtr<UPrimitiveComponent> ReturnTraceComponent;

    UPROPERTY(Transient)
    TObjectPtr<UEMRInteractableComponent> CachedInteractableComponent = nullptr;

    float VisualFillAmount = 0.0f;
    float VisualFillTransitionStart = 0.0f;
    float VisualFillTransitionTarget = 0.0f;
    float VisualFillTransitionDuration = 0.0f;
    float VisualFillTransitionElapsed = 0.0f;
    bool bVisualFillTransitionActive = false;

    bool bWasCarried = false;
    bool bItemVisibilityBlocksTrace = true;
};
