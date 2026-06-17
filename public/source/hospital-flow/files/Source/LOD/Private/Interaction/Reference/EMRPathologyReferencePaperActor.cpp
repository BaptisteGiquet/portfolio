#include "Interaction/Reference/EMRPathologyReferencePaperActor.h"

#include "Blueprint/UserWidget.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRFixedPlacementComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Containers/Array.h"

AEMRPathologyReferencePaperActor::AEMRPathologyReferencePaperActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    PaperWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PaperWidget"));
    PaperWidgetComponent->SetupAttachment(GetRootComponent());
    PaperWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    PaperWidgetComponent->SetDrawAtDesiredSize(true);
    PaperWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PaperWidgetComponent->SetGenerateOverlapEvents(false);
    PaperWidgetComponent->SetWindowFocusable(false);

    FixedPlacementComponent = CreateDefaultSubobject<UEMRFixedPlacementComponent>(TEXT("FixedPlacementComponent"));

    AnchorPointComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AnchorPoint"));
    AnchorPointComponent->SetupAttachment(GetRootComponent());

    AnchorTraceComponent = CreateDefaultSubobject<USphereComponent>(TEXT("AnchorTraceComponent"));
    AnchorTraceComponent->SetupAttachment(AnchorPointComponent);
    AnchorTraceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AnchorTraceComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    AnchorTraceComponent->SetGenerateOverlapEvents(false);
    AnchorTraceComponent->SetUsingAbsoluteLocation(true);
    AnchorTraceComponent->SetUsingAbsoluteRotation(true);
    AnchorTraceComponent->SetUsingAbsoluteScale(true);

}

void AEMRPathologyReferencePaperActor::BeginPlay()
{
    Super::BeginPlay();

    if (UEMRInteractableComponent* Interactable = FindComponentByClass<UEMRInteractableComponent>())
    {
        Interactable->SetInteractionEventTag(EMRTags::Event::Item::Pick);
    }

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        Carryable->SetPlaceObjectEventTag(FGameplayTag());
        Carryable->SetLockedInPlace(true);
        bWasCarried = Carryable->IsCarried();
    }

    UpdateAnchorTraceCollision(bWasCarried);
    InitializePaperWidget();

    if (HasAuthority())
    {
        if (AnchorPointComponent)
        {
            SetAnchorTransform(AnchorPointComponent->GetComponentTransform());
        }

        SetReturnTraceComponent(AnchorTraceComponent);
        ReturnToAnchor();
    }

    SyncAnchorTraceToFixedAnchor(true);
}

void AEMRPathologyReferencePaperActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Keep the confirm trace target pinned to the fixed world anchor.
    SyncAnchorTraceToFixedAnchor(false);

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
}

void AEMRPathologyReferencePaperActor::SetAnchorTransform(const FTransform& InTransform)
{
    if (!HasAuthority())
    {
        return;
    }

    if (FixedPlacementComponent)
    {
        FixedPlacementComponent->SetAnchorTransform(InTransform);
    }

    SyncAnchorTraceToFixedAnchor(true);
}

void AEMRPathologyReferencePaperActor::ReturnToAnchor()
{
    if (!HasAuthority() || !FixedPlacementComponent || !FixedPlacementComponent->HasAnchor())
    {
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
}

void AEMRPathologyReferencePaperActor::SetReturnTraceComponent(UPrimitiveComponent* InComponent)
{
    if (!HasAuthority())
    {
        return;
    }

    ReturnTraceComponent = InComponent;
    UpdateAnchorTraceCollision(bWasCarried);
}

bool AEMRPathologyReferencePaperActor::TryReturnToAnchorFromTrace(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance)
{
    if (!HasAuthority() || !FixedPlacementComponent || !FixedPlacementComponent->HasAnchor() || !ReturnTraceComponent.IsValid() || !RequestingActor)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    float SphereRadius = 0.0f;
    if (const USphereComponent* SphereComponent = Cast<USphereComponent>(ReturnTraceComponent.Get()))
    {
        SphereRadius = SphereComponent->GetScaledSphereRadius();
    }
    else
    {
        SphereRadius = ReturnTraceComponent->Bounds.SphereRadius;
    }

    const FVector SafeViewDirection = ViewDirection.GetSafeNormal();
    if (SafeViewDirection.IsNearlyZero())
    {
        return false;
    }

    if (TraceDistance <= 0.0f || SphereRadius <= 0.0f)
    {
        return false;
    }

    const FVector TraceEnd = ViewLocation + (SafeViewDirection * TraceDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PathologyReferencePaperReturnTrace), false, RequestingActor);
    QueryParams.AddIgnoredActor(RequestingActor);

    TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
    for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
    {
        if (!PrimitiveComponent || PrimitiveComponent == ReturnTraceComponent.Get())
        {
            continue;
        }

        QueryParams.AddIgnoredComponent(PrimitiveComponent);
    }

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
    const UPrimitiveComponent* HitComponent = Hit.GetComponent();
    const bool bHitAnchor = bHit && HitComponent && HitComponent == ReturnTraceComponent.Get();
    if (!bHitAnchor)
    {
        return false;
    }

    ReturnToAnchor();
    return true;
}

bool AEMRPathologyReferencePaperActor::IsCarriedBy(const AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    return GetAttachParentActor() == Actor;
}

bool AEMRPathologyReferencePaperActor::EMRReturnToAnchorFromTrace_Implementation(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance)
{
    return TryReturnToAnchorFromTrace(RequestingActor, ViewLocation, ViewDirection, TraceDistance);
}

void AEMRPathologyReferencePaperActor::EMRReturnToAnchor_Implementation()
{
    ReturnToAnchor();
}

bool AEMRPathologyReferencePaperActor::EMRIsCarriedBy_Implementation(const AActor* Actor) const
{
    return IsCarriedBy(Actor);
}

bool AEMRPathologyReferencePaperActor::CanInteractWithWorldWidgetsWhileCarried_Implementation() const
{
    return true;
}

bool AEMRPathologyReferencePaperActor::IsInteractableEnabled_Implementation() const
{
    if (!Super::IsInteractableEnabled_Implementation())
    {
        return false;
    }

    const UEMRCarryableComponent* Carryable = GetCarryableComponent();
    return !(Carryable && Carryable->IsCarried());
}

bool AEMRPathologyReferencePaperActor::GetFirstPersonCarryWidgetSettings_Implementation(FEMRFirstPersonCarryWidgetSettings& OutSettings) const
{
    TSubclassOf<UUserWidget> WidgetClass = PaperWidgetClass;
    if (!WidgetClass && PaperWidgetComponent)
    {
        WidgetClass = PaperWidgetComponent->GetWidgetClass();
    }

    if (!WidgetClass)
    {
        return false;
    }

    OutSettings.WidgetClass = WidgetClass;
    OutSettings.bDrawAtDesiredSize = true;
    OutSettings.bShouldShow = bShowFirstPersonWidget;
    OutSettings.RelativeTransform = FTransform::Identity;

    if (PaperWidgetComponent)
    {
        const UStaticMeshComponent* MeshComponent = FindComponentByClass<UStaticMeshComponent>();
        const FTransform BaseTransform = MeshComponent ? MeshComponent->GetComponentTransform() : GetActorTransform();
        OutSettings.RelativeTransform = PaperWidgetComponent->GetComponentTransform().GetRelativeTransform(BaseTransform);
    }

    return true;
}

void AEMRPathologyReferencePaperActor::UpdateFirstPersonCarryWidget_Implementation(UUserWidget* Widget)
{
    if (UEMRPathologyReferencePaperWidget* PaperWidget = Cast<UEMRPathologyReferencePaperWidget>(Widget))
    {
        PaperWidget->InitializePaper(PaperType);
    }
}

void AEMRPathologyReferencePaperActor::ResetFirstPersonCarryWidget_Implementation(UUserWidget* Widget)
{
}

void AEMRPathologyReferencePaperActor::InitializePaperWidget()
{
    if (!PaperWidgetComponent)
    {
        return;
    }

    if (PaperWidgetClass && PaperWidgetComponent->GetWidgetClass() != PaperWidgetClass)
    {
        PaperWidgetComponent->SetWidgetClass(PaperWidgetClass);
    }

    if (!PaperWidgetComponent->GetUserWidgetObject())
    {
        PaperWidgetComponent->InitWidget();
    }

    if (UEMRPathologyReferencePaperWidget* PaperWidget = Cast<UEMRPathologyReferencePaperWidget>(PaperWidgetComponent->GetUserWidgetObject()))
    {
        PaperWidget->InitializePaper(PaperType);
    }
}

void AEMRPathologyReferencePaperActor::SyncAnchorTraceToFixedAnchor(bool bForce)
{
    if (!FixedPlacementComponent || !FixedPlacementComponent->HasAnchor() || !AnchorTraceComponent)
    {
        return;
    }

    const FTransform AnchorTransform = FixedPlacementComponent->GetAnchorTransform();
    const bool bNeedsSync = bForce || !AnchorTraceComponent->GetComponentTransform().Equals(AnchorTransform, 0.1f);
    if (bNeedsSync)
    {
        AnchorTraceComponent->SetWorldTransform(AnchorTransform);
    }
}

void AEMRPathologyReferencePaperActor::UpdateAnchorTraceCollision(bool bEnableBlocking)
{
    if (!HasAuthority() || !ReturnTraceComponent.IsValid())
    {
        return;
    }

    UPrimitiveComponent* TraceComponent = ReturnTraceComponent.Get();
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
