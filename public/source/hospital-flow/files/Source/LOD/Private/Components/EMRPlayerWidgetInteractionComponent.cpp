#include "Components/EMRPlayerWidgetInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

UEMRPlayerWidgetInteractionComponent::UEMRPlayerWidgetInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UEMRPlayerWidgetInteractionComponent::TryHandleWorldWidgetClick(
    EAbilityInputID AbilityInputID,
    UCameraComponent* Camera,
    UWidgetInteractionComponent* WorldWidgetInteraction,
    ECollisionChannel TraceChannel,
    float TraceDistance) const
{
    const APawn* OwningPawn = Cast<APawn>(GetOwner());
    const UEnum* ChannelEnum = StaticEnum<ECollisionChannel>();
    const FString ChannelName = ChannelEnum
    ? ChannelEnum->GetNameStringByValue(static_cast<int64>(TraceChannel))
    : FString::Printf(TEXT("%d"), static_cast<int32>(TraceChannel));

    UE_LOG(LogTemp, VeryVerbose, TEXT("[WorldWidgetClick] Enter player=%s local=%d input=%d channel=%s distance=%.1f"),
        *GetNameSafe(GetOwner()),
        OwningPawn && OwningPawn->IsLocallyControlled(),
        static_cast<int32>(AbilityInputID),
        *ChannelName,
        TraceDistance);

    if (!OwningPawn || !OwningPawn->IsLocallyControlled() || AbilityInputID != EAbilityInputID::Confirm)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("[WorldWidgetClick] Early-out: local=%d input=%d"),
            OwningPawn && OwningPawn->IsLocallyControlled(),
            static_cast<int32>(AbilityInputID));
        return false;
    }

    UWorld* World = GetWorld();
    if (!World || !Camera || !WorldWidgetInteraction)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[WorldWidgetClick] Early-out: world=%s camera=%s widgetInteraction=%s"),
            World ? TEXT("ok") : TEXT("null"),
            *GetNameSafe(Camera),
            *GetNameSafe(WorldWidgetInteraction));
        return false;
    }

    const FVector TraceStart = Camera->GetComponentLocation();
    const FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * TraceDistance);
    UE_LOG(LogTemp, VeryVerbose, TEXT("[WorldWidgetClick] Trace start=%s end=%s"),
        *TraceStart.ToCompactString(),
        *TraceEnd.ToCompactString());

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorldWidgetInteractTrace), false, GetOwner());
    QueryParams.AddIgnoredActor(GetOwner());

    TArray<FHitResult> HitResults;
    const bool bHit = World->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, TraceChannel, QueryParams);
    UE_LOG(LogTemp, VeryVerbose, TEXT("[WorldWidgetClick] Trace result hit=%d count=%d"), bHit, HitResults.Num());
    if (!bHit || HitResults.IsEmpty())
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("[WorldWidgetClick] Trace miss"));
        return false;
    }

    for (const FHitResult& Hit : HitResults)
    {
        UPrimitiveComponent* HitComponent = Hit.GetComponent();
        const UWidgetComponent* HitWidget = Cast<UWidgetComponent>(HitComponent);
        const ECollisionEnabled::Type CollisionEnabled = HitComponent ? HitComponent->GetCollisionEnabled() : ECollisionEnabled::NoCollision;
        const ECollisionResponse Response = HitComponent ? HitComponent->GetCollisionResponseToChannel(TraceChannel) : ECR_Ignore;
        UE_LOG(LogTemp, VeryVerbose, TEXT("[WorldWidgetClick] Hit component=%s actor=%s dist=%.1f widget=%d collision=%d response=%d visible=%d hidden=%d"),
            *GetNameSafe(HitComponent),
            *GetNameSafe(Hit.GetActor()),
            Hit.Distance,
            HitWidget != nullptr,
            static_cast<int32>(CollisionEnabled),
            static_cast<int32>(Response),
            HitComponent ? HitComponent->IsVisible() : 0,
            HitComponent ? HitComponent->bHiddenInGame : 0);

        if (HitWidget)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("[WorldWidgetClick] Widget class=%s owner=%s"),
                *GetNameSafe(HitWidget->GetWidgetClass()),
                *GetNameSafe(HitWidget->GetOwner()));
        }
    }

    const FHitResult* BestWidgetHit = nullptr;
    for (const FHitResult& Hit : HitResults)
    {
        if (Cast<UWidgetComponent>(Hit.GetComponent()))
        {
            if (!BestWidgetHit || Hit.Distance < BestWidgetHit->Distance)
            {
                BestWidgetHit = &Hit;
            }
        }
    }

    if (!BestWidgetHit)
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("[WorldWidgetClick] No widget component found in hits"));
        return false;
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("[WorldWidgetClick] Best widget hit component=%s dist=%.1f"),
        *GetNameSafe(BestWidgetHit->GetComponent()),
        BestWidgetHit->Distance);

    const bool bWasActive = WorldWidgetInteraction->IsActive();
    const bool bWasHitTesting = WorldWidgetInteraction->bEnableHitTesting;
    const EWidgetInteractionSource PreviousSource = WorldWidgetInteraction->InteractionSource;
    WorldWidgetInteraction->SetActive(true);
    WorldWidgetInteraction->bEnableHitTesting = true;
    WorldWidgetInteraction->InteractionSource = EWidgetInteractionSource::Custom;
    WorldWidgetInteraction->SetCustomHitResult(*BestWidgetHit);
    WorldWidgetInteraction->PressPointerKey(EKeys::LeftMouseButton);
    WorldWidgetInteraction->ReleasePointerKey(EKeys::LeftMouseButton);
    WorldWidgetInteraction->InteractionSource = PreviousSource;
    WorldWidgetInteraction->bEnableHitTesting = bWasHitTesting;
    WorldWidgetInteraction->SetActive(bWasActive);

    UE_LOG(LogTemp, Verbose, TEXT("[WorldWidgetClick] Clicked widget component %s (%s)"),
        *GetNameSafe(BestWidgetHit->GetComponent()),
        *GetNameSafe(GetOwner()));
    return true;
}

bool UEMRPlayerWidgetInteractionComponent::HasWorldWidgetUnderCrosshair(
    FHitResult& OutHit,
    const UCameraComponent* Camera,
    const UWidgetInteractionComponent* WorldWidgetInteraction,
    ECollisionChannel TraceChannel,
    float TraceDistance) const
{
    OutHit = FHitResult();

    const APawn* OwningPawn = Cast<APawn>(GetOwner());
    UWorld* World = GetWorld();
    if (!OwningPawn || !OwningPawn->IsLocallyControlled() || !World || !Camera || !WorldWidgetInteraction)
    {
        return false;
    }

    const FVector TraceStart = Camera->GetComponentLocation();
    const FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * TraceDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorldWidgetDetectTrace), false, GetOwner());
    QueryParams.AddIgnoredActor(GetOwner());

    TArray<FHitResult> HitResults;
    const bool bHit = World->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, TraceChannel, QueryParams);
    if (!bHit || HitResults.IsEmpty())
    {
        return false;
    }

    const FHitResult* BestWidgetHit = nullptr;
    for (const FHitResult& Hit : HitResults)
    {
        if (Cast<UWidgetComponent>(Hit.GetComponent()))
        {
            if (!BestWidgetHit || Hit.Distance < BestWidgetHit->Distance)
            {
                BestWidgetHit = &Hit;
            }
        }
    }

    if (!BestWidgetHit)
    {
        return false;
    }

    OutHit = *BestWidgetHit;
    return true;
}

