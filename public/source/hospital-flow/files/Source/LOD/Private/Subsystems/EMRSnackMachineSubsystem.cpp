#include "Subsystems/EMRSnackMachineSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Data/EMREconomySystemGenerics.h"
#include "Environment/EMRSnackMachineActor.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "Framework/EMRAssetManager.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/Attributes/EMRPatientAttributeSet.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRBaseMachine.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogSnackMachineSubsystem, Log, All);

void UEMRSnackMachineSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (!EnsureServerWorld())
    {
        return;
    }

    ResolveDifficultyTuning();
    DiscoverMachines();
    TryBindWaitingRoom();
    ScheduleTripRequestTimer();
}

void UEMRSnackMachineSubsystem::Deinitialize()
{
    if (UEMRWaitingRoomManagerComponent* WaitingRoomManager = CachedWaitingRoomManager.Get())
    {
        WaitingRoomManager->OnWaitingPatientsChanged.RemoveDynamic(this, &ThisClass::HandleWaitingPatientsChanged);
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(WaitingRoomRetryTimer);
        World->GetTimerManager().ClearTimer(TripRequestTimer);
    }

    for (TPair<TWeakObjectPtr<AEMRPatient>, FEMRSnackTripState>& Pair : TripStates)
    {
        AEMRPatient* Patient = Pair.Key.Get();
        FEMRSnackTripState& State = Pair.Value;

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(State.TripFinishTimer);
            World->GetTimerManager().ClearTimer(State.DeferredCallTimer);
        }

        if (Patient)
        {
            Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);
        }
    }

    RegisteredMachines.Reset();
    TripStates.Reset();
    CachedDifficultyTuning.Reset();
    CachedWaitingRoomManager.Reset();
    bPendingTripRequest = false;

    Super::Deinitialize();
}

void UEMRSnackMachineSubsystem::RegisterMachine(AEMRSnackMachineActor* Machine)
{
    if (!EnsureServerWorld() || !Machine || RegisteredMachines.Contains(Machine))
    {
        return;
    }

    RegisteredMachines.Add(Machine);

    if (bPendingTripRequest && Machine->IsAvailable())
    {
        bPendingTripRequest = false;
        TryStartRandomSnackTrip();
        ScheduleTripRequestTimer();
    }
}

void UEMRSnackMachineSubsystem::UnregisterMachine(AEMRSnackMachineActor* Machine)
{
    if (!Machine)
    {
        return;
    }

    RegisteredMachines.Remove(Machine);

    for (TPair<TWeakObjectPtr<AEMRPatient>, FEMRSnackTripState>& Pair : TripStates)
    {
        AEMRPatient* Patient = Pair.Key.Get();
        FEMRSnackTripState& State = Pair.Value;
        if (State.AssignedMachine.Get() != Machine)
        {
            continue;
        }

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(State.TripFinishTimer);
            World->GetTimerManager().ClearTimer(State.DeferredCallTimer);
        }

        State.bTripActive = false;
        State.bAtMachine = false;
        State.AssignedMachine.Reset();

        if (Machine)
        {
            Machine->SetOccupied(false);
            Machine->ClearAssignment();
        }

        if (Patient)
        {
            Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);
            SendPatientBackToWaitingSeat(Patient);
            if (State.bPendingCall)
            {
                TryProcessDeferredCall(Patient);
            }
        }
    }

    if (bPendingTripRequest && FindAvailableMachine())
    {
        bPendingTripRequest = false;
        TryStartRandomSnackTrip();
        ScheduleTripRequestTimer();
    }
}

bool UEMRSnackMachineSubsystem::IsSnackTripActive(const AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return false;
    }

    if (const FEMRSnackTripState* State = TripStates.Find(Patient))
    {
        return State->bTripActive;
    }

    return false;
}

bool UEMRSnackMachineSubsystem::TryDeferCallForPatient(AEMRPatient* Patient)
{
    if (!EnsureServerWorld() || !Patient)
    {
        return false;
    }

    FEMRSnackTripState* State = TripStates.Find(Patient);
    if (!State || !State->bTripActive)
    {
        return false;
    }

    State->bPendingCall = true;
    return true;
}

void UEMRSnackMachineSubsystem::NotifyMachineArrival(AEMRSnackMachineActor* Machine, AEMRPatient* Patient)
{
    if (!EnsureServerWorld() || !Machine || !Patient)
    {
        return;
    }

    FEMRSnackTripState* State = TripStates.Find(Patient);
    if (!State || State->AssignedMachine.Get() != Machine || State->bAtMachine)
    {
        return;
    }

    const FEMRSnackMachineUpgradeTuning* Tuning = GetSnackUpgradeTuning();
    if (!Tuning)
    {
        return;
    }

    State->bAtMachine = true;
    State->bTripActive = true;
    Machine->SetOccupied(true);

    Machine->PlayUseStartSound();

    float MinSeconds = Tuning->SnackUseMinSeconds;
    float MaxSeconds = Tuning->SnackUseMaxSeconds;
    if (MaxSeconds < MinSeconds)
    {
        Swap(MinSeconds, MaxSeconds);
    }

    const float Duration = FMath::FRandRange(MinSeconds, MaxSeconds);
    FTimerDelegate FinishDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::FinishSnackTrip, Machine);
    GetWorld()->GetTimerManager().SetTimer(State->TripFinishTimer, FinishDelegate, Duration, false);
}

void UEMRSnackMachineSubsystem::HandlePatientDestroyed(AActor* DestroyedActor)
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

    FEMRSnackTripState* State = TripStates.Find(Patient);
    if (!State)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(State->TripFinishTimer);
        World->GetTimerManager().ClearTimer(State->DeferredCallTimer);
    }

    if (AEMRSnackMachineActor* Machine = State->AssignedMachine.Get())
    {
        Machine->SetOccupied(false);
        Machine->ClearAssignment();
    }

    TripStates.Remove(Patient);

    if (bPendingTripRequest)
    {
        bPendingTripRequest = false;
        TryStartRandomSnackTrip();
        ScheduleTripRequestTimer();
    }
}

void UEMRSnackMachineSubsystem::ScheduleTripRequestTimer()
{
    const FEMRSnackMachineUpgradeTuning* Tuning = GetSnackUpgradeTuning();
    if (!Tuning)
    {
        return;
    }

    float MinSeconds = Tuning->TripRequestMinSeconds;
    float MaxSeconds = Tuning->TripRequestMaxSeconds;
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

void UEMRSnackMachineSubsystem::HandleTripRequestTimer()
{
    if (!EnsureServerWorld())
    {
        return;
    }

    if (RegisteredMachines.Num() == 0)
    {
        ScheduleTripRequestTimer();
        return;
    }

    if (!FindAvailableMachine())
    {
        bPendingTripRequest = true;
        return;
    }

    bPendingTripRequest = false;
    if (!TryStartRandomSnackTrip())
    {
        bPendingTripRequest = true;
    }

    ScheduleTripRequestTimer();
}

bool UEMRSnackMachineSubsystem::TryStartRandomSnackTrip()
{
    if (!EnsureServerWorld())
    {
        return false;
    }

    AEMRSnackMachineActor* Machine = FindAvailableMachine();
    if (!Machine)
    {
        return false;
    }

    UEMRWaitingRoomManagerComponent* Manager = CachedWaitingRoomManager.Get();
    if (!Manager)
    {
        return false;
    }

    const TArray<FEMRWaitingPatientEntry> WaitingPatients = Manager->GetWaitingPatients();
    if (WaitingPatients.IsEmpty())
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

        const FEMRSnackTripState* State = TripStates.Find(Patient);
        if (State && (State->bHasAttempted || State->bTripActive))
        {
            continue;
        }

        EligiblePatients.Add(Patient);
    }

    if (EligiblePatients.IsEmpty())
    {
        return false;
    }

    const int32 Index = FMath::RandRange(0, EligiblePatients.Num() - 1);
    AEMRPatient* SelectedPatient = EligiblePatients[Index];

    FEMRSnackTripState& State = TripStates.FindOrAdd(SelectedPatient);
    if (State.bHasAttempted || State.bTripActive)
    {
        return false;
    }

    State.bHasAttempted = true;
    StartSnackTrip(SelectedPatient, Machine);
    return true;
}

void UEMRSnackMachineSubsystem::HandleWaitingPatientsChanged()
{
    if (!EnsureServerWorld() || !bPendingTripRequest || !FindAvailableMachine())
    {
        return;
    }

    bPendingTripRequest = false;
    if (!TryStartRandomSnackTrip())
    {
        bPendingTripRequest = true;
        return;
    }

    ScheduleTripRequestTimer();
}

void UEMRSnackMachineSubsystem::StartSnackTrip(AEMRPatient* Patient, AEMRSnackMachineActor* Machine)
{
    if (!Patient || !Machine)
    {
        return;
    }

    Patient->ClearSeatedAnimationState();

    FEMRSnackTripState& State = TripStates.FindOrAdd(Patient);
    State.bTripActive = true;
    State.bAtMachine = false;
    State.AssignedMachine = Machine;

    Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);
    Patient->OnDestroyed.AddDynamic(this, &ThisClass::HandlePatientDestroyed);

    Machine->AssignToPatient(Patient);

    if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
    {
        const FTransform ApproachTransform = Machine->GetApproachTransform();
        NavigationSubsystem->BroadcastNavigationIntent(
            Patient,
            EMRTags::GMS::AI::Navigation::MoveToSnackMachine,
            Machine,
            ApproachTransform.GetLocation(),
            ApproachTransform.GetRotation().Rotator(),
            EMRTags::Machine::SnackMachine);
    }
}

void UEMRSnackMachineSubsystem::FinishSnackTrip(AEMRSnackMachineActor* Machine)
{
    if (!EnsureServerWorld() || !Machine)
    {
        return;
    }

    AEMRPatient* Patient = nullptr;
    FEMRSnackTripState* State = nullptr;
    for (TPair<TWeakObjectPtr<AEMRPatient>, FEMRSnackTripState>& Pair : TripStates)
    {
        if (Pair.Value.AssignedMachine.Get() == Machine)
        {
            Patient = Pair.Key.Get();
            State = &Pair.Value;
            break;
        }
    }

    Machine->SetOccupied(false);
    Machine->ClearAssignment();

    if (!Patient || !State)
    {
        if (bPendingTripRequest)
        {
            bPendingTripRequest = false;
            TryStartRandomSnackTrip();
            ScheduleTripRequestTimer();
        }
        return;
    }

    Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);

    State->bTripActive = false;
    State->bAtMachine = false;
    State->AssignedMachine.Reset();

    ApplySnackRewards(Patient);
    SendPatientBackToWaitingSeat(Patient);

    if (State->bPendingCall)
    {
        TryProcessDeferredCall(Patient);
    }

    if (bPendingTripRequest)
    {
        bPendingTripRequest = false;
        TryStartRandomSnackTrip();
        ScheduleTripRequestTimer();
    }
}

void UEMRSnackMachineSubsystem::TryProcessDeferredCall(AEMRPatient* Patient)
{
    if (!EnsureServerWorld() || !Patient)
    {
        return;
    }

    FEMRSnackTripState* State = TripStates.Find(Patient);
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
            const FGameplayTag NavigationTag = NavigationSubsystem->GetNavigationMessageTagForMachine(EMRTags::Machine::Reception);
            NavigationSubsystem->BroadcastNavigationIntent(
                Patient,
                NavigationTag,
                ReceptionMachine,
                ReceptionMachine->GetPatientWaitPointLocation(),
                ReceptionMachine->GetPatientWaitPointRotation(),
                EMRTags::Machine::Reception);
        }
    }

    State->bPendingCall = false;
}

void UEMRSnackMachineSubsystem::ApplySnackRewards(AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return;
    }

    const FEMRSnackMachineUpgradeTuning* Tuning = GetSnackUpgradeTuning();
    if (!Tuning)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (Tuning->RevenuePerTrip > 0.0f)
    {
        AEMRNightShiftGameState* NightShiftGameState = World->GetGameState<AEMRNightShiftGameState>();
        UAbilitySystemComponent* TeamASC = NightShiftGameState ? NightShiftGameState->GetAbilitySystemComponent() : nullptr;
        if (!TeamASC)
        {
            UE_LOG(LogSnackMachineSubsystem, Warning, TEXT("[SnackMachine] Team ASC not available for trip reward."));
        }
        else
        {
            TArray<const UEMREconomySystemGenerics*> LoadedEconomy;
            if (!UEMRAssetManager::Get().CollectLoadedEconomySystemGenerics(LoadedEconomy) || LoadedEconomy.IsEmpty())
            {
                UE_LOG(LogSnackMachineSubsystem, Warning, TEXT("[SnackMachine] EconomySystemGenerics not loaded for trip reward."));
            }
            else if (const UEMREconomySystemGenerics* EconomyConfig = LoadedEconomy[0])
            {
                const TSubclassOf<UGameplayEffect> GrantMoneyEffect = EconomyConfig->GetGrantMoneyEffect();
                if (!GrantMoneyEffect)
                {
                    UE_LOG(LogSnackMachineSubsystem, Warning, TEXT("[SnackMachine] Missing GrantMoney effect in economy config."));
                }
                else
                {
                    FGameplayEffectContextHandle MoneyEffectContext = TeamASC->MakeEffectContext();
                    MoneyEffectContext.AddSourceObject(Patient);

                    FGameplayEffectSpecHandle MoneySpecHandle = TeamASC->MakeOutgoingSpec(GrantMoneyEffect, 1.0f, MoneyEffectContext);
                    if (MoneySpecHandle.IsValid())
                    {
                        MoneySpecHandle.Data->SetSetByCallerMagnitude(EMRTags::SetByCaller::GrantMoney, Tuning->RevenuePerTrip);
                        TeamASC->ApplyGameplayEffectSpecToSelf(*MoneySpecHandle.Data.Get());
                    }
                }
            }
        }
    }

    if (Tuning->PatienceRestoreAmount > 0.0f)
    {
        if (UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent())
        {
            const float MaxPatience = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetMaxPatienceAttribute());
            const float CurrentPatience = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetPatienceAttribute());
            const float NewPatience = FMath::Clamp(CurrentPatience + Tuning->PatienceRestoreAmount, 0.0f, MaxPatience);
            PatientASC->SetNumericAttributeBase(UEMRPatientAttributeSet::GetPatienceAttribute(), NewPatience);
        }
    }
}

void UEMRSnackMachineSubsystem::SendPatientBackToWaitingSeat(AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return;
    }

    UEMRWaitingRoomManagerComponent* Manager = CachedWaitingRoomManager.Get();
    if (!Manager)
    {
        return;
    }

    if (UEMRWaitingSeatComponent* WaitingSeat = Manager->GetSeatForPatient(Patient))
    {
        if (UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent())
        {
            FGameplayEventData EventData;
            EventData.EventTag = EMRTags::Abilities::MoveToSeat;
            EventData.Target = Patient;
            EventData.OptionalObject = WaitingSeat;
            PatientASC->HandleGameplayEvent(EventData.EventTag, &EventData);
        }
    }
}

void UEMRSnackMachineSubsystem::ResolveDifficultyTuning()
{
    if (!EnsureServerWorld())
    {
        return;
    }

    TArray<const UEMRSubsystemConfig*> Configs;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(Configs) || Configs.IsEmpty())
    {
        UEMRAssetManager::Get().LoadSubsystemConfig(FStreamableDelegate::CreateUObject(this, &ThisClass::ResolveDifficultyTuning));
        UE_LOG(LogSnackMachineSubsystem, Verbose, TEXT("[SnackMachine] SubsystemConfig not loaded yet."));
        return;
    }

    const UEMRSubsystemConfig* Config = Configs[0];
    CachedDifficultyTuning = Config ? Config->GetDifficultyTuning() : nullptr;
    if (!CachedDifficultyTuning.IsValid())
    {
        UE_LOG(LogSnackMachineSubsystem, Warning, TEXT("[SnackMachine] DifficultyTuning missing in SubsystemConfig."));
        return;
    }

    ScheduleTripRequestTimer();
}

void UEMRSnackMachineSubsystem::TryBindWaitingRoom()
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
                Manager->OnWaitingPatientsChanged.RemoveDynamic(this, &ThisClass::HandleWaitingPatientsChanged);
                Manager->OnWaitingPatientsChanged.AddDynamic(this, &ThisClass::HandleWaitingPatientsChanged);
                return;
            }
        }
    }

    FTimerDelegate RetryDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::TryBindWaitingRoom);
    World->GetTimerManager().SetTimer(WaitingRoomRetryTimer, RetryDelegate, 1.0f, false);
}

void UEMRSnackMachineSubsystem::DiscoverMachines()
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

    for (TActorIterator<AEMRSnackMachineActor> It(World); It; ++It)
    {
        RegisterMachine(*It);
    }
}

AEMRSnackMachineActor* UEMRSnackMachineSubsystem::FindAvailableMachine() const
{
    for (const TWeakObjectPtr<AEMRSnackMachineActor>& MachinePtr : RegisteredMachines)
    {
        if (AEMRSnackMachineActor* Machine = MachinePtr.Get())
        {
            if (Machine->IsAvailable())
            {
                return Machine;
            }
        }
    }

    return nullptr;
}

bool UEMRSnackMachineSubsystem::EnsureServerWorld() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}

const FEMRSnackMachineUpgradeTuning* UEMRSnackMachineSubsystem::GetSnackUpgradeTuning() const
{
    if (!CachedDifficultyTuning.IsValid())
    {
        return nullptr;
    }

    return &CachedDifficultyTuning->GetSnackMachineUpgradeTuning();
}

