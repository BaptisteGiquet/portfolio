#include "Components/EMRInteractableDetectionComponent.h"

#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "LOD/EMRCollisionChannels.h"

UEMRInteractableDetectionComponent::UEMRInteractableDetectionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    PrimaryComponentTick.TickInterval = 0.05f;
}


void UEMRInteractableDetectionComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCameraComponent = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr;
}


void UEMRInteractableDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CanTrace())
    {
        UpdateTarget(nullptr);
        return;
    }

    FVector ViewLocation;
    FVector ViewDirection;
    if (!GatherViewPoint(ViewLocation, ViewDirection))
    {
        UpdateTarget(nullptr);
        return;
    }

    AActor* BestTarget = FindBestInteractable(ViewLocation, ViewDirection);
    UpdateTarget(BestTarget);
}


bool UEMRInteractableDetectionComponent::CanTrace() const
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}
	
    const APawn* OwningPawn = Cast<APawn>(GetOwner());
    if (!OwningPawn)
    {
        return false;
    }

    if (!OwningPawn->IsLocallyControlled())
    {
        return false;
    }
	
    return true;
}


bool UEMRInteractableDetectionComponent::GatherViewPoint(FVector& OutLocation, FVector& OutDirection) const
{
    if (CachedCameraComponent.IsValid())
    {
        OutLocation = CachedCameraComponent->GetComponentLocation();
        OutDirection = CachedCameraComponent->GetForwardVector();
        return true;
    }

    if (const AActor* OwningActor = GetOwner())
    {
        FRotator ViewRot = FRotator::ZeroRotator;
        OwningActor->GetActorEyesViewPoint(OutLocation, ViewRot);
        OutDirection = ViewRot.Vector();
        return true;
    }

    return false;
}


AActor* UEMRInteractableDetectionComponent::FindBestInteractable(const FVector& ViewLocation, const FVector& ViewDirection)
{
    const FVector TraceEnd = ViewLocation + (ViewDirection * TraceDistance);
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceRadius);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InteractableDetection), false, GetOwner());

    TArray<FHitResult> HitResults;
    const bool bHit = GetWorld()->SweepMultiByChannel(HitResults, ViewLocation, TraceEnd, FQuat::Identity, ECC_Visibility, SphereShape, QueryParams);

    if (bDebugDraw)
    {
        const FColor DebugColor = bHit ? FColor::Green : FColor::Red;
        DrawDebugLine(GetWorld(), ViewLocation, TraceEnd, DebugColor, false, 0.f, 0, 1.f);
        DrawDebugSphere(GetWorld(), TraceEnd, TraceRadius, 12, DebugColor, false, 0.f, 0, 1.f);
    }

    if (!bHit)
    {
        return nullptr;
    }

    AActor* BestActor = nullptr;
    float BestScore = -1.f;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor)
        {
            continue;
        }

        if (!HitActor->GetClass()->ImplementsInterface(UEMRInteractableInterface::StaticClass()))
        {
            continue;
        }

        if (!IEMRInteractableInterface::Execute_IsInteractableEnabled(HitActor))
        {
            continue;
        }

        const FVector DirectionToHit = (Hit.ImpactPoint - ViewLocation).GetSafeNormal();
        const float Dot = FVector::DotProduct(DirectionToHit, ViewDirection);
        const float ClampedDot = FMath::Clamp(Dot, -1.f, 1.f);
        const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDot));

        if (AngleDegrees > MaxInteractAngle)
        {
            continue;
        }

        const float Distance = Hit.Distance;
        const float Score = (1.f - (AngleDegrees / MaxInteractAngle)) + (TraceDistance - Distance);

        if (Score > BestScore)
        {
            BestScore = Score;
            BestActor = HitActor;
        }
    }

    return BestActor;
}

void UEMRInteractableDetectionComponent::UpdateTarget(AActor* NewTarget)
{
    if (CurrentTarget.Get() == NewTarget)
        {
                return;
    }

    CurrentTarget = NewTarget;
    OnTargetChanged.Broadcast(NewTarget);
}
