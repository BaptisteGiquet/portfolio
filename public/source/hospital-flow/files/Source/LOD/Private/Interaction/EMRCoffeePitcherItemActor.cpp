#include "Interaction/EMRCoffeePitcherItemActor.h"

#include "Components/PrimitiveComponent.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRCoffeeMachineActor.h"
#include "Interaction/EMRFixedPlacementComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Net/UnrealNetwork.h"

namespace
{
    constexpr TCHAR CoffeePitcherUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");
}

AEMRCoffeePitcherItemActor::AEMRCoffeePitcherItemActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    bReplicates = true;

    FixedPlacementComponent = CreateDefaultSubobject<UEMRFixedPlacementComponent>(TEXT("FixedPlacementComponent"));
}

void AEMRCoffeePitcherItemActor::BeginPlay()
{
    Super::BeginPlay();

    CachedInteractableComponent = FindComponentByClass<UEMRInteractableComponent>();
    if (CachedInteractableComponent)
    {
        CachedInteractableComponent->SetInteractionEventTag(EMRTags::Event::Item::Pick);
    }

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        Carryable->SetPlaceObjectEventTag(FGameplayTag());
        Carryable->SetLockedInPlace(true);
        bWasCarried = Carryable->IsCarried();
    }

    UpdateAnchorTraceCollision(bWasCarried);
    UpdateItemVisibilityTraceCollision();
    RefreshFillVisual();
    OnRep_BrewingState();
}

void AEMRCoffeePitcherItemActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateItemVisibilityTraceCollision();

    if (bVisualFillTransitionActive)
    {
        VisualFillTransitionElapsed += FMath::Max(DeltaSeconds, 0.0f);
        const float Alpha = VisualFillTransitionDuration > KINDA_SMALL_NUMBER
            ? FMath::Clamp(VisualFillTransitionElapsed / VisualFillTransitionDuration, 0.0f, 1.0f)
            : 1.0f;

        VisualFillAmount = FMath::Lerp(VisualFillTransitionStart, VisualFillTransitionTarget, Alpha);
        SetFilledOverlayAmount(VisualFillAmount);

        if (Alpha >= 1.0f)
        {
            bVisualFillTransitionActive = false;
            VisualFillAmount = VisualFillTransitionTarget;
        }
    }

    if (!HasAuthority())
    {
        return;
    }

    const UEMRCarryableComponent* Carryable = GetCarryableComponent();
    const bool bIsCarriedNow = Carryable && Carryable->IsCarried();
    if (bIsCarriedNow != bWasCarried)
    {
        bWasCarried = bIsCarriedNow;
        UpdateAnchorTraceCollision(bIsCarriedNow);
    }

    if (bIsBrewing)
    {
        const float BrewElapsed = GetSynchronizedServerTimeSeconds() - BrewStartServerTimeSeconds;
        if (BrewElapsed >= BrewDurationSeconds)
        {
            bIsBrewing = false;
            RemainingServings = MaxServings;
            OnRep_BrewingState();
            OnRep_RemainingServings();
            ForceNetUpdate();

            UE_LOG(
                LogTemp,
                Warning,
                TEXT("%s CoffeePitcher brewing completed. Remaining=%d/%d"),
                CoffeePitcherUpgradesFlowLogPrefix,
                RemainingServings,
                MaxServings);
        }
    }
}

void AEMRCoffeePitcherItemActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, RemainingServings);
    DOREPLIFETIME(ThisClass, MaxServings);
    DOREPLIFETIME(ThisClass, PatienceRestorePercentOfMax);
    DOREPLIFETIME(ThisClass, bIsBrewing);
    DOREPLIFETIME(ThisClass, BrewStartServerTimeSeconds);
    DOREPLIFETIME(ThisClass, BrewDurationSeconds);
    DOREPLIFETIME(ThisClass, BrewStartFillAmount);
    DOREPLIFETIME(ThisClass, BrewTargetFillAmount);
}

void AEMRCoffeePitcherItemActor::ConfigureCoffeePayload(const int32 InMaxServings, const float InPatienceRestorePercentOfMax)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeePitcher ConfigureCoffeePayload ignored: no authority."), CoffeePitcherUpgradesFlowLogPrefix);
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CoffeePitcher ConfigureCoffeePayload MaxServings=%d RestorePct=%.2f"),
        CoffeePitcherUpgradesFlowLogPrefix,
        InMaxServings,
        InPatienceRestorePercentOfMax);
    MaxServings = FMath::Max(1, InMaxServings);
    PatienceRestorePercentOfMax = FMath::Clamp(InPatienceRestorePercentOfMax, 0.0f, 1.0f);
    RemainingServings = FMath::Clamp(RemainingServings, 0, MaxServings);
    RefreshFillVisual();
}

bool AEMRCoffeePitcherItemActor::ConsumeServingAndGetRestoreAmount(const float MaxPatience, float& OutRestoreAmount)
{
    OutRestoreAmount = 0.0f;

    if (!HasAuthority() || RemainingServings <= 0 || MaxPatience <= 0.0f)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CoffeePitcher ConsumeServing rejected auth=%d remaining=%d maxPatience=%.2f"),
            CoffeePitcherUpgradesFlowLogPrefix,
            HasAuthority() ? 1 : 0,
            RemainingServings,
            MaxPatience);
        return false;
    }

    OutRestoreAmount = FMath::Clamp(MaxPatience * PatienceRestorePercentOfMax, 0.0f, MaxPatience);
    RemainingServings = FMath::Max(0, RemainingServings - 1);
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CoffeePitcher ConsumeServing success restore=%.2f remaining=%d/%d"),
        CoffeePitcherUpgradesFlowLogPrefix,
        OutRestoreAmount,
        RemainingServings,
        MaxServings);
    StartVisualFillTransition(GetCurrentTargetFillAmount(), UseDepleteVisualDurationSeconds);
    ForceNetUpdate();
    return OutRestoreAmount > 0.0f;
}

void AEMRCoffeePitcherItemActor::FillToMaxServings()
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeePitcher FillToMaxServings ignored: no authority."), CoffeePitcherUpgradesFlowLogPrefix);
        return;
    }

    RemainingServings = MaxServings;
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CoffeePitcher FillToMaxServings set remaining=%d"),
        CoffeePitcherUpgradesFlowLogPrefix,
        RemainingServings);
    RefreshFillVisual();
    ForceNetUpdate();
}

bool AEMRCoffeePitcherItemActor::StartBrewFill(const float InDurationSeconds)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeePitcher StartBrewFill ignored: no authority."), CoffeePitcherUpgradesFlowLogPrefix);
        return false;
    }

    if (bIsBrewing)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeePitcher StartBrewFill ignored: already brewing."), CoffeePitcherUpgradesFlowLogPrefix);
        return false;
    }

    if (IsFull())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeePitcher StartBrewFill ignored: pitcher already full."), CoffeePitcherUpgradesFlowLogPrefix);
        return false;
    }

    if (IsCurrentlyCarried())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeePitcher StartBrewFill rejected: pitcher is currently carried."), CoffeePitcherUpgradesFlowLogPrefix);
        return false;
    }

    bIsBrewing = true;
    BrewDurationSeconds = FMath::Max(InDurationSeconds, 0.1f);
    BrewStartServerTimeSeconds = GetSynchronizedServerTimeSeconds();
    BrewStartFillAmount = VisualFillAmount;
    BrewTargetFillAmount = 1.0f;

    OnRep_BrewingState();
    ForceNetUpdate();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CoffeePitcher StartBrewFill started duration=%.2f startFill=%.2f"),
        CoffeePitcherUpgradesFlowLogPrefix,
        BrewDurationSeconds,
        BrewStartFillAmount);

    return true;
}

bool AEMRCoffeePitcherItemActor::IsCurrentlyCarried() const
{
    const UEMRCarryableComponent* Carryable = GetCarryableComponent();
    return Carryable && Carryable->IsCarried();
}

void AEMRCoffeePitcherItemActor::SetOwningMachine(AEMRCoffeeMachineActor* InOwningMachine)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeePitcher SetOwningMachine ignored: no authority."), CoffeePitcherUpgradesFlowLogPrefix);
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CoffeePitcher SetOwningMachine machine=%s"),
        CoffeePitcherUpgradesFlowLogPrefix,
        *GetNameSafe(InOwningMachine));
    OwningMachine = InOwningMachine;
}

void AEMRCoffeePitcherItemActor::SetAnchorTransform(const FTransform& InTransform)
{
    if (!HasAuthority() || !FixedPlacementComponent)
    {
        return;
    }

    FixedPlacementComponent->SetAnchorTransform(InTransform);
}

void AEMRCoffeePitcherItemActor::SetReturnTraceComponent(UPrimitiveComponent* InComponent)
{
    if (!HasAuthority())
    {
        return;
    }

    ReturnTraceComponent = InComponent;
    UpdateAnchorTraceCollision(bWasCarried);
}

void AEMRCoffeePitcherItemActor::ReturnToAnchor()
{
    if (!HasAuthority() || !FixedPlacementComponent || !FixedPlacementComponent->HasAnchor())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CoffeePitcher ReturnToAnchor skipped auth=%d hasFixedPlacement=%d hasAnchor=%d"),
            CoffeePitcherUpgradesFlowLogPrefix,
            HasAuthority() ? 1 : 0,
            FixedPlacementComponent ? 1 : 0,
            (FixedPlacementComponent && FixedPlacementComponent->HasAnchor()) ? 1 : 0);
        return;
    }

    const FTransform AnchorTransform = FixedPlacementComponent->GetAnchorTransform();

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        Carryable->DropAtLocation(AnchorTransform.GetLocation());
        Carryable->SetLockedInPlace(true);
    }

    SetActorRotation(AnchorTransform.GetRotation());
    UpdateAnchorTraceCollision(false);
    UpdateItemVisibilityTraceCollision();
    SetPickupEnabled(!bIsBrewing);
    UE_LOG(LogTemp, Warning, TEXT("%s CoffeePitcher returned to anchor."), CoffeePitcherUpgradesFlowLogPrefix);
}

bool AEMRCoffeePitcherItemActor::TryReturnToAnchorFromTrace(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, const float TraceDistance)
{
    if (!HasAuthority() || !FixedPlacementComponent || !FixedPlacementComponent->HasAnchor() || !ReturnTraceComponent.IsValid())
    {
        return false;
    }

    if (!RequestingActor || TraceDistance <= 0.0f)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const FVector SafeViewDirection = ViewDirection.GetSafeNormal();
    if (SafeViewDirection.IsNearlyZero())
    {
        return false;
    }

    const UPrimitiveComponent* TraceComponent = ReturnTraceComponent.Get();
    if (!TraceComponent)
    {
        return false;
    }

    const FVector TraceEnd = ViewLocation + (SafeViewDirection * TraceDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CoffeePitcherReturnTrace), false, RequestingActor);
    QueryParams.AddIgnoredActor(RequestingActor);
    QueryParams.AddIgnoredActor(this);

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
    if (!bHit || Hit.GetComponent() != TraceComponent)
    {
        return false;
    }

    ReturnToAnchor();
    return true;
}

bool AEMRCoffeePitcherItemActor::IsCarriedBy(const AActor* Actor) const
{
    return Actor && GetAttachParentActor() == Actor;
}

bool AEMRCoffeePitcherItemActor::EMRReturnToAnchorFromTrace_Implementation(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance)
{
    return TryReturnToAnchorFromTrace(RequestingActor, ViewLocation, ViewDirection, TraceDistance);
}

void AEMRCoffeePitcherItemActor::EMRReturnToAnchor_Implementation()
{
    ReturnToAnchor();
}

bool AEMRCoffeePitcherItemActor::EMRIsCarriedBy_Implementation(const AActor* Actor) const
{
    return IsCarriedBy(Actor);
}

void AEMRCoffeePitcherItemActor::UpdateAnchorTraceCollision(bool bEnableBlocking)
{
    if (!HasAuthority() || !ReturnTraceComponent.IsValid())
    {
        return;
    }

    UPrimitiveComponent* TraceComponent = ReturnTraceComponent.Get();
    if (!TraceComponent)
    {
        return;
    }

    // When the machine mesh is used as the return trace target, do not mutate its collision
    // from pitcher state transitions.
    if (OwningMachine.IsValid())
    {
        UPrimitiveComponent* OwningMachineRoot = Cast<UPrimitiveComponent>(OwningMachine->GetRootComponent());
        if (OwningMachineRoot && TraceComponent == OwningMachineRoot)
        {
            return;
        }
    }

    TraceComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    TraceComponent->SetGenerateOverlapEvents(false);

    if (bEnableBlocking)
    {
        TraceComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        TraceComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
    else
    {
        TraceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AEMRCoffeePitcherItemActor::UpdateItemVisibilityTraceCollision()
{
    UPrimitiveComponent* RootPrimitiveComponent = Cast<UPrimitiveComponent>(GetRootComponent());
    if (!RootPrimitiveComponent)
    {
        return;
    }

    const bool bAnchored = FixedPlacementComponent && FixedPlacementComponent->HasAnchor() && !IsCurrentlyCarried();
    const bool bShouldIgnoreVisibility = bAnchored && IsEmpty();
    const bool bShouldBlockVisibility = !bShouldIgnoreVisibility;
    if (bItemVisibilityBlocksTrace == bShouldBlockVisibility)
    {
        return;
    }

    bItemVisibilityBlocksTrace = bShouldBlockVisibility;
    RootPrimitiveComponent->SetCollisionResponseToChannel(ECC_Visibility, bShouldBlockVisibility ? ECR_Block : ECR_Ignore);
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CoffeePitcher visibility trace response changed pitcher=%s blockVisibility=%d anchored=%d carried=%d brewing=%d remaining=%d/%d"),
        CoffeePitcherUpgradesFlowLogPrefix,
        *GetNameSafe(this),
        bShouldBlockVisibility ? 1 : 0,
        bAnchored ? 1 : 0,
        IsCurrentlyCarried() ? 1 : 0,
        bIsBrewing ? 1 : 0,
        RemainingServings,
        MaxServings);
}

void AEMRCoffeePitcherItemActor::RefreshFillVisual()
{
    StartVisualFillTransition(GetCurrentTargetFillAmount(), 0.0f);
}

void AEMRCoffeePitcherItemActor::OnRep_RemainingServings()
{
    if (bIsBrewing)
    {
        return;
    }

    StartVisualFillTransition(GetCurrentTargetFillAmount(), UseDepleteVisualDurationSeconds);
}

void AEMRCoffeePitcherItemActor::OnRep_BrewingState()
{
    SetPickupEnabled(!bIsBrewing);

    if (bIsBrewing)
    {
        const float BrewEndTime = BrewStartServerTimeSeconds + BrewDurationSeconds;
        const float RemainingDuration = FMath::Max(0.0f, BrewEndTime - GetSynchronizedServerTimeSeconds());
        StartVisualFillTransition(BrewTargetFillAmount, RemainingDuration);
        return;
    }

    StartVisualFillTransition(GetCurrentTargetFillAmount(), 0.0f);
}

void AEMRCoffeePitcherItemActor::SetPickupEnabled(const bool bEnabled)
{
    if (!CachedInteractableComponent)
    {
        CachedInteractableComponent = FindComponentByClass<UEMRInteractableComponent>();
    }

    if (CachedInteractableComponent)
    {
        CachedInteractableComponent->SetEnabled(bEnabled);
    }
}

void AEMRCoffeePitcherItemActor::StartVisualFillTransition(const float TargetFillAmount, const float DurationSeconds)
{
    const float ClampedTarget = FMath::Clamp(TargetFillAmount, 0.0f, 1.0f);
    const float ClampedDuration = FMath::Max(DurationSeconds, 0.0f);

    if (ClampedDuration <= KINDA_SMALL_NUMBER)
    {
        bVisualFillTransitionActive = false;
        VisualFillTransitionElapsed = 0.0f;
        VisualFillTransitionDuration = 0.0f;
        VisualFillTransitionStart = ClampedTarget;
        VisualFillTransitionTarget = ClampedTarget;
        VisualFillAmount = ClampedTarget;
        SetFilledOverlayAmount(VisualFillAmount);
        return;
    }

    VisualFillTransitionStart = VisualFillAmount;
    VisualFillTransitionTarget = ClampedTarget;
    VisualFillTransitionDuration = ClampedDuration;
    VisualFillTransitionElapsed = 0.0f;
    bVisualFillTransitionActive = !FMath::IsNearlyEqual(VisualFillTransitionStart, VisualFillTransitionTarget);

    if (!bVisualFillTransitionActive)
    {
        VisualFillAmount = VisualFillTransitionTarget;
        SetFilledOverlayAmount(VisualFillAmount);
    }
}

float AEMRCoffeePitcherItemActor::GetCurrentTargetFillAmount() const
{
    return MaxServings > 0
    ? static_cast<float>(RemainingServings) / static_cast<float>(MaxServings)
    : 0.0f;
}

float AEMRCoffeePitcherItemActor::GetSynchronizedServerTimeSeconds() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return 0.0f;
    }

    const AEMRNightShiftGameState* RunGS = World->GetGameState<AEMRNightShiftGameState>();
    if (RunGS)
    {
        return RunGS->GetServerWorldTimeSeconds();
    }

    return World->GetTimeSeconds();
}
