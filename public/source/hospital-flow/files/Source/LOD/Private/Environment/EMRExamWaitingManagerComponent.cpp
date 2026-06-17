#include "Environment/EMRExamWaitingManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Characters/Patient/EMRPatient.h"
#include "Components/BoxComponent.h"
#include "Environment/EMRExamWaitingArea.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "GAS/EMRTags.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UObjectIterator.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogExamWaitingManager, Log, All);

namespace
{
    void ClearPatientSeatedStateForReleasedExamSeat(UEMRWaitingSeatComponent* Seat, AActor* ReleasingActor)
    {
        if (!Seat)
        {
            return;
        }

        AActor* CandidateActor = Seat->GetOccupant();
        if (!CandidateActor)
        {
            CandidateActor = Seat->GetReservedActor();
        }

        if (!CandidateActor || (ReleasingActor && CandidateActor != ReleasingActor))
        {
            return;
        }

        if (AEMRPatient* Patient = Cast<AEMRPatient>(CandidateActor))
        {
            Patient->ClearSeatedAnimationState();
        }
    }

    bool ReleaseReceptionSeatIfPresent(UWorld* World, AActor* Patient)
    {
        if (!World || !Patient)
        {
            return false;
        }

        for (TActorIterator<AEMRWaitingRoomArea> It(World); It; ++It)
        {
            if (AEMRWaitingRoomArea* WaitingArea = *It)
            {
                if (UEMRWaitingRoomManagerComponent* Manager = WaitingArea->GetManagerComponent())
                {
                    if (Manager->GetSeatForPatient(Patient))
                    {
                        return Manager->RemovePatientFromQueue(Patient);
                    }
                }
            }
        }

        return false;
    }
}

UEMRExamWaitingManagerComponent::UEMRExamWaitingManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UEMRExamWaitingManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UEMRExamWaitingManagerComponent, RegisteredSeats);
    DOREPLIFETIME(UEMRExamWaitingManagerComponent, WaitingPatients);
}

void UEMRExamWaitingManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoRegisterSeats)
    {
        UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Auto-register seats at BeginPlay on %s"), *GetOwner()->GetName());
        AutoRegisterSeats();
        UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Registered seats after BeginPlay: %d"), RegisteredSeats.Num());
    }
}

void UEMRExamWaitingManagerComponent::RegisterSeat(UEMRWaitingSeatComponent* Seat)
{
    if (!Seat || RegisteredSeats.Contains(Seat))
    {
        UE_LOG(LogExamWaitingManager, VeryVerbose, TEXT("[ExamWaiting] Skip register seat (invalid or already registered)."));
        return;
    }

    RegisteredSeats.Add(Seat);
    Seat->SetWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientExamWaiting);
    UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Registered seat %s | Total=%d"), *Seat->GetName(), RegisteredSeats.Num());

    ValidateSeatApproach(Seat);

    if (Seat->IsFree())
    {
        if (FEMRExamWaitingEntry* QueuedEntry = FindOldestQueuedEntry())
        {
            UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Seat %s immediately assigned to queued patient %s"), *Seat->GetName(), *QueuedEntry->Patient->GetName());
            AssignSeatToQueuedEntry(*QueuedEntry, Seat);
        }
    }
}

void UEMRExamWaitingManagerComponent::UnregisterSeat(UEMRWaitingSeatComponent* Seat)
{
    if (!Seat)
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] Attempted to unregister null seat."));
        return;
    }

    UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Unregister seat %s"), *Seat->GetName());
    ReleaseSeat(Seat);
    Seat->ClearWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientExamWaiting);
    RegisteredSeats.Remove(Seat);
}

bool UEMRExamWaitingManagerComponent::RequestSeatForPatient(AActor* Patient, UEMRWaitingSeatComponent*& OutSeat)
{
    OutSeat = nullptr;

    if (!Patient)
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] RequestSeatForPatient called with null patient."));
        return false;
    }

    if (FEMRExamWaitingEntry* ExistingEntry = FindEntryByPatient(Patient))
    {
        OutSeat = ExistingEntry->Seat.Get();
        UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Patient %s already queued%s"), *Patient->GetName(), OutSeat ? *FString::Printf(TEXT(" with seat %s"), *OutSeat->GetName()) : TEXT(" without seat"));
        return OutSeat != nullptr;
    }

    for (UEMRWaitingSeatComponent* Seat : RegisteredSeats)
    {
        if (Seat && Seat->IsFree())
        {
            OutSeat = Seat;
            UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Assigning free seat %s to patient %s"), *Seat->GetName(), *Patient->GetName());
            return AssignSpecificSeatToPatient(Seat, Patient);
        }
    }

    FEMRExamWaitingEntry& Entry = WaitingPatients.AddDefaulted_GetRef();
    Entry.Patient = Patient;
    Entry.Seat = nullptr;
    Entry.ArrivalTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Queued patient %s (no seat available). Queue size=%d"), *Patient->GetName(), WaitingPatients.Num());
    return false;
}

bool UEMRExamWaitingManagerComponent::AssignSpecificSeatToPatient(UEMRWaitingSeatComponent* Seat, AActor* Patient)
{
    if (!Seat || !Patient)
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] AssignSpecificSeatToPatient called with invalid params."));
        return false;
    }

    if (!Seat->IsFree())
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] Seat %s is not free for patient %s (state=%s)."), *Seat->GetName(), *Patient->GetName(), *UEnum::GetValueAsString(TEXT("EEMRWaitingSeatState"), Seat->GetSeatState()));
        return false;
    }

    if (!Seat->ReserveSeat(Patient))
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] Seat %s refused reservation for patient %s"), *Seat->GetName(), *Patient->GetName());
        return false;
    }

    Seat->SetWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientExamWaiting);

    FEMRExamWaitingEntry& Entry = WaitingPatients.AddDefaulted_GetRef();
    Entry.Patient = Patient;
    Entry.Seat = Seat;
    Entry.ArrivalTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    ReleaseReceptionSeatIfPresent(GetWorld(), Patient);

    OnSeatAssigned.Broadcast(Seat, Patient);
    UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Seat %s reserved for patient %s | Total waiting=%d"), *Seat->GetName(), *Patient->GetName(), WaitingPatients.Num());
    NotifyQueuedPatient(Patient, Seat);
    return true;
}

void UEMRExamWaitingManagerComponent::ReleaseSeat(UEMRWaitingSeatComponent* Seat, AActor* ReleasingActor)
{
    FEMRExamWaitingEntry* Entry = FindEntryBySeat(Seat);
    if (!Seat && !Entry)
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] ReleaseSeat called with no seat and no entry."));
        return;
    }

    if (Seat && (Seat->IsReserved() || Seat->IsOccupied()))
    {
        UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Releasing seat %s for actor %s"), *Seat->GetName(), ReleasingActor ? *ReleasingActor->GetName() : TEXT("<none>"));
        ClearPatientSeatedStateForReleasedExamSeat(Seat, ReleasingActor);
        Seat->ReleaseSeat(ReleasingActor);
    }

    if (Entry)
    {
        const int32 EntryIndex = static_cast<int32>(Entry - WaitingPatients.GetData());
        WaitingPatients.RemoveAtSwap(EntryIndex);
        UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Removed entry for seat %s | Remaining=%d"), Seat ? *Seat->GetName() : TEXT("<null>"), WaitingPatients.Num());
    }

    OnSeatReleased.Broadcast(Seat);

    if (Seat && Seat->IsFree())
    {
        if (FEMRExamWaitingEntry* QueuedEntry = FindOldestQueuedEntry())
        {
            AssignSeatToQueuedEntry(*QueuedEntry, Seat);
        }
    }
}

void UEMRExamWaitingManagerComponent::ReleaseSeatForPatient(AActor* Patient)
{
    if (FEMRExamWaitingEntry* Entry = FindEntryByPatient(Patient))
    {
        ReleaseSeat(Entry->Seat.Get(), Patient);
    }
}

UEMRWaitingSeatComponent* UEMRExamWaitingManagerComponent::GetSeatForPatient(const AActor* Patient) const
{
    if (const FEMRExamWaitingEntry* Entry = WaitingPatients.FindByPredicate([Patient](const FEMRExamWaitingEntry& InEntry)
        { return InEntry.Patient.Get() == Patient; }))
    {
        return Entry->Seat.Get();
    }

    return nullptr;
}

bool UEMRExamWaitingManagerComponent::HasFreeSeat() const
{
    for (UEMRWaitingSeatComponent* Seat : RegisteredSeats)
    {
        if (Seat && Seat->IsFree())
        {
            return true;
        }
    }

    return false;
}

bool UEMRExamWaitingManagerComponent::GetOverflowWaitTarget(AActor* Patient, UEMRWaitingSeatComponent*& OutSeat, FTransform& OutTransform) const
{
    OutSeat = nullptr;
    OutTransform = FTransform::Identity;

    if (RegisteredSeats.Num() <= 0)
    {
        return false;
    }

    // Prefer an actually assigned seat when one exists.
    if (Patient)
    {
        if (UEMRWaitingSeatComponent* AssignedSeat = GetSeatForPatient(Patient))
        {
            OutSeat = AssignedSeat;
            OutTransform = AssignedSeat->GetApproachTransform();
            return true;
        }
    }

    UEMRWaitingSeatComponent* BestSeat = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();
    const AActor* OwnerActor = GetOwner();
    const FVector Origin = Patient
    ? Patient->GetActorLocation()
    : (OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector);

    for (UEMRWaitingSeatComponent* Seat : RegisteredSeats)
    {
        if (!Seat)
        {
            continue;
        }

        const FVector CandidateLocation = Seat->GetApproachTransform().GetLocation();
        const float DistSq = FVector::DistSquared(Origin, CandidateLocation);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestSeat = Seat;
        }
    }

    if (!BestSeat)
    {
        return false;
    }

    OutSeat = BestSeat;
    OutTransform = BestSeat->GetApproachTransform();
    return true;
}

FEMRExamWaitingEntry* UEMRExamWaitingManagerComponent::FindEntryBySeat(const UEMRWaitingSeatComponent* Seat)
{
    return WaitingPatients.FindByPredicate([Seat](const FEMRExamWaitingEntry& Entry)
    {
        return Entry.Seat.Get() == Seat;
    });
}

FEMRExamWaitingEntry* UEMRExamWaitingManagerComponent::FindEntryByPatient(const AActor* Patient)
{
    return WaitingPatients.FindByPredicate([Patient](const FEMRExamWaitingEntry& Entry)
    {
        return Entry.Patient.Get() == Patient;
    });
}

FEMRExamWaitingEntry* UEMRExamWaitingManagerComponent::FindOldestQueuedEntry()
{
    FEMRExamWaitingEntry* OldestEntry = nullptr;
    float OldestArrival = TNumericLimits<float>::Max();

    for (FEMRExamWaitingEntry& Entry : WaitingPatients)
    {
        if (Entry.Seat.IsValid())
        {
            continue;
        }

        if (Entry.ArrivalTime < OldestArrival)
        {
            OldestArrival = Entry.ArrivalTime;
            OldestEntry = &Entry;
        }
    }

    return OldestEntry;
}

bool UEMRExamWaitingManagerComponent::AssignSeatToQueuedEntry(FEMRExamWaitingEntry& Entry, UEMRWaitingSeatComponent* Seat)
{
    if (!Seat || !Seat->IsFree() || !Entry.Patient.IsValid())
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] AssignSeatToQueuedEntry failed for seat %s"), Seat ? *Seat->GetName() : TEXT("<null>"));
        return false;
    }

    if (!Seat->ReserveSeat(Entry.Patient.Get()))
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] Seat %s refused reservation for queued patient %s"), *Seat->GetName(), *Entry.Patient->GetName());
        return false;
    }

    Seat->SetWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientExamWaiting);

    Entry.Seat = Seat;
    ReleaseReceptionSeatIfPresent(GetWorld(), Entry.Patient.Get());
    OnSeatAssigned.Broadcast(Seat, Entry.Patient.Get());
    UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Queued patient %s assigned to seat %s"), *Entry.Patient->GetName(), *Seat->GetName());
    NotifyQueuedPatient(Entry.Patient.Get(), Seat);
    return true;
}

void UEMRExamWaitingManagerComponent::NotifyQueuedPatient(AActor* PatientActor, UEMRWaitingSeatComponent* Seat) const
{
    if (!PatientActor || !Seat)
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] NotifyQueuedPatient aborted: Patient or Seat missing."));
        return;
    }

    const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(PatientActor);
    if (!AbilitySystemInterface)
    {
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] Patient %s has no AbilitySystemInterface."), *PatientActor->GetName());
        return;
    }

    if (UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent())
    {
        FGameplayEventData EventData;
        EventData.EventTag = EMRTags::Abilities::MoveToSeat;
        EventData.Target = PatientActor;
        EventData.OptionalObject = Seat;
        ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
        UE_LOG(LogExamWaitingManager, Log, TEXT("[ExamWaiting] Notified patient %s to move to exam seat %s"), *PatientActor->GetName(), *Seat->GetName());
    }
}

void UEMRExamWaitingManagerComponent::AutoRegisterSeats()
{
    TArray<UEMRWaitingSeatComponent*> SeatComponents;
    GatherSeatComponentsFromOwner(SeatComponents);
    GatherSeatComponentsFromAttachedActors(SeatComponents);

    if (bSearchAreaVolumeForSeats)
    {
        GatherSeatComponentsFromAreaVolume(SeatComponents);
    }

    for (UEMRWaitingSeatComponent* SeatComponent : SeatComponents)
    {
        RegisterSeat(SeatComponent);
    }
}

void UEMRExamWaitingManagerComponent::GatherSeatComponentsFromOwner(TArray<UEMRWaitingSeatComponent*>& OutSeats) const
{
    if (const AActor* OwnerActor = GetOwner())
    {
        TInlineComponentArray<UEMRWaitingSeatComponent*> OwnerSeatComponents;
        OwnerActor->GetComponents(OwnerSeatComponents);
        OutSeats.Append(OwnerSeatComponents);
    }
}

void UEMRExamWaitingManagerComponent::GatherSeatComponentsFromAttachedActors(TArray<UEMRWaitingSeatComponent*>& OutSeats) const
{
    if (const AActor* OwnerActor = GetOwner())
    {
        TArray<AActor*> AttachedActors;
        OwnerActor->GetAttachedActors(AttachedActors);

        for (AActor* Attached : AttachedActors)
        {
            if (!Attached)
            {
                continue;
            }

            TInlineComponentArray<UEMRWaitingSeatComponent*> AttachedSeatComponents;
            Attached->GetComponents(AttachedSeatComponents);
            OutSeats.Append(AttachedSeatComponents);
        }
    }
}

void UEMRExamWaitingManagerComponent::GatherSeatComponentsFromAreaVolume(TArray<UEMRWaitingSeatComponent*>& OutSeats) const
{
    const AEMRExamWaitingArea* WaitingArea = Cast<AEMRExamWaitingArea>(GetOwner());
    if (!WaitingArea)
    {
        return;
    }

    const UBoxComponent* AreaVolume = WaitingArea->GetAreaVolume();
    if (!AreaVolume)
    {
        return;
    }

    const FBoxSphereBounds Bounds = AreaVolume->Bounds;
    const FBox AreaBox = Bounds.GetBox();

    for (TObjectIterator<UEMRWaitingSeatComponent> It; It; ++It)
    {
        UEMRWaitingSeatComponent* SeatComponent = *It;
        if (!SeatComponent || SeatComponent->GetWorld() != GetWorld())
        {
            continue;
        }

        if (AreaBox.IsInsideOrOn(SeatComponent->GetComponentLocation()))
        {
            OutSeats.Add(SeatComponent);
        }
    }
}

bool UEMRExamWaitingManagerComponent::ValidateSeatApproach(UEMRWaitingSeatComponent* Seat) const
{
    if (!Seat)
    {
        return false;
    }

    const UWorld* World = GetWorld();
    const UNavigationSystemV1* NavSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
    if (!NavSystem)
    {
        UE_LOG(LogExamWaitingManager, Verbose, TEXT("[ExamWaiting] No navigation system to validate seat %s approach."), *Seat->GetName());
        return false;
    }

    const FTransform ApproachTransform = Seat->GetApproachTransform();
    FNavLocation ProjectedLocation;
    const bool bOnNavmesh = NavSystem->ProjectPointToNavigation(ApproachTransform.GetLocation(), ProjectedLocation);

    if (!bOnNavmesh)
    {
        const FString TargetKind = Seat->HasApproachPoint() ? TEXT("explicit approach point") : TEXT("derived offset");
        UE_LOG(LogExamWaitingManager, Warning, TEXT("[ExamWaiting] Seat %s %s is not on the navmesh. Consider adjusting the approach point/offset."), *Seat->GetName(), *TargetKind);
    }
    else
    {
        UE_LOG(LogExamWaitingManager, VeryVerbose, TEXT("[ExamWaiting] Seat %s approach validated on navmesh at %s"), *Seat->GetName(), *ProjectedLocation.Location.ToString());
    }

    return bOnNavmesh;
}

void UEMRExamWaitingManagerComponent::OnRep_WaitingPatients(const TArray<FEMRExamWaitingEntry>& PreviousPatients)
{
    TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UEMRWaitingSeatComponent>> PreviousPatientSeats;

    for (const FEMRExamWaitingEntry& PreviousEntry : PreviousPatients)
    {
        if (PreviousEntry.Patient.IsValid())
        {
            PreviousPatientSeats.Add(PreviousEntry.Patient, PreviousEntry.Seat);
        }
    }

    for (const FEMRExamWaitingEntry& Entry : WaitingPatients)
    {
        if (!Entry.Patient.IsValid())
        {
            continue;
        }

        if (const TWeakObjectPtr<UEMRWaitingSeatComponent>* PreviousSeatPtr = PreviousPatientSeats.Find(Entry.Patient))
        {
            if (Entry.Seat.IsValid() && PreviousSeatPtr->Get() != Entry.Seat)
            {
                OnSeatAssigned.Broadcast(Entry.Seat.Get(), Entry.Patient.Get());
            }

            PreviousPatientSeats.Remove(Entry.Patient);
        }
        else if (Entry.Seat.IsValid())
        {
            OnSeatAssigned.Broadcast(Entry.Seat.Get(), Entry.Patient.Get());
        }
    }

    for (const TPair<TWeakObjectPtr<AActor>, TWeakObjectPtr<UEMRWaitingSeatComponent>>& RemovedEntry : PreviousPatientSeats)
    {
        UEMRWaitingSeatComponent* Seat = RemovedEntry.Value.Get();
        OnSeatReleased.Broadcast(Seat);
    }
}
