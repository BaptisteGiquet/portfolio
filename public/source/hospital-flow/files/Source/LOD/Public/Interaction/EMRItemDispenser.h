#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMRItemDispenser.generated.h"

class UStaticMeshComponent;
class UPrimitiveComponent;
class USceneComponent;
class UWidgetComponent;
class UEMRInteractableComponent;
class AEMRPlayerCharacter;
class UEMRItemData;
class UEMRItemDispenserSlotWidget;
class UEMRItemDispenserOrderWidget;
class USoundBase;
struct FTimerHandle;

USTRUCT(BlueprintType)
struct FEMRItemDispenserDigitButtonDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent"))
    FComponentReference ButtonMesh;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser", meta = (ClampMin = "0", ClampMax = "9"))
    int32 Digit = 0;
};

USTRUCT(BlueprintType)
struct FEMRItemDispenserSlotDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.StaticMeshComponent"))
    FComponentReference DisplayMesh;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser", meta = (UseComponentPicker, AllowedClasses = "/Script/UMG.WidgetComponent"))
    FComponentReference SlotWidget;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser")
    TObjectPtr<const UEMRItemData> ItemData = nullptr;
};

UENUM()
enum class EEMRItemDispenserButtonType : uint8
{
    Digit,
    Confirm,
    Reset,
    Increase,
    Decrease
};

UENUM()
enum class EEMRItemDispenserOrderState : uint8
{
    Idle,
    Selected,
    Dispensing
};

UCLASS()
class LOD_API AEMRItemDispenser : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMRItemDispenser();
    virtual void Tick(float DeltaSeconds) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    void HandleDigitPressed(int32 Digit, AEMRPlayerCharacter* InInstigator);
    void HandleConfirmPressed(AEMRPlayerCharacter* InInstigator);
    void HandleResetPressed(AEMRPlayerCharacter* InInstigator);
    void HandleIncreasePressed(AEMRPlayerCharacter* InInstigator);
    void HandleDecreasePressed(AEMRPlayerCharacter* InInstigator);

    bool GetDigitForComponent(const UPrimitiveComponent* Component, int32& OutDigit) const;
    bool IsConfirmButtonComponent(const UPrimitiveComponent* Component) const;
    bool IsResetButtonComponent(const UPrimitiveComponent* Component) const;
    bool IsIncreaseButtonComponent(const UPrimitiveComponent* Component) const;
    bool IsDecreaseButtonComponent(const UPrimitiveComponent* Component) const;

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Dispenser|Components")
    TObjectPtr<UStaticMeshComponent> MachineMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Dispenser|Components")
    TObjectPtr<UStaticMeshComponent> TrayMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Dispenser|Components")
    TObjectPtr<USceneComponent> TrayItemAnchor = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Dispenser|Components")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Dispenser|Components")
    TObjectPtr<UWidgetComponent> OrderSummaryWidget = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Slots")
    TArray<FEMRItemDispenserSlotDef> Slots;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Keypad")
    TArray<FEMRItemDispenserDigitButtonDef> DigitButtons;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Keypad", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent"))
    FComponentReference ConfirmButtonMesh;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Keypad", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent"))
    FComponentReference ResetButtonMesh;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Keypad", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent"))
    FComponentReference IncreaseButtonMesh;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Keypad", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent"))
    FComponentReference DecreaseButtonMesh;

    UPROPERTY(EditInstanceOnly, Category = "EMR|Dispenser|Spawn", meta = (AllowedClasses = "/Script/Engine.TargetPoint"))
    TObjectPtr<AActor> ItemSpawnPoint = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Spawn", meta = (ClampMin = "0.05"))
    float SpawnInterval = 0.15f;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Order", meta = (ClampMin = "1", ClampMax = "20"))
    int32 MaxOrderQuantity = 20;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Dispenser|Audio")
    TObjectPtr<USoundBase> SuccessResultSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Dispenser|Audio")
    TObjectPtr<USoundBase> FailureResultSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Tray")
    FVector TrayLiftOffset = FVector(0.f, 0.f, 25.f);

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Tray", meta = (ClampMin = "0.05"))
    float TrayLiftDuration = 0.5f;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Keypad")
    FVector DigitButtonPressOffset = FVector(0.f, 0.f, -2.f);

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Keypad")
    FVector QuantityButtonPressOffset = FVector(0.f, 0.f, -2.f);

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Keypad", meta = (ClampMin = "0.0"))
    float DigitButtonLerpSpeed = 18.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|Dispenser|Keypad", meta = (ClampMin = "0.0"))
    float QuantityButtonPressHoldDuration = 0.08f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Dispenser|Audio")
    TObjectPtr<USoundBase> DigitButtonPressSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Dispenser|Audio")
    TObjectPtr<USoundBase> ConfirmButtonPressSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Dispenser|Audio")
    TObjectPtr<USoundBase> ResetButtonPressSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Dispenser|Audio")
    TObjectPtr<USoundBase> IncreaseButtonPressSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Dispenser|Audio")
    TObjectPtr<USoundBase> DecreaseButtonPressSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Dispenser|Audio")
    TObjectPtr<USoundBase> DispenseStartSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Dispenser|Audio", meta = (ClampMin = "0.0"))
    float DispenseStartSoundDelay = 0.08f;

    UPROPERTY(ReplicatedUsing = OnRep_SlotCodes)
    TArray<int32> SlotCodes;

    UPROPERTY(ReplicatedUsing = OnRep_PressedDigitsMask)
    uint16 PressedDigitsMask = 0;

    UPROPERTY(ReplicatedUsing = OnRep_OrderState)
    EEMRItemDispenserOrderState OrderState = EEMRItemDispenserOrderState::Idle;

    UPROPERTY(ReplicatedUsing = OnRep_SelectedSlotIndex)
    int32 SelectedSlotIndex = INDEX_NONE;

    UPROPERTY(ReplicatedUsing = OnRep_SelectedQuantity)
    int32 SelectedQuantity = 1;

    UPROPERTY(ReplicatedUsing = OnRep_DisplayedTotalCost)
    int32 DisplayedTotalCost = 0;

    UPROPERTY(ReplicatedUsing = OnRep_IncreaseButtonPressed)
    bool bIncreaseButtonPressed = false;

    UPROPERTY(ReplicatedUsing = OnRep_DecreaseButtonPressed)
    bool bDecreaseButtonPressed = false;

    UPROPERTY(ReplicatedUsing = OnRep_ConfirmButtonPressed)
    bool bConfirmButtonPressed = false;

    UPROPERTY(ReplicatedUsing = OnRep_ResetButtonPressed)
    bool bResetButtonPressed = false;

    UFUNCTION()
    void OnRep_SlotCodes();

    UFUNCTION()
    void OnRep_PressedDigitsMask();

    UFUNCTION()
    void OnRep_OrderState();

    UFUNCTION()
    void OnRep_SelectedSlotIndex();

    UFUNCTION()
    void OnRep_SelectedQuantity();

    UFUNCTION()
    void OnRep_DisplayedTotalCost();

    UFUNCTION()
    void OnRep_IncreaseButtonPressed();

    UFUNCTION()
    void OnRep_DecreaseButtonPressed();

    UFUNCTION()
    void OnRep_ConfirmButtonPressed();

    UFUNCTION()
    void OnRep_ResetButtonPressed();

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayDispenseStartSound();

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayButtonPressSound(EEMRItemDispenserButtonType ButtonType);

private:
    TArray<int32> CurrentInputDigits;
    TMap<int32, FTransform> DigitButtonStartTransforms;
    TMap<int32, FVector> DigitButtonTargetLocations;
    FTransform ConfirmButtonStartTransform = FTransform::Identity;
    FTransform ResetButtonStartTransform = FTransform::Identity;
    FTransform IncreaseButtonStartTransform = FTransform::Identity;
    FTransform DecreaseButtonStartTransform = FTransform::Identity;
    FVector ConfirmButtonTargetLocation = FVector::ZeroVector;
    FVector ResetButtonTargetLocation = FVector::ZeroVector;
    FVector IncreaseButtonTargetLocation = FVector::ZeroVector;
    FVector DecreaseButtonTargetLocation = FVector::ZeroVector;
    bool bHasConfirmButtonStartTransform = false;
    bool bHasResetButtonStartTransform = false;
    bool bHasIncreaseButtonStartTransform = false;
    bool bHasDecreaseButtonStartTransform = false;
    bool bHasConfirmButtonTargetLocation = false;
    bool bHasResetButtonTargetLocation = false;
    bool bHasIncreaseButtonTargetLocation = false;
    bool bHasDecreaseButtonTargetLocation = false;
    FTimerHandle DispenseTimerHandle;
    FTimerHandle DispenseStartSoundTimerHandle;
    FTimerHandle ConfirmButtonReleaseTimerHandle;
    FTimerHandle ResetButtonReleaseTimerHandle;
    FTimerHandle IncreaseButtonReleaseTimerHandle;
    FTimerHandle DecreaseButtonReleaseTimerHandle;
    int32 RemainingSpawnCount = 0;
    TWeakObjectPtr<const UEMRItemData> ActiveItemData;

    void CacheButtonTransforms();
    void ApplyDigitButtonState(int32 Digit, bool bPressed, bool bImmediate = false);
    void ApplyConfirmButtonState(bool bPressed, bool bImmediate = false);
    void ApplyResetButtonState(bool bPressed, bool bImmediate = false);
    void ApplyIncreaseButtonState(bool bPressed, bool bImmediate = false);
    void ApplyDecreaseButtonState(bool bPressed, bool bImmediate = false);
    void ResetAllDigitButtons();
    void UpdateDigitButtonLerp(float DeltaSeconds);
    void ConfigureButtonCollision(UPrimitiveComponent* Component) const;

    void GenerateSlotCodes();
    void UpdateSlotWidgets();
    void UpdateOrderSummaryWidget();
    void RefreshDisplayedTotalCostFromInput();
    float GetSalesDiscountPercentPerStack() const;
    float GetActiveSalesDiscountPercent() const;
    bool IsSalesDiscountActive() const;
    int32 GetDiscountedUnitPrice(int32 BaseUnitPrice) const;
    FLinearColor GetCurrentPriceTextColor() const;
    bool TryResolveSlotFromInput(int32& OutSlotIndex) const;
    bool TrySpendMoney(float Cost, UObject* SourceObject) const;
    void SpawnDispensedItem(const UEMRItemData* ItemData);
    void BeginOrderSelection(int32 SlotIndex);
    void CancelOrderSelection();
    void BeginDispensingOrder(AEMRPlayerCharacter* InInstigator);
    void HandleDispenseTick();
    void FinishOrder();
    void PlayResultCue(bool bSuccess);
    USoundBase* ResolveButtonPressSound(EEMRItemDispenserButtonType ButtonType) const;
    void HandleConfirmButtonRelease();
    void HandleResetButtonRelease();
    void HandleIncreaseButtonRelease();
    void HandleDecreaseButtonRelease();
    void ResetInputDigits();
    void HandleDispenseStartSoundDelay();
};
