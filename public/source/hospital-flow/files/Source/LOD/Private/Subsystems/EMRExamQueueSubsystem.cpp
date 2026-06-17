
#include "Subsystems/EMRExamQueueSubsystem.h"

#include "Characters/Patient/EMRPatient.h"
#include "GAS/EMRTags.h"
#include "AbilitySystemComponent.h"
#include "Data/EMRExamQueueMapping.h"
#include "Data/EMRExamRequirementsForPathologyMapping.h"
#include "Data/EMRExamMachineCompletionMapping.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Subsystems/EMRTreatmentSubsystem.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Environment/EMRExamWaitingArea.h"
#include "Environment/EMRExamWaitingManagerComponent.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "Framework/EMRAssetManager.h"
#include "Interaction/EMRBaseMachine.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Data/EMRNightShiftDefinition.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogExamRoutingDebug, Log, All);

namespace
{
    FString JoinTagsForLog(const TArray<FGameplayTag>& Tags)
    {
        TArray<FString> AsStrings;
        AsStrings.Reserve(Tags.Num());
        for (const FGameplayTag& Tag : Tags)
        {
            if (Tag.IsValid())
            {
                AsStrings.Add(Tag.ToString());
            }
        }

        return AsStrings.Num() > 0 ? FString::Join(AsStrings, TEXT(", ")) : TEXT("<none>");
    }

    UEMRWaitingRoomManagerComponent* FindReceptionWaitingManager(UWorld* World)
    {
        if (!World)
        {
            return nullptr;
        }

        for (TActorIterator<AEMRWaitingRoomArea> It(World); It; ++It)
        {
            if (AEMRWaitingRoomArea* WaitingArea = *It)
            {
                if (UEMRWaitingRoomManagerComponent* Manager = WaitingArea->GetManagerComponent())
                {
                    return Manager;
                }
            }
        }

        return nullptr;
    }

    bool RemovePatientFromReceptionQueue(UWorld* World, AEMRPatient* Patient)
    {
        if (!Patient)
        {
            return false;
        }

        if (UEMRWaitingRoomManagerComponent* Manager = FindReceptionWaitingManager(World))
        {
            return Manager->RemovePatientFromQueue(Patient);
        }

        return false;
    }

    void SendPatientToReceptionMachine(UEMRExamQueueSubsystem* Subsystem, AEMRPatient* Patient)
    {
        if (!Subsystem || !Patient)
        {
            return;
        }

        if (UEMRNavigationIntentSubsystem* NavigationSubsystem = Subsystem->GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
        {
            if (AEMRBaseMachine* ClosestMachine = NavigationSubsystem->FindClosestMachine(Patient, EMRTags::Machine::Reception))
            {
                if (!ClosestMachine->IsOccupied() || ClosestMachine->GetAssignedPatient() == Patient)
                {
                    ClosestMachine->SetOccupiedByPatient(Patient);
                }

                NavigationSubsystem->BroadcastNavigationIntent(
                    Patient,
                    EMRTags::GMS::AI::Navigation::MoveToReception,
                    ClosestMachine,
                    ClosestMachine->GetPatientWaitPointLocation(),
                    ClosestMachine->GetPatientWaitPointRotation());
            }
        }
    }

    AEMRBaseMachine* FindClosestAvailableMachineForPatient(
        const UWorld* World,
        const AActor* OriginActor,
        const FGameplayTag& MachineTypeTag,
        const AEMRPatient* Patient)
    {
        if (!World || !OriginActor || !MachineTypeTag.IsValid())
        {
            return nullptr;
        }

        AEMRBaseMachine* ClosestMachine = nullptr;
        float ClosestDistSq = TNumericLimits<float>::Max();

        for (TActorIterator<AEMRBaseMachine> It(World); It; ++It)
        {
            AEMRBaseMachine* Candidate = *It;
            if (!Candidate)
            {
                continue;
            }

            const FGameplayTag CandidateMachineTag = Candidate->GetMachineTypeTag();
            if (!CandidateMachineTag.MatchesTag(MachineTypeTag) && !MachineTypeTag.MatchesTag(CandidateMachineTag))
            {
                continue;
            }

            if (Candidate->IsOccupied() && (!Patient || Candidate->GetAssignedPatient() != Patient))
            {
                UE_LOG(
                    LogExamRoutingDebug,
                    Warning,
                    TEXT("[ExamRoutingDebug] Candidate machine rejected (occupied by other): patient=%s machine=%s machineTag=%s requestedTag=%s assignedPatient=%s"),
                    *GetNameSafe(Patient),
                    *GetNameSafe(Candidate),
                    *CandidateMachineTag.ToString(),
                    *MachineTypeTag.ToString(),
                    *GetNameSafe(Candidate->GetAssignedPatient()));
                continue;
            }

            const float DistSq = FVector::DistSquared(OriginActor->GetActorLocation(), Candidate->GetActorLocation());
            UE_LOG(
                LogExamRoutingDebug,
                Warning,
                TEXT("[ExamRoutingDebug] Candidate machine accepted: patient=%s machine=%s machineTag=%s requestedTag=%s distSq=%.1f"),
                *GetNameSafe(Patient),
                *GetNameSafe(Candidate),
                *CandidateMachineTag.ToString(),
                *MachineTypeTag.ToString(),
                DistSq);
            if (DistSq < ClosestDistSq)
            {
                ClosestDistSq = DistSq;
                ClosestMachine = Candidate;
            }
        }

        return ClosestMachine;
    }
}


void UEMRExamQueueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

	LoadSubsystemConfig();
}


void UEMRExamQueueSubsystem::Deinitialize()
{
    ExamQueueMap.Empty();
    CachedExamQueueMappings.Reset();
    CachedRequiredExamsByPathology.Reset();
    CachedRequiredCompletionsByPathology.Reset();
    PatientCompletedExamEffects.Empty();
    PatientReturnToReceptionEffects.Empty();
    CachedExamCompletionMappings.Reset();
    Super::Deinitialize();
}


void UEMRExamQueueSubsystem::AddPatientToExamQueue(AEMRPatient* Patient, const TArray<FGameplayTag>& ExamTags)
{
    if (!Patient)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExamQueueSubsystem] Patient is NULL"));
        return;
    }

    Patient->SetNextMachineOnList(nullptr);

    RemovePatientFromReceptionQueue(GetWorld(), Patient);
    RemoveReturnToReceptionTag(Patient);

    if (ExamTags.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExamQueueSubsystem] No exam tags provided"));
        return;
    }

    bool bLabAnalyzerOnly = true;
    for (const FGameplayTag& ExamTag : ExamTags)
    {
        if (!ExamTag.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root))
        {
            bLabAnalyzerOnly = false;
            break;
        }
    }

    bool bAssignedLabBed = false;
    if (bLabAnalyzerOnly)
    {
        if (UEMRTreatmentSubsystem* TreatmentSubsystem = GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>())
        {
            if (TreatmentSubsystem->HasAvailableBed())
            {
                bAssignedLabBed = TreatmentSubsystem->TryAssignPatientToBedForExam(Patient);
            }
        }
    }

    UE_LOG(
        LogExamRoutingDebug,
        Warning,
        TEXT("[ExamRoutingDebug] AddPatientToExamQueue start: patient=%s exams=[%s] labOnly=%s labBedAssigned=%s"),
        *GetNameSafe(Patient),
        *JoinTagsForLog(ExamTags),
        bLabAnalyzerOnly ? TEXT("true") : TEXT("false"),
        bAssignedLabBed ? TEXT("true") : TEXT("false"));

    
    // Add Patient to Queues
    for (const FGameplayTag& ExamTag : ExamTags)
    {
        TArray<AEMRPatient*>& Queue = ExamQueueMap.FindOrAdd(ExamTag);
        Queue.Add(Patient);

        ApplyQueueTagToPatient(Patient, ExamTag);

        OnPatientQueuedForExam.Broadcast(Patient, ExamTag);

        if (ExamTag.MatchesTagExact(EMRTags::Abilities::Exam::XRay))
        {
            UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Patient queued for XRay patient=%s queueSize=%d"),
                *GetNameSafe(Patient), Queue.Num());
        }
    }

	// Change Patient Phase
	FGameplayTag PhaseTag = EMRTags::Patient::Phase::WaitingExam;
	ApplyPhaseEffect(Patient, PhaseTag);

    if (bLabAnalyzerOnly && !bAssignedLabBed)
    {
        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] Lab-only queue with no bed assignment, requesting lab waiting seat once: patient=%s"),
            *GetNameSafe(Patient));
        AssignExamWaitingSeat(Patient, EMRTags::Machine::LabAnalyzer);
    }

    UE_LOG(
        LogExamRoutingDebug,
        Warning,
        TEXT("[ExamRoutingDebug] AddPatientToExamQueue dispatching next intent: patient=%s bLabAnalyzerOnly=%s"),
        *GetNameSafe(Patient),
        bLabAnalyzerOnly ? TEXT("true") : TEXT("false"));

    DispatchNextQueuedExamIntent(Patient, FGameplayTag::EmptyTag, bLabAnalyzerOnly);
}

void UEMRExamQueueSubsystem::BypassExamQueueToTreatment(AEMRPatient* Patient)
{
    if (!Patient)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExamQueueSubsystem] Cannot bypass exam queue for null patient"));
        return;
    }

    RemovePatientFromReceptionQueue(GetWorld(), Patient);
    RemoveReturnToReceptionTag(Patient);
    TransitionPatientToTreatment(Patient);
}


void UEMRExamQueueSubsystem::ChangePatientPhaseTag(AEMRPatient* Patient, FGameplayTag NewPhaseTag)
{
	if (!Patient || !NewPhaseTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] Invalid patient or phase tag"));
		return;
	}

	UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	

	FActiveGameplayEffectHandle* PhaseHandle = PatientPhaseEffects.Find(Patient);
	if (PhaseHandle && PhaseHandle->IsValid())
	{
		ASC->RemoveActiveGameplayEffect(*PhaseHandle);
		PatientPhaseEffects.Remove(Patient);
	}

	ApplyPhaseEffect(Patient, NewPhaseTag);
}


AEMRPatient* UEMRExamQueueSubsystem::GetNextPatientFromExamQueue(FGameplayTag ExamTag)
{
    if (!ExamTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] Cannot dequeue with invalid tag"));
        return nullptr;
    }

    TArray<AEMRPatient*>* Queue = ExamQueueMap.Find(ExamTag);
    if (!Queue || Queue->Num() == 0)
    {
        return nullptr;
    }

	
    AEMRPatient* Patient = (*Queue)[0];
    if (Patient)
    {
    	ChangePatientPhaseTag(Patient, EMRTags::Patient::Phase::UnderExam);
        ReleaseReceptionSeatIfPresent(Patient);
    }

    return Patient;
}


AEMRPatient* UEMRExamQueueSubsystem::GetNextPatientForMachine(FGameplayTag MachineTag)
{
    if (!MachineTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] Cannot dequeue for invalid machine tag"));
        return nullptr;
    }

    if (CachedExamQueueMappings.Num() == 0)
    {
        RebuildExamQueueMappingsCache();
    }

    for (const FEMRExamQueueMapping& Mapping : CachedExamQueueMappings)
    {
        if (!Mapping.ExamTag.IsValid() || !Mapping.MachineTypeTag.IsValid())
        {
            continue;
        }

        if (!MachineTag.MatchesTag(Mapping.MachineTypeTag) && !Mapping.MachineTypeTag.MatchesTag(MachineTag))
        {
            continue;
        }

        if (TArray<AEMRPatient*>* Queue = ExamQueueMap.Find(Mapping.ExamTag))
        {
            if (Queue->Num() > 0)
            {
                UE_LOG(
                    LogExamRoutingDebug,
                    Warning,
                    TEXT("[ExamRoutingDebug] GetNextPatientForMachine scanning queue: machineTag=%s exam=%s queueSize=%d"),
                    *MachineTag.ToString(),
                    *Mapping.ExamTag.ToString(),
                    Queue->Num());

                for (int32 QueueIndex = 0; QueueIndex < Queue->Num(); ++QueueIndex)
                {
                    AEMRPatient* Patient = (*Queue)[QueueIndex];
                    UE_LOG(
                        LogExamRoutingDebug,
                        Warning,
                        TEXT("[ExamRoutingDebug] GetNextPatientForMachine candidate: machineTag=%s exam=%s queueIndex=%d patient=%s"),
                        *MachineTag.ToString(),
                        *Mapping.ExamTag.ToString(),
                        QueueIndex,
                        *GetNameSafe(Patient));

                    if (!Patient)
                    {
                        continue;
                    }

                    bool bAssignedToAnotherMachine = false;
                    for (TActorIterator<AEMRBaseMachine> It(GetWorld()); It; ++It)
                    {
                        const AEMRBaseMachine* CandidateMachine = *It;
                        if (!CandidateMachine || CandidateMachine->GetAssignedPatient() != Patient)
                        {
                            continue;
                        }

                        const FGameplayTag CandidateMachineTag = CandidateMachine->GetMachineTypeTag();
                        if (!CandidateMachineTag.MatchesTag(MachineTag) && !MachineTag.MatchesTag(CandidateMachineTag))
                        {
                            bAssignedToAnotherMachine = true;
                            UE_LOG(
                                LogExamRoutingDebug,
                                Warning,
                                TEXT("[ExamRoutingDebug] GetNextPatientForMachine lock: patient=%s requestedMachineTag=%s existingMachine=%s existingMachineTag=%s"),
                                *GetNameSafe(Patient),
                                *MachineTag.ToString(),
                                *GetNameSafe(CandidateMachine),
                                *CandidateMachineTag.ToString());
                            break;
                        }
                    }

                    if (bAssignedToAnotherMachine)
                    {
                        UE_LOG(
                            LogExamRoutingDebug,
                            Warning,
                            TEXT("[ExamRoutingDebug] GetNextPatientForMachine skip due to lock: patient=%s requestedMachineTag=%s"),
                            *GetNameSafe(Patient),
                            *MachineTag.ToString());
                        continue;
                    }

                    ChangePatientPhaseTag(Patient, EMRTags::Patient::Phase::UnderExam);
                    ReleaseExamWaitingSeat(Patient, MachineTag);
                    ReleaseReceptionSeatIfPresent(Patient);
                    UE_LOG(
                        LogExamRoutingDebug,
                        Warning,
                        TEXT("[ExamRoutingDebug] GetNextPatientForMachine selected: patient=%s machineTag=%s exam=%s"),
                        *GetNameSafe(Patient),
                        *MachineTag.ToString(),
                        *Mapping.ExamTag.ToString());
                    return Patient;
                }
            }
        }
    }

    return nullptr;
}


bool UEMRExamQueueSubsystem::IsQueueEmpty(FGameplayTag ExamTag) const
{
    const TArray<AEMRPatient*>* Queue = ExamQueueMap.Find(ExamTag);
    return !Queue || Queue->Num() == 0;
}


int32 UEMRExamQueueSubsystem::GetQueueSize(FGameplayTag ExamTag) const
{
    const TArray<AEMRPatient*>* Queue = ExamQueueMap.Find(ExamTag);
    return Queue ? Queue->Num() : 0;
}


void UEMRExamQueueSubsystem::CompleteExamForPatient(AEMRPatient* Patient, FGameplayTag ExamType, bool bSuccess)
{
    if (!Patient || !ExamType.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] CompleteExamForPatient: Invalid patient or exam type"));
        return;
    }

    if (ExamType.MatchesTagExact(EMRTags::Abilities::Exam::XRay))
    {
        UE_LOG(LogTemp, Log, TEXT("[XRayFlow] CompleteExamForPatient start patient=%s success=%s"),
            *GetNameSafe(Patient), bSuccess ? TEXT("true") : TEXT("false"));
    }

    ApplyCompletedExamTag(Patient, ExamType);

    const FEMRExamQueueMapping* Mapping = GetExamQueueMapping(ExamType);
    if ((!Mapping || !Mapping->ExamTag.IsValid()) && ExamType.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root))
    {
        Mapping = GetExamQueueMapping(EMRTags::Abilities::Exam::LabAnalyzer::Root);
    }

    if (!Mapping || !Mapping->ExamTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] No queue tag found for exam: %s"), *ExamType.ToString());
        return;
    }

    const FGameplayTag QueueTag = Mapping->QueueTag;
    if (!QueueTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] No queue tag found for exam: %s"), *ExamType.ToString());
        return;
    }

    const FGameplayTag MatchRootTag = ExamType.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root)
    ? EMRTags::Abilities::Exam::LabAnalyzer::Root
    : FGameplayTag::EmptyTag;
    const FGameplayTag QueueExamTag = ResolveQueuedExamTagForPatient(Patient, ExamType, QueueTag, Mapping->ExamTag, MatchRootTag);
    TArray<AEMRPatient*>* Queue = QueueExamTag.IsValid() ? ExamQueueMap.Find(QueueExamTag) : nullptr;
    if (!Queue)
    {
        UE_LOG(LogTemp, Error, TEXT("[ExamQueue] ❌ ExamQueue not found for exam type: %s"), *ExamType.ToString());
        return;
    }

    UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] CompleteExamForPatient: Patient ASC missing"));
        return;
    }

    const bool bPatientIsLeaving = ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving);

    UE_LOG(
        LogExamRoutingDebug,
        Warning,
        TEXT("[ExamRoutingDebug] CompleteExamForPatient: patient=%s exam=%s success=%s queueExamTag=%s queueFound=%s leaving=%s"),
        *GetNameSafe(Patient),
        *ExamType.ToString(),
        bSuccess ? TEXT("true") : TEXT("false"),
        *QueueExamTag.ToString(),
        Queue ? TEXT("true") : TEXT("false"),
        bPatientIsLeaving ? TEXT("true") : TEXT("false"));

    if (Queue->Contains(Patient))
    {
        Queue->Remove(Patient);
        RemoveQueueTagFromPatient(Patient, QueueExamTag);
    }

    ReleaseExamWaitingSeat(Patient, Mapping->MachineTypeTag);


    if (!HasAnyQueueTags(Patient))
    {
        Patient->SetNextMachineOnList(nullptr);

        if (bPatientIsLeaving)
        {
            OnExamCompleted.Broadcast(ExamType);
            ExamCompletedNativeDelegate.Broadcast(ExamType);
            if (ExamType.MatchesTagExact(EMRTags::Abilities::Exam::XRay))
            {
                UE_LOG(LogTemp, Log, TEXT("[XRayFlow] CompleteExamForPatient finished patient=%s leaving=true"),
                    *GetNameSafe(Patient));
            }
            return;
        }

        if (DoesPatientSatisfyRequiredExams(Patient))
        {
            TransitionPatientToTreatment(Patient);
            if (ExamType.MatchesTagExact(EMRTags::Abilities::Exam::XRay))
            {
                UE_LOG(LogTemp, Log, TEXT("[XRayFlow] CompleteExamForPatient transition=treatment patient=%s"),
                    *GetNameSafe(Patient));
            }
        }
        else
        {
            SendPatientBackToReception(Patient);
            if (ExamType.MatchesTagExact(EMRTags::Abilities::Exam::XRay))
            {
                UE_LOG(LogTemp, Log, TEXT("[XRayFlow] CompleteExamForPatient transition=reception patient=%s"),
                    *GetNameSafe(Patient));
            }
        }
    }
    else
    {
        if (!bPatientIsLeaving)
        {
            const TArray<FGameplayTag> RemainingQueuedExams = GetQueuedExamTagsForPatient(Patient);
            bool bLabOnlyRemaining = RemainingQueuedExams.Num() > 0;
            for (const FGameplayTag& RemainingExamTag : RemainingQueuedExams)
            {
                if (!IsLabAnalyzerExamTag(RemainingExamTag))
                {
                    bLabOnlyRemaining = false;
                    break;
                }
            }

            UE_LOG(
                LogExamRoutingDebug,
                Warning,
                TEXT("[ExamRoutingDebug] CompleteExamForPatient remaining queued exams: patient=%s queued=[%s] labOnlyRemaining=%s"),
                *GetNameSafe(Patient),
                *JoinTagsForLog(RemainingQueuedExams),
                bLabOnlyRemaining ? TEXT("true") : TEXT("false"));

            if (bLabOnlyRemaining)
            {
                if (UEMRTreatmentSubsystem* TreatmentSubsystem = GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>())
                {
                    if (TreatmentSubsystem->HasAvailableBed())
                    {
                        if (TreatmentSubsystem->TryAssignPatientToBedForExam(Patient))
                        {
                            ReleaseExamWaitingSeat(Patient, EMRTags::Machine::LabAnalyzer);
                        }
                        else
                        {
                            AssignExamWaitingSeat(Patient, EMRTags::Machine::LabAnalyzer);
                        }
                    }
                    else
                    {
                        AssignExamWaitingSeat(Patient, EMRTags::Machine::LabAnalyzer);
                    }
                }
            }

            ChangePatientPhaseTag(Patient, EMRTags::Patient::Phase::WaitingExam);
            DispatchNextQueuedExamIntent(Patient, ExamType, bLabOnlyRemaining);
        }
    }

    OnExamCompleted.Broadcast(ExamType);
    ExamCompletedNativeDelegate.Broadcast(ExamType);

    if (ExamType.MatchesTagExact(EMRTags::Abilities::Exam::XRay))
    {
        UE_LOG(LogTemp, Log, TEXT("[XRayFlow] CompleteExamForPatient end patient=%s remainingQueues=%s"),
            *GetNameSafe(Patient), HasAnyQueueTags(Patient) ? TEXT("true") : TEXT("false"));
    }
}

FGameplayTagContainer UEMRExamQueueSubsystem::GetMissingRequiredExamsForPatient(AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return FGameplayTagContainer::EmptyContainer;
    }

    FGameplayTagContainer RequiredExamTags = GetRequiredExamTagsForPatient(Patient);
    if (RequiredExamTags.IsEmpty())
    {
        return RequiredExamTags;
    }

    const FGameplayTagContainer CompletedTags = GetCompletedExamTagsOnPatient(Patient);
    for (const FGameplayTag& CompletedTag : CompletedTags)
    {
        const FGameplayTag ExamTag = ConvertCompletionTagToExamTag(CompletedTag);
        if (ExamTag.IsValid())
        {
            RequiredExamTags.RemoveTag(ExamTag);
        }
    }

    return RequiredExamTags;
}


void UEMRExamQueueSubsystem::RemovePatientFromAllQueues(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }
	
    for (auto& Pair : ExamQueueMap)
    {
        FGameplayTag ExamTag = Pair.Key;
        TArray<AEMRPatient*>& Queue = Pair.Value;

        if (Queue.Remove(Patient) > 0)
        {
            RemoveQueueTagFromPatient(Patient, ExamTag);
            ReleaseExamWaitingSeat(Patient, GetMachineTagForExam(ExamTag));
        }
    }

    Patient->SetNextMachineOnList(nullptr);
}


TArray<AEMRPatient*> UEMRExamQueueSubsystem::GetAllPatientsInExamQueue(FGameplayTag ExamTag) const
{
    const TArray<AEMRPatient*>* Queue = ExamQueueMap.Find(ExamTag);
    return Queue ? *Queue : TArray<AEMRPatient*>();
}


void UEMRExamQueueSubsystem::ApplyQueueTagToPatient(AEMRPatient* Patient, FGameplayTag ExamTag)
{
	if (!Patient || !ExamTag.IsValid())
	{
		return;
	}

	ApplyQueueTagEffect(Patient, ExamTag);
}


void UEMRExamQueueSubsystem::RemoveQueueTagFromPatient(AEMRPatient* Patient, FGameplayTag ExamTag)
{
	if (!Patient || !ExamTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FEMRExamQueueEffectHandles* PatientEffects = PatientQueueEffects.Find(Patient);
	if (PatientEffects)
	{
		FActiveGameplayEffectHandle* Handle = PatientEffects->EffectHandles.Find(ExamTag);
		if (Handle && Handle->IsValid())
		{
			ASC->RemoveActiveGameplayEffect(*Handle);
			PatientEffects->EffectHandles.Remove(ExamTag);

			if (PatientEffects->EffectHandles.Num() == 0)
			{
				PatientQueueEffects.Remove(Patient);
			}
		}
	}
}


void UEMRExamQueueSubsystem::RemoveExamCompletedTagsFromPatient(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
	}

	UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const FGameplayTagContainer CompletedTags = GetCompletedExamTagsOnPatient(Patient);
	for (const FGameplayTag& CompletedTag : CompletedTags)
	{
		FEMRExamCompletionEffectHandles* PatientEffects = PatientCompletedExamEffects.Find(Patient);
		if (PatientEffects)
		{
			FActiveGameplayEffectHandle* Handle = PatientEffects->EffectHandles.Find(CompletedTag);
			if (Handle && Handle->IsValid())
			{
				ASC->RemoveActiveGameplayEffect(*Handle);
				PatientEffects->EffectHandles.Remove(CompletedTag);

				if (PatientEffects->EffectHandles.Num() == 0)
				{
					PatientCompletedExamEffects.Remove(Patient);
				}
			}
		}
    }
}


const FEMRExamQueueMapping* UEMRExamQueueSubsystem::GetExamQueueMapping(const FGameplayTag& ExamTag) const
{
    if (!ExamTag.IsValid())
    {
        return nullptr;
    }

    if (CachedExamQueueMappings.Num() == 0)
    {
        RebuildExamQueueMappingsCache();
    }

    for (const FEMRExamQueueMapping& Mapping : CachedExamQueueMappings)
    {
        if (!Mapping.ExamTag.IsValid())
        {
            continue;
        }

        const bool bMatches = Mapping.bMatchByHierarchy ? ExamTag.MatchesTag(Mapping.ExamTag) : ExamTag.MatchesTagExact(Mapping.ExamTag);
        if (bMatches)
        {
            return &Mapping;
        }
    }

    return nullptr;
}


FGameplayTag UEMRExamQueueSubsystem::GetQueueTagForExam(FGameplayTag ExamTag) const
{
    if (!ExamTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] Invalid ExamTag"));
        return FGameplayTag::EmptyTag;
    }

    const FEMRExamQueueMapping* Mapping = GetExamQueueMapping(ExamTag);
    if (Mapping && Mapping->QueueTag.IsValid())
    {
        return Mapping->QueueTag;
    }

    UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] No queue tag found for exam: %s"), *ExamTag.ToString());
    return FGameplayTag::EmptyTag;
}

FGameplayTag UEMRExamQueueSubsystem::ResolveQueuedExamTagForPatient(
    AEMRPatient* Patient,
    const FGameplayTag& ExamType,
    const FGameplayTag& QueueTag,
    const FGameplayTag& PreferredExamTag,
    const FGameplayTag& MatchRootTag)
{
    if (!Patient || !QueueTag.IsValid())
    {
        return FGameplayTag::EmptyTag;
    }

    if (PreferredExamTag.IsValid())
    {
        if (TArray<AEMRPatient*>* PreferredQueue = ExamQueueMap.Find(PreferredExamTag))
        {
            if (PreferredQueue->Contains(Patient))
            {
                return PreferredExamTag;
            }
        }
    }

    if (const FEMRExamQueueEffectHandles* PatientEffects = PatientQueueEffects.Find(Patient))
    {
        for (const auto& EffectPair : PatientEffects->EffectHandles)
        {
            const FGameplayTag EffectExamTag = EffectPair.Key;
            if (!EffectExamTag.IsValid())
            {
                continue;
            }

            if (MatchRootTag.IsValid() && !EffectExamTag.MatchesTag(MatchRootTag))
            {
                continue;
            }

            if (GetQueueTagForExam(EffectExamTag) == QueueTag)
            {
                return EffectExamTag;
            }
        }
    }

    for (const auto& Pair : ExamQueueMap)
    {
        if (!Pair.Key.IsValid() || !Pair.Value.Contains(Patient))
        {
            continue;
        }

        if (MatchRootTag.IsValid() && !Pair.Key.MatchesTag(MatchRootTag))
        {
            continue;
        }

        if (GetQueueTagForExam(Pair.Key) == QueueTag)
        {
            return Pair.Key;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] Could not resolve queue tag for exam: %s"), *ExamType.ToString());
    return FGameplayTag::EmptyTag;
}


FGameplayTag UEMRExamQueueSubsystem::GetMachineTagForExam(FGameplayTag ExamTag) const
{
    const FEMRExamQueueMapping* Mapping = GetExamQueueMapping(ExamTag);
    if (Mapping)
    {
        return Mapping->MachineTypeTag;
    }

    return FGameplayTag::EmptyTag;
}

void UEMRExamQueueSubsystem::AssignExamWaitingSeat(AEMRPatient* Patient, const FGameplayTag& MachineTag)
{
    if (!Patient || !MachineTag.IsValid())
    {
        return;
    }

    if (UEMRExamWaitingManagerComponent* Manager = FindExamWaitingManager(MachineTag))
    {
        UEMRWaitingSeatComponent* AssignedSeat = nullptr;
        const bool bSeatAssigned = Manager->RequestSeatForPatient(Patient, AssignedSeat);
        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] AssignExamWaitingSeat: patient=%s machineTag=%s manager=%s assigned=%s seat=%s"),
            *GetNameSafe(Patient),
            *MachineTag.ToString(),
            *GetNameSafe(Manager),
            bSeatAssigned ? TEXT("true") : TEXT("false"),
            *GetNameSafe(AssignedSeat));

        if (!bSeatAssigned)
        {
            UEMRWaitingSeatComponent* OverflowSeat = nullptr;
            FTransform OverflowTransform = FTransform::Identity;
            const bool bHasOverflowTarget = Manager->GetOverflowWaitTarget(Patient, OverflowSeat, OverflowTransform);

            UE_LOG(
                LogExamRoutingDebug,
                Warning,
                TEXT("[ExamRoutingDebug] AssignExamWaitingSeat overflow fallback: patient=%s machineTag=%s hasTarget=%s seat=%s"),
                *GetNameSafe(Patient),
                *MachineTag.ToString(),
                bHasOverflowTarget ? TEXT("true") : TEXT("false"),
                *GetNameSafe(OverflowSeat));

            if (bHasOverflowTarget)
            {
                if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
                {
                    const FGameplayTag NavigationTag = NavigationSubsystem->GetNavigationMessageTagForMachine(MachineTag);
                    AActor* OverflowTargetActor = OverflowSeat ? OverflowSeat->GetOwner() : nullptr;

                    NavigationSubsystem->BroadcastNavigationIntent(
                        Patient,
                        NavigationTag,
                        OverflowTargetActor,
                        OverflowTransform.GetLocation(),
                        OverflowTransform.GetRotation().Rotator(),
                        MachineTag);

                    UE_LOG(
                        LogExamRoutingDebug,
                        Warning,
                        TEXT("[ExamRoutingDebug] AssignExamWaitingSeat overflow navigation: patient=%s machineTag=%s navTag=%s targetActor=%s location=%s"),
                        *GetNameSafe(Patient),
                        *MachineTag.ToString(),
                        *NavigationTag.ToString(),
                        *GetNameSafe(OverflowTargetActor),
                        *OverflowTransform.GetLocation().ToCompactString());
                }
            }
        }
    }
    else
    {
        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] AssignExamWaitingSeat: no waiting manager found for patient=%s machineTag=%s"),
            *GetNameSafe(Patient),
            *MachineTag.ToString());
    }
}

void UEMRExamQueueSubsystem::ReleaseExamWaitingSeat(AEMRPatient* Patient, const FGameplayTag& MachineTag)
{
    if (!Patient || !MachineTag.IsValid())
    {
        return;
    }

    if (UEMRExamWaitingManagerComponent* Manager = FindExamWaitingManager(MachineTag))
    {
        Manager->ReleaseSeatForPatient(Patient);
        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] ReleaseExamWaitingSeat: patient=%s machineTag=%s manager=%s"),
            *GetNameSafe(Patient),
            *MachineTag.ToString(),
            *GetNameSafe(Manager));
    }
    else
    {
        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] ReleaseExamWaitingSeat: no waiting manager found for patient=%s machineTag=%s"),
            *GetNameSafe(Patient),
            *MachineTag.ToString());
    }
}

bool UEMRExamQueueSubsystem::AreAllExamWaitingSeatsFull() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogExamRoutingDebug, Warning, TEXT("[ExamRoutingDebug] AreAllExamWaitingSeatsFull: no world."));
        return false;
    }

    bool bFoundAnyExamWaitingManager = false;

    for (TActorIterator<AEMRExamWaitingArea> It(World); It; ++It)
    {
        const AEMRExamWaitingArea* WaitingArea = *It;
        if (!WaitingArea)
        {
            continue;
        }

        const UEMRExamWaitingManagerComponent* Manager = WaitingArea->GetManagerComponent();
        if (!Manager)
        {
            continue;
        }

        bFoundAnyExamWaitingManager = true;
        if (Manager->HasFreeSeat())
        {
            UE_LOG(
                LogExamRoutingDebug,
                Warning,
                TEXT("[ExamRoutingDebug] AreAllExamWaitingSeatsFull: found free seat in manager=%s machineTag=%s"),
                *GetNameSafe(Manager),
                *Manager->GetMachineTypeTag().ToString());
            return false;
        }
    }

    if (!bFoundAnyExamWaitingManager)
    {
        UE_LOG(LogExamRoutingDebug, Warning, TEXT("[ExamRoutingDebug] AreAllExamWaitingSeatsFull: no exam waiting manager found."));
        return false;
    }

    UE_LOG(LogExamRoutingDebug, Warning, TEXT("[ExamRoutingDebug] AreAllExamWaitingSeatsFull: all seats are occupied."));
    return true;
}

bool UEMRExamQueueSubsystem::TryAssignLabAnalyzerWaitingPatientToBed()
{
    UEMRTreatmentSubsystem* TreatmentSubsystem = GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>();
    if (!TreatmentSubsystem || !TreatmentSubsystem->HasAvailableBed())
    {
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TryAssignLabAnalyzerWaitingPatientToBed skipped. HasTreatmentSubsystem=%s HasAvailableBed=%s"),
            TreatmentSubsystem ? TEXT("true") : TEXT("false"),
            (TreatmentSubsystem && TreatmentSubsystem->HasAvailableBed()) ? TEXT("true") : TEXT("false"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TryAssignLabAnalyzerWaitingPatientToBed start."));

    AEMRPatient* Candidate = nullptr;
    for (const TPair<FGameplayTag, TArray<AEMRPatient*>>& Pair : ExamQueueMap)
    {
        if (!Pair.Key.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root))
        {
            continue;
        }

        for (AEMRPatient* Patient : Pair.Value)
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

            Candidate = Patient;
            break;
        }

        if (Candidate)
        {
            break;
        }
    }

    if (!Candidate)
    {
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TryAssignLabAnalyzerWaitingPatientToBed no candidate."));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TryAssignLabAnalyzerWaitingPatientToBed candidate=%s"), *GetNameSafe(Candidate));

    if (TreatmentSubsystem->TryAssignPatientToBedForExam(Candidate))
    {
        ReleaseExamWaitingSeat(Candidate, EMRTags::Machine::LabAnalyzer);
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TryAssignLabAnalyzerWaitingPatientToBed assigned and released seat. Patient=%s"),
            *GetNameSafe(Candidate));
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] TryAssignLabAnalyzerWaitingPatientToBed failed to assign. Patient=%s"),
        *GetNameSafe(Candidate));
    return false;
}

UEMRExamWaitingManagerComponent* UEMRExamQueueSubsystem::FindExamWaitingManager(const FGameplayTag& MachineTag) const
{
    if (!MachineTag.IsValid())
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AEMRExamWaitingArea> It(World); It; ++It)
    {
        if (AEMRExamWaitingArea* WaitingArea = *It)
        {
            if (UEMRExamWaitingManagerComponent* Manager = WaitingArea->GetManagerComponent())
            {
                const FGameplayTag AreaTag = Manager->GetMachineTypeTag();
                if (!AreaTag.IsValid() || MachineTag.MatchesTag(AreaTag) || AreaTag.MatchesTag(MachineTag))
                {
                    return Manager;
                }
            }
        }
    }

    return nullptr;
}

bool UEMRExamQueueSubsystem::ReleaseReceptionSeatIfPresent(AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
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


void UEMRExamQueueSubsystem::ApplyPhaseEffect(AEMRPatient* Patient, FGameplayTag PhaseTag)
{
	if (!Patient || !PhaseTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const UEMRPatientData* PatientData = Patient->GetPatientData();
	if (!PatientData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QueueSubsystem] Patient has no PatientData"));
		return;
	}

	TSubclassOf<UGameplayEffect> PhaseEffect = PatientData->GetAddTagToPatientEffect();
	if (!PhaseEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QueueSubsystem] No phase effect found for tag: %s"), 
			   *PhaseTag.ToString());
		return;
	}

	// Apply effect
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(PhaseEffect,1.0f,EffectContext);

	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->DynamicGrantedTags.AddTag(PhaseTag);
		FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		PatientPhaseEffects.Add(Patient, ActiveHandle);
	}
}


void UEMRExamQueueSubsystem::ApplyQueueTagEffect(AEMRPatient* Patient, FGameplayTag ExamTag)
{
	if (!Patient || !ExamTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
	const UEMRPatientData* PatientData = Patient->GetPatientData();

	if (!ASC || !PatientData)
	{
		return;
	}

	FGameplayTag QueueTag = GetQueueTagForExam(ExamTag);
	if (!QueueTag.IsValid())
	{
		return;
	}

	TSubclassOf<UGameplayEffect> QueueEffect = PatientData->GetAddTagToPatientEffect();
	if (!QueueEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QueueSubsystem] No queue effect for: %s"),
			   *QueueTag.ToString());
		return;
	}

	// Apply effect
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(QueueEffect,1.0f,EffectContext);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->DynamicGrantedTags.AddTag(QueueTag);
		FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

		// Store handle for later removal
		StoreQueueEffectHandle(Patient, ExamTag, ActiveHandle);
	}
}


void UEMRExamQueueSubsystem::StoreQueueEffectHandle(AEMRPatient* Patient, FGameplayTag ExamTag, FActiveGameplayEffectHandle Handle)
{
    if (!Patient || !ExamTag.IsValid() || !Handle.IsValid())
    {
        return;
    }

    FEMRExamQueueEffectHandles& PatientEffects = PatientQueueEffects.FindOrAdd(Patient);
    PatientEffects.EffectHandles.Add(ExamTag, Handle);
}


void UEMRExamQueueSubsystem::NotifyMachinesOfType(FGameplayTag ExamTag)
{
    if (!ExamTag.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[ExamQueueSubsystem::NotifyMachines] Invalid ExamTag"));
        return;
    }

    if (!GetMachineTagForExam(ExamTag).IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem::NotifyMachines] No machine mapping found for exam tag %s"), *ExamTag.ToString());
    }

    OnPatientAddedToExamQueue.Broadcast(ExamTag);
}


bool UEMRExamQueueSubsystem::HasAnyQueueTags(AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return false;
    }

	UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

    return ASC->HasMatchingGameplayTag(EMRTags::Patient::ExamQueue::Root);
}

TArray<FGameplayTag> UEMRExamQueueSubsystem::GetQueuedExamTagsForPatient(AEMRPatient* Patient) const
{
    TArray<FGameplayTag> QueuedExamTags;
    if (!Patient)
    {
        return QueuedExamTags;
    }

    for (const TPair<FGameplayTag, TArray<AEMRPatient*>>& Pair : ExamQueueMap)
    {
        if (Pair.Key.IsValid() && Pair.Value.Contains(Patient))
        {
            QueuedExamTags.AddUnique(Pair.Key);
        }
    }

    return QueuedExamTags;
}

bool UEMRExamQueueSubsystem::IsLabAnalyzerExamTag(const FGameplayTag& ExamTag) const
{
    return ExamTag.IsValid() && ExamTag.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root);
}

FGameplayTag UEMRExamQueueSubsystem::ResolveDeterministicNextMachineQueuedExam(AEMRPatient* Patient) const
{
    TArray<FGameplayTag> QueuedExamTags = GetQueuedExamTagsForPatient(Patient);
    if (QueuedExamTags.IsEmpty())
    {
        return FGameplayTag::EmptyTag;
    }

    if (CachedExamQueueMappings.Num() == 0)
    {
        RebuildExamQueueMappingsCache();
    }

    for (const FEMRExamQueueMapping& Mapping : CachedExamQueueMappings)
    {
        if (!Mapping.ExamTag.IsValid() || IsLabAnalyzerExamTag(Mapping.ExamTag))
        {
            continue;
        }

        for (const FGameplayTag& QueuedExamTag : QueuedExamTags)
        {
            if (IsLabAnalyzerExamTag(QueuedExamTag))
            {
                continue;
            }

            const bool bMatches = Mapping.bMatchByHierarchy
                ? QueuedExamTag.MatchesTag(Mapping.ExamTag)
                : QueuedExamTag.MatchesTagExact(Mapping.ExamTag);
            if (bMatches)
            {
                return QueuedExamTag;
            }
        }
    }

    QueuedExamTags.RemoveAll([this](const FGameplayTag& QueuedTag)
    {
        return IsLabAnalyzerExamTag(QueuedTag);
    });

    QueuedExamTags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
    {
        return A.ToString() < B.ToString();
    });

    return QueuedExamTags.IsEmpty() ? FGameplayTag::EmptyTag : QueuedExamTags[0];
}

void UEMRExamQueueSubsystem::ClearStaleMachineAssignmentsForPatient(AEMRPatient* Patient, const FGameplayTag& PreserveMachineTag) const
{
    if (!Patient)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AEMRBaseMachine> It(World); It; ++It)
    {
        AEMRBaseMachine* Machine = *It;
        if (!Machine || Machine->GetAssignedPatient() != Patient)
        {
            continue;
        }

        if (PreserveMachineTag.IsValid() && Machine->GetMachineTypeTag().MatchesTag(PreserveMachineTag))
        {
            continue;
        }

        Machine->ClearOccupiedPatient(Patient);
    }
}

bool UEMRExamQueueSubsystem::IsMachineTypeCurrentlyAvailable(const FGameplayTag& MachineTypeTag, const AEMRPatient* Patient) const
{
    if (!MachineTypeTag.IsValid())
    {
        return false;
    }

    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    for (TActorIterator<AEMRBaseMachine> It(World); It; ++It)
    {
        const AEMRBaseMachine* Machine = *It;
        if (!Machine || !Machine->GetMachineTypeTag().MatchesTag(MachineTypeTag))
        {
            continue;
        }

        if (!Machine->IsOccupied() || (Patient && Machine->GetAssignedPatient() == Patient))
        {
            return true;
        }
    }

    return false;
}

AEMRBaseMachine* UEMRExamQueueSubsystem::FindMachineAssignedToPatient(const FGameplayTag& MachineTypeTag, const AEMRPatient* Patient) const
{
    if (!MachineTypeTag.IsValid() || !Patient)
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AEMRBaseMachine> It(World); It; ++It)
    {
        AEMRBaseMachine* Machine = *It;
        if (!Machine || !Machine->GetMachineTypeTag().MatchesTag(MachineTypeTag))
        {
            continue;
        }

        if (Machine->GetAssignedPatient() == Patient)
        {
            return Machine;
        }
    }

    return nullptr;
}

void UEMRExamQueueSubsystem::DispatchNextQueuedExamIntent(AEMRPatient* Patient, const FGameplayTag& CompletedExamTag, bool bLabOnlyRemaining)
{
    (void)CompletedExamTag;

    if (!Patient || bLabOnlyRemaining)
    {
        if (Patient)
        {
            Patient->SetNextMachineOnList(nullptr);
        }

        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent skipped: patient=%s completedExam=%s labOnlyRemaining=%s"),
            *GetNameSafe(Patient),
            *CompletedExamTag.ToString(),
            bLabOnlyRemaining ? TEXT("true") : TEXT("false"));
        return;
    }

    FGameplayTag NextExamTag = ResolveDeterministicNextMachineQueuedExam(Patient);
    UE_LOG(
        LogExamRoutingDebug,
        Warning,
        TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent start: patient=%s completedExam=%s deterministicNextExam=%s"),
        *GetNameSafe(Patient),
        *CompletedExamTag.ToString(),
        *NextExamTag.ToString());

    {
        const TArray<FGameplayTag> QueuedExamTags = GetQueuedExamTagsForPatient(Patient);
        const UWorld* World = GetWorld();
        float BestDistSq = TNumericLimits<float>::Max();
        FGameplayTag BestExamTag;

        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent queued exams: patient=%s queued=[%s]"),
            *GetNameSafe(Patient),
            *JoinTagsForLog(QueuedExamTags));

        for (const FGameplayTag& QueuedExamTag : QueuedExamTags)
        {
            if (!QueuedExamTag.IsValid() || IsLabAnalyzerExamTag(QueuedExamTag))
            {
                continue;
            }

            const FGameplayTag CandidateMachineTag = GetMachineTagForExam(QueuedExamTag);
            if (!CandidateMachineTag.IsValid())
            {
                continue;
            }

            AEMRBaseMachine* CandidateMachine = FindClosestAvailableMachineForPatient(World, Patient, CandidateMachineTag, Patient);
            if (!CandidateMachine)
            {
                UE_LOG(
                    LogExamRoutingDebug,
                    Warning,
                    TEXT("[ExamRoutingDebug] No available machine for candidate exam: patient=%s exam=%s machineTag=%s"),
                    *GetNameSafe(Patient),
                    *QueuedExamTag.ToString(),
                    *CandidateMachineTag.ToString());
                continue;
            }

            const float DistSq = FVector::DistSquared(Patient->GetActorLocation(), CandidateMachine->GetActorLocation());
            UE_LOG(
                LogExamRoutingDebug,
                Warning,
                TEXT("[ExamRoutingDebug] Candidate exam distance: patient=%s exam=%s machine=%s machineTag=%s distSq=%.1f"),
                *GetNameSafe(Patient),
                *QueuedExamTag.ToString(),
                *GetNameSafe(CandidateMachine),
                *CandidateMachineTag.ToString(),
                DistSq);
            if (DistSq < BestDistSq)
            {
                BestDistSq = DistSq;
                BestExamTag = QueuedExamTag;
            }
        }

        if (BestExamTag.IsValid())
        {
            NextExamTag = BestExamTag;
            UE_LOG(
                LogExamRoutingDebug,
                Warning,
                TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent nearest-available override: patient=%s selectedExam=%s distSq=%.1f"),
                *GetNameSafe(Patient),
                *NextExamTag.ToString(),
                BestDistSq);
        }
        else
        {
            UE_LOG(
                LogExamRoutingDebug,
                Warning,
                TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent no available non-lab exam candidate, keeping deterministic exam=%s"),
                *NextExamTag.ToString());
        }
    }

    if (!NextExamTag.IsValid())
    {
        Patient->SetNextMachineOnList(nullptr);
        return;
    }

    const FGameplayTag NextMachineTag = GetMachineTagForExam(NextExamTag);
    if (!NextMachineTag.IsValid())
    {
        Patient->SetNextMachineOnList(nullptr);
        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent abort: invalid machine tag for exam=%s patient=%s"),
            *NextExamTag.ToString(),
            *GetNameSafe(Patient));
        return;
    }

    ClearStaleMachineAssignmentsForPatient(Patient, NextMachineTag);

    const bool bMachineAvailable = IsMachineTypeCurrentlyAvailable(NextMachineTag, Patient);
    AEMRBaseMachine* PotentialNextMachine = FindMachineAssignedToPatient(NextMachineTag, Patient);
    if (!PotentialNextMachine && bMachineAvailable)
    {
        PotentialNextMachine = FindClosestAvailableMachineForPatient(GetWorld(), Patient, NextMachineTag, Patient);
    }
    Patient->SetNextMachineOnList(PotentialNextMachine);

    if (!bMachineAvailable)
    {
        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent machine unavailable -> waiting seat: patient=%s exam=%s machineTag=%s"),
            *GetNameSafe(Patient),
            *NextExamTag.ToString(),
            *NextMachineTag.ToString());
        AssignExamWaitingSeat(Patient, NextMachineTag);
    }
    else
    {
        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent machine available: patient=%s exam=%s machineTag=%s"),
            *GetNameSafe(Patient),
            *NextExamTag.ToString(),
            *NextMachineTag.ToString());
    }

    if (AEMRBaseMachine* AssignedMachine = FindMachineAssignedToPatient(NextMachineTag, Patient))
    {
        if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
        {
            ReleaseExamWaitingSeat(Patient, NextMachineTag);
            const FGameplayTag NavigationTag = NavigationSubsystem->GetNavigationMessageTagForMachine(NextMachineTag);
            NavigationSubsystem->BroadcastNavigationIntent(
                Patient,
                NavigationTag,
                AssignedMachine,
                AssignedMachine->GetPatientWaitPointLocation(),
                AssignedMachine->GetPatientWaitPointRotation(),
                NextMachineTag);

            UE_LOG(
                LogExamRoutingDebug,
                Warning,
                TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent navigation broadcast: patient=%s exam=%s machine=%s machineTag=%s navTag=%s"),
                *GetNameSafe(Patient),
                *NextExamTag.ToString(),
                *GetNameSafe(AssignedMachine),
                *NextMachineTag.ToString(),
                *NavigationTag.ToString());
        }
    }
    else
    {
        UE_LOG(
            LogExamRoutingDebug,
            Warning,
            TEXT("[ExamRoutingDebug] DispatchNextQueuedExamIntent no assigned machine yet, notifying machines: patient=%s exam=%s machineTag=%s"),
            *GetNameSafe(Patient),
            *NextExamTag.ToString(),
            *NextMachineTag.ToString());
        NotifyMachinesOfType(NextExamTag);
    }
}


bool UEMRExamQueueSubsystem::DoesPatientSatisfyRequiredExams(AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return false;
    }

    FGameplayTagContainer RequiredTags = GetRequiredExamCompletionTagsForPatient(Patient);
    if (RequiredTags.IsEmpty())
    {
        return true;
    }

    if (const UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent())
    {
        return ASC->HasAllMatchingGameplayTags(RequiredTags);
    }

    return false;
}


void UEMRExamQueueSubsystem::SendPatientBackToReception(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    ChangePatientPhaseTag(Patient, EMRTags::Patient::Phase::WaitingInReception);

    ApplyReturnToReceptionTag(Patient);
}


void UEMRExamQueueSubsystem::ApplyReturnToReceptionTag(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
    const UEMRPatientData* PatientData = Patient->GetPatientData();

    if (!ASC || !PatientData)
    {
        return;
    }

    if (const FActiveGameplayEffectHandle* Handle = PatientReturnToReceptionEffects.Find(Patient))
    {
        if (Handle->IsValid())
        {
            return;
        }
    }

    TSubclassOf<UGameplayEffect> TagEffect = PatientData->GetAddTagToPatientEffect();
    if (!TagEffect)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] AddTagsToPatientEffect missing on PatientData"));
        return;
    }

    FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
    EffectContext.AddSourceObject(this);

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(TagEffect, 1.0f, EffectContext);

    if (SpecHandle.IsValid())
    {
        SpecHandle.Data->DynamicGrantedTags.AddTag(EMRTags::Patient::Phase::WaitingInReception);
        FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        PatientReturnToReceptionEffects.Add(Patient, ActiveHandle);

        UEMRWaitingSeatComponent* AssignedSeat = nullptr;
        if (UEMRWaitingRoomManagerComponent* ReceptionManager = FindReceptionWaitingManager(GetWorld()))
        {
            ReceptionManager->RemovePatientFromQueue(Patient);
            ReceptionManager->RequestSeatForPatient(Patient, AssignedSeat);
        }

        if (AssignedSeat)
        {
            FGameplayEventData EventData;
            EventData.EventTag = EMRTags::Abilities::MoveToSeat;
            EventData.Target = Patient;
            EventData.OptionalObject = AssignedSeat;
            ASC->HandleGameplayEvent(EventData.EventTag, &EventData);
        }
        else
        {
            SendPatientToReceptionMachine(this, Patient);
        }

        OnPatientSentBackToReception.Broadcast(Patient);
    }
}


void UEMRExamQueueSubsystem::RemoveReturnToReceptionTag(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    FActiveGameplayEffectHandle* Handle = PatientReturnToReceptionEffects.Find(Patient);
    if (!Handle)
    {
        return;
    }

    if (Handle->IsValid())
    {
        if (UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent())
        {
            ASC->RemoveActiveGameplayEffect(*Handle);
        }
    }

    PatientReturnToReceptionEffects.Remove(Patient);
}


void UEMRExamQueueSubsystem::TransitionPatientToTreatment(AEMRPatient* Patient) 
{
    if (!Patient)
    {
        return;
    }

    RemoveExamCompletedTagsFromPatient(Patient);

    if (const UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent())
    {
        if (ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving))
        {
            UE_LOG(LogTemp, Verbose, TEXT("[ExamQueueSubsystem] Skipping treatment transition for leaving patient %s"), *GetNameSafe(Patient));
            return;
        }
    }


    UEMRTreatmentSubsystem* TreatmentSubsystem = GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>();
    if (TreatmentSubsystem)
    {
        if (TreatmentSubsystem->IsPatientAssignedToBed(Patient))
        {
            TreatmentSubsystem->PromoteExamWaitingPatientToTreatment(Patient);
        }
        else
        {
            TreatmentSubsystem->AddPatientToTreatmentQueue(Patient);
            Patient->SetCanReceiveTreatment(true);
        }
	
    	
    	UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
    	if (!ASC)
    	{
    		return;
    	}
    	
    	FActiveGameplayEffectHandle* PhaseHandle = PatientPhaseEffects.Find(Patient);
    	if (PhaseHandle && PhaseHandle->IsValid())
    	{
    		ASC->RemoveActiveGameplayEffect(*PhaseHandle);
    		PatientPhaseEffects.Remove(Patient);
    	}
    	
    	RemoveReturnToReceptionTag(Patient);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ExamQueueSubsystem] TreatmentSubsystem not found!"));
    }
}


void UEMRExamQueueSubsystem::ApplyCompletedExamTag(AEMRPatient* Patient, const FGameplayTag& ExamTag)
{
    if (!Patient || !ExamTag.IsValid())
    {
        return;
    }

    const FGameplayTag CompletedTag = ConvertExamTagToCompletionTag(ExamTag);
    if (!CompletedTag.IsValid())
    {
        return;
    }

    UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
    const UEMRPatientData* PatientData = Patient->GetPatientData();
    if (!ASC || !PatientData)
    {
        return;
    }

    if (ASC->HasMatchingGameplayTag(CompletedTag))
    {
        return;
    }

    TSubclassOf<UGameplayEffect> TagEffect = PatientData->GetAddTagToPatientEffect();
    if (!TagEffect)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] AddTagsToPatientEffect missing on PatientData"));
        return;
    }

    FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
    EffectContext.AddSourceObject(this);

    FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(TagEffect, 1.0f, EffectContext);
    if (SpecHandle.IsValid())
    {
        SpecHandle.Data->DynamicGrantedTags.AddTag(CompletedTag);
        FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        StoreCompletedExamEffectHandle(Patient, CompletedTag, ActiveHandle);
    }
}


void UEMRExamQueueSubsystem::StoreCompletedExamEffectHandle(AEMRPatient* Patient, FGameplayTag CompletedTag, FActiveGameplayEffectHandle Handle)
{
    if (!Patient || !CompletedTag.IsValid() || !Handle.IsValid())
    {
        return;
    }

    FEMRExamCompletionEffectHandles& PatientEffects = PatientCompletedExamEffects.FindOrAdd(Patient);
    PatientEffects.EffectHandles.Add(CompletedTag, Handle);
}


FGameplayTagContainer UEMRExamQueueSubsystem::GetCompletedExamTagsOnPatient(AEMRPatient* Patient) const
{
    FGameplayTagContainer Result;
    if (!Patient)
    {
        return Result;
    }

    if (const UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent())
    {
        FGameplayTagContainer OwnedTags;
        ASC->GetOwnedGameplayTags(OwnedTags);
        for (const FGameplayTag& Tag : OwnedTags)
        {
            if (Tag.MatchesTag(EMRTags::Patient::ExamCompleted::Root))
            {
                Result.AddTag(Tag);
            }
        }
    }

    return Result;
}


FGameplayTagContainer UEMRExamQueueSubsystem::GetRequiredExamCompletionTagsForPatient(AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return FGameplayTagContainer::EmptyContainer;
    }

    if (CachedRequiredCompletionsByPathology.Num() == 0)
    {
        BuildExamRequirementMappingsCache();
    }

    const UEMRPatientData* PatientData = Patient->GetPatientData();
    if (!PatientData)
    {
        return FGameplayTagContainer::EmptyContainer;
    }

    const FGameplayTagContainer Pathologies = PatientData->GetPathologies();
	
	FGameplayTagContainer RequiredTags;
	for (const FGameplayTag& Pathology : Pathologies)
	{
		if (const FGameplayTagContainer* CompletionTags = CachedRequiredCompletionsByPathology.Find(Pathology))
		{
			RequiredTags.AppendTags(*CompletionTags);
		}
	}
	return RequiredTags;
}


FGameplayTagContainer UEMRExamQueueSubsystem::GetRequiredExamTagsForPatient(AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return FGameplayTagContainer::EmptyContainer;
    }

    if (CachedRequiredExamsByPathology.Num() == 0)
    {
        BuildExamRequirementMappingsCache();
    }

	const UEMRPatientData* PatientData = Patient->GetPatientData();
	if (!PatientData)
	{
	    return FGameplayTagContainer::EmptyContainer;
	}

    const FGameplayTagContainer Pathologies = PatientData->GetPathologies();
    FGameplayTagContainer RequiredTags;

    for (const FGameplayTag& Pathology : Pathologies)
    {
        if (const FGameplayTagContainer* ExamTags = CachedRequiredExamsByPathology.Find(Pathology))
        {
            RequiredTags.AppendTags(*ExamTags);
        }
    }
    return RequiredTags;
}


FGameplayTag UEMRExamQueueSubsystem::ConvertExamTagToCompletionTag(const FGameplayTag& ExamTag) const
{
    if (!ExamTag.IsValid())
    {
        return FGameplayTag::EmptyTag;
    }

    if (CachedExamCompletionMappings.Num() == 0)
    {
        BuildExamCompletionMappingsCache();
    }

    for (const FEMRExamMachineCompletionMapping& Mapping : CachedExamCompletionMappings)
    {
        if (!Mapping.ExamTag.IsValid() || !Mapping.CompletionTag.IsValid())
        {
            continue;
        }

        const bool bMatch = Mapping.bMatchExamByHierarchy ? ExamTag.MatchesTag(Mapping.ExamTag) : ExamTag.MatchesTagExact(Mapping.ExamTag);
        if (bMatch)
        {
            return Mapping.CompletionTag;
        }
    }

    return FGameplayTag::EmptyTag;
}


FGameplayTag UEMRExamQueueSubsystem::ConvertCompletionTagToExamTag(const FGameplayTag& CompletedTag) const
{
    if (!CompletedTag.IsValid())
    {
        return FGameplayTag::EmptyTag;
    }

    if (CachedExamCompletionMappings.Num() == 0)
    {
        BuildExamCompletionMappingsCache();
    }

    for (const FEMRExamMachineCompletionMapping& Mapping : CachedExamCompletionMappings)
    {
        if (!Mapping.ExamTag.IsValid() || !Mapping.CompletionTag.IsValid())
        {
            continue;
        }

        const bool bMatch = Mapping.bMatchCompletionByHierarchy ? CompletedTag.MatchesTag(Mapping.CompletionTag) : CompletedTag.MatchesTagExact(Mapping.CompletionTag);
        if (bMatch)
        {
            return Mapping.ExamTag;
        }
    }

    return FGameplayTag::EmptyTag;
}



void UEMRExamQueueSubsystem::RebuildExamQueueMappingsCache() const
{
    CachedExamQueueMappings.Reset();

	TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
    UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);
	
    if (LoadedSubsystemConfig.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] SubsystemConfig not loaded"));
        return;
    }

    UDataTable* MappingTable = LoadedSubsystemConfig[0]->GetExamQueueMappingTable();
    if (!MappingTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] ExamQueueMappingTable is not set"));
        return;
    }

    static const FString Context(TEXT("ExamQueueMappings"));
    TArray<FEMRExamQueueMapping*> Rows;
    MappingTable->GetAllRows(Context, Rows);

    for (const FEMRExamQueueMapping* Row : Rows)
    {
        if (!Row || !Row->ExamTag.IsValid() || !Row->QueueTag.IsValid() || !Row->MachineTypeTag.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] Invalid ExamQueueMapping row skipped (Exam: %s, Machine: %s, Queue: %s)"),
                   Row ? *Row->ExamTag.ToString() : TEXT("None"),
                   Row ? *Row->MachineTypeTag.ToString() : TEXT("None"),
                   Row ? *Row->QueueTag.ToString() : TEXT("None"));
            continue;
        }

        CachedExamQueueMappings.Add(*Row);
    }
}


void UEMRExamQueueSubsystem::BuildExamRequirementMappingsCache() const
{
    CachedRequiredExamsByPathology.Reset();
    CachedRequiredCompletionsByPathology.Reset();

    TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
    UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);

    if (LoadedSubsystemConfig.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] SubsystemConfig not loaded"));
        return;
    }

    UDataTable* MappingTable = LoadedSubsystemConfig[0]->GetExamRequirementsFromPathologyTable();
    if (!MappingTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] Exam requirements table is not set"));
        return;
    }

    static const FString Context(TEXT("ExamRequirementMappings"));
    TArray<FEMRExamRequirementsForPathologyMapping*> Rows;
    MappingTable->GetAllRows(Context, Rows);

    for (const FEMRExamRequirementsForPathologyMapping* Row : Rows)
    {
	    if (!Row || !Row->Pathology.IsValid())
	    {
	    	continue;
	    }

        FGameplayTagContainer& ExamTags = CachedRequiredExamsByPathology.FindOrAdd(Row->Pathology);
        ExamTags.AppendTags(Row->RequiredExams);

        // Conversion et stockage des completion tags
        FGameplayTagContainer& CompletionTags = CachedRequiredCompletionsByPathology.FindOrAdd(Row->Pathology);
        for (const FGameplayTag& ExamTag : Row->RequiredExams)
        {
            const FGameplayTag CompletedTag = ConvertExamTagToCompletionTag(ExamTag);
            if (CompletedTag.IsValid())
            {
                CompletionTags.AddTag(CompletedTag);
            }
        }
    }
}


void UEMRExamQueueSubsystem::BuildExamCompletionMappingsCache() const
{
	CachedExamCompletionMappings.Reset();

	TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
	UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);

	if (LoadedSubsystemConfig.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] SubsystemConfig not loaded"));
		return;
	}

	UDataTable* MappingTable = LoadedSubsystemConfig[0]->GetExamCompletionMappingTable();
	if (!MappingTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ExamQueueSubsystem] Exam completion mapping table is not set"));
		return;
	}

	static const FString Context(TEXT("ExamCompletionMappings"));
	TArray<FEMRExamMachineCompletionMapping*> Rows;
	MappingTable->GetAllRows(Context, Rows);

    for (const FEMRExamMachineCompletionMapping* Row : Rows)
    {
        if (!Row || !Row->ExamTag.IsValid() || !Row->CompletionTag.IsValid())
        {
            continue;
        }

        CachedExamCompletionMappings.Add(*Row);
    }
}


void UEMRExamQueueSubsystem::LoadSubsystemConfig()
{
    UEMRAssetManager::Get().LoadSubsystemConfig(FStreamableDelegate::CreateUObject(this, &ThisClass::OnLoadedSubsystemConfig));
}


void UEMRExamQueueSubsystem::OnLoadedSubsystemConfig()
{
    RebuildExamQueueMappingsCache();
    BuildExamRequirementMappingsCache();
    BuildExamCompletionMappingsCache();
}
