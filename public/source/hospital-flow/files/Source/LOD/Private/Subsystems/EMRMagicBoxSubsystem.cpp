#include "Subsystems/EMRMagicBoxSubsystem.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRAIController.h"
#include "Characters/Patient/EMRPatient.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Environment/EMRMagicBoxActor.h"
#include "Framework/EMRAssetManager.h"
#include "GAS/EMRTags.h"
#include "Engine/GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Subsystems/EMRTreatmentSubsystem.h"
#include "TimerManager.h"

namespace
{
    constexpr TCHAR MagicBoxFlowLogPrefix[] = TEXT("[MagicBoxFlow]");

    void StopPatientMovementForMagicBox(AEMRPatient* Patient)
    {
        if (!Patient)
        {
            return;
        }

        if (AController* Controller = Patient->GetController())
        {
            if (AAIController* AIController = Cast<AAIController>(Controller))
            {
                AIController->StopMovement();
            }

            if (AEMRAIController* EMRAIController = Cast<AEMRAIController>(Controller))
            {
                EMRAIController->ResetBlackboardState();
            }
        }

        if (UCharacterMovementComponent* MovementComponent = Patient->GetCharacterMovement())
        {
            MovementComponent->DisableMovement();
        }
    }

    void RestorePatientMovementAfterMagicBox(AEMRPatient* Patient)
    {
        if (!Patient)
        {
            return;
        }

        if (UCharacterMovementComponent* MovementComponent = Patient->GetCharacterMovement())
        {
            MovementComponent->SetMovementMode(MOVE_Walking);
        }
    }
}

void UEMRMagicBoxSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem OnWorldBeginPlay world=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(&InWorld));

    if (!EnsureServerWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem OnWorldBeginPlay skipped: not server world"), MagicBoxFlowLogPrefix);
        return;
    }

    ResolveDifficultyTuning();
}

void UEMRMagicBoxSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem Deinitialize tripCount=%d boxCount=%d"), MagicBoxFlowLogPrefix, TripStates.Num(), RegisteredMagicBoxes.Num());
    for (TPair<TWeakObjectPtr<AEMRPatient>, FEMRMagicBoxTripState>& Pair : TripStates)
    {
        FEMRMagicBoxTripState& State = Pair.Value;
        ClearTripTimers(State);
        if (AEMRPatient* Patient = Pair.Key.Get())
        {
            Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);
        }

        if (AEMRMagicBoxActor* MagicBox = State.AssignedMagicBox.Get())
        {
            MagicBox->NotifyTreatmentAborted(Pair.Key.Get());
            MagicBox->SetOccupied(false);
            MagicBox->ClearAssignment();
        }
    }

    RegisteredMagicBoxes.Reset();
    TripStates.Reset();
    CachedDifficultyTuning.Reset();

    Super::Deinitialize();
}

void UEMRMagicBoxSubsystem::RegisterMagicBox(AEMRMagicBoxActor* MagicBox)
{
    if (!EnsureServerWorld() || !MagicBox || RegisteredMagicBoxes.Contains(MagicBox))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s Subsystem RegisterMagicBox ignored box=%s server=%d already=%d"),
            MagicBoxFlowLogPrefix,
            *GetNameSafe(MagicBox),
            EnsureServerWorld() ? 1 : 0,
            RegisteredMagicBoxes.Contains(MagicBox) ? 1 : 0);
        return;
    }

    RegisteredMagicBoxes.Add(MagicBox);
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem RegisterMagicBox added box=%s total=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(MagicBox), RegisteredMagicBoxes.Num());
}

void UEMRMagicBoxSubsystem::UnregisterMagicBox(AEMRMagicBoxActor* MagicBox)
{
    if (!MagicBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem UnregisterMagicBox ignored null"), MagicBoxFlowLogPrefix);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem UnregisterMagicBox box=%s beforeCount=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(MagicBox), RegisteredMagicBoxes.Num());
    RegisteredMagicBoxes.Remove(MagicBox);

    TArray<AEMRPatient*> PatientsToRequeue;
    TArray<TWeakObjectPtr<AEMRPatient>> TripEntriesToRemove;
    for (TPair<TWeakObjectPtr<AEMRPatient>, FEMRMagicBoxTripState>& Pair : TripStates)
    {
        AEMRPatient* Patient = Pair.Key.Get();
        FEMRMagicBoxTripState& State = Pair.Value;
        if (State.AssignedMagicBox.Get() != MagicBox)
        {
            continue;
        }

        ClearTripTimers(State);

        State.bTripActive = false;
        State.bAtMagicBox = false;
        State.bEnteringBox = false;
        State.bTreatmentStarted = false;
        State.AssignedMagicBox.Reset();
        TripEntriesToRemove.Add(Pair.Key);

        if (Patient)
        {
            Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);
            RestorePatientMovementAfterMagicBox(Patient);
            Patient->SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::MagicBox, false);
            UE_LOG(LogTemp, Warning, TEXT("%s Subsystem UnregisterMagicBox resumed patience drain patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
            PatientsToRequeue.Add(Patient);
        }
    }

    MagicBox->NotifyTreatmentAborted(nullptr);
    MagicBox->SetOccupied(false);
    MagicBox->ClearAssignment();

    for (const TWeakObjectPtr<AEMRPatient>& PatientKey : TripEntriesToRemove)
    {
        TripStates.Remove(PatientKey);
    }

    const UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    if (UEMRTreatmentSubsystem* TreatmentSubsystem = GameInstance ? GameInstance->GetSubsystem<UEMRTreatmentSubsystem>() : nullptr)
    {
        for (AEMRPatient* Patient : PatientsToRequeue)
        {
            if (!Patient)
            {
                continue;
            }

            if (const UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent())
            {
                if (ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving))
                {
                    continue;
                }
            }

            TreatmentSubsystem->AddPatientToTreatmentQueue(Patient);
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem UnregisterMagicBox done box=%s requeued=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(MagicBox), PatientsToRequeue.Num());
}

bool UEMRMagicBoxSubsystem::TryAssignPatient(AEMRPatient* Patient)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem TryAssignPatient start patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    if (!EnsureServerWorld() || !Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem TryAssignPatient failed precondition server=%d patientValid=%d"), MagicBoxFlowLogPrefix, EnsureServerWorld() ? 1 : 0, Patient ? 1 : 0);
        return false;
    }

    if (const UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent())
    {
        if (ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving))
        {
            UE_LOG(LogTemp, Warning, TEXT("%s Subsystem TryAssignPatient rejected leaving patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
            return false;
        }
    }

    if (FEMRMagicBoxTripState* ExistingState = TripStates.Find(Patient))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem TryAssignPatient already tracked patient=%s active=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient), ExistingState->bTripActive ? 1 : 0);
        return ExistingState->bTripActive;
    }

    AEMRMagicBoxActor* MagicBox = FindAvailableMagicBox();
    if (!MagicBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem TryAssignPatient failed no available box patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return false;
    }

    FEMRMagicBoxTripState& State = TripStates.FindOrAdd(Patient);
    ClearTripTimers(State);
    State.bTripActive = true;
    State.bAtMagicBox = false;
    State.bEnteringBox = false;
    State.bTreatmentStarted = false;
    State.AssignedMagicBox = MagicBox;

    Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);
    Patient->OnDestroyed.AddDynamic(this, &ThisClass::HandlePatientDestroyed);

    MagicBox->AssignToPatient(Patient);
    MagicBox->SetDoorsOpen(false);
    Patient->SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::MagicBox, true);
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem TryAssignPatient suppressed patience drain patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem TryAssignPatient assigned patient=%s box=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient), *GetNameSafe(MagicBox));

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GameInstance ? GameInstance->GetSubsystem<UEMRNavigationIntentSubsystem>() : nullptr)
    {
        const FTransform ApproachTransform = MagicBox->GetApproachTransform();
        AActor* ApproachTargetActor = MagicBox->GetApproachNavigationTargetActor();
        NavigationSubsystem->BroadcastNavigationIntent(
            Patient,
            EMRTags::GMS::AI::Navigation::MoveToMagicBox,
            ApproachTargetActor,
            ApproachTransform.GetLocation(),
            ApproachTransform.GetRotation().Rotator(),
            EMRTags::Machine::MagicBox);
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s Subsystem TryAssignPatient navigation sent patient=%s box=%s navTargetActor=%s target=%s"),
            MagicBoxFlowLogPrefix,
            *GetNameSafe(Patient),
            *GetNameSafe(MagicBox),
            *GetNameSafe(ApproachTargetActor),
            *ApproachTransform.GetLocation().ToCompactString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem TryAssignPatient no navigation subsystem patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    }

    return true;
}

void UEMRMagicBoxSubsystem::NotifyMagicBoxArrival(AEMRMagicBoxActor* MagicBox, AEMRPatient* Patient)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem NotifyMagicBoxArrival start box=%s patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(MagicBox), *GetNameSafe(Patient));
    if (!EnsureServerWorld() || !MagicBox || !Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem NotifyMagicBoxArrival failed precondition server=%d boxValid=%d patientValid=%d"), MagicBoxFlowLogPrefix, EnsureServerWorld() ? 1 : 0, MagicBox ? 1 : 0, Patient ? 1 : 0);
        return;
    }

    FEMRMagicBoxTripState* State = TripStates.Find(Patient);
    if (!State || State->AssignedMagicBox.Get() != MagicBox || State->bAtMagicBox)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s Subsystem NotifyMagicBoxArrival ignored stateMismatch hasState=%d assignedBox=%s atBox=%d"),
            MagicBoxFlowLogPrefix,
            State ? 1 : 0,
            State ? *GetNameSafe(State->AssignedMagicBox.Get()) : TEXT("None"),
            State && State->bAtMagicBox ? 1 : 0);
        return;
    }

    State->bAtMagicBox = true;
    State->bTripActive = true;
    State->bEnteringBox = false;
    State->bTreatmentStarted = false;
    Patient->SetActorRotation(MagicBox->GetApproachTransform().GetRotation().Rotator());
    MagicBox->SetOccupied(true);
    MagicBox->SetDoorsOpen(true);

    const float EntryDelay = MagicBox->GetDoorMoveDurationSeconds() + MagicBox->GetEntryDelayAfterDoorOpenSeconds();
    FTimerDelegate EntryDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::HandlePatientEnterMagicBox, Patient);
    GetWorld()->GetTimerManager().SetTimer(State->EntryTimerHandle, EntryDelegate, EntryDelay, false);
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem NotifyMagicBoxArrival scheduled entry patient=%s delay=%.2f"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient), EntryDelay);
}

void UEMRMagicBoxSubsystem::HandlePatientDestroyed(AActor* DestroyedActor)
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
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem HandlePatientDestroyed patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));

    FEMRMagicBoxTripState* State = TripStates.Find(Patient);
    if (!State)
    {
        return;
    }

    ClearTripTimers(*State);

    if (AEMRMagicBoxActor* MagicBox = State->AssignedMagicBox.Get())
    {
        MagicBox->NotifyTreatmentAborted(Patient);
        MagicBox->SetOccupied(false);
        MagicBox->ClearAssignment();
    }

    TripStates.Remove(Patient);
}

void UEMRMagicBoxSubsystem::HandlePatientEnterMagicBox(AEMRPatient* Patient)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem HandlePatientEnterMagicBox start patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    if (!EnsureServerWorld() || !Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem HandlePatientEnterMagicBox failed precondition"), MagicBoxFlowLogPrefix);
        return;
    }

    FEMRMagicBoxTripState* State = TripStates.Find(Patient);
    if (!State)
    {
        return;
    }

    AEMRMagicBoxActor* MagicBox = State->AssignedMagicBox.Get();
    if (!MagicBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem HandlePatientEnterMagicBox missing assigned box patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return;
    }

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UEMRNavigationIntentSubsystem* NavigationSubsystem = GameInstance ? GameInstance->GetSubsystem<UEMRNavigationIntentSubsystem>() : nullptr;
    const FTransform InsideTransform = MagicBox->GetInsideTransform();

    State->bEnteringBox = true;
    if (AController* Controller = Patient->GetController())
    {
        if (AEMRAIController* EMRAIController = Cast<AEMRAIController>(Controller))
        {
            EMRAIController->ResetBlackboardState();
        }
    }

    if (NavigationSubsystem)
    {
        AActor* InsideTargetActor = MagicBox->GetInsideNavigationTargetActor();
        NavigationSubsystem->BroadcastNavigationIntent(
            Patient,
            EMRTags::GMS::AI::Navigation::MoveToMagicBox,
            InsideTargetActor,
            InsideTransform.GetLocation(),
            InsideTransform.GetRotation().Rotator(),
            EMRTags::Machine::MagicBox);
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s Subsystem HandlePatientEnterMagicBox sent inside navigation patient=%s navTargetActor=%s insideTarget=%s"),
            MagicBoxFlowLogPrefix,
            *GetNameSafe(Patient),
            *GetNameSafe(InsideTargetActor),
            *InsideTransform.GetLocation().ToCompactString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem HandlePatientEnterMagicBox no navigation subsystem patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    }

    FTimerDelegate PollDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::PollPatientInsideArrival, Patient);
    GetWorld()->GetTimerManager().SetTimer(State->InsideArrivalPollTimerHandle, PollDelegate, 0.20f, true);
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem HandlePatientEnterMagicBox started inside arrival poll patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
}

void UEMRMagicBoxSubsystem::PollPatientInsideArrival(AEMRPatient* Patient)
{
    if (!EnsureServerWorld() || !Patient)
    {
        return;
    }

    FEMRMagicBoxTripState* State = TripStates.Find(Patient);
    if (!State || !State->bEnteringBox)
    {
        return;
    }

    AEMRMagicBoxActor* MagicBox = State->AssignedMagicBox.Get();
    if (!MagicBox)
    {
        return;
    }

    const FTransform InsideTransform = MagicBox->GetInsideTransform();
    const float DistSq = FVector::DistSquared(Patient->GetActorLocation(), InsideTransform.GetLocation());
    constexpr float InsideArrivalRadius = 90.0f;
    if (DistSq > FMath::Square(InsideArrivalRadius))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s Subsystem PollPatientInsideArrival waiting patient=%s dist=%.1f"),
            MagicBoxFlowLogPrefix,
            *GetNameSafe(Patient),
            FMath::Sqrt(DistSq));
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(State->InsideArrivalPollTimerHandle);
    State->bEnteringBox = false;

    StopPatientMovementForMagicBox(Patient);
    Patient->SetActorRotation(InsideTransform.GetRotation().Rotator());
    MagicBox->SetDoorsOpen(false);

    FTimerDelegate StartTreatmentDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::StartMagicTreatment, Patient);
    GetWorld()->GetTimerManager().SetTimer(State->StartTreatmentTimerHandle, StartTreatmentDelegate, MagicBox->GetDoorMoveDurationSeconds(), false);
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s Subsystem PollPatientInsideArrival reached inside patient=%s startTreatmentDelay=%.2f"),
        MagicBoxFlowLogPrefix,
        *GetNameSafe(Patient),
        MagicBox->GetDoorMoveDurationSeconds());
}

void UEMRMagicBoxSubsystem::StartMagicTreatment(AEMRPatient* Patient)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem StartMagicTreatment start patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    if (!EnsureServerWorld() || !Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem StartMagicTreatment failed precondition"), MagicBoxFlowLogPrefix);
        return;
    }

    FEMRMagicBoxTripState* State = TripStates.Find(Patient);
    if (!State)
    {
        return;
    }

    AEMRMagicBoxActor* MagicBox = State->AssignedMagicBox.Get();
    if (!MagicBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem StartMagicTreatment missing assigned box patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return;
    }

    State->bTreatmentStarted = true;
    const float Duration = GetMagicBoxTreatmentDurationSeconds();
    MagicBox->NotifyTreatmentStarted(Patient, Duration);

    FTimerDelegate FinishDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::FinishMagicTreatment, Patient);
    GetWorld()->GetTimerManager().SetTimer(State->FinishTreatmentTimerHandle, FinishDelegate, Duration, false);
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem StartMagicTreatment scheduled finish patient=%s duration=%.2f"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient), Duration);
}

void UEMRMagicBoxSubsystem::FinishMagicTreatment(AEMRPatient* Patient)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinishMagicTreatment start patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    if (!EnsureServerWorld() || !Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinishMagicTreatment failed precondition"), MagicBoxFlowLogPrefix);
        return;
    }

    FEMRMagicBoxTripState* State = TripStates.Find(Patient);
    if (!State)
    {
        return;
    }

    AEMRMagicBoxActor* MagicBox = State->AssignedMagicBox.Get();
    if (!MagicBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinishMagicTreatment missing assigned box patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return;
    }

    State->bTreatmentStarted = false;
    MagicBox->NotifyTreatmentFinished(Patient);

    const float ExitDelay = MagicBox->GetDoorMoveDurationSeconds() + MagicBox->GetExitDelayAfterDoorOpenSeconds();
    FTimerDelegate FinalizeDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::FinalizeMagicTreatment, Patient);
    GetWorld()->GetTimerManager().SetTimer(State->ExitTimerHandle, FinalizeDelegate, ExitDelay, false);
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinishMagicTreatment scheduled finalize patient=%s delay=%.2f"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient), ExitDelay);
}

void UEMRMagicBoxSubsystem::FinalizeMagicTreatment(AEMRPatient* Patient)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinalizeMagicTreatment start patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    if (!EnsureServerWorld() || !Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinalizeMagicTreatment failed precondition"), MagicBoxFlowLogPrefix);
        return;
    }

    FEMRMagicBoxTripState* State = TripStates.Find(Patient);
    if (!State)
    {
        return;
    }

    AEMRMagicBoxActor* MagicBox = State->AssignedMagicBox.Get();
    if (!MagicBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinalizeMagicTreatment missing assigned box patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return;
    }

    ClearTripTimers(*State);
    MagicBox->SetOccupied(false);
    MagicBox->ClearAssignment();

    Patient->OnDestroyed.RemoveDynamic(this, &ThisClass::HandlePatientDestroyed);
    RestorePatientMovementAfterMagicBox(Patient);
    Patient->SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::MagicBox, false);
    UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinalizeMagicTreatment resumed patience drain patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));

    TripStates.Remove(Patient);

    const UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    if (UEMRTreatmentSubsystem* TreatmentSubsystem = GameInstance ? GameInstance->GetSubsystem<UEMRTreatmentSubsystem>() : nullptr)
    {
        const bool bResolved = TreatmentSubsystem->ResolvePatientAsFullyTreated(Patient);
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinalizeMagicTreatment resolved patient=%s resolved=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient), bResolved ? 1 : 0);
        if (!bResolved)
        {
            TreatmentSubsystem->AddPatientToTreatmentQueue(Patient);
            UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinalizeMagicTreatment requeued patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        }

        TreatmentSubsystem->HandleMagicBoxFreed();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem FinalizeMagicTreatment no treatment subsystem patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    }
}

AEMRMagicBoxActor* UEMRMagicBoxSubsystem::FindAvailableMagicBox() const
{
    for (const TWeakObjectPtr<AEMRMagicBoxActor>& MagicBoxPtr : RegisteredMagicBoxes)
    {
        if (AEMRMagicBoxActor* MagicBox = MagicBoxPtr.Get())
        {
            if (MagicBox->IsAvailable())
            {
                return MagicBox;
            }
        }
    }

    return nullptr;
}

bool UEMRMagicBoxSubsystem::EnsureServerWorld() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}

void UEMRMagicBoxSubsystem::ResolveDifficultyTuning()
{
    TArray<const UEMRSubsystemConfig*> Configs;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(Configs) || Configs.IsEmpty())
    {
        CachedDifficultyTuning.Reset();
        return;
    }

    const UEMRSubsystemConfig* Config = Configs[0];
    CachedDifficultyTuning = Config ? Config->GetDifficultyTuning() : nullptr;
}

float UEMRMagicBoxSubsystem::GetMagicBoxTreatmentDurationSeconds() const
{
    if (!CachedDifficultyTuning.IsValid())
    {
        const_cast<UEMRMagicBoxSubsystem*>(this)->ResolveDifficultyTuning();
    }

    if (const UEMRDifficultyTuningData* DifficultyTuning = CachedDifficultyTuning.Get())
    {
        return FMath::Max(DifficultyTuning->GetMagicBoxUpgradeTuning().TreatmentDurationSeconds, 0.1f);
    }

    return 60.0f;
}

void UEMRMagicBoxSubsystem::ClearTripTimers(FEMRMagicBoxTripState& State) const
{
    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(State.EntryTimerHandle);
        TimerManager.ClearTimer(State.InsideArrivalPollTimerHandle);
        TimerManager.ClearTimer(State.StartTreatmentTimerHandle);
        TimerManager.ClearTimer(State.FinishTreatmentTimerHandle);
        TimerManager.ClearTimer(State.ExitTimerHandle);
        UE_LOG(LogTemp, Warning, TEXT("%s Subsystem ClearTripTimers"), MagicBoxFlowLogPrefix);
    }
}
