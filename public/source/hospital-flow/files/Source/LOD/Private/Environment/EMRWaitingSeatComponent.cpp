#include "Environment/EMRWaitingSeatComponent.h"

#include "Components/ArrowComponent.h"
#include "Environment/EMRWaitingSeatActor.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogWaitingSeat, Log, All);

UEMRWaitingSeatComponent::UEMRWaitingSeatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

bool UEMRWaitingSeatComponent::ReserveSeat(AActor* InReservedFor)
{
    if (!IsFree() || !InReservedFor)
    {
        UE_LOG(LogWaitingSeat, Warning, TEXT("[Seat] Reservation failed. Seat=%s Occupant=%s State=%s"), *GetName(), InReservedFor ? *InReservedFor->GetName() : TEXT("<null>"), *UEnum::GetValueAsString(TEXT("EEMRWaitingSeatState"), SeatState));
        return false;
    }

    SeatState = EEMRWaitingSeatState::Reserved;
    ReservedActor = InReservedFor;
    CurrentOccupant.Reset();
    return true;
}

void UEMRWaitingSeatComponent::ReleaseSeat(AActor* ReleasingActor)
{
    if (ReservedActor.IsValid() && ReservedActor.Get() != ReleasingActor)
    {
        UE_LOG(LogWaitingSeat, Warning, TEXT("[Seat] Release requested by mismatched actor. Seat=%s ReservedFor=%s Releaser=%s"), *GetName(), *ReservedActor->GetName(), ReleasingActor ? *ReleasingActor->GetName() : TEXT("<null>"));
        return;
    }

    UE_LOG(LogWaitingSeat, Log, TEXT("[Seat] Released for %s"), ReleasingActor ? *ReleasingActor->GetName() : TEXT("<null>"));
    ClearSeat();
}

void UEMRWaitingSeatComponent::OccupySeat(AActor* InOccupant)
{
    if (!InOccupant)
    {
        UE_LOG(LogWaitingSeat, Warning, TEXT("[Seat] OccupySeat failed: occupant is null for seat %s"), *GetName());
        return;
    }

    ReservedActor = InOccupant;
    CurrentOccupant = InOccupant;
    SeatState = EEMRWaitingSeatState::Occupied;
    UE_LOG(LogWaitingSeat, Log, TEXT("[Seat] %s occupied by %s"), *GetName(), *InOccupant->GetName());
}


bool UEMRWaitingSeatComponent::HasApproachPoint() const
{
    const AEMRWaitingSeatActor* WaitingSeatActor = Cast<AEMRWaitingSeatActor>(GetOwner());
    return WaitingSeatActor && WaitingSeatActor->ApproachArrow;
}

FTransform UEMRWaitingSeatComponent::GetApproachTransform() const
{
    if (const AEMRWaitingSeatActor* WaitingSeatActor = Cast<AEMRWaitingSeatActor>(GetOwner()))
    {
        if (const UArrowComponent* ApproachArrow = WaitingSeatActor->ApproachArrow)
        {
            return ApproachArrow->GetComponentTransform();
        }
    }

    const FTransform SeatTransform = GetSeatTransform();
    FRotator ApproachRotation = SeatTransform.Rotator();
    ApproachRotation.Roll = 0.f;
    ApproachRotation.Pitch = 0.f;

    const FVector ApproachLocation = SeatTransform.TransformPositionNoScale(ApproachOffset);
    return FTransform(ApproachRotation, ApproachLocation, FVector::OneVector);
}

void UEMRWaitingSeatComponent::ClearSeat()
{
    SeatState = EEMRWaitingSeatState::Free;
    ReservedActor.Reset();
    CurrentOccupant.Reset();
    UE_LOG(LogWaitingSeat, Log, TEXT("[Seat] Cleared seat %s (Owner=%s)"), *GetName(), *GetOwner()->GetName());
}

void UEMRWaitingSeatComponent::SetWaitingSeatAnimationTag(FGameplayTag InSeatAnimationTag)
{
    SeatAnimationTag = InSeatAnimationTag;
}

void UEMRWaitingSeatComponent::ClearWaitingSeatAnimationTag(FGameplayTag ExpectedSeatAnimationTag)
{
    if (SeatAnimationTag.MatchesTagExact(ExpectedSeatAnimationTag))
    {
        SeatAnimationTag = FGameplayTag();
    }
}
