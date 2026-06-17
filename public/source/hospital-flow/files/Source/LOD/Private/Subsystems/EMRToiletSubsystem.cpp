#include "Subsystems/EMRToiletSubsystem.h"

#include "Characters/Patient/EMRPatient.h"
#include "Environment/EMRToiletSlotActor.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "Framework/EMRAssetManager.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRBaseMachine.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Data/EMRToiletConfig.h"
#include "AbilitySystemComponent.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogToiletSubsystem, Log, All);

void UEMRToiletSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (!EnsureServerWorld())
    {
        return;
    }

    ResolveConfig();
    DiscoverSlots();
    TryBindWaitingRoom();
    ScheduleTripRequestTimer();
}

void UEMRToiletSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(WaitingRoomRetryTimer);
        World->GetTimerManager().ClearTimer(TripRequestTimer);
    }

    RegisteredSlots.Reset();
    TripStates.Reset();
    DirtySlotCount = 0;
    ReputationDrainHandle.Invalidate();
    CachedWaitingRoomManager.Reset();
    CachedConfig.Reset();
    bPendingTripRequest = false;

    Super::Deinitialize();
}

void UEMRToiletSubsystem::RegisterSlot(AEMRToiletSlotActor* Slot)
{
    if (!EnsureServerWorld() || !Slot)
    {
        return;
    }

    if (RegisteredSlots.Contains(Slot))
    {
        return;
    }

    if (CachedConfig.IsValid() && Slot->GetCurrentDirtChance() <= 0.0f)
    {
        Slot->SetCurrentDirtChance(CachedConfig->GetDirtBaseChance());
    }

    RegisteredSlots.Add(Slot);

    if (Slot->IsDirty())
    {
        DirtySlotCount = FMath::Max(DirtySlotCount + 1, 1);
        ApplyReputationDrainIfNeeded();
    }
}

void UEMRToiletSubsystem::UnregisterSlot(AEMRToiletSlotActor* Slot)
{
    if (!Slot)
    {
        return;
    }

    RegisteredSlots.Remove(Slot);

    if (Slot->IsDirty())
    {
        DirtySlotCount = FMath::Max(DirtySlotCount - 1, 0);
        if (DirtySlotCount == 0)
        {
            ClearReputationDrainIfNeeded();
        }
    }

    for (TPair<TWeakObjectPtr<AEMRPatient>, FEMRToiletTripState>& Pair : TripStates)
    {
        FEMRToiletTripState& State = Pair.Value;
        if (State.AssignedSlot.Get() == Slot)
        {
            State.bTripActive = false;
            State.bInToilet = false;
            State.AssignedSlot.Reset();
        }
    }
}

bool UEMRToiletSubsystem::IsToiletTripActive(const AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return false;
    }

    if (const FEMRToiletTripState* State = TripStates.Find(Patient))
    {
        return State->bTripActive;
    }

    return false;
}

bool UEMRToiletSubsystem::TryDeferCallForPatient(AEMRPatient* Patient)
{
    if (!EnsureServerWorld() || !Patient)
    {
        return false;
    }

    FEMRToiletTripState* State = TripStates.Find(Patient);
    if (!State || !State->bTripActive)
    {
        return false;
    }

    State->bPendingCall = true;
    return true;
}

void UEMRToiletSubsystem::NotifySlotArrival(AEMRToiletSlotActor* Slot, AEMRPatient* Patient)
{
    if (!EnsureServerWorld() || !Slot || !Patient)
    {
        return;
    }

    FEMRToiletTripState* State = TripStates.Find(Patient);
    if (!State || State->AssignedSlot.Get() != Slot)
    {
        return;
    }

    if (State->bInToilet)
    {
        return;
    }

    const UEMRToiletConfig* Config = CachedConfig.Get();
    if (!Config)
    {
        return;
    }

    State->bInToilet = true;
    State->bTripActive = true;
    Slot->SetOccupied(true);
    Patient->SetSeatedAnimationState(true, EMRTags::SeatAnimation::PatientToilet);

    float MinSeconds = Config->GetToiletUseMinSeconds();
    float MaxSeconds = Config->GetToiletUseMaxSeconds();
    if (MaxSeconds < MinSeconds)
    {
        Swap(MinSeconds, MaxSeconds);
    }

    const float Duration = FMath::FRandRange(MinSeconds, MaxSeconds);
    FTimerDelegate FinishDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::FinishToiletTrip, Slot);
    GetWorld()->GetTimerManager().SetTimer(State->TripFinishTimer, FinishDelegate, Duration, false);
}

void UEMRToiletSubsystem::CleanSlot(AEMRToiletSlotActor* Slot)
{
    if (!EnsureServerWorld() || !Slot || !Slot->IsDirty())
    {
        return;
    }

    const UEMRToiletConfig* Config = CachedConfig.Get();
    if (Config)
    {
        Slot->SetCurrentDirtChance(Config->GetDirtBaseChance());
    }

    Slot->SetDirty(false);

    DirtySlotCount = FMath::Max(DirtySlotCount - 1, 0);
    if (DirtySlotCount == 0)
    {
        ClearReputationDrainIfNeeded();
    }
}

void UEMRToiletSubsystem::ScheduleTripRequestTimer()
{
    const UEMRToiletConfig* Config = CachedConfig.Get();
    if (!Config)
    {
        return;
    }

    float MinSeconds = Config->GetTripRequestMinSeconds();
    float MaxSeconds = Config->GetTripRequestMaxSeconds();
    if (MaxSeconds < MinSeconds)
    {
        Swap(MinSeconds, MaxSeconds);
    }

    const float Delay = FMath::FRandRange(MinSeconds, MaxSeconds);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TripRequestTimer);
        World->GetTimerManager().SetTimer(TripRequestTimer, this, &ThisClass::HandleTripRequestTimer, Delay, false);
    }
}

void UEMRToiletSubsystem::HandleTripRequestTimer()
{
    if (!EnsureServerWorld())
    {
        return;
    }

    if (RegisteredSlots.Num() == 0)
    {
        ScheduleTripRequestTimer();
        return;
    }

    if (!FindAvailableSlot())
    {
        bPendingTripRequest = true;
        return;
    }

    bPendingTripRequest = false;
    TryStartRandomToiletTrip();
    ScheduleTripRequestTimer();
}

bool UEMRToiletSubsystem::TryStartRandomToiletTrip()
{
    if (!EnsureServerWorld())
    {
        return false;
    }

    const UEMRToiletConfig* Config = CachedConfig.Get();
    if (!Config)
    {
        return false;
    }

    AEMRToiletSlotActor* Slot = FindAvailableSlot();
    if (!Slot)
    {
        return false;
    }

    UEMRWaitingRoomManagerComponent* Manager = CachedWaitingRoomManager.Get();
    if (!Manager)
    {
        return false;
    }

    const TArray<FEMRWaitingPatientEntry> WaitingPatients = Manager->GetWaitingPatients();
    if (WaitingPatients.Num() == 0)
    {
        return false;
    }

    TArray<AEMRPatient*> EligiblePatients;
    EligiblePatients.Reserve(WaitingPatients.Num());

    for (const FEMRWaitingPatientEntry& Entry : WaitingPatients)
    {
        AActor* PatientActor = Entry.Patient.Get();
        UEMRWaitingSeatComponent* Seat = Entry.Seat.Get();
        if (!PatientActor || !Seat)
        {
            continue;
        }

        AEMRPatient* Patient = Cast<AEMRPatient>(PatientActor);
        if (!Patient)
        {
            continue;
        }

        FEMRToiletTripState* State = TripStates.Find(Patient);
        if (State && (State->bHasAttempted || State->bTripActive))
        {
            continue;
        }

        EligiblePatients.Add(Patient);
    }

    if (EligiblePatients.Num() == 0)
    {
        return false;
    }

    const int32 Index = FMath::RandRange(0, EligiblePatients.Num() - 1);
    AEMRPatient* SelectedPatient = EligiblePatients[Index];

    FEMRToiletTripState& State = TripStates.FindOrAdd(SelectedPatient);
    if (State.bHasAttempted || State.bTripActive)
    {
        return false;
    }

    State.bHasAttempted = true;
    StartToiletTrip(SelectedPatient, Slot);
    return true;
}

void UEMRToiletSubsystem::HandlePatientDestroyed(AActor* DestroyedActor)
{
    if (!EnsureServerWorld())
    {
        return;
    }

    AEMRPatient* Patient = Cast<AEMRPatient>(DestroyedActor);
    if (!Patient)
    {
        return;
    }

    FEMRToiletTripState* State = TripStates.Find(Patient);
    if (!State)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(State->TripFinishTimer);
        World->GetTimerManager().ClearTimer(State->DeferredCallTimer);
    }

    if (AEMRToiletSlotActor* Slot = State->AssignedSlot.Get())
    {
        Slot->SetOccupied(false);
        Slot->ClearAssignment();
    }

    TripStates.Remove(Patient);

    if (bPendingTripRequest)
    {
        bPendingTripRequest = false;
        TryStartRandomToiletTrip();
        ScheduleTripRequestTimer();
    }
}

void UEMRToiletSubsystem::StartToiletTrip(AEMRPatient* Patient, AEMRToiletSlotActor* Slot)
{
    if (!Patient || !Slot)
    {
        return;
    }

    Patient->ClearSeatedAnimationState();

    FEMRToiletTripState& State = TripStates.FindOrAdd(Patient);
    State.bTripActive = true;
    State.bInToilet = false;
    State.AssignedSlot = Slot;

    Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);
    Patient->OnDestroyed.AddDynamic(this, &ThisClass::HandlePatientDestroyed);

    Slot->AssignToPatient(Patient);

    if (CachedConfig.IsValid() && Slot->GetCurrentDirtChance() <= 0.0f)
    {
        Slot->SetCurrentDirtChance(CachedConfig->GetDirtBaseChance());
    }

    if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
    {
        const FTransform ApproachTransform = Slot->GetApproachTransform();
        NavigationSubsystem->BroadcastNavigationIntent(
            Patient,
            EMRTags::GMS::AI::Navigation::MoveToBathroom,
            Slot,
            ApproachTransform.GetLocation(),
            ApproachTransform.GetRotation().Rotator());
    }
}

void UEMRToiletSubsystem::FinishToiletTrip(AEMRToiletSlotActor* Slot)
{
    if (!EnsureServerWorld() || !Slot)
    {
        return;
    }

    AEMRPatient* Patient = nullptr;
    FEMRToiletTripState* State = nullptr;
    for (TPair<TWeakObjectPtr<AEMRPatient>, FEMRToiletTripState>& Pair : TripStates)
    {
        if (Pair.Value.AssignedSlot.Get() == Slot)
        {
            Patient = Pair.Key.Get();
            State = &Pair.Value;
            break;
        }
    }

    Slot->SetOccupied(false);

    bool bBecameDirty = false;
    const UEMRToiletConfig* Config = CachedConfig.Get();
    if (Config && !Slot->IsDirty())
    {
        const float Roll = FMath::FRand();
        const float Chance = Slot->GetCurrentDirtChance();
        if (Roll <= Chance)
        {
            Slot->SetDirty(true);
            bBecameDirty = true;
            DirtySlotCount = FMath::Max(DirtySlotCount + 1, 1);
            ApplyReputationDrainIfNeeded();
        }
        else
        {
            Slot->SetCurrentDirtChance(Chance + Config->GetDirtChanceIncrement());
        }
    }

    if (bBecameDirty || !Slot->IsDirty())
    {
        Slot->PlayUseOutcomeSound(bBecameDirty);
    }

    Slot->ClearAssignment();

    if (!Patient || !State)
    {
        if (bPendingTripRequest)
        {
            bPendingTripRequest = false;
            TryStartRandomToiletTrip();
            ScheduleTripRequestTimer();
        }
        return;
    }

    Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);
    Patient->ClearSeatedAnimationState();

    State->bTripActive = false;
    State->bInToilet = false;
    State->AssignedSlot.Reset();

    if (UEMRWaitingRoomManagerComponent* Manager = CachedWaitingRoomManager.Get())
    {
        if (UEMRWaitingSeatComponent* WaitingSeat = Manager->GetSeatForPatient(Patient))
        {
            if (UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent())
            {
                FGameplayEventData EventData;
                EventData.EventTag = EMRTags::Abilities::MoveToSeat;
                EventData.Target = Patient;
                EventData.OptionalObject = WaitingSeat;
                ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
            }
        }
    }

    if (State->bPendingCall)
    {
        TryProcessDeferredCall(Patient);
    }

    if (bPendingTripRequest)
    {
        bPendingTripRequest = false;
        TryStartRandomToiletTrip();
        ScheduleTripRequestTimer();
    }
}

void UEMRToiletSubsystem::TryProcessDeferredCall(AEMRPatient* Patient)
{
    if (!EnsureServerWorld() || !Patient)
    {
        return;
    }

    FEMRToiletTripState* State = TripStates.Find(Patient);
    if (!State || !State->bPendingCall)
    {
        return;
    }

    UEMRWaitingRoomManagerComponent* Manager = CachedWaitingRoomManager.Get();
    if (!Manager)
    {
        State->bPendingCall = false;
        return;
    }

    if (!Manager->GetSeatForPatient(Patient))
    {
        State->bPendingCall = false;
        return;
    }

    AEMRBaseMachine* ReceptionMachine = nullptr;
    if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (const UEMRNavigationIntentSubsystem* NavigationSubsystem = GameInstance->GetSubsystem<UEMRNavigationIntentSubsystem>())
        {
            ReceptionMachine = NavigationSubsystem->FindClosestMachine(Patient, EMRTags::Machine::Reception);
        }
    }

    if (ReceptionMachine && ReceptionMachine->IsOccupied())
    {
        FTimerDelegate RetryDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::TryProcessDeferredCall, Patient);
        GetWorld()->GetTimerManager().SetTimer(State->DeferredCallTimer, RetryDelegate, 1.0f, false);
        return;
    }

    Manager->CallPatient(Patient);
    if (ReceptionMachine)
    {
        ReceptionMachine->SetOccupiedByPatient(Patient);
        if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
        {
            const FGameplayTag NavTag = NavigationSubsystem->GetNavigationMessageTagForMachine(EMRTags::Machine::Reception);
            NavigationSubsystem->BroadcastNavigationIntent(
                Patient,
                NavTag,
                ReceptionMachine,
                ReceptionMachine->GetPatientWaitPointLocation(),
                ReceptionMachine->GetPatientWaitPointRotation(),
                EMRTags::Machine::Reception);
        }
    }

    State->bPendingCall = false;
}

void UEMRToiletSubsystem::ApplyReputationDrainIfNeeded()
{
    if (DirtySlotCount <= 0 || ReputationDrainHandle.IsValid())
    {
        return;
    }

    const UEMRToiletConfig* Config = CachedConfig.Get();
    if (!Config)
    {
        return;
    }

    const TSubclassOf<UGameplayEffect> DrainEffect = Config->GetReputationDrainEffect();
    if (!DrainEffect)
    {
        UE_LOG(LogToiletSubsystem, Warning, TEXT("[Toilet] ReputationDrainEffect missing."));
        return;
    }

    AEMRNightShiftGameState* RunGS = GetWorld()->GetGameState<AEMRNightShiftGameState>();
    if (!RunGS)
    {
        return;
    }

    UAbilitySystemComponent* ASC = RunGS->GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    const float DrainPerSecond = Config->GetReputationDrainPerSecond();
    if (DrainPerSecond <= 0.0f)
    {
        return;
    }

    FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DrainEffect, 1.0f, ContextHandle);
    if (SpecHandle.IsValid())
    {
        SpecHandle.Data->SetSetByCallerMagnitude(EMRTags::SetByCaller::ReputationDrain, -DrainPerSecond);
        ReputationDrainHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    }
}

void UEMRToiletSubsystem::ClearReputationDrainIfNeeded()
{
    if (!ReputationDrainHandle.IsValid())
    {
        return;
    }

    AEMRNightShiftGameState* RunGS = GetWorld()->GetGameState<AEMRNightShiftGameState>();
    if (UAbilitySystemComponent* ASC = RunGS ? RunGS->GetAbilitySystemComponent() : nullptr)
    {
        ASC->RemoveActiveGameplayEffect(ReputationDrainHandle);
    }

    ReputationDrainHandle.Invalidate();
}

void UEMRToiletSubsystem::ResolveConfig()
{
    if (!EnsureServerWorld())
    {
        return;
    }

    TArray<const UEMRSubsystemConfig*> Configs;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(Configs) || Configs.Num() == 0)
    {
        UEMRAssetManager::Get().LoadSubsystemConfig(FStreamableDelegate::CreateUObject(this, &ThisClass::ResolveConfig));
        UE_LOG(LogToiletSubsystem, Verbose, TEXT("[Toilet] SubsystemConfig not loaded yet."));
        return;
    }

    const UEMRSubsystemConfig* Config = Configs[0];
    CachedConfig = Config ? Config->GetToiletConfig() : nullptr;

    if (!CachedConfig.IsValid())
    {
        UE_LOG(LogToiletSubsystem, Warning, TEXT("[Toilet] ToiletConfig missing in SubsystemConfig."));
        return;
    }

    for (const TWeakObjectPtr<AEMRToiletSlotActor>& SlotPtr : RegisteredSlots)
    {
        if (AEMRToiletSlotActor* Slot = SlotPtr.Get())
        {
            if (Slot->GetCurrentDirtChance() <= 0.0f)
            {
                Slot->SetCurrentDirtChance(CachedConfig->GetDirtBaseChance());
            }
        }
    }

    ScheduleTripRequestTimer();
}

void UEMRToiletSubsystem::TryBindWaitingRoom()
{
    if (!EnsureServerWorld())
    {
        return;
    }

    if (CachedWaitingRoomManager.IsValid())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AEMRWaitingRoomArea> It(World); It; ++It)
    {
        if (AEMRWaitingRoomArea* Area = *It)
        {
            if (UEMRWaitingRoomManagerComponent* Manager = Area->GetManagerComponent())
            {
                CachedWaitingRoomManager = Manager;
                return;
            }
        }
    }

    FTimerDelegate RetryDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::TryBindWaitingRoom);
    World->GetTimerManager().SetTimer(WaitingRoomRetryTimer, RetryDelegate, 1.0f, false);
}

void UEMRToiletSubsystem::DiscoverSlots()
{
    if (!EnsureServerWorld())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AEMRToiletSlotActor> It(World); It; ++It)
    {
        RegisterSlot(*It);
    }
}

AEMRToiletSlotActor* UEMRToiletSubsystem::FindAvailableSlot() const
{
    for (const TWeakObjectPtr<AEMRToiletSlotActor>& SlotPtr : RegisteredSlots)
    {
        if (AEMRToiletSlotActor* Slot = SlotPtr.Get())
        {
            if (Slot->IsAvailable())
            {
                return Slot;
            }
        }
    }

    return nullptr;
}

bool UEMRToiletSubsystem::EnsureServerWorld() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}
