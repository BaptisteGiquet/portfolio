#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMRHubTabletActor.generated.h"

class UWidgetComponent;
class UStaticMeshComponent;
class USceneComponent;
class UUserWidget;
class UEMRInteractableComponent;

UENUM(BlueprintType)
enum class EEMRHubTabletWidgetMode : uint8
{
    NightShiftSelection UMETA(DisplayName = "NightShift Selection"),
    UpgradeVoting UMETA(DisplayName = "Upgrade Voting")
};

UCLASS()
class LOD_API AEMRHubTabletActor : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMRHubTabletActor();

    virtual void BeginPlay() override;
    virtual void OnRep_Owner() override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Hub|Tablet")
    void SetTabletInteractionEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "EMR|Hub|Tablet")
    void RefreshOwnerPlayer();

    UFUNCTION(BlueprintCallable, Category = "EMR|Hub|Tablet")
    void SetTabletWidgetMode(EEMRHubTabletWidgetMode NewWidgetMode);

    UFUNCTION(BlueprintPure, Category = "EMR|Hub|Tablet")
    UWidgetComponent* GetTabletWidgetComponent() const { return TabletWidgetComponent; }

    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, Category = "EMR|Hub|Tablet")
    TObjectPtr<USceneComponent> TabletRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Hub|Tablet")
    TObjectPtr<UStaticMeshComponent> TabletMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Hub|Tablet")
    TObjectPtr<UWidgetComponent> TabletWidgetComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Hub|Tablet")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Tablet")
    TSubclassOf<UUserWidget> TabletWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Tablet")
    TSubclassOf<UUserWidget> NightShiftSelectionWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Tablet")
    TSubclassOf<UUserWidget> UpgradeVoteWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Tablet")
    bool bOverrideTabletWidgetTransform = false;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Tablet")
    FTransform TabletWidgetRelativeTransform = FTransform::Identity;

private:
    TSubclassOf<UUserWidget> ResolveWidgetClassForMode(EEMRHubTabletWidgetMode WidgetMode) const;

    UPROPERTY(Transient)
    EEMRHubTabletWidgetMode CurrentWidgetMode = EEMRHubTabletWidgetMode::NightShiftSelection;

    void InitializeOwnerPlayer();
    void InitializeWidgetComponent();
};
