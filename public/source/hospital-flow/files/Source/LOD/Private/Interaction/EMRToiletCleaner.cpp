#include "Interaction/EMRToiletCleaner.h"

#include "GAS/EMRTags.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRFixedPlacementComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"

AEMRToiletCleaner::AEMRToiletCleaner()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    bReplicates = true;

    FixedPlacementComponent = CreateDefaultSubobject<UEMRFixedPlacementComponent>(TEXT("FixedPlacementComponent"));
}

void AEMRToiletCleaner::BeginPlay()
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
    }

    if (UEMRCarryableComponent* Carryable = GetCarryableComponent())
    {
        bWasCarried = Carryable->IsCarried();
    }
    UpdateAnchorTraceCollision(bWasCarried);
}

void AEMRToiletCleaner::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority())
    {
        return;
    }

    UEMRCarryableComponent* Carryable = GetCarryableComponent();
    const bool bIsCarriedNow = Carryable && Carryable->IsCarried();
    if (bIsCarriedNow != bWasCarried)
    {
        bWasCarried = bIsCarriedNow;
        UpdateAnchorTraceCollision(bIsCarriedNow);
    }
}

void AEMRToiletCleaner::SetAnchorTransform(const FTransform& InTransform)
{
    if (!HasAuthority())
    {
        return;
    }

    if (FixedPlacementComponent)
    {
        FixedPlacementComponent->SetAnchorTransform(InTransform);
    }
}

void AEMRToiletCleaner::SetReturnTraceComponent(UPrimitiveComponent* InComponent)
{
    if (!HasAuthority())
    {
        return;
    }

    ReturnTraceComponent = InComponent;
    UpdateAnchorTraceCollision(bWasCarried);
}

void AEMRToiletCleaner::ReturnToAnchor()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!FixedPlacementComponent || !FixedPlacementComponent->HasAnchor())
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

void AEMRToiletCleaner::UpdateAnchorTraceCollision(bool bEnableBlocking)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!ReturnTraceComponent.IsValid())
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

bool AEMRToiletCleaner::TryReturnToAnchorFromTrace(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance)
{
    if (!HasAuthority())
    {
        return false;
    }

    if (!FixedPlacementComponent || !FixedPlacementComponent->HasAnchor())
    {
        return false;
    }

    if (!ReturnTraceComponent.IsValid())
    {
        return false;
    }

    if (!RequestingActor)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const FVector SphereCenter = ReturnTraceComponent->GetComponentLocation();
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

    if (TraceDistance <= 0.0f)
    {
        return false;
    }

    if (SphereRadius <= 0.0f)
    {
        return false;
    }

    const FVector TraceEnd = ViewLocation + (SafeViewDirection * TraceDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ToiletCleanerReturnTrace), false, RequestingActor);
    QueryParams.AddIgnoredActor(RequestingActor);
    QueryParams.AddIgnoredActor(this);

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
    const UPrimitiveComponent* HitComponent = Hit.GetComponent();
    const bool bHitAnchor = bHit && HitComponent && HitComponent == ReturnTraceComponent.Get();

    if (!bHit)
    {
        return false;
    }

    if (!bHitAnchor)
    {
        return false;
    }

    ReturnToAnchor();
    return true;
}

bool AEMRToiletCleaner::IsCarriedBy(const AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    return GetAttachParentActor() == Actor;
}

bool AEMRToiletCleaner::EMRReturnToAnchorFromTrace_Implementation(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance)
{
    return TryReturnToAnchorFromTrace(RequestingActor, ViewLocation, ViewDirection, TraceDistance);
}

void AEMRToiletCleaner::EMRReturnToAnchor_Implementation()
{
    ReturnToAnchor();
}

bool AEMRToiletCleaner::EMRIsCarriedBy_Implementation(const AActor* Actor) const
{
    return IsCarriedBy(Actor);
}
