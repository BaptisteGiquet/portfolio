
#include "Environment/EMRTreatmentWaitingManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Characters/Patient/EMRPatient.h"
#include "Components/BoxComponent.h"
#include "Environment/EMRTreatmentWaitingArea.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "GAS/EMRTags.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UObjectIterator.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogTreatmentWaitingManager, Log, All);

namespace
{
    void ClearPatientSeatedStateForReleasedTreatmentSeat(UEMRWaitingSeatComponent* Seat, AActor* ReleasingActor)
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

    bool ReleaseReceptionSeatIfPresentForTreatment(UWorld* World, AActor* Patient)
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

UEMRTreatmentWaitingManagerComponent::UEMRTreatmentWaitingManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UEMRTreatmentWaitingManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UEMRTreatmentWaitingManagerComponent, RegisteredSeats);
    DOREPLIFETIME(UEMRTreatmentWaitingManagerComponent, WaitingPatients);
}

void UEMRTreatmentWaitingManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoRegisterSeats)
    {
        UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Auto-register seats at BeginPlay on %s"), *GetOwner()->GetName());
        AutoRegisterSeats();
        UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Registered seats after BeginPlay: %d"), RegisteredSeats.Num());
    }
}

void UEMRTreatmentWaitingManagerComponent::RegisterSeat(UEMRWaitingSeatComponent* Seat)
{
    if (!Seat || RegisteredSeats.Contains(Seat))
    {
        UE_LOG(LogTreatmentWaitingManager, VeryVerbose, TEXT("[TreatmentWaiting] Skip register seat (invalid or already registered)."));
        return;
    }

    RegisteredSeats.Add(Seat);
    Seat->SetWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientTreatmentWaiting);
    UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Registered seat %s | Total=%d"), *Seat->GetName(), RegisteredSeats.Num());

    ValidateSeatApproach(Seat);

    if (Seat->IsFree())
    {
        if (FEMRTreatmentWaitingEntry* QueuedEntry = FindOldestQueuedEntry())
        {
            UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Seat %s immediately assigned to queued patient %s"), *Seat->GetName(), *QueuedEntry->Patient->GetName());
            AssignSeatToQueuedEntry(*QueuedEntry, Seat);
        }
    }
}

void UEMRTreatmentWaitingManagerComponent::UnregisterSeat(UEMRWaitingSeatComponent* Seat)
{
    if (!Seat)
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] Attempted to unregister null seat."));
        return;
    }

    UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Unregister seat %s"), *Seat->GetName());
    ReleaseSeat(Seat);
    Seat->ClearWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientTreatmentWaiting);
    RegisteredSeats.Remove(Seat);
}

bool UEMRTreatmentWaitingManagerComponent::RequestSeatForPatient(AActor* Patient, UEMRWaitingSeatComponent*& OutSeat)
{
    OutSeat = nullptr;

    if (!Patient)
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] RequestSeatForPatient called with null patient."));
        return false;
    }

    if (FEMRTreatmentWaitingEntry* ExistingEntry = FindEntryByPatient(Patient))
    {
        OutSeat = ExistingEntry->Seat.Get();
        UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Patient %s already queued%s"), *Patient->GetName(), OutSeat ? *FString::Printf(TEXT(" with seat %s"), *OutSeat->GetName()) : TEXT(" without seat"));
        return OutSeat != nullptr;
    }

    for (UEMRWaitingSeatComponent* Seat : RegisteredSeats)
    {
        if (Seat && Seat->IsFree())
        {
            OutSeat = Seat;
            UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Assigning free seat %s to patient %s"), *Seat->GetName(), *Patient->GetName());
            return AssignSpecificSeatToPatient(Seat, Patient);
        }
    }

    FEMRTreatmentWaitingEntry& Entry = WaitingPatients.AddDefaulted_GetRef();
    Entry.Patient = Patient;
    Entry.Seat = nullptr;
    Entry.ArrivalTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Queued patient %s (no seat available). Queue size=%d"), *Patient->GetName(), WaitingPatients.Num());
    return false;
}

bool UEMRTreatmentWaitingManagerComponent::AssignSpecificSeatToPatient(UEMRWaitingSeatComponent* Seat, AActor* Patient)
{
    if (!Seat || !Patient)
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] AssignSpecificSeatToPatient called with invalid params."));
        return false;
    }

    if (!Seat->IsFree())
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] Seat %s is not free for patient %s (state=%s)."), *Seat->GetName(), *Patient->GetName(), *UEnum::GetValueAsString(TEXT("EEMRWaitingSeatState"), Seat->GetSeatState()));
        return false;
    }

    if (!Seat->ReserveSeat(Patient))
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] Seat %s refused reservation for patient %s"), *Seat->GetName(), *Patient->GetName());
        return false;
    }

    Seat->SetWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientTreatmentWaiting);

    FEMRTreatmentWaitingEntry& Entry = WaitingPatients.AddDefaulted_GetRef();
    Entry.Patient = Patient;
    Entry.Seat = Seat;
    Entry.ArrivalTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    ReleaseReceptionSeatIfPresentForTreatment(GetWorld(), Patient);

    OnSeatAssigned.Broadcast(Seat, Patient);
    UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Seat %s reserved for patient %s | Total waiting=%d"), *Seat->GetName(), *Patient->GetName(), WaitingPatients.Num());
    NotifyQueuedPatient(Patient, Seat);
    return true;
}

void UEMRTreatmentWaitingManagerComponent::ReleaseSeat(UEMRWaitingSeatComponent* Seat, AActor* ReleasingActor)
{
    FEMRTreatmentWaitingEntry* Entry = FindEntryBySeat(Seat);
    if (!Seat && !Entry)
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] ReleaseSeat called with no seat and no entry."));
        return;
    }

    if (Seat && (Seat->IsReserved() || Seat->IsOccupied()))
    {
        UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Releasing seat %s for actor %s"), *Seat->GetName(), ReleasingActor ? *ReleasingActor->GetName() : TEXT("<none>"));
        ClearPatientSeatedStateForReleasedTreatmentSeat(Seat, ReleasingActor);
        Seat->ReleaseSeat(ReleasingActor);
    }

    if (Entry)
    {
        const int32 EntryIndex = static_cast<int32>(Entry - WaitingPatients.GetData());
        WaitingPatients.RemoveAtSwap(EntryIndex);
        UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Removed entry for seat %s | Remaining=%d"), Seat ? *Seat->GetName() : TEXT("<null>"), WaitingPatients.Num());
    }

    OnSeatReleased.Broadcast(Seat);

    if (Seat && Seat->IsFree())
    {
        if (FEMRTreatmentWaitingEntry* QueuedEntry = FindOldestQueuedEntry())
        {
            AssignSeatToQueuedEntry(*QueuedEntry, Seat);
        }
    }
}

void UEMRTreatmentWaitingManagerComponent::ReleaseSeatForPatient(AActor* Patient)
{
    if (FEMRTreatmentWaitingEntry* Entry = FindEntryByPatient(Patient))
    {
        ReleaseSeat(Entry->Seat.Get(), Patient);
    }
}

UEMRWaitingSeatComponent* UEMRTreatmentWaitingManagerComponent::GetSeatForPatient(const AActor* Patient) const
{
    if (const FEMRTreatmentWaitingEntry* Entry = WaitingPatients.FindByPredicate([Patient](const FEMRTreatmentWaitingEntry& InEntry)
        { return InEntry.Patient.Get() == Patient; }))
    {
        return Entry->Seat.Get();
    }

    return nullptr;
}

bool UEMRTreatmentWaitingManagerComponent::HasFreeSeat() const
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

FEMRTreatmentWaitingEntry* UEMRTreatmentWaitingManagerComponent::FindEntryBySeat(const UEMRWaitingSeatComponent* Seat)
{
    return WaitingPatients.FindByPredicate([Seat](const FEMRTreatmentWaitingEntry& Entry)
    {
        return Entry.Seat.Get() == Seat;
    });
}

FEMRTreatmentWaitingEntry* UEMRTreatmentWaitingManagerComponent::FindEntryByPatient(const AActor* Patient)
{
    return WaitingPatients.FindByPredicate([Patient](const FEMRTreatmentWaitingEntry& Entry)
    {
        return Entry.Patient.Get() == Patient;
    });
}

FEMRTreatmentWaitingEntry* UEMRTreatmentWaitingManagerComponent::FindOldestQueuedEntry()
{
    FEMRTreatmentWaitingEntry* OldestEntry = nullptr;
    float OldestArrival = TNumericLimits<float>::Max();

    for (FEMRTreatmentWaitingEntry& Entry : WaitingPatients)
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

bool UEMRTreatmentWaitingManagerComponent::AssignSeatToQueuedEntry(FEMRTreatmentWaitingEntry& Entry, UEMRWaitingSeatComponent* Seat)
{
    if (!Seat || !Seat->IsFree() || !Entry.Patient.IsValid())
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] AssignSeatToQueuedEntry failed for seat %s"), Seat ? *Seat->GetName() : TEXT("<null>"));
        return false;
    }

    if (!Seat->ReserveSeat(Entry.Patient.Get()))
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] Seat %s refused reservation for queued patient %s"), *Seat->GetName(), *Entry.Patient->GetName());
        return false;
    }

    Seat->SetWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientTreatmentWaiting);

    Entry.Seat = Seat;
    ReleaseReceptionSeatIfPresentForTreatment(GetWorld(), Entry.Patient.Get());
    OnSeatAssigned.Broadcast(Seat, Entry.Patient.Get());
    UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Queued patient %s assigned to seat %s"), *Entry.Patient->GetName(), *Seat->GetName());
    NotifyQueuedPatient(Entry.Patient.Get(), Seat);
    return true;
}

void UEMRTreatmentWaitingManagerComponent::NotifyQueuedPatient(AActor* PatientActor, UEMRWaitingSeatComponent* Seat) const
{
    if (!PatientActor || !Seat)
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] NotifyQueuedPatient aborted: Patient or Seat missing."));
        return;
    }

    const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(PatientActor);
    if (!AbilitySystemInterface)
    {
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] Patient %s has no AbilitySystemInterface."), *PatientActor->GetName());
        return;
    }

    if (UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent())
    {
        FGameplayEventData EventData;
        EventData.EventTag = EMRTags::Abilities::MoveToSeat;
        EventData.Target = PatientActor;
        EventData.OptionalObject = Seat;
        ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
        UE_LOG(LogTreatmentWaitingManager, Log, TEXT("[TreatmentWaiting] Notified patient %s to move to treatment seat %s"), *PatientActor->GetName(), *Seat->GetName());
    }
}

void UEMRTreatmentWaitingManagerComponent::AutoRegisterSeats()
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

void UEMRTreatmentWaitingManagerComponent::GatherSeatComponentsFromOwner(TArray<UEMRWaitingSeatComponent*>& OutSeats) const
{
    if (const AActor* OwnerActor = GetOwner())
    {
        TInlineComponentArray<UEMRWaitingSeatComponent*> OwnerSeatComponents;
        OwnerActor->GetComponents(OwnerSeatComponents);
        OutSeats.Append(OwnerSeatComponents);
    }
}

void UEMRTreatmentWaitingManagerComponent::GatherSeatComponentsFromAttachedActors(TArray<UEMRWaitingSeatComponent*>& OutSeats) const
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

void UEMRTreatmentWaitingManagerComponent::GatherSeatComponentsFromAreaVolume(TArray<UEMRWaitingSeatComponent*>& OutSeats) const
{
    const AEMRTreatmentWaitingArea* WaitingArea = Cast<AEMRTreatmentWaitingArea>(GetOwner());
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

bool UEMRTreatmentWaitingManagerComponent::ValidateSeatApproach(UEMRWaitingSeatComponent* Seat) const
{
    if (!Seat)
    {
        return false;
    }

    const UWorld* World = GetWorld();
    const UNavigationSystemV1* NavSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
    if (!NavSystem)
    {
        UE_LOG(LogTreatmentWaitingManager, Verbose, TEXT("[TreatmentWaiting] No navigation system to validate seat %s approach."), *Seat->GetName());
        return false;
    }

    const FTransform ApproachTransform = Seat->GetApproachTransform();
    FNavLocation ProjectedLocation;
    const bool bOnNavmesh = NavSystem->ProjectPointToNavigation(ApproachTransform.GetLocation(), ProjectedLocation);

    if (!bOnNavmesh)
    {
        const FString TargetKind = Seat->HasApproachPoint() ? TEXT("explicit approach point") : TEXT("derived offset");
        UE_LOG(LogTreatmentWaitingManager, Warning, TEXT("[TreatmentWaiting] Seat %s %s is not on the navmesh. Consider adjusting the approach point/offset."), *Seat->GetName(), *TargetKind);
    }
    else
    {
        UE_LOG(LogTreatmentWaitingManager, VeryVerbose, TEXT("[TreatmentWaiting] Seat %s approach validated on navmesh at %s"), *Seat->GetName(), *ProjectedLocation.Location.ToString());
    }

    return bOnNavmesh;
}

void UEMRTreatmentWaitingManagerComponent::OnRep_WaitingPatients(const TArray<FEMRTreatmentWaitingEntry>& PreviousPatients)
{
    TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UEMRWaitingSeatComponent>> PreviousPatientSeats;

    for (const FEMRTreatmentWaitingEntry& PreviousEntry : PreviousPatients)
    {
        if (PreviousEntry.Patient.IsValid())
        {
            PreviousPatientSeats.Add(PreviousEntry.Patient, PreviousEntry.Seat);
        }
    }

    for (const FEMRTreatmentWaitingEntry& Entry : WaitingPatients)
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
