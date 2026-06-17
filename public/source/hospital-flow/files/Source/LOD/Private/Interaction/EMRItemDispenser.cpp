#include "Interaction/EMRItemDispenser.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Data/EMREconomySystemGenerics.h"
#include "Framework/EMRAssetManager.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/Attributes/EMRTeamAttributeSet.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRInteractableComponent.h"
#include "LOD/EMRCollisionChannels.h"
#include "Net/UnrealNetwork.h"
#include "Shop/EMRItemActor.h"
#include "Shop/EMRItemData.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Interaction/EMRItemDispenserOrderWidget.h"
#include "UI/Interaction/EMRItemDispenserSlotWidget.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
    int32 BuildCodeValue(const TArray<int32>& Digits)
    {
        int32 CodeValue = 0;
        for (int32 Digit : Digits)
        {
            CodeValue = (CodeValue * 10) + FMath::Clamp(Digit, 0, 9);
        }
        return CodeValue;
    }

    int32 BuildRandomNonZeroCode()
    {
        TArray<int32> Digits;
        Digits.Reserve(9);
        for (int32 Digit = 1; Digit <= 9; ++Digit)
        {
            Digits.Add(Digit);
        }

        for (int32 Index = 0; Index < Digits.Num() - 1; ++Index)
        {
            const int32 SwapIndex = FMath::RandRange(Index, Digits.Num() - 1);
            Digits.Swap(Index, SwapIndex);
        }

        int32 CodeValue = 0;
        for (int32 DigitIndex = 0; DigitIndex < 4; ++DigitIndex)
        {
            CodeValue = (CodeValue * 10) + Digits[DigitIndex];
        }
        return CodeValue;
    }
}

AEMRItemDispenser::AEMRItemDispenser()
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(false);

    MachineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MachineMesh"));
    SetRootComponent(MachineMesh);

    TrayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrayMesh"));
    TrayMesh->SetupAttachment(MachineMesh);

    TrayItemAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("TrayItemAnchor"));
    TrayItemAnchor->SetupAttachment(TrayMesh);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
    InteractableComponent->SetDisplayName(FText::FromString(TEXT("Item Dispenser")));
    InteractableComponent->SetInteractionEventTag(EMRTags::Machine::ItemDispenser);

    OrderSummaryWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OrderSummaryWidget"));
    OrderSummaryWidget->SetupAttachment(MachineMesh);
    OrderSummaryWidget->SetWidgetSpace(EWidgetSpace::World);
    OrderSummaryWidget->SetDrawAtDesiredSize(true);
    OrderSummaryWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OrderSummaryWidget->SetGenerateOverlapEvents(false);
    OrderSummaryWidget->SetWindowFocusable(false);

}

void AEMRItemDispenser::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateDigitButtonLerp(DeltaSeconds);
}

void AEMRItemDispenser::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, SlotCodes);
    DOREPLIFETIME(ThisClass, PressedDigitsMask);
    DOREPLIFETIME(ThisClass, OrderState);
    DOREPLIFETIME(ThisClass, SelectedSlotIndex);
    DOREPLIFETIME(ThisClass, SelectedQuantity);
    DOREPLIFETIME(ThisClass, DisplayedTotalCost);
    DOREPLIFETIME(ThisClass, bConfirmButtonPressed);
    DOREPLIFETIME(ThisClass, bResetButtonPressed);
    DOREPLIFETIME(ThisClass, bIncreaseButtonPressed);
    DOREPLIFETIME(ThisClass, bDecreaseButtonPressed);
}

void AEMRItemDispenser::BeginPlay()
{
    Super::BeginPlay();

    CacheButtonTransforms();

    if (HasAuthority())
    {
        GenerateSlotCodes();
        OnRep_SlotCodes();
    }

    UpdateOrderSummaryWidget();
}

void AEMRItemDispenser::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    CacheButtonTransforms();
    UpdateSlotWidgets();
    UpdateOrderSummaryWidget();

    for (const FEMRItemDispenserDigitButtonDef& ButtonDef : DigitButtons)
    {
        if (UPrimitiveComponent* ButtonComponent = Cast<UPrimitiveComponent>(ButtonDef.ButtonMesh.GetComponent(this)))
        {
            ConfigureButtonCollision(ButtonComponent);
        }
    }

    if (UPrimitiveComponent* ConfirmComponent = Cast<UPrimitiveComponent>(ConfirmButtonMesh.GetComponent(this)))
    {
        ConfigureButtonCollision(ConfirmComponent);
    }

    if (UPrimitiveComponent* ResetComponent = Cast<UPrimitiveComponent>(ResetButtonMesh.GetComponent(this)))
    {
        ConfigureButtonCollision(ResetComponent);
    }

    if (UPrimitiveComponent* IncreaseComponent = Cast<UPrimitiveComponent>(IncreaseButtonMesh.GetComponent(this)))
    {
        ConfigureButtonCollision(IncreaseComponent);
    }

    if (UPrimitiveComponent* DecreaseComponent = Cast<UPrimitiveComponent>(DecreaseButtonMesh.GetComponent(this)))
    {
        ConfigureButtonCollision(DecreaseComponent);
    }
}

FGameplayEventData AEMRItemDispenser::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    if (!InteractableComponent)
    {
        return FGameplayEventData();
    }

    return InteractableComponent->BuildInteractionEventData(InterActor);
}

FText AEMRItemDispenser::GetInteractableDisplayName_Implementation() const
{
    return InteractableComponent ? InteractableComponent->GetDisplayName() : FText::FromString(GetName());
}

bool AEMRItemDispenser::IsInteractableEnabled_Implementation() const
{
    return InteractableComponent ? InteractableComponent->IsEnabled() : true;
}

void AEMRItemDispenser::HandleDigitPressed(int32 Digit, AEMRPlayerCharacter* InInstigator)
{
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Digit pressed=%d instigator=%s state=%d"),
        *GetNameSafe(this),
        Digit,
        *GetNameSafe(InInstigator),
        static_cast<int32>(OrderState));

    if (!HasAuthority() || OrderState != EEMRItemDispenserOrderState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Digit ignored (authority=%d, state=%d)"),
            *GetNameSafe(this),
            HasAuthority() ? 1 : 0,
            static_cast<int32>(OrderState));
        return;
    }

    if (Digit < 0 || Digit > 9)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Digit out of range: %d"), *GetNameSafe(this), Digit);
        return;
    }

    const uint16 DigitMask = static_cast<uint16>(1 << Digit);
    if ((PressedDigitsMask & DigitMask) != 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Digit already pressed: %d"), *GetNameSafe(this), Digit);
        return;
    }

    PressedDigitsMask |= DigitMask;
    CurrentInputDigits.Add(Digit);
    ApplyDigitButtonState(Digit, true);
    RefreshDisplayedTotalCostFromInput();

    Multicast_PlayButtonPressSound(EEMRItemDispenserButtonType::Digit);
}

void AEMRItemDispenser::HandleConfirmPressed(AEMRPlayerCharacter* InInstigator)
{
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Confirm pressed instigator=%s state=%d"),
        *GetNameSafe(this),
        *GetNameSafe(InInstigator),
        static_cast<int32>(OrderState));

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Confirm ignored (no authority)"), *GetNameSafe(this));
        return;
    }

    if (OrderState == EEMRItemDispenserOrderState::Dispensing)
    {
        return;
    }

    bConfirmButtonPressed = true;
    ApplyConfirmButtonState(true);

    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(ConfirmButtonReleaseTimerHandle);
        if (QuantityButtonPressHoldDuration > 0.f)
        {
            TimerManager.SetTimer(
                ConfirmButtonReleaseTimerHandle,
                this,
                &ThisClass::HandleConfirmButtonRelease,
                QuantityButtonPressHoldDuration,
                false);
        }
        else
        {
            HandleConfirmButtonRelease();
        }
    }

    Multicast_PlayButtonPressSound(EEMRItemDispenserButtonType::Confirm);

    int32 SlotIndex = INDEX_NONE;
    const bool bResolvedSlot = TryResolveSlotFromInput(SlotIndex) && Slots.IsValidIndex(SlotIndex);
    ResetInputDigits();

    if (!bResolvedSlot)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Confirm failed: invalid code (resolved=%d)"),
            *GetNameSafe(this),
            SlotIndex);
        PlayResultCue(false);
        return;
    }

    const UEMRItemData* ItemData = Slots[SlotIndex].ItemData;
    if (!ItemData || !ItemData->GetWorldItemClass())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Confirm failed: missing item data/class for slot %d"),
            *GetNameSafe(this),
            SlotIndex);
        PlayResultCue(false);
        return;
    }

    BeginOrderSelection(SlotIndex);
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Order selected slot=%d quantity=%d"),
        *GetNameSafe(this),
        SlotIndex,
        SelectedQuantity);
    BeginDispensingOrder(InInstigator);
}

void AEMRItemDispenser::HandleResetPressed(AEMRPlayerCharacter* InInstigator)
{
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Reset pressed instigator=%s state=%d"),
        *GetNameSafe(this),
        *GetNameSafe(InInstigator),
        static_cast<int32>(OrderState));

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Reset ignored (no authority)"), *GetNameSafe(this));
        return;
    }

    if (OrderState == EEMRItemDispenserOrderState::Dispensing)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Reset ignored (dispensing)"), *GetNameSafe(this));
        return;
    }

    bResetButtonPressed = true;
    ApplyResetButtonState(true);

    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(ResetButtonReleaseTimerHandle);
        if (QuantityButtonPressHoldDuration > 0.f)
        {
            TimerManager.SetTimer(
                ResetButtonReleaseTimerHandle,
                this,
                &ThisClass::HandleResetButtonRelease,
                QuantityButtonPressHoldDuration,
                false);
        }
        else
        {
            HandleResetButtonRelease();
        }
    }

    Multicast_PlayButtonPressSound(EEMRItemDispenserButtonType::Reset);

    if (OrderState == EEMRItemDispenserOrderState::Selected)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Order cancelled"), *GetNameSafe(this));
        CancelOrderSelection();
        return;
    }

    SelectedQuantity = 1;
    ResetInputDigits();
}

void AEMRItemDispenser::HandleIncreasePressed(AEMRPlayerCharacter* InInstigator)
{
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Increase pressed state=%d quantity=%d"),
        *GetNameSafe(this),
        static_cast<int32>(OrderState),
        SelectedQuantity);

    if (!HasAuthority() || OrderState == EEMRItemDispenserOrderState::Dispensing)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Increase ignored (authority=%d, state=%d)"),
            *GetNameSafe(this),
            HasAuthority() ? 1 : 0,
            static_cast<int32>(OrderState));
        return;
    }

    SelectedQuantity = FMath::Clamp(SelectedQuantity + 1, 1, MaxOrderQuantity);
    RefreshDisplayedTotalCostFromInput();
    bIncreaseButtonPressed = true;
    ApplyIncreaseButtonState(true);

    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(IncreaseButtonReleaseTimerHandle);
        if (QuantityButtonPressHoldDuration > 0.f)
        {
            TimerManager.SetTimer(
                IncreaseButtonReleaseTimerHandle,
                this,
                &ThisClass::HandleIncreaseButtonRelease,
                QuantityButtonPressHoldDuration,
                false);
        }
        else
        {
            HandleIncreaseButtonRelease();
        }
    }

    Multicast_PlayButtonPressSound(EEMRItemDispenserButtonType::Increase);
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Quantity increased to %d"), *GetNameSafe(this), SelectedQuantity);
}

void AEMRItemDispenser::HandleDecreasePressed(AEMRPlayerCharacter* InInstigator)
{
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Decrease pressed state=%d quantity=%d"),
        *GetNameSafe(this),
        static_cast<int32>(OrderState),
        SelectedQuantity);

    if (!HasAuthority() || OrderState == EEMRItemDispenserOrderState::Dispensing)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Decrease ignored (authority=%d, state=%d)"),
            *GetNameSafe(this),
            HasAuthority() ? 1 : 0,
            static_cast<int32>(OrderState));
        return;
    }

    SelectedQuantity = FMath::Clamp(SelectedQuantity - 1, 1, MaxOrderQuantity);
    RefreshDisplayedTotalCostFromInput();
    bDecreaseButtonPressed = true;
    ApplyDecreaseButtonState(true);

    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(DecreaseButtonReleaseTimerHandle);
        if (QuantityButtonPressHoldDuration > 0.f)
        {
            TimerManager.SetTimer(
                DecreaseButtonReleaseTimerHandle,
                this,
                &ThisClass::HandleDecreaseButtonRelease,
                QuantityButtonPressHoldDuration,
                false);
        }
        else
        {
            HandleDecreaseButtonRelease();
        }
    }

    Multicast_PlayButtonPressSound(EEMRItemDispenserButtonType::Decrease);
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Quantity decreased to %d"), *GetNameSafe(this), SelectedQuantity);
}

bool AEMRItemDispenser::GetDigitForComponent(const UPrimitiveComponent* Component, int32& OutDigit) const
{
    if (!Component)
    {
        return false;
    }

    for (const FEMRItemDispenserDigitButtonDef& ButtonDef : DigitButtons)
    {
        UPrimitiveComponent* ButtonComponent = Cast<UPrimitiveComponent>(ButtonDef.ButtonMesh.GetComponent(const_cast<AEMRItemDispenser*>(this)));
        if (ButtonComponent && Component == ButtonComponent)
        {
            OutDigit = ButtonDef.Digit;
            return true;
        }
    }

    return false;
}

bool AEMRItemDispenser::IsConfirmButtonComponent(const UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return false;
    }

    UPrimitiveComponent* ConfirmComponent = Cast<UPrimitiveComponent>(ConfirmButtonMesh.GetComponent(const_cast<AEMRItemDispenser*>(this)));
    return ConfirmComponent && Component == ConfirmComponent;
}

bool AEMRItemDispenser::IsResetButtonComponent(const UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return false;
    }

    UPrimitiveComponent* ResetComponent = Cast<UPrimitiveComponent>(ResetButtonMesh.GetComponent(const_cast<AEMRItemDispenser*>(this)));
    return ResetComponent && Component == ResetComponent;
}

bool AEMRItemDispenser::IsIncreaseButtonComponent(const UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return false;
    }

    UPrimitiveComponent* IncreaseComponent = Cast<UPrimitiveComponent>(IncreaseButtonMesh.GetComponent(const_cast<AEMRItemDispenser*>(this)));
    return IncreaseComponent && Component == IncreaseComponent;
}

bool AEMRItemDispenser::IsDecreaseButtonComponent(const UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return false;
    }

    UPrimitiveComponent* DecreaseComponent = Cast<UPrimitiveComponent>(DecreaseButtonMesh.GetComponent(const_cast<AEMRItemDispenser*>(this)));
    return DecreaseComponent && Component == DecreaseComponent;
}

void AEMRItemDispenser::OnRep_SlotCodes()
{
    UpdateSlotWidgets();
}

void AEMRItemDispenser::OnRep_PressedDigitsMask()
{
    if (DigitButtonStartTransforms.IsEmpty())
    {
        CacheButtonTransforms();
    }

    for (const FEMRItemDispenserDigitButtonDef& ButtonDef : DigitButtons)
    {
        const int32 Digit = ButtonDef.Digit;
        const uint16 DigitMask = static_cast<uint16>(1 << Digit);
        const bool bPressed = (PressedDigitsMask & DigitMask) != 0;
        ApplyDigitButtonState(Digit, bPressed);
    }
}

void AEMRItemDispenser::OnRep_OrderState()
{
}

void AEMRItemDispenser::OnRep_SelectedSlotIndex()
{
}

void AEMRItemDispenser::OnRep_SelectedQuantity()
{
    UpdateOrderSummaryWidget();
}

void AEMRItemDispenser::OnRep_DisplayedTotalCost()
{
    UpdateOrderSummaryWidget();
}

void AEMRItemDispenser::OnRep_IncreaseButtonPressed()
{
    if (!bHasIncreaseButtonStartTransform)
    {
        CacheButtonTransforms();
    }
    ApplyIncreaseButtonState(bIncreaseButtonPressed);
}

void AEMRItemDispenser::OnRep_DecreaseButtonPressed()
{
    if (!bHasDecreaseButtonStartTransform)
    {
        CacheButtonTransforms();
    }
    ApplyDecreaseButtonState(bDecreaseButtonPressed);
}

void AEMRItemDispenser::OnRep_ConfirmButtonPressed()
{
    if (!bHasConfirmButtonStartTransform)
    {
        CacheButtonTransforms();
    }
    ApplyConfirmButtonState(bConfirmButtonPressed);
}

void AEMRItemDispenser::OnRep_ResetButtonPressed()
{
    if (!bHasResetButtonStartTransform)
    {
        CacheButtonTransforms();
    }
    ApplyResetButtonState(bResetButtonPressed);
}

void AEMRItemDispenser::Multicast_PlayDispenseStartSound_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer || !DispenseStartSound)
    {
        return;
    }

    const FVector SoundLocation = TrayItemAnchor ? TrayItemAnchor->GetComponentLocation() : GetActorLocation();
    UGameplayStatics::PlaySoundAtLocation(this, DispenseStartSound, SoundLocation);
}

void AEMRItemDispenser::Multicast_PlayButtonPressSound_Implementation(EEMRItemDispenserButtonType ButtonType)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    USoundBase* ButtonSound = ResolveButtonPressSound(ButtonType);
    if (!ButtonSound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, ButtonSound, GetActorLocation());
}

void AEMRItemDispenser::CacheButtonTransforms()
{
    DigitButtonStartTransforms.Reset();
    DigitButtonTargetLocations.Reset();
    bHasConfirmButtonStartTransform = false;
    bHasResetButtonStartTransform = false;
    bHasIncreaseButtonStartTransform = false;
    bHasDecreaseButtonStartTransform = false;
    bHasConfirmButtonTargetLocation = false;
    bHasResetButtonTargetLocation = false;
    bHasIncreaseButtonTargetLocation = false;
    bHasDecreaseButtonTargetLocation = false;

    for (const FEMRItemDispenserDigitButtonDef& ButtonDef : DigitButtons)
    {
        if (UPrimitiveComponent* ButtonComponent = Cast<UPrimitiveComponent>(ButtonDef.ButtonMesh.GetComponent(this)))
        {
            const FTransform StartTransform = ButtonComponent->GetRelativeTransform();
            DigitButtonStartTransforms.FindOrAdd(ButtonDef.Digit) = StartTransform;
            DigitButtonTargetLocations.FindOrAdd(ButtonDef.Digit) = StartTransform.GetLocation();
        }
    }

    if (UPrimitiveComponent* ConfirmButtonComponent = Cast<UPrimitiveComponent>(ConfirmButtonMesh.GetComponent(this)))
    {
        ConfirmButtonStartTransform = ConfirmButtonComponent->GetRelativeTransform();
        ConfirmButtonTargetLocation = ConfirmButtonStartTransform.GetLocation();
        bHasConfirmButtonStartTransform = true;
        bHasConfirmButtonTargetLocation = true;
    }

    if (UPrimitiveComponent* ResetButtonComponent = Cast<UPrimitiveComponent>(ResetButtonMesh.GetComponent(this)))
    {
        ResetButtonStartTransform = ResetButtonComponent->GetRelativeTransform();
        ResetButtonTargetLocation = ResetButtonStartTransform.GetLocation();
        bHasResetButtonStartTransform = true;
        bHasResetButtonTargetLocation = true;
    }

    if (UPrimitiveComponent* IncreaseButtonComponent = Cast<UPrimitiveComponent>(IncreaseButtonMesh.GetComponent(this)))
    {
        IncreaseButtonStartTransform = IncreaseButtonComponent->GetRelativeTransform();
        IncreaseButtonTargetLocation = IncreaseButtonStartTransform.GetLocation();
        bHasIncreaseButtonStartTransform = true;
        bHasIncreaseButtonTargetLocation = true;
    }

    if (UPrimitiveComponent* DecreaseButtonComponent = Cast<UPrimitiveComponent>(DecreaseButtonMesh.GetComponent(this)))
    {
        DecreaseButtonStartTransform = DecreaseButtonComponent->GetRelativeTransform();
        DecreaseButtonTargetLocation = DecreaseButtonStartTransform.GetLocation();
        bHasDecreaseButtonStartTransform = true;
        bHasDecreaseButtonTargetLocation = true;
    }
}

void AEMRItemDispenser::ApplyDigitButtonState(int32 Digit, bool bPressed, bool bImmediate)
{
    const FEMRItemDispenserDigitButtonDef* ButtonDef = DigitButtons.FindByPredicate([Digit](const FEMRItemDispenserDigitButtonDef& Def)
    {
        return Def.Digit == Digit;
    });

    if (!ButtonDef)
    {
        return;
    }

    UPrimitiveComponent* ButtonComponent = Cast<UPrimitiveComponent>(ButtonDef->ButtonMesh.GetComponent(this));
    if (!ButtonComponent)
    {
        return;
    }

    const FTransform* StartTransform = DigitButtonStartTransforms.Find(Digit);
    if (!StartTransform)
    {
        return;
    }

    FVector TargetLocation = StartTransform->GetLocation();
    if (bPressed)
    {
        TargetLocation += DigitButtonPressOffset;
    }

    DigitButtonTargetLocations.FindOrAdd(Digit) = TargetLocation;

    if (bImmediate || DigitButtonLerpSpeed <= 0.f)
    {
        ButtonComponent->SetRelativeLocation(TargetLocation);
        return;
    }

    SetActorTickEnabled(true);
}

void AEMRItemDispenser::ApplyConfirmButtonState(bool bPressed, bool bImmediate)
{
    UPrimitiveComponent* ConfirmButtonComponent = Cast<UPrimitiveComponent>(ConfirmButtonMesh.GetComponent(this));
    if (!ConfirmButtonComponent || !bHasConfirmButtonStartTransform)
    {
        return;
    }

    ConfirmButtonTargetLocation = ConfirmButtonStartTransform.GetLocation();
    if (bPressed)
    {
        ConfirmButtonTargetLocation += QuantityButtonPressOffset;
    }
    bHasConfirmButtonTargetLocation = true;

    if (bImmediate || DigitButtonLerpSpeed <= 0.f)
    {
        ConfirmButtonComponent->SetRelativeLocation(ConfirmButtonTargetLocation);
        return;
    }

    SetActorTickEnabled(true);
}

void AEMRItemDispenser::ApplyResetButtonState(bool bPressed, bool bImmediate)
{
    UPrimitiveComponent* ResetButtonComponent = Cast<UPrimitiveComponent>(ResetButtonMesh.GetComponent(this));
    if (!ResetButtonComponent || !bHasResetButtonStartTransform)
    {
        return;
    }

    ResetButtonTargetLocation = ResetButtonStartTransform.GetLocation();
    if (bPressed)
    {
        ResetButtonTargetLocation += QuantityButtonPressOffset;
    }
    bHasResetButtonTargetLocation = true;

    if (bImmediate || DigitButtonLerpSpeed <= 0.f)
    {
        ResetButtonComponent->SetRelativeLocation(ResetButtonTargetLocation);
        return;
    }

    SetActorTickEnabled(true);
}

void AEMRItemDispenser::ApplyIncreaseButtonState(bool bPressed, bool bImmediate)
{
    UPrimitiveComponent* IncreaseButtonComponent = Cast<UPrimitiveComponent>(IncreaseButtonMesh.GetComponent(this));
    if (!IncreaseButtonComponent || !bHasIncreaseButtonStartTransform)
    {
        return;
    }

    IncreaseButtonTargetLocation = IncreaseButtonStartTransform.GetLocation();
    if (bPressed)
    {
        IncreaseButtonTargetLocation += QuantityButtonPressOffset;
    }
    bHasIncreaseButtonTargetLocation = true;

    if (bImmediate || DigitButtonLerpSpeed <= 0.f)
    {
        IncreaseButtonComponent->SetRelativeLocation(IncreaseButtonTargetLocation);
        return;
    }

    SetActorTickEnabled(true);
}

void AEMRItemDispenser::ApplyDecreaseButtonState(bool bPressed, bool bImmediate)
{
    UPrimitiveComponent* DecreaseButtonComponent = Cast<UPrimitiveComponent>(DecreaseButtonMesh.GetComponent(this));
    if (!DecreaseButtonComponent || !bHasDecreaseButtonStartTransform)
    {
        return;
    }

    DecreaseButtonTargetLocation = DecreaseButtonStartTransform.GetLocation();
    if (bPressed)
    {
        DecreaseButtonTargetLocation += QuantityButtonPressOffset;
    }
    bHasDecreaseButtonTargetLocation = true;

    if (bImmediate || DigitButtonLerpSpeed <= 0.f)
    {
        DecreaseButtonComponent->SetRelativeLocation(DecreaseButtonTargetLocation);
        return;
    }

    SetActorTickEnabled(true);
}

void AEMRItemDispenser::ResetAllDigitButtons()
{
    for (const FEMRItemDispenserDigitButtonDef& ButtonDef : DigitButtons)
    {
        ApplyDigitButtonState(ButtonDef.Digit, false);
    }
}

void AEMRItemDispenser::UpdateDigitButtonLerp(float DeltaSeconds)
{
    const bool bHasButtonTargets = !DigitButtonTargetLocations.IsEmpty()
    || bHasConfirmButtonTargetLocation
    || bHasResetButtonTargetLocation
    || bHasIncreaseButtonTargetLocation
    || bHasDecreaseButtonTargetLocation;
    if (!bHasButtonTargets || DigitButtonLerpSpeed <= 0.f)
    {
        SetActorTickEnabled(false);
        return;
    }

    bool bAnyButtonMoving = false;
    for (const FEMRItemDispenserDigitButtonDef& ButtonDef : DigitButtons)
    {
        UPrimitiveComponent* ButtonComponent = Cast<UPrimitiveComponent>(ButtonDef.ButtonMesh.GetComponent(this));
        const FVector* TargetLocation = DigitButtonTargetLocations.Find(ButtonDef.Digit);
        if (!ButtonComponent || !TargetLocation)
        {
            continue;
        }

        const FVector CurrentLocation = ButtonComponent->GetRelativeLocation();
        const FVector NewLocation = FMath::VInterpTo(CurrentLocation, *TargetLocation, DeltaSeconds, DigitButtonLerpSpeed);
        ButtonComponent->SetRelativeLocation(NewLocation);

        if (!NewLocation.Equals(*TargetLocation, 0.01f))
        {
            bAnyButtonMoving = true;
        }
        else if (!CurrentLocation.Equals(*TargetLocation, 0.01f))
        {
            ButtonComponent->SetRelativeLocation(*TargetLocation);
        }
    }

    if (bHasConfirmButtonTargetLocation)
    {
        if (UPrimitiveComponent* ConfirmButtonComponent = Cast<UPrimitiveComponent>(ConfirmButtonMesh.GetComponent(this)))
        {
            const FVector CurrentLocation = ConfirmButtonComponent->GetRelativeLocation();
            const FVector NewLocation = FMath::VInterpTo(CurrentLocation, ConfirmButtonTargetLocation, DeltaSeconds, DigitButtonLerpSpeed);
            ConfirmButtonComponent->SetRelativeLocation(NewLocation);

            if (!NewLocation.Equals(ConfirmButtonTargetLocation, 0.01f))
            {
                bAnyButtonMoving = true;
            }
            else if (!CurrentLocation.Equals(ConfirmButtonTargetLocation, 0.01f))
            {
                ConfirmButtonComponent->SetRelativeLocation(ConfirmButtonTargetLocation);
            }
        }
    }

    if (bHasResetButtonTargetLocation)
    {
        if (UPrimitiveComponent* ResetButtonComponent = Cast<UPrimitiveComponent>(ResetButtonMesh.GetComponent(this)))
        {
            const FVector CurrentLocation = ResetButtonComponent->GetRelativeLocation();
            const FVector NewLocation = FMath::VInterpTo(CurrentLocation, ResetButtonTargetLocation, DeltaSeconds, DigitButtonLerpSpeed);
            ResetButtonComponent->SetRelativeLocation(NewLocation);

            if (!NewLocation.Equals(ResetButtonTargetLocation, 0.01f))
            {
                bAnyButtonMoving = true;
            }
            else if (!CurrentLocation.Equals(ResetButtonTargetLocation, 0.01f))
            {
                ResetButtonComponent->SetRelativeLocation(ResetButtonTargetLocation);
            }
        }
    }

    if (bHasIncreaseButtonTargetLocation)
    {
        if (UPrimitiveComponent* IncreaseButtonComponent = Cast<UPrimitiveComponent>(IncreaseButtonMesh.GetComponent(this)))
        {
            const FVector CurrentLocation = IncreaseButtonComponent->GetRelativeLocation();
            const FVector NewLocation = FMath::VInterpTo(CurrentLocation, IncreaseButtonTargetLocation, DeltaSeconds, DigitButtonLerpSpeed);
            IncreaseButtonComponent->SetRelativeLocation(NewLocation);

            if (!NewLocation.Equals(IncreaseButtonTargetLocation, 0.01f))
            {
                bAnyButtonMoving = true;
            }
            else if (!CurrentLocation.Equals(IncreaseButtonTargetLocation, 0.01f))
            {
                IncreaseButtonComponent->SetRelativeLocation(IncreaseButtonTargetLocation);
            }
        }
    }

    if (bHasDecreaseButtonTargetLocation)
    {
        if (UPrimitiveComponent* DecreaseButtonComponent = Cast<UPrimitiveComponent>(DecreaseButtonMesh.GetComponent(this)))
        {
            const FVector CurrentLocation = DecreaseButtonComponent->GetRelativeLocation();
            const FVector NewLocation = FMath::VInterpTo(CurrentLocation, DecreaseButtonTargetLocation, DeltaSeconds, DigitButtonLerpSpeed);
            DecreaseButtonComponent->SetRelativeLocation(NewLocation);

            if (!NewLocation.Equals(DecreaseButtonTargetLocation, 0.01f))
            {
                bAnyButtonMoving = true;
            }
            else if (!CurrentLocation.Equals(DecreaseButtonTargetLocation, 0.01f))
            {
                DecreaseButtonComponent->SetRelativeLocation(DecreaseButtonTargetLocation);
            }
        }
    }

    if (!bAnyButtonMoving)
    {
        SetActorTickEnabled(false);
    }
}

void AEMRItemDispenser::ConfigureButtonCollision(UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return;
    }

    Component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Component->SetCollisionResponseToAllChannels(ECR_Ignore);
    Component->SetCollisionResponseToChannel(ECC_InteractableWidgets, ECR_Block);
    Component->SetGenerateOverlapEvents(false);
}

void AEMRItemDispenser::GenerateSlotCodes()
{
    const int32 SlotCount = Slots.Num();
    SlotCodes.SetNum(SlotCount);

    TSet<int32> UsedCodes;
    for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
    {
        int32 CodeValue = 0;
        bool bAssigned = false;
        for (int32 Attempt = 0; Attempt < 100; ++Attempt)
        {
            CodeValue = BuildRandomNonZeroCode();
            if (!UsedCodes.Contains(CodeValue))
            {
                bAssigned = true;
                break;
            }
        }

        if (!bAssigned)
        {
            CodeValue = BuildRandomNonZeroCode();
        }

        UsedCodes.Add(CodeValue);
        SlotCodes[SlotIndex] = CodeValue;
    }
}

void AEMRItemDispenser::UpdateSlotWidgets()
{
    const FLinearColor CurrentPriceTextColor = GetCurrentPriceTextColor();

    for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
    {
        const FEMRItemDispenserSlotDef& SlotDef = Slots[SlotIndex];
        if (UStaticMeshComponent* DisplayComponent = Cast<UStaticMeshComponent>(SlotDef.DisplayMesh.GetComponent(const_cast<AEMRItemDispenser*>(this))))
        {
            if (SlotDef.ItemData)
            {
                if (UStaticMesh* ItemMesh = SlotDef.ItemData->GetWorldItemMesh())
                {
                    DisplayComponent->SetStaticMesh(ItemMesh);
                }
            }
        }

        UWidgetComponent* SlotWidgetComponent = Cast<UWidgetComponent>(SlotDef.SlotWidget.GetComponent(const_cast<AEMRItemDispenser*>(this)));
        if (!SlotWidgetComponent)
        {
            continue;
        }

        if (!SlotWidgetComponent->GetWidget())
        {
            SlotWidgetComponent->InitWidget();
        }

        UEMRItemDispenserSlotWidget* SlotWidget = Cast<UEMRItemDispenserSlotWidget>(SlotWidgetComponent->GetUserWidgetObject());
        if (!SlotWidget)
        {
            continue;
        }

        const int32 CodeValue = SlotCodes.IsValidIndex(SlotIndex) ? SlotCodes[SlotIndex] : 0;
        const FString CodeString = FString::Printf(TEXT("%04d"), CodeValue);
        SlotWidget->SetProductCodeText(FText::FromString(CodeString));

        const int32 BaseCostValue = SlotDef.ItemData ? SlotDef.ItemData->GetItemPrice() : 0;
        const int32 CostValue = GetDiscountedUnitPrice(BaseCostValue);
        SlotWidget->SetCostText(FText::AsNumber(CostValue));
        SlotWidget->SetCostColor(CurrentPriceTextColor);
    }
}

void AEMRItemDispenser::UpdateOrderSummaryWidget()
{
    if (!OrderSummaryWidget)
    {
        return;
    }

    if (!OrderSummaryWidget->GetUserWidgetObject())
    {
        OrderSummaryWidget->InitWidget();
    }

    UEMRItemDispenserOrderWidget* SummaryWidget = Cast<UEMRItemDispenserOrderWidget>(OrderSummaryWidget->GetUserWidgetObject());
    if (!SummaryWidget)
    {
        return;
    }

    SummaryWidget->SetQuantityText(FText::AsNumber(FMath::Max(1, SelectedQuantity)));
    SummaryWidget->SetTotalCostText(FText::AsNumber(FMath::Max(0, DisplayedTotalCost)));
    SummaryWidget->SetTotalCostColor(GetCurrentPriceTextColor());
}

void AEMRItemDispenser::RefreshDisplayedTotalCostFromInput()
{
    int32 PreviewSlotIndex = INDEX_NONE;
    int32 NewCost = 0;
    if (TryResolveSlotFromInput(PreviewSlotIndex) && Slots.IsValidIndex(PreviewSlotIndex))
    {
        if (const UEMRItemData* PreviewItemData = Slots[PreviewSlotIndex].ItemData)
        {
            const int32 UnitPrice = GetDiscountedUnitPrice(PreviewItemData->GetItemPrice());
            NewCost = FMath::Max(0, UnitPrice * FMath::Max(1, SelectedQuantity));
        }
    }

    DisplayedTotalCost = NewCost;
    UpdateOrderSummaryWidget();
}

float AEMRItemDispenser::GetSalesDiscountPercentPerStack() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return 0.0f;
    }

    const AEMRNightShiftGameState* GameState = World->GetGameState<AEMRNightShiftGameState>();
    if (!GameState)
    {
        return 0.0f;
    }

    const UEMRDifficultyTuningData* DifficultyTuning = GameState->GetDifficultyTuningData();
    if (!DifficultyTuning)
    {
        return 0.0f;
    }

    return FMath::Max(0.0f, DifficultyTuning->GetItemDispenserSalesUpgradeTuning().DiscountPercentPerStack);
}

float AEMRItemDispenser::GetActiveSalesDiscountPercent() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return 0.0f;
    }

    const AEMRNightShiftGameState* GameState = World->GetGameState<AEMRNightShiftGameState>();
    if (!GameState)
    {
        return 0.0f;
    }

    const int32 StackCount = FMath::Max(0, GameState->GetActiveRunUpgradeStackCount(EMRTags::RunUpgrade::ItemDispenserSales));
    const float TotalPercent = static_cast<float>(StackCount) * GetSalesDiscountPercentPerStack();
    return FMath::Max(0.0f, TotalPercent);
}

bool AEMRItemDispenser::IsSalesDiscountActive() const
{
    return GetActiveSalesDiscountPercent() > 0.0f;
}

int32 AEMRItemDispenser::GetDiscountedUnitPrice(const int32 BaseUnitPrice) const
{
    const float SafeBasePrice = static_cast<float>(FMath::Max(0, BaseUnitPrice));
    const float DiscountPercent = FMath::Max(0.0f, GetActiveSalesDiscountPercent());
    const float DiscountMultiplier = FMath::Max(0.0f, 1.0f - (DiscountPercent / 100.0f));
    const float DiscountedPrice = FMath::FloorToFloat(SafeBasePrice * DiscountMultiplier);
    return FMath::Max(0, FMath::FloorToInt(DiscountedPrice));
}

FLinearColor AEMRItemDispenser::GetCurrentPriceTextColor() const
{
    if (!IsSalesDiscountActive())
    {
        return FLinearColor::White;
    }

    const UWorld* World = GetWorld();
    if (!World)
    {
        return FLinearColor::White;
    }

    const AEMRNightShiftGameState* GameState = World->GetGameState<AEMRNightShiftGameState>();
    if (!GameState)
    {
        return FLinearColor::White;
    }

    const UEMRDifficultyTuningData* DifficultyTuning = GameState->GetDifficultyTuningData();
    if (!DifficultyTuning)
    {
        return FLinearColor::White;
    }

    return DifficultyTuning->GetItemDispenserSalesUpgradeTuning().DiscountedPriceColor;
}

bool AEMRItemDispenser::TryResolveSlotFromInput(int32& OutSlotIndex) const
{
    OutSlotIndex = INDEX_NONE;

    if (CurrentInputDigits.Num() != 4)
    {
        return false;
    }

    const int32 CodeValue = BuildCodeValue(CurrentInputDigits);
    for (int32 SlotIndex = 0; SlotIndex < SlotCodes.Num(); ++SlotIndex)
    {
        if (SlotCodes[SlotIndex] == CodeValue)
        {
            OutSlotIndex = SlotIndex;
            return true;
        }
    }

    return false;
}

bool AEMRItemDispenser::TrySpendMoney(float Cost, UObject* SourceObject) const
{
    if (Cost <= 0.f)
    {
        UE_LOG(LogTemp, Log, TEXT("[ItemDispenser] Spend skipped (cost=%.2f) dispenser=%s"), Cost, *GetNameSafe(this));
        return true;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Spend failed: no World dispenser=%s"), *GetNameSafe(this));
        return false;
    }

    AEMRNightShiftGameState* GameState = World->GetGameState<AEMRNightShiftGameState>();
    if (!GameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Spend failed: no GameState dispenser=%s"), *GetNameSafe(this));
        return false;
    }

    UAbilitySystemComponent* TeamASC = GameState->GetAbilitySystemComponent();
    if (!TeamASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Spend failed: no TeamASC dispenser=%s"), *GetNameSafe(this));
        return false;
    }

    TArray<const UEMREconomySystemGenerics*> LoadedEconomy;
    if (!UEMRAssetManager::Get().CollectLoadedEconomySystemGenerics(LoadedEconomy) || LoadedEconomy.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Spend failed: EconomySystemGenerics not loaded dispenser=%s"), *GetNameSafe(this));
        return false;
    }

    const UEMREconomySystemGenerics* EconomyConfig = LoadedEconomy[0];
    if (!EconomyConfig || !EconomyConfig->GetSpendMoneyEffect())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Spend failed: SpendMoneyEffect missing dispenser=%s"), *GetNameSafe(this));
        return false;
    }

    const float PreRevenue = TeamASC->GetNumericAttribute(UEMRTeamAttributeSet::GetTotalRevenueAttribute());

    FGameplayEffectContextHandle EffectContext = TeamASC->MakeEffectContext();
    EffectContext.AddSourceObject(SourceObject);

    FGameplayEffectSpecHandle SpecHandle = TeamASC->MakeOutgoingSpec(EconomyConfig->GetSpendMoneyEffect(), 1.0f, EffectContext);
    if (!SpecHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Spend failed: GE spec invalid dispenser=%s"), *GetNameSafe(this));
        return false;
    }

    SpecHandle.Data->SetSetByCallerMagnitude(EMRTags::SetByCaller::SpendMoney, -Cost);
    TeamASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

    const float PostRevenue = TeamASC->GetNumericAttribute(UEMRTeamAttributeSet::GetTotalRevenueAttribute());
    UE_LOG(LogTemp, Log, TEXT("[ItemDispenser] Spend applied cost=%.2f revenue %.2f -> %.2f dispenser=%s"),
        Cost, PreRevenue, PostRevenue, *GetNameSafe(this));
    return true;
}

void AEMRItemDispenser::SpawnDispensedItem(const UEMRItemData* ItemData)
{
    if (!ItemData)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const TSubclassOf<AActor> ItemClass = ItemData->GetWorldItemClass();
    if (!ItemClass)
    {
        return;
    }

    FTransform SpawnTransform = GetActorTransform();
    if (ItemSpawnPoint)
    {
        SpawnTransform = ItemSpawnPoint->GetActorTransform();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s missing ItemSpawnPoint, spawning at dispenser."), *GetName());
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* SpawnedActor = World->SpawnActor<AActor>(ItemClass, SpawnTransform, SpawnParams);
    if (!SpawnedActor)
    {
        return;
    }

    if (AEMRItemActor* ItemActor = Cast<AEMRItemActor>(SpawnedActor))
    {
        ItemActor->SetItemData(ItemData);
        ItemActor->SetQuantity(1);
    }

    TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(SpawnedActor);
    for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
    {
        if (PrimitiveComponent)
        {
            PrimitiveComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        }
    }
}

void AEMRItemDispenser::BeginOrderSelection(int32 SlotIndex)
{
    if (!Slots.IsValidIndex(SlotIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s BeginOrderSelection invalid slot=%d"), *GetNameSafe(this), SlotIndex);
        return;
    }

    ActiveItemData = Slots[SlotIndex].ItemData;
    SelectedSlotIndex = SlotIndex;
    SelectedQuantity = FMath::Clamp(SelectedQuantity, 1, MaxOrderQuantity);
    OrderState = EEMRItemDispenserOrderState::Selected;
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s OrderState=Selected slot=%d"), *GetNameSafe(this), SlotIndex);
    RefreshDisplayedTotalCostFromInput();
}

void AEMRItemDispenser::CancelOrderSelection()
{
    OrderState = EEMRItemDispenserOrderState::Idle;
    SelectedSlotIndex = INDEX_NONE;
    SelectedQuantity = 1;
    ActiveItemData.Reset();
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s OrderState=Idle (cancel)"), *GetNameSafe(this));

    ResetInputDigits();
}

void AEMRItemDispenser::BeginDispensingOrder(AEMRPlayerCharacter* InInstigator)
{
    if (!HasAuthority() || OrderState != EEMRItemDispenserOrderState::Selected)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s BeginDispensing ignored (authority=%d, state=%d)"),
            *GetNameSafe(this),
            HasAuthority() ? 1 : 0,
            static_cast<int32>(OrderState));
        return;
    }

    const UEMRItemData* ItemData = ActiveItemData.Get();
    if (!ItemData || !ItemData->GetWorldItemClass())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Dispense failed: missing item data/class"), *GetNameSafe(this));
        PlayResultCue(false);
        return;
    }

    const int32 DiscountedUnitPrice = GetDiscountedUnitPrice(ItemData->GetItemPrice());
    const float Cost = static_cast<float>(FMath::Max(0, DiscountedUnitPrice * FMath::Max(0, SelectedQuantity)));
    if (!TrySpendMoney(Cost, this))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Dispense failed: spend money cost=%.2f"), *GetNameSafe(this), Cost);
        PlayResultCue(false);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DispenseTimerHandle);
    }

    const int32 OrderedQuantity = FMath::Max(SelectedQuantity, 0);

    OrderState = EEMRItemDispenserOrderState::Dispensing;
    RemainingSpawnCount = OrderedQuantity;
    SelectedQuantity = 1;
    UpdateOrderSummaryWidget();
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s OrderState=Dispensing remaining=%d"), *GetNameSafe(this), RemainingSpawnCount);

    PlayResultCue(true);

    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(DispenseStartSoundTimerHandle);

        if (DispenseStartSound)
        {
            if (DispenseStartSoundDelay > 0.f)
            {
                TimerManager.SetTimer(
                    DispenseStartSoundTimerHandle,
                    this,
                    &ThisClass::HandleDispenseStartSoundDelay,
                    DispenseStartSoundDelay,
                    false);
            }
            else
            {
                Multicast_PlayDispenseStartSound();
            }
        }
    }

    HandleDispenseTick();
}

void AEMRItemDispenser::HandleDispenseTick()
{
    if (!HasAuthority() || OrderState != EEMRItemDispenserOrderState::Dispensing)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Dispense tick ignored (authority=%d, state=%d)"),
            *GetNameSafe(this),
            HasAuthority() ? 1 : 0,
            static_cast<int32>(OrderState));
        return;
    }

    if (RemainingSpawnCount <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Dispense finished (no remaining)"), *GetNameSafe(this));
        FinishOrder();
        return;
    }

    SpawnDispensedItem(ActiveItemData.Get());
    --RemainingSpawnCount;
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s Spawned item, remaining=%d"), *GetNameSafe(this), RemainingSpawnCount);

    if (RemainingSpawnCount > 0)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(DispenseTimerHandle, this, &ThisClass::HandleDispenseTick, SpawnInterval, false);
        }
        return;
    }

    FinishOrder();
}

void AEMRItemDispenser::FinishOrder()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DispenseTimerHandle);
    }

    OrderState = EEMRItemDispenserOrderState::Idle;
    SelectedSlotIndex = INDEX_NONE;
    SelectedQuantity = 1;
    RemainingSpawnCount = 0;
    ActiveItemData.Reset();
    UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] %s OrderState=Idle (finish)"), *GetNameSafe(this));

    ResetInputDigits();
}

void AEMRItemDispenser::PlayResultCue(bool bSuccess)
{
    if (!HasAuthority())
    {
        return;
    }

    USoundBase* ResultSound = bSuccess ? SuccessResultSound : FailureResultSound;
    if (!ResultSound)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AEMRPlayerCharacter> It(World); It; ++It)
    {
        AEMRPlayerCharacter* Player = *It;
        if (!Player)
        {
            continue;
        }

        Player->Client_PlayWorldSoundAtLocation(ResultSound, GetActorLocation());
    }
}

USoundBase* AEMRItemDispenser::ResolveButtonPressSound(EEMRItemDispenserButtonType ButtonType) const
{
    switch (ButtonType)
    {
    case EEMRItemDispenserButtonType::Digit:
        return DigitButtonPressSound;
    case EEMRItemDispenserButtonType::Confirm:
        return ConfirmButtonPressSound;
    case EEMRItemDispenserButtonType::Reset:
        return ResetButtonPressSound;
    case EEMRItemDispenserButtonType::Increase:
        return IncreaseButtonPressSound;
    case EEMRItemDispenserButtonType::Decrease:
        return DecreaseButtonPressSound;
    default:
        return nullptr;
    }
}

void AEMRItemDispenser::HandleIncreaseButtonRelease()
{
    bIncreaseButtonPressed = false;
    ApplyIncreaseButtonState(false);
}

void AEMRItemDispenser::HandleConfirmButtonRelease()
{
    bConfirmButtonPressed = false;
    ApplyConfirmButtonState(false);
}

void AEMRItemDispenser::HandleResetButtonRelease()
{
    bResetButtonPressed = false;
    ApplyResetButtonState(false);
}

void AEMRItemDispenser::HandleDecreaseButtonRelease()
{
    bDecreaseButtonPressed = false;
    ApplyDecreaseButtonState(false);
}

void AEMRItemDispenser::ResetInputDigits()
{
    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(IncreaseButtonReleaseTimerHandle);
        TimerManager.ClearTimer(DecreaseButtonReleaseTimerHandle);
    }

    PressedDigitsMask = 0;
    CurrentInputDigits.Reset();
    DisplayedTotalCost = 0;
    bIncreaseButtonPressed = false;
    bDecreaseButtonPressed = false;
    ResetAllDigitButtons();
    ApplyIncreaseButtonState(false);
    ApplyDecreaseButtonState(false);
    UpdateOrderSummaryWidget();
}

void AEMRItemDispenser::HandleDispenseStartSoundDelay()
{
    Multicast_PlayDispenseStartSound();
}
