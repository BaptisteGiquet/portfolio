#include "Environment/EMRWaitingRoomManagerComponent.h"

#include "Characters/Patient/EMRPatient.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/EMRTags.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogWaitingRoomManager, Log, All);

namespace
{
    void ClearPatientSeatedStateForReleasedReceptionSeat(UEMRWaitingSeatComponent* Seat, AActor* ReleasingActor)
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
}


UEMRWaitingRoomManagerComponent::UEMRWaitingRoomManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UEMRWaitingRoomManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UEMRWaitingRoomManagerComponent, RegisteredSeats);
    DOREPLIFETIME(UEMRWaitingRoomManagerComponent, WaitingPatients);
}

void UEMRWaitingRoomManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoRegisterSeats)
    {
        UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Auto-register seats at BeginPlay on %s"), *GetOwner()->GetName());
        AutoRegisterSeats();
        UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Registered seats after BeginPlay: %d"), RegisteredSeats.Num());
    }
}

void UEMRWaitingRoomManagerComponent::RegisterSeat(UEMRWaitingSeatComponent* Seat)
{
    if (!Seat || RegisteredSeats.Contains(Seat))
    {
        UE_LOG(LogWaitingRoomManager, VeryVerbose, TEXT("[Manager] Skip register seat (invalid or already registered)."));
        return;
    }

    RegisteredSeats.Add(Seat);
    Seat->SetWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientWaitingRoom);
    UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Registered seat %s | Total=%d"), *Seat->GetName(), RegisteredSeats.Num());

    ValidateSeatApproach(Seat);

    if (Seat->IsFree())
    {
        TryAssignQueuedPatientsToFreeSeats();
    }
}

void UEMRWaitingRoomManagerComponent::UnregisterSeat(UEMRWaitingSeatComponent* Seat)
{
    if (!Seat)
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] Attempted to unregister null seat."));
        return;
    }

    UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Unregister seat %s"), *Seat->GetName());
    ReleaseSeat(Seat);
    Seat->ClearWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientWaitingRoom);
    RegisteredSeats.Remove(Seat);
}

bool UEMRWaitingRoomManagerComponent::AssignSeatToPatient(AActor* Patient)
{
    UEMRWaitingSeatComponent* AssignedSeat = nullptr;
    return RequestSeatForPatient(Patient, AssignedSeat);
}

bool UEMRWaitingRoomManagerComponent::RequestSeatForPatient(AActor* Patient, UEMRWaitingSeatComponent*& OutSeat)
{
    OutSeat = nullptr;

    RemoveInvalidEntries();

    if (!Patient)
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] RequestSeatForPatient called with null patient."));
        return false;
    }

    if (FEMRWaitingPatientEntry* ExistingEntry = FindEntryByPatient(Patient))
    {
        OutSeat = ExistingEntry->Seat.Get();
        UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Patient %s already queued%s"), *Patient->GetName(), OutSeat ? *FString::Printf(TEXT(" with seat %s"), *OutSeat->GetName()) : TEXT(" without seat"));
        return OutSeat != nullptr;
    }

    if (UEMRWaitingSeatComponent* Seat = FindRandomFreeSeat())
    {
        OutSeat = Seat;
        UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Assigning random free seat %s to patient %s"), *Seat->GetName(), *Patient->GetName());
        return AssignSpecificSeatToPatient(Seat, Patient);
    }

    FEMRWaitingPatientEntry& Entry = WaitingPatients.AddDefaulted_GetRef();
    Entry.Patient = Patient;
    Entry.Seat = nullptr;
    Entry.ArrivalTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Queued patient %s (no seat available). Queue size=%d"), *Patient->GetName(), WaitingPatients.Num());
    OnWaitingPatientsChanged.Broadcast();
    return false;
}

bool UEMRWaitingRoomManagerComponent::AssignSpecificSeatToPatient(UEMRWaitingSeatComponent* Seat, AActor* Patient)
{
    if (!Seat || !Patient || !Seat->IsFree())
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] AssignSpecificSeatToPatient failed: Seat=%s Patient=%s State=%s"),
            Seat ? *Seat->GetName() : TEXT("<null>"),
            Patient ? *Patient->GetName() : TEXT("<null>"),
            Seat ? *UEnum::GetValueAsString(TEXT("EEMRWaitingSeatState"), Seat->GetSeatState()) : TEXT("<n/a>"));
        return false;
    }

    if (RegisteredSeats.Contains(Seat) == false)
    {
        UE_LOG(LogWaitingRoomManager, Verbose, TEXT("[Manager] Seat %s not registered; registering now."), *Seat->GetName());
        RegisterSeat(Seat);
    }

    if (FindEntryByPatient(Patient))
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] Patient %s already has a queue entry; cannot assign seat %s."), *Patient->GetName(), *Seat->GetName());
        return false;
    }

    if (!Seat->ReserveSeat(Patient))
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] Seat %s refused reservation for patient %s"), *Seat->GetName(), *Patient->GetName());
        return false;
    }

    Seat->SetWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientWaitingRoom);

    FEMRWaitingPatientEntry& Entry = WaitingPatients.AddDefaulted_GetRef();
    Entry.Patient = Patient;
    Entry.Seat = Seat;
    Entry.ArrivalTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    OnSeatAssigned.Broadcast(Seat, Patient);
    UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Seat %s reserved for patient %s | Total waiting=%d"), *Seat->GetName(), *Patient->GetName(), WaitingPatients.Num());
    return true;
}

void UEMRWaitingRoomManagerComponent::ReleaseSeat(UEMRWaitingSeatComponent* Seat, AActor* ReleasingActor)
{
    FEMRWaitingPatientEntry* Entry = FindEntryBySeat(Seat);
    if (!Seat && !Entry)
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] ReleaseSeat called with no seat and no entry."));
        return;
    }

    RemoveInvalidEntries();

    Entry = FindEntryBySeat(Seat);

    if (Seat && (Seat->IsReserved() || Seat->IsOccupied()))
    {
        UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Releasing seat %s for actor %s"), *Seat->GetName(), ReleasingActor ? *ReleasingActor->GetName() : TEXT("<none>"));
        ClearPatientSeatedStateForReleasedReceptionSeat(Seat, ReleasingActor);
        Seat->ReleaseSeat(ReleasingActor);
    }

    if (Entry)
    {
        const int32 EntryIndex = static_cast<int32>(Entry - WaitingPatients.GetData());
        WaitingPatients.RemoveAtSwap(EntryIndex);
        UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Removed entry for seat %s | Remaining=%d"), Seat ? *Seat->GetName() : TEXT("<null>"), WaitingPatients.Num());
    }

    OnSeatReleased.Broadcast(Seat);

    if (Seat && Seat->IsFree())
    {
        TryAssignQueuedPatientsToFreeSeats();
    }
}

void UEMRWaitingRoomManagerComponent::CallPatient(AActor* Patient)
{
    FEMRWaitingPatientEntry* Entry = FindEntryByPatient(Patient);
    if (!Entry)
    {
        return;
    }

    RemoveInvalidEntries();

    Entry = FindEntryByPatient(Patient);
    if (!Entry)
    {
        return;
    }

    const int32 EntryIndex = static_cast<int32>(Entry - WaitingPatients.GetData());

    if (UEMRWaitingSeatComponent* Seat = Entry->Seat.Get())
    {
        ReleaseSeat(Seat, Patient);
    }
    else
    {
        WaitingPatients.RemoveAtSwap(EntryIndex);
    }
    OnPatientCalled.Broadcast(Patient);
}

bool UEMRWaitingRoomManagerComponent::RemovePatientFromQueue(AActor* Patient)
{
    RemoveInvalidEntries();

    FEMRWaitingPatientEntry* Entry = FindEntryByPatient(Patient);
    if (!Entry)
    {
        return false;
    }

    if (UEMRWaitingSeatComponent* Seat = Entry->Seat.Get())
    {
        ReleaseSeat(Seat, Patient);
        OnPatientCalled.Broadcast(Patient);
    }
    else
    {
        const int32 EntryIndex = static_cast<int32>(Entry - WaitingPatients.GetData());
        WaitingPatients.RemoveAtSwap(EntryIndex);
        OnPatientCalled.Broadcast(Patient);
    }

    return true;
}

UEMRWaitingSeatComponent* UEMRWaitingRoomManagerComponent::GetSeatForPatient(const AActor* Patient) const
{
    if (const FEMRWaitingPatientEntry* Entry = WaitingPatients.FindByPredicate([Patient](const FEMRWaitingPatientEntry& InEntry)
        { return InEntry.Patient.Get() == Patient; }))
    {
        return Entry->Seat.Get();
    }

    return nullptr;
}

TArray<FEMRWaitingPatientEntry> UEMRWaitingRoomManagerComponent::GetNextCallablePatients() const
{
    TArray<FEMRWaitingPatientEntry> SortedEntries = WaitingPatients;

    SortedEntries.RemoveAllSwap([](const FEMRWaitingPatientEntry& Entry)
    {
        return !Entry.Patient.IsValid() || !Entry.Seat.IsValid();
    });

    SortedEntries.Sort([](const FEMRWaitingPatientEntry& Lhs, const FEMRWaitingPatientEntry& Rhs)
    {
        return Lhs.ArrivalTime < Rhs.ArrivalTime;
    });

    return SortedEntries;
}

bool UEMRWaitingRoomManagerComponent::HasFreeSeat() const
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

UEMRWaitingSeatComponent* UEMRWaitingRoomManagerComponent::FindFirstFreeSeat() const
{
    for (UEMRWaitingSeatComponent* Seat : RegisteredSeats)
    {
        if (Seat && Seat->IsFree())
        {
            return Seat;
        }
    }

    return nullptr;
}

UEMRWaitingSeatComponent* UEMRWaitingRoomManagerComponent::FindRandomFreeSeat() const
{
    TArray<UEMRWaitingSeatComponent*> FreeSeats;
    FreeSeats.Reserve(RegisteredSeats.Num());

    for (UEMRWaitingSeatComponent* Seat : RegisteredSeats)
    {
        if (Seat && Seat->IsFree())
        {
            FreeSeats.Add(Seat);
        }
    }

    if (FreeSeats.IsEmpty())
    {
        return nullptr;
    }

    const int32 SelectedIndex = FMath::RandRange(0, FreeSeats.Num() - 1);
    return FreeSeats[SelectedIndex];
}

void UEMRWaitingRoomManagerComponent::AutoRegisterSeats()
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

FEMRWaitingPatientEntry* UEMRWaitingRoomManagerComponent::FindEntryBySeat(const UEMRWaitingSeatComponent* Seat)
{
    return WaitingPatients.FindByPredicate([Seat](const FEMRWaitingPatientEntry& Entry)
    {
        return Entry.Seat.Get() == Seat;
    });
}

FEMRWaitingPatientEntry* UEMRWaitingRoomManagerComponent::FindEntryByPatient(const AActor* Patient)
{
    return WaitingPatients.FindByPredicate([Patient](const FEMRWaitingPatientEntry& Entry)
    {
        return Entry.Patient.Get() == Patient;
    });
}

FEMRWaitingPatientEntry* UEMRWaitingRoomManagerComponent::FindOldestQueuedEntry()
{
    FEMRWaitingPatientEntry* OldestEntry = nullptr;
    float OldestArrival = TNumericLimits<float>::Max();

    for (FEMRWaitingPatientEntry& Entry : WaitingPatients)
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

bool UEMRWaitingRoomManagerComponent::AssignSeatToQueuedEntry(FEMRWaitingPatientEntry& Entry, UEMRWaitingSeatComponent* Seat)
{
    if (!Seat || !Seat->IsFree() || !Entry.Patient.IsValid())
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] AssignSeatToQueuedEntry failed for seat %s"), Seat ? *Seat->GetName() : TEXT("<null>"));
        return false;
    }

    if (!Seat->ReserveSeat(Entry.Patient.Get()))
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] Seat %s refused reservation for queued patient %s"), *Seat->GetName(), *Entry.Patient->GetName());
        return false;
    }

    Seat->SetWaitingSeatAnimationTag(EMRTags::SeatAnimation::PatientWaitingRoom);

    Entry.Seat = Seat;
    OnSeatAssigned.Broadcast(Seat, Entry.Patient.Get());
    UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Queued patient %s assigned to seat %s"), *Entry.Patient->GetName(), *Seat->GetName());
    NotifyQueuedPatient(Entry.Patient.Get(), Seat);
    return true;
}

void UEMRWaitingRoomManagerComponent::TryAssignQueuedPatientsToFreeSeats()
{
    while (true)
    {
        FEMRWaitingPatientEntry* QueuedEntry = FindOldestQueuedEntry();
        if (!QueuedEntry)
        {
            return;
        }

        UEMRWaitingSeatComponent* FreeSeat = FindRandomFreeSeat();
        if (!FreeSeat)
        {
            return;
        }

        if (!AssignSeatToQueuedEntry(*QueuedEntry, FreeSeat))
        {
            UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] Failed to assign queued patient to random free seat; stopping assignment loop."));
            return;
        }
    }
}

void UEMRWaitingRoomManagerComponent::RemoveInvalidEntries()
{
    for (int32 Index = WaitingPatients.Num() - 1; Index >= 0; --Index)
    {
        FEMRWaitingPatientEntry& Entry = WaitingPatients[Index];
        if (!Entry.Patient.IsValid())
        {
            if (UEMRWaitingSeatComponent* Seat = Entry.Seat.Get())
            {
                Seat->ReleaseSeat(Entry.Patient.Get());
                OnSeatReleased.Broadcast(Seat);
            }

            WaitingPatients.RemoveAtSwap(Index);
        }
    }
}

void UEMRWaitingRoomManagerComponent::NotifyQueuedPatient(AActor* PatientActor, UEMRWaitingSeatComponent* Seat) const
{
    if (!PatientActor || !Seat)
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] NotifyQueuedPatient aborted: Patient or Seat missing."));
        return;
    }

    const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(PatientActor);
    if (!AbilitySystemInterface)
    {
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] Patient %s has no AbilitySystemInterface."), *PatientActor->GetName());
        return;
    }

    if (UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent())
    {
        FGameplayEventData EventData;
        EventData.EventTag = EMRTags::Abilities::MoveToSeat;
        EventData.Target = PatientActor;
        EventData.OptionalObject = Seat;
        ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
        UE_LOG(LogWaitingRoomManager, Log, TEXT("[Manager] Notified patient %s to move to seat %s"), *PatientActor->GetName(), *Seat->GetName());
    }
}

bool UEMRWaitingRoomManagerComponent::ValidateSeatApproach(UEMRWaitingSeatComponent* Seat) const
{
    if (!Seat)
    {
        return false;
    }

    const UWorld* World = GetWorld();
    const UNavigationSystemV1* NavSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
    if (!NavSystem)
    {
        UE_LOG(LogWaitingRoomManager, Verbose, TEXT("[Manager] No navigation system to validate seat %s approach."), *Seat->GetName());
        return false;
    }

    const FTransform ApproachTransform = Seat->GetApproachTransform();
    FNavLocation ProjectedLocation;
    const bool bOnNavmesh = NavSystem->ProjectPointToNavigation(ApproachTransform.GetLocation(), ProjectedLocation);

    if (!bOnNavmesh)
    {
        const FString TargetKind = Seat->HasApproachPoint() ? TEXT("explicit approach point") : TEXT("derived offset");
        UE_LOG(LogWaitingRoomManager, Warning, TEXT("[Manager] Seat %s %s is not on the navmesh. Consider adjusting the approach point/offset."), *Seat->GetName(), *TargetKind);
    }
    else
    {
        UE_LOG(LogWaitingRoomManager, VeryVerbose, TEXT("[Manager] Seat %s approach validated on navmesh at %s"), *Seat->GetName(), *ProjectedLocation.Location.ToString());
    }

    return bOnNavmesh;
}


void UEMRWaitingRoomManagerComponent::GatherSeatComponentsFromOwner(TArray<UEMRWaitingSeatComponent*>& OutSeats) const
{
    if (const AActor* OwnerActor = GetOwner())
    {
        TInlineComponentArray<UEMRWaitingSeatComponent*> OwnerSeatComponents;
        OwnerActor->GetComponents(OwnerSeatComponents);
        OutSeats.Append(OwnerSeatComponents);
    }
}

void UEMRWaitingRoomManagerComponent::GatherSeatComponentsFromAttachedActors(TArray<UEMRWaitingSeatComponent*>& OutSeats) const
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

void UEMRWaitingRoomManagerComponent::GatherSeatComponentsFromAreaVolume(TArray<UEMRWaitingSeatComponent*>& OutSeats) const
{
    const AEMRWaitingRoomArea* WaitingRoomArea = Cast<AEMRWaitingRoomArea>(GetOwner());
    if (!WaitingRoomArea)
    {
        return;
    }

    const UBoxComponent* AreaVolume = WaitingRoomArea->GetAreaVolume();
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


void UEMRWaitingRoomManagerComponent::OnRep_WaitingPatients(const TArray<FEMRWaitingPatientEntry>& PreviousPatients)
{
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UEMRWaitingSeatComponent>> PreviousPatientSeats;

	for (const FEMRWaitingPatientEntry& PreviousEntry : PreviousPatients)
	{
		if (PreviousEntry.Patient.IsValid())
		{
			PreviousPatientSeats.Add(PreviousEntry.Patient, PreviousEntry.Seat);
		}
	}

	for (const FEMRWaitingPatientEntry& Entry : WaitingPatients)
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

		if (AActor* PatientActor = RemovedEntry.Key.Get())
		{
			OnPatientCalled.Broadcast(PatientActor);
		}
	}

	OnWaitingPatientsChanged.Broadcast();
}
