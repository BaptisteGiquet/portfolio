#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "EMRInteractableComponent.generated.h"

class UPrimitiveComponent;
class UWidgetComponent;
class UEMRInteractableNameWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableHoverChanged, bool, bNowHighlighted);

UENUM(BlueprintType)
enum class EEMRInteractableOutlineProfile : uint8
{
    None UMETA(DisplayName = "None"),
    GAInteract UMETA(DisplayName = "GA Interact"),
    ConfirmInput UMETA(DisplayName = "Confirm Input")
};

/**
 * Lightweight component to expose interaction metadata and optional highlighting.
 */
UCLASS(ClassGroup = (EMR), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UEMRInteractableComponent();
	

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    void SetInteractionEventTag(const FGameplayTag& InTag);

    /** Toggle custom depth highlight on this actor's primitive components. */
    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    void ApplyHighlight();

    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    void ClearHighlight();

    /** Show the interactable name widget (if present on the owning actor). */
    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    void ShowNameWidget();

    /** Hide the interactable name widget (if present on the owning actor). */
    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    void HideNameWidget();

    /** Build the event payload consumed by the interact ability. */
    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    FGameplayEventData BuildInteractionEventData(AActor* InteractingActor, const FGameplayTag& OverrideEventTag = FGameplayTag()) const;

    UFUNCTION(BlueprintPure, Category = "EMR|Interaction")
    FText GetDisplayName() const { return DisplayName; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    void SetDisplayName(const FText& InDisplayName);

    UFUNCTION(BlueprintPure, Category = "EMR|Interaction|Outline")
    EEMRInteractableOutlineProfile GetOutlineProfile() const { return OutlineProfile; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction|Outline")
    void SetOutlineProfile(EEMRInteractableOutlineProfile InOutlineProfile) { OutlineProfile = InOutlineProfile; }

    UFUNCTION(BlueprintPure, Category = "EMR|Interaction|Outline")
    int32 GetCustomDepthStencilValue() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Interaction")
    bool IsEnabled() const { return bIsEnabled; }

    UFUNCTION(BlueprintCallable, Category = "EMR|Interaction")
    void SetEnabled(bool bInEnabled) { bIsEnabled = bInEnabled; }

    /** Fired when the hover state changes due to highlighting. */
    UPROPERTY(BlueprintAssignable, Category = "EMR|Interaction")
    FOnInteractableHoverChanged OnInteractableHoverChanged;


protected:
    /** Gameplay event sent to the interacting ability. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction")
    FGameplayTag InteractionEventTag;

    /** Name shown to players when targeting this interactable. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction")
    FText DisplayName;

    /** Whether this interactable should be selectable. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction")
    bool bIsEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction|Outline")
    EEMRInteractableOutlineProfile OutlineProfile = EEMRInteractableOutlineProfile::GAInteract;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction|Outline")
    bool bOverrideOutlineStencil = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Interaction|Outline", meta = (ClampMin = "0", ClampMax = "255", EditCondition = "bOverrideOutlineStencil"))
    int32 CustomDepthStencilValue = 1;

private:
    int32 ResolveOutlineStencilValue() const;
    void CacheNameWidget();
    void RefreshNameWidgetText();

    /** Widget component placed on the owning actor to show its display name. */
    UPROPERTY(Transient)
    TWeakObjectPtr<UWidgetComponent> CachedNameWidgetComponent;

    /** Cached instance of the name widget. */
    UPROPERTY(Transient)
    TWeakObjectPtr<UEMRInteractableNameWidget> CachedNameWidgetInstance;
};
