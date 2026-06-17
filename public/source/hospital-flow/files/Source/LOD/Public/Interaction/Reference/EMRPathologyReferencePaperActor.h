#pragma once

#include "CoreMinimal.h"
#include "Interfaces/EMRAnchoredCarryItemInterface.h"
#include "Interfaces/EMRCarriedWorldWidgetInteractionInterface.h"
#include "Interfaces/EMRFirstPersonCarryViewInterface.h"
#include "Shop/EMRItemActor.h"
#include "UI/Reference/EMRPathologyReferencePaperWidget.h"
#include "EMRPathologyReferencePaperActor.generated.h"

class USceneComponent;
class UEMRFixedPlacementComponent;
class UPrimitiveComponent;
class USphereComponent;
class UWidgetComponent;
class UUserWidget;

UCLASS()
class LOD_API AEMRPathologyReferencePaperActor : public AEMRItemActor, public IEMRAnchoredCarryItemInterface, public IEMRCarriedWorldWidgetInteractionInterface, public IEMRFirstPersonCarryViewInterface
{
    GENERATED_BODY()

public:
    AEMRPathologyReferencePaperActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Reference")
    void SetAnchorTransform(const FTransform& InTransform);

    UFUNCTION(BlueprintCallable, Category = "EMR|Reference")
    void ReturnToAnchor();

    UFUNCTION(BlueprintCallable, Category = "EMR|Reference")
    void SetReturnTraceComponent(UPrimitiveComponent* InComponent);

    bool TryReturnToAnchorFromTrace(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance);
    bool IsCarriedBy(const AActor* Actor) const;

    virtual bool EMRReturnToAnchorFromTrace_Implementation(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance) override;
    virtual void EMRReturnToAnchor_Implementation() override;
    virtual bool EMRIsCarriedBy_Implementation(const AActor* Actor) const override;
    virtual bool CanInteractWithWorldWidgetsWhileCarried_Implementation() const override;

    virtual bool GetFirstPersonCarryWidgetSettings_Implementation(FEMRFirstPersonCarryWidgetSettings& OutSettings) const override;
    virtual void UpdateFirstPersonCarryWidget_Implementation(UUserWidget* Widget) override;
    virtual void ResetFirstPersonCarryWidget_Implementation(UUserWidget* Widget) override;

protected:
    virtual void BeginPlay() override;
    void InitializePaperWidget();
    void SyncAnchorTraceToFixedAnchor(bool bForce);
    void UpdateAnchorTraceCollision(bool bEnableBlocking);

    UPROPERTY(VisibleAnywhere, Category = "EMR|Reference")
    TObjectPtr<UWidgetComponent> PaperWidgetComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Reference")
    TObjectPtr<UEMRFixedPlacementComponent> FixedPlacementComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Reference")
    TObjectPtr<USceneComponent> AnchorPointComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Reference")
    TObjectPtr<USphereComponent> AnchorTraceComponent = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|Reference")
    EEMRPathologyReferencePaperType PaperType = EEMRPathologyReferencePaperType::Exams;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Reference")
    TSubclassOf<UUserWidget> PaperWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Reference")
    bool bShowFirstPersonWidget = true;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UPrimitiveComponent> ReturnTraceComponent;

    bool bWasCarried = false;
};
