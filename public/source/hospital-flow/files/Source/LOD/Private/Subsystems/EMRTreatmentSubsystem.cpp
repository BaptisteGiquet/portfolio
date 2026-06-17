
#include "Subsystems/EMRTreatmentSubsystem.h"

#include "Interaction/EMRTreatmentBed.h"
#include "Characters/Patient/EMRPatient.h"
#include "Environment/EMRHospitalExit.h"
#include "GAS/EMRTags.h"
#include "GAS/Attributes/EMRPatientAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Framework/EMRAssetManager.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Subsystems/EMRExamQueueSubsystem.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Data/EMREconomySystemGenerics.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Engine/StreamableManager.h"
#include "Interaction/EMRBaseMachine.h"
#include "Subsystems/EMRMagicBoxSubsystem.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Environment/EMRTreatmentWaitingManagerComponent.h"
#include "Environment/EMRTreatmentWaitingArea.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

namespace
{
    constexpr TCHAR TreatmentMagicBoxFlowLogPrefix[] = TEXT("[MagicBoxFlow]");
}


void UEMRTreatmentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

	LoadSubsystemConfig();
}


void UEMRTreatmentSubsystem::Deinitialize()
{
    for (auto& Pair : PatientPhaseEffects)
    {
        if (Pair.Key && Pair.Value.IsValid())
        {
            if (UAbilitySystemComponent* ASC = Pair.Key->GetAbilitySystemComponent())
            {
                ASC->RemoveActiveGameplayEffect(Pair.Value);
            }
        }
    }

    RegisteredBeds.Empty();
    WaitingQueue.Empty();
    BedAssignmentsMap.Empty();
    LockedPatients.Empty();
    PatientPhaseEffects.Empty();
    CachedTreatmentQueueMappings.Empty();

    Super::Deinitialize();
    UE_LOG(LogTemp, Log, TEXT("[TreatmentSubsystem] Deinitialized"));
}


void UEMRTreatmentSubsystem::AddPatientToTreatmentQueue(AEMRPatient* Patient)
{
    if (!Patient)
    {
        UE_LOG(LogTemp, Error, TEXT("[TreatmentSubsystem] Patient is NULL"));
        return;
    }

    if (WaitingQueue.Contains(Patient))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Patient %s already in queue"),
            *Patient->GetName());
        return;
    }

    WaitingQueue.Add(Patient);

    ChangePatientPhaseTag(Patient, EMRTags::Patient::Phase::WaitingTreatment);

    AssignTreatmentWaitingSeat(Patient);
    UE_LOG(LogTemp, Warning, TEXT("%s Treatment AddToQueue patient=%s queueSize=%d"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(Patient), WaitingQueue.Num());

    if (TryAssignPatientToMagicBox(Patient))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Treatment AddToQueue assigned to magic box patient=%s"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s Treatment AddToQueue no magic box assignment patient=%s trying bed"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    TryAssignPatientToBed();
}

void UEMRTreatmentSubsystem::HandlePatientAbandonment(AEMRPatient* Patient)
{
    if (!Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] TreatmentAbandonment ignored. Patient=null"));
        return;
    }

    UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
    const bool bAlreadyLeaving = ASC && ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving);
    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TreatmentAbandonment start. Patient=%s AlreadyLeaving=%s"),
        *GetNameSafe(Patient),
        bAlreadyLeaving ? TEXT("true") : TEXT("false"));

    WaitingQueue.Remove(Patient);

    ReleaseTreatmentWaitingSeat(Patient);

    bool bReleasedFromBed = false;
    for (TPair<AEMRTreatmentBed*, AEMRPatient*>& Pair : BedAssignmentsMap)
    {
        if (Pair.Value == Patient)
        {
            if (Pair.Key)
            {
                UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Releasing bed for patient. Patient=%s Bed=%s"),
                    *GetNameSafe(Patient),
                    *GetNameSafe(Pair.Key));
                Pair.Key->ReleasePatient();
            }

            Pair.Value = nullptr;
            bReleasedFromBed = true;
        }
    }

    if (!bReleasedFromBed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] No bed assignment found in map. Patient=%s"), *GetNameSafe(Patient));

        if (AActor* AttachParent = Patient->GetAttachParentActor())
        {
            if (AEMRTreatmentBed* ParentBed = Cast<AEMRTreatmentBed>(AttachParent))
            {
                UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Releasing bed via attach parent. Patient=%s Bed=%s"),
                    *GetNameSafe(Patient),
                    *GetNameSafe(ParentBed));
                ParentBed->ReleasePatient();
                bReleasedFromBed = true;
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Patient attached to non-bed actor. Patient=%s Parent=%s"),
                    *GetNameSafe(Patient),
                    *GetNameSafe(AttachParent));
            }
        }

        if (UWorld* World = GetWorld())
        {
            for (TActorIterator<AEMRTreatmentBed> It(World); It; ++It)
            {
                AEMRTreatmentBed* Bed = *It;
                if (Bed && Bed->GetCurrentPatient() == Patient)
                {
                    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Releasing bed via world scan. Patient=%s Bed=%s"),
                        *GetNameSafe(Patient),
                        *GetNameSafe(Bed));
                    Bed->ReleasePatient();
                    bReleasedFromBed = true;
                    break;
                }
            }
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (FTimerHandle* TimerHandle = LockTimers.Find(Patient))
        {
            World->GetTimerManager().ClearTimer(*TimerHandle);
            LockTimers.Remove(Patient);
        }
    }

    LockedPatients.Remove(Patient);

    if (ASC && !bAlreadyLeaving)
    {
        if (FEMRTreatmentQueueEffectHandles* PatientEffects = PatientQueueEffects.Find(Patient))
        {
            for (const TPair<FGameplayTag, FActiveGameplayEffectHandle>& EffectPair : PatientEffects->EffectHandles)
            {
                if (EffectPair.Value.IsValid())
                {
                    ASC->RemoveActiveGameplayEffect(EffectPair.Value);
                }
            }

            PatientQueueEffects.Remove(Patient);
        }

        ChangePatientPhaseTag(Patient, EMRTags::Patient::Phase::Leaving);
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Sending patient to exit. Patient=%s"), *GetNameSafe(Patient));
    SendPatientToExit(Patient);
    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TreatmentAbandonment end. Patient=%s"), *GetNameSafe(Patient));
}


void UEMRTreatmentSubsystem::RegisterTreatmentBed(AEMRTreatmentBed* Bed)
{
    if (!Bed || RegisteredBeds.Contains(Bed))
    {
        return;
    }

    RegisteredBeds.Add(Bed);
    BedAssignmentsMap.Add(Bed, nullptr); // Lit libre au départ


    TryAssignPatientToBed();
}


void UEMRTreatmentSubsystem::UnregisterTreatmentBed(AEMRTreatmentBed* Bed)
{
    if (!Bed)
    {
        return;
    }

    AEMRPatient* AssignedPatient = BedAssignmentsMap.FindRef(Bed);
    if (AssignedPatient && GetWorld() && !GetWorld()->bIsTearingDown)
    {
        WaitingQueue.Add(AssignedPatient);
        ChangePatientPhaseTag(AssignedPatient, EMRTags::Patient::Phase::WaitingTreatment);
        AssignTreatmentWaitingSeat(AssignedPatient);
    }

    RegisteredBeds.Remove(Bed);
    BedAssignmentsMap.Remove(Bed);

}


void UEMRTreatmentSubsystem::CompleteTreatmentForPatient(AEMRPatient* Patient, FGameplayTag TreatmentCompleted, bool bSuccess)
{
    if (!Patient || !TreatmentCompleted.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] CompleteTreatmentForPatient invalid params"));
        return;
    }

    if (!bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Treatment %s failed for %s"), *TreatmentCompleted.ToString(), *GetNameSafe(Patient));
        return;
    }

    UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Patient ASC missing"));
        return;
    }

    // Remove queue tag associated with this treatment
    const FGameplayTag QueueTag = GetQueueTagForTreatment(TreatmentCompleted);
    if (QueueTag.IsValid())
    {
        if (FEMRTreatmentQueueEffectHandles* PatientHandles = PatientQueueEffects.Find(Patient))
        {
            if (FActiveGameplayEffectHandle* Handle = PatientHandles->EffectHandles.Find(TreatmentCompleted))
            {
                if (Handle->IsValid())
                {
                    ASC->RemoveActiveGameplayEffect(*Handle);
                }
            	
                PatientHandles->EffectHandles.Remove(TreatmentCompleted);
                if (PatientHandles->EffectHandles.Num() == 0)
                {
                    PatientQueueEffects.Remove(Patient);
                }
            }
        }
    }

    // Remove pathology tags that no longer require treatments
    bool bPathologyRemoved = false;
    FGameplayTagContainer PatientPathologies = Patient->GetPatientData() ? Patient->GetPatientData()->GetPathologies() : FGameplayTagContainer();

    for (const FGameplayTag& PathologyTag : PatientPathologies)
    {
        const FGameplayTagContainer* RequiredTreatments = CachedTreatmentsByPathology.Find(PathologyTag);
        if (!RequiredTreatments)
        {
            continue;
        }

        bool bHasPendingTreatment = false;
        for (const FGameplayTag& RequiredTreatment : RequiredTreatments->GetGameplayTagParents())
        {
            if (PatientQueueEffects.Contains(Patient) && PatientQueueEffects[Patient].EffectHandles.Contains(RequiredTreatment))
            {
                bHasPendingTreatment = true;
                break;
            }
        }

        if (!bHasPendingTreatment && ASC->HasMatchingGameplayTag(PathologyTag))
        {
            ASC->RemoveLooseGameplayTag(PathologyTag);
            bPathologyRemoved = true;
        }
    }

    if (bPathologyRemoved)
    {
        UE_LOG(LogTemp, Log, TEXT("[TreatmentSubsystem] Removed pathology tags for %s"), *GetNameSafe(Patient));
    }

    const bool bPatientFullyCured = !ASC->HasMatchingGameplayTag(EMRTags::Patient::Pathology::Root);

    // Handle payout and exit if no more pathologies
    if (bPatientFullyCured)
    {
        const float FinalMoneyReward = ComputePatientMoneyReward(Patient, ASC);
        const float FinalReputationReward = ComputePatientReputationReward(Patient, ASC);
        ApplyPatientReward(Patient, FinalMoneyReward, FinalReputationReward);
        if (AEMRNightShiftGameState* NightShiftGameState = GetWorld()->GetGameState<AEMRNightShiftGameState>())
        {
            NightShiftGameState->RegisterPatientPaid();
        }
        ChangePatientPhaseTag(Patient, EMRTags::Patient::Phase::Paid);
        ChangePatientPhaseTag(Patient, EMRTags::Patient::Phase::Leaving);


        AEMRTreatmentBed* PatientBed = FindBedAssignedToPatient(Patient);
        if (PatientBed)
        {
            PatientBed->ReleasePatient();
        }

        SendPatientToExit(Patient);
    }

    TreatmentResolvedNativeDelegate.Broadcast(Patient, TreatmentCompleted, bPatientFullyCured);
}



void UEMRTreatmentSubsystem::OnBedFreed(AEMRTreatmentBed* Bed)
{
    if (!Bed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] BedFreed ignored. Bed=null"));
        return;
    }

    const AEMRPatient* PreviousAssigned = BedAssignmentsMap.FindRef(Bed);
    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] BedFreed start. Bed=%s AssignedPatient=%s Occupied=%s"),
        *GetNameSafe(Bed),
        *GetNameSafe(PreviousAssigned),
        Bed->IsOccupied() ? TEXT("true") : TEXT("false"));

    BedAssignmentsMap.Add(Bed, nullptr);
	
    TryAssignPatientToBed();

    const AEMRPatient* AssignedAfter = BedAssignmentsMap.FindRef(Bed);
    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] BedFreed after treatment queue. Bed=%s AssignedPatient=%s Occupied=%s"),
        *GetNameSafe(Bed),
        *GetNameSafe(AssignedAfter),
        Bed->IsOccupied() ? TEXT("true") : TEXT("false"));

    if (AssignedAfter == nullptr)
    {
        if (UEMRExamQueueSubsystem* ExamQueueSubsystem = GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>())
        {
            const bool bAssignedLab = ExamQueueSubsystem->TryAssignLabAnalyzerWaitingPatientToBed();
            UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] BedFreed TryAssignLabAnalyzerWaitingPatientToBed=%s"),
                bAssignedLab ? TEXT("true") : TEXT("false"));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] BedFreed end. Bed=%s AssignedPatient=%s Occupied=%s"),
        *GetNameSafe(Bed),
        *GetNameSafe(BedAssignmentsMap.FindRef(Bed)),
        Bed->IsOccupied() ? TEXT("true") : TEXT("false"));
}


bool UEMRTreatmentSubsystem::HasAvailableBed() const
{
    for (const auto& Pair : BedAssignmentsMap)
    {
        if (Pair.Value == nullptr) // Lit libre
        {
            return true;
        }
    }
    return false;
}


bool UEMRTreatmentSubsystem::AreAllTreatmentWaitingSeatsFull() const
{
    const UEMRTreatmentWaitingManagerComponent* Manager = FindTreatmentWaitingManager();
    if (!Manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] AreAllTreatmentWaitingSeatsFull: no treatment waiting manager found."));
        return false;
    }

    if (Manager->HasFreeSeat())
    {
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] AreAllTreatmentWaitingSeatsFull: all treatment waiting seats are occupied."));
    return true;
}


int32 UEMRTreatmentSubsystem::GetWaitingQueueSize() const
{
    return WaitingQueue.Num();
}


bool UEMRTreatmentSubsystem::TryAssignPatientToBedForExam(AEMRPatient* Patient)
{
    if (!Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] TryAssignPatientToBedForExam ignored. Patient=null"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TryAssignPatientToBedForExam start. Patient=%s"), *GetNameSafe(Patient));

    AEMRTreatmentBed* FreeBed = nullptr;
    for (const auto& Pair : BedAssignmentsMap)
    {
        if (Pair.Value == nullptr)
        {
            FreeBed = Pair.Key;
            break;
        }
    }

    if (!FreeBed)
    {
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TryAssignPatientToBedForExam no free bed. Patient=%s"),
            *GetNameSafe(Patient));
        return false;
    }

    if (FindBedAssignedToPatient(Patient))
    {
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TryAssignPatientToBedForExam already assigned. Patient=%s"),
            *GetNameSafe(Patient));
        return true;
    }

    const AEMRPatient* ExistingPatient = BedAssignmentsMap.FindRef(FreeBed);
    if (ExistingPatient || FreeBed->IsOccupied())
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] TryAssignPatientToBedForExam bed not available. Patient=%s Bed=%s AssignedPatient=%s Occupied=%s"),
            *GetNameSafe(Patient),
            *GetNameSafe(FreeBed),
            *GetNameSafe(ExistingPatient),
            FreeBed->IsOccupied() ? TEXT("true") : TEXT("false"));
        return false;
    }

    FreeBed->AssignPatient(Patient);
    if (FreeBed->GetCurrentPatient() != Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] TryAssignPatientToBedForExam bed rejected patient. Patient=%s Bed=%s CurrentPatient=%s"),
            *GetNameSafe(Patient),
            *GetNameSafe(FreeBed),
            *GetNameSafe(FreeBed->GetCurrentPatient()));
        return false;
    }

    BedAssignmentsMap.Add(FreeBed, Patient);
    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] TryAssignPatientToBedForExam assigned. Patient=%s Bed=%s"),
        *GetNameSafe(Patient),
        *GetNameSafe(FreeBed));
    ReleaseReceptionSeatIfPresent(Patient);

    if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
    {
        const FVector PatientWaitPointLocation = FreeBed->GetPatientWaitPointLocation();
        const FRotator PatientWaitPointRotation = FreeBed->GetPatientWaitPointRotation();
        NavigationSubsystem->BroadcastNavigationIntent(Patient, EMRTags::GMS::AI::Navigation::MoveToBed, FreeBed, PatientWaitPointLocation, PatientWaitPointRotation);
    }

    return true;
}

bool UEMRTreatmentSubsystem::IsPatientAssignedToBed(AEMRPatient* Patient) const
{
    return FindBedAssignedToPatient(Patient) != nullptr;
}

void UEMRTreatmentSubsystem::PromoteExamWaitingPatientToTreatment(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    ChangePatientPhaseTag(Patient, EMRTags::Patient::Phase::WaitingTreatment);
    Patient->SetCanReceiveTreatment(true);

    FGameplayTagContainer TreatmentsTags = GetTreatmentsFromPatient(Patient);
    for (const FGameplayTag& Tag : TreatmentsTags)
    {
        ApplyQueueTagToPatient(Patient, Tag);
    }
}

bool UEMRTreatmentSubsystem::ResolvePatientAsFullyTreated(AEMRPatient* Patient)
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

    if (ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving))
    {
        return true;
    }

    if (!ASC->HasMatchingGameplayTag(EMRTags::Patient::Pathology::Root))
    {
        return true;
    }

    TArray<FGameplayTag> TreatmentArray;
    {
        FGameplayTagContainer TreatmentTags = GetTreatmentsFromPatient(Patient);
        TreatmentTags.GetGameplayTagArray(TreatmentArray);
    }

    if (const FEMRTreatmentQueueEffectHandles* QueueHandles = PatientQueueEffects.Find(Patient))
    {
        for (const TPair<FGameplayTag, FActiveGameplayEffectHandle>& Pair : QueueHandles->EffectHandles)
        {
            if (Pair.Key.IsValid())
            {
                TreatmentArray.AddUnique(Pair.Key);
            }
        }
    }

    bool bResolvedAnyTreatment = false;
    for (const FGameplayTag& TreatmentTag : TreatmentArray)
    {
        if (!TreatmentTag.IsValid())
        {
            continue;
        }

        bResolvedAnyTreatment = true;
        CompleteTreatmentForPatient(Patient, TreatmentTag, true);

        if (ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving)
            || !ASC->HasMatchingGameplayTag(EMRTags::Patient::Pathology::Root))
        {
            return true;
        }
    }

    if (!bResolvedAnyTreatment)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] ResolvePatientAsFullyTreated missing valid treatment tag for patient %s"), *GetNameSafe(Patient));
        return false;
    }

    return !ASC->HasMatchingGameplayTag(EMRTags::Patient::Pathology::Root);
}

void UEMRTreatmentSubsystem::HandleMagicBoxFreed()
{
    TryAssignPatientToBed();
}

void UEMRTreatmentSubsystem::TryAssignPatientToBed()
{
    if (WaitingQueue.Num() == 0)
    {
        return;
    }

    AEMRPatient* WaitingPatient = WaitingQueue[0];
    UE_LOG(LogTemp, Warning, TEXT("%s Treatment TryAssignToBed queueHead=%s queueSize=%d"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(WaitingPatient), WaitingQueue.Num());
    if (TryAssignPatientToMagicBox(WaitingPatient))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Treatment TryAssignToBed diverted queueHead to magic box patient=%s"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(WaitingPatient));
        if (WaitingQueue.Num() > 0)
        {
            TryAssignPatientToBed();
        }
        return;
    }

    AEMRTreatmentBed* FreeBed = nullptr;
    for (const auto& Pair : BedAssignmentsMap)
    {
        if (Pair.Value == nullptr)
        {
            FreeBed = Pair.Key;
            break;
        }
    }

    if (!FreeBed)
    {
        return;
    }

    AEMRPatient* Patient = WaitingQueue[0];
    WaitingQueue.RemoveAt(0);

    AssignPatientToBed(Patient, FreeBed);
}

bool UEMRTreatmentSubsystem::TryAssignPatientToMagicBox(AEMRPatient* Patient)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Treatment TryAssignPatientToMagicBox start patient=%s"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(Patient));
    if (!Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Treatment TryAssignPatientToMagicBox fail patient null"), TreatmentMagicBoxFlowLogPrefix);
        return false;
    }

    if (FindBedAssignedToPatient(Patient))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Treatment TryAssignPatientToMagicBox rejected patient already has bed patient=%s"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Treatment TryAssignPatientToMagicBox fail no world patient=%s"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return false;
    }

    UEMRMagicBoxSubsystem* MagicBoxSubsystem = World->GetSubsystem<UEMRMagicBoxSubsystem>();
    if (!MagicBoxSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Treatment TryAssignPatientToMagicBox fail no subsystem patient=%s"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return false;
    }

    if (!MagicBoxSubsystem->TryAssignPatient(Patient))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Treatment TryAssignPatientToMagicBox fail subsystem rejected patient=%s"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(Patient));
        return false;
    }

    WaitingQueue.Remove(Patient);
    ReleaseTreatmentWaitingSeat(Patient);
    ReleaseReceptionSeatIfPresent(Patient);
    UE_LOG(LogTemp, Warning, TEXT("%s Treatment TryAssignPatientToMagicBox success patient=%s newQueueSize=%d"), TreatmentMagicBoxFlowLogPrefix, *GetNameSafe(Patient), WaitingQueue.Num());
    return true;
}


void UEMRTreatmentSubsystem::AssignPatientToBed(AEMRPatient* Patient, AEMRTreatmentBed* Bed)
{
    if (!Patient || !Bed)
    {
        return;
    }

    // Prevent double-assignment of the same patient
    if (AEMRTreatmentBed* ExistingBed = FindBedAssignedToPatient(Patient))
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Patient %s is already assigned to bed %s"), *GetNameSafe(Patient), *GetNameSafe(ExistingBed));
        return;
    }

    // Ensure the bed is effectively free both in the map and on the actor
    const AEMRPatient* ExistingPatient = BedAssignmentsMap.FindRef(Bed);
    if (ExistingPatient || Bed->IsOccupied())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Bed %s is already occupied when assigning %s"), *GetNameSafe(Bed), *GetNameSafe(Patient));

        // Requeue the patient so they are not lost
        if (!WaitingQueue.Contains(Patient))
        {
            WaitingQueue.Insert(Patient, 0);
            AssignTreatmentWaitingSeat(Patient);
        }
        return;
    }

    Bed->AssignPatient(Patient);

    // If the bed rejected the patient (e.g., already had one), requeue and abort
    if (Bed->GetCurrentPatient() != Patient)
    {
        UE_LOG(LogTemp, Error, TEXT("[TreatmentSubsystem] Bed %s rejected patient %s during assignment"), *GetNameSafe(Bed), *GetNameSafe(Patient));
        if (!WaitingQueue.Contains(Patient))
        {
            WaitingQueue.Insert(Patient, 0);
            AssignTreatmentWaitingSeat(Patient);
        }
        return;
    }

    ReleaseTreatmentWaitingSeat(Patient);

    FGameplayTagContainer TreatmentsTags = GetTreatmentsFromPatient(Patient);
    for (const FGameplayTag& Tag : TreatmentsTags)
    {
        ApplyQueueTagToPatient(Patient, Tag);
    }

    BedAssignmentsMap.Add(Bed, Patient);
    ReleaseReceptionSeatIfPresent(Patient);

    if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
    {
        const FVector PatientWaitPointLocation = Bed->GetPatientWaitPointLocation();
        const FRotator PatientWaitPointRotation = Bed->GetPatientWaitPointRotation();
        NavigationSubsystem->BroadcastNavigationIntent(Patient, EMRTags::GMS::AI::Navigation::MoveToBed, Bed, PatientWaitPointLocation, PatientWaitPointRotation);
    }
}


AEMRTreatmentBed* UEMRTreatmentSubsystem::FindBedAssignedToPatient(AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return nullptr;
    }

    for (const TPair<AEMRTreatmentBed*, AEMRPatient*>& Pair : BedAssignmentsMap)
    {
        if (Pair.Value == Patient)
        {
            return Pair.Key;
        }
    }

    return nullptr;
}


void UEMRTreatmentSubsystem::ChangePatientPhaseTag(AEMRPatient* Patient, FGameplayTag NewPhaseTag)
{
	if (!Patient || !NewPhaseTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Invalid patient or phase tag"));
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


void UEMRTreatmentSubsystem::ApplyQueueTagToPatient(AEMRPatient* Patient, FGameplayTag TreatmentTag)
{
	if (!Patient || !TreatmentTag.IsValid())
	{
		return;
	}

	ApplyQueueTagEffect(Patient, TreatmentTag);
}


void UEMRTreatmentSubsystem::ApplyQueueTagEffect(AEMRPatient* Patient, FGameplayTag TreatmentTag)
{
	if (!Patient || !TreatmentTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent();
	const UEMRPatientData* PatientData = Patient->GetPatientData();

	if (!ASC || !PatientData)
	{
		return;
	}

	FGameplayTag QueueTag = GetQueueTagForTreatment(TreatmentTag);
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
		StoreQueueEffectHandle(Patient, TreatmentTag, ActiveHandle);
	}
}


FGameplayTag UEMRTreatmentSubsystem::GetQueueTagForTreatment(FGameplayTag TreatmentTag)
{
	if (!TreatmentTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] Invalid ExamTag"));
		return FGameplayTag::EmptyTag;
	}

	if (CachedTreatmentQueueMappings.Num() == 0)
	{
		BuildTreatmentQueueMappingsCache();
	}

	for (const FEMRTreatmentQueueMapping& Mapping : CachedTreatmentQueueMappings)
	{
		if (!Mapping.TreatmentTag.IsValid() || !Mapping.QueueTag.IsValid())
		{
			continue;
		}

		if (TreatmentTag.MatchesTagExact(Mapping.TreatmentTag))
		{
			return Mapping.QueueTag;
		}
	}
    
	UE_LOG(LogTemp, Warning, TEXT("[ExamQueue] No queue tag found for exam: %s"), *TreatmentTag.ToString());
	return FGameplayTag::EmptyTag;
}


void UEMRTreatmentSubsystem::StoreQueueEffectHandle(AEMRPatient* Patient, FGameplayTag TreatmentTag, FActiveGameplayEffectHandle Handle)
{
	if (!Patient || !TreatmentTag.IsValid() || !Handle.IsValid())
	{
		return;
	}

	FEMRTreatmentQueueEffectHandles& PatientEffects = PatientQueueEffects.FindOrAdd(Patient);
	PatientEffects.EffectHandles.Add(TreatmentTag, Handle);
}


FGameplayTagContainer UEMRTreatmentSubsystem::GetTreatmentsFromPatient(AEMRPatient* Patient)
{
	if (!Patient || !Patient->GetPatientData())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Invalid Patient"));
		return FGameplayTagContainer();
	}

	if (CachedTreatmentsByPathology.Num() == 0)
	{
		BuildTreatmentsFromPathologyMappingsCache();
	}

	FGameplayTagContainer PatientPathologies = Patient->GetPatientData()->GetPathologies();
	FGameplayTagContainer PatientTreatmentsNeeded;
	for (const FGameplayTag& Pathology : PatientPathologies)
	{
		if (const FGameplayTagContainer* Treatments = CachedTreatmentsByPathology.Find(Pathology))
		{
			PatientTreatmentsNeeded.AppendTags(*Treatments);
		}
	}
	
	return PatientTreatmentsNeeded;
}


void UEMRTreatmentSubsystem::ApplyPhaseEffect(AEMRPatient* Patient, FGameplayTag PhaseTag)
{
    if (!Patient || !PhaseTag.IsValid())
    {
        return;
	}

	if (!GetWorld() || GetWorld()->bIsTearingDown)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] World is tearing down, skipping phase effect"));
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
		UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Patient has no PatientData"));
		return;
	}

	TSubclassOf<UGameplayEffect> PhaseEffect = PatientData->GetAddTagToPatientEffect();
	if (!PhaseEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QueueSubsystem] No queue effect for: %s"), *PhaseTag.ToString());
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
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[TreatmentSubsystem] Failed to create spec for phase effect"));
    }
}


float UEMRTreatmentSubsystem::ComputePatientMoneyReward(AEMRPatient* Patient, UAbilitySystemComponent* PatientASC) const
{
    if (!Patient || !PatientASC)
    {
        return 0.f;
    }

    if (!Patient->HasCachedRewards())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Missing cached rewards for patient %s"), *GetNameSafe(Patient));
        return 0.f;
    }

    return FMath::Max(0.f, Patient->GetCachedMoneyReward());
}


float UEMRTreatmentSubsystem::ComputePatientReputationReward(AEMRPatient* Patient, UAbilitySystemComponent* PatientASC) const
{
	if (!Patient || !PatientASC)
	{
		return 0.f;
	}

    if (!Patient->HasCachedRewards())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Missing cached rewards for patient %s"), *GetNameSafe(Patient));
        return 0.f;
    }

	return FMath::Max(0.f, Patient->GetCachedReputationReward());
}


void UEMRTreatmentSubsystem::ApplyPatientReward(AEMRPatient* Patient, float MoneyRewardAmount, float ReputationRewardAmount)
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

    AEMRNightShiftGameState* NightShiftGameState = World->GetGameState<AEMRNightShiftGameState>();
    UAbilitySystemComponent* TeamASC = NightShiftGameState ? NightShiftGameState->GetAbilitySystemComponent() : nullptr;
    if (!TeamASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Team ASC not available for payment"));
        return;
    }

    TArray<const UEMREconomySystemGenerics*> LoadedEconomy;
    if (!UEMRAssetManager::Get().CollectLoadedEconomySystemGenerics(LoadedEconomy) || LoadedEconomy.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] EconomySystemGenerics not loaded"));
        return;
    }

    const UEMREconomySystemGenerics* EconomyConfig = LoadedEconomy[0];
    if (!EconomyConfig)
    {
        return;
    }

    if (MoneyRewardAmount > 0.f)
    {
        TSubclassOf<UGameplayEffect> GrantMoneyEffect = EconomyConfig->GetGrantMoneyEffect();
        if (!GrantMoneyEffect)
        {
            UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Missing GrantMoney config"));
        }
        else
        {
            FGameplayEffectContextHandle MoneyEffectContext = TeamASC->MakeEffectContext();
            MoneyEffectContext.AddSourceObject(Patient);

            FGameplayEffectSpecHandle MoneySpecHandle = TeamASC->MakeOutgoingSpec(GrantMoneyEffect, 1.0f, MoneyEffectContext);
            if (MoneySpecHandle.IsValid())
            {
                MoneySpecHandle.Data->SetSetByCallerMagnitude(EMRTags::SetByCaller::GrantMoney, MoneyRewardAmount);
                TeamASC->ApplyGameplayEffectSpecToSelf(*MoneySpecHandle.Data.Get());
            }
        }
    }

    if (ReputationRewardAmount > 0.f)
    {
        TSubclassOf<UGameplayEffect> GrantReputationEffect = EconomyConfig->GetGrantReputationEffect();
        if (!GrantReputationEffect)
        {
            UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] Missing GrantReputation effect"));
        }
        else
        {
            FGameplayEffectContextHandle ReputationEffectContext = TeamASC->MakeEffectContext();
            ReputationEffectContext.AddSourceObject(Patient);

            FGameplayEffectSpecHandle ReputationSpecHandle = TeamASC->MakeOutgoingSpec(GrantReputationEffect, 1.0f, ReputationEffectContext);
            if (ReputationSpecHandle.IsValid())
            {
                ReputationSpecHandle.Data->SetSetByCallerMagnitude(EMRTags::SetByCaller::GrantReputation, ReputationRewardAmount);
                TeamASC->ApplyGameplayEffectSpecToSelf(*ReputationSpecHandle.Data.Get());
            }
        }
    }
}

void UEMRTreatmentSubsystem::SendPatientToExit(AEMRPatient* Patient)
{
    if (!Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] SendToExit ignored. Patient=null"));
        return;
    }

    AActor* ExitActor = FindClosestHospitalExit(Patient);
    FVector ExitLocation = ExitActor ? ExitActor->GetActorLocation() : FVector::ZeroVector;
    FRotator ExitRotation = ExitActor ? ExitActor->GetActorRotation() : FRotator::ZeroRotator;

    if (const AEMRHospitalExit* HospitalExit = Cast<AEMRHospitalExit>(ExitActor))
    {
        const FTransform ExitTransform = HospitalExit->GetExitTransform();
        ExitLocation = ExitTransform.GetLocation();
        ExitRotation = ExitTransform.Rotator();
    }

    if (UEMRNavigationIntentSubsystem* NavigationSubsystem = GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
    {
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Broadcast MoveToExit. Patient=%s Exit=%s Location=%s Rotation=%s"),
            *GetNameSafe(Patient),
            *GetNameSafe(ExitActor),
            *ExitLocation.ToCompactString(),
            *ExitRotation.ToCompactString());
        NavigationSubsystem->BroadcastNavigationIntent(Patient, EMRTags::GMS::AI::Navigation::MoveToExit, ExitActor, ExitLocation, ExitRotation);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] Navigation subsystem missing for MoveToExit. Patient=%s"),
            *GetNameSafe(Patient));
    }
}


AActor* UEMRTreatmentSubsystem::FindClosestHospitalExit(AActor* OriginActor) const
{
    if (!OriginActor || !GetWorld())
    {
        return nullptr;
    }

    AActor* ClosestExit = nullptr;
    float ClosestDistanceSq = TNumericLimits<float>::Max();

    for (TActorIterator<AEMRHospitalExit> It(GetWorld()); It; ++It)
    {
        const float DistanceSq = FVector::DistSquared(OriginActor->GetActorLocation(), It->GetActorLocation());
        if (DistanceSq < ClosestDistanceSq)
        {
            ClosestDistanceSq = DistanceSq;
            ClosestExit = *It;
        }
    }

    return ClosestExit;
}


void UEMRTreatmentSubsystem::BuildTreatmentQueueMappingsCache() const
{
	CachedTreatmentQueueMappings.Reset();

	TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
	UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);
	
	if (LoadedSubsystemConfig.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreatmentQueueSubsystem] SubsystemConfig not loaded"));
		return;
	}

	UDataTable* MappingTable = LoadedSubsystemConfig[0]->GetTreatmentQueueMappingTable();
	if (!MappingTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreatmentQueueSubsystem] TreatmentQueueMappingTable is not set"));
		return;
	}

	static const FString Context(TEXT("TreatmentQueueMappings"));
	TArray<FEMRTreatmentQueueMapping*> Rows;
	MappingTable->GetAllRows(Context, Rows);

	for (const FEMRTreatmentQueueMapping* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}

		CachedTreatmentQueueMappings.Add(*Row);
	}
}


void UEMRTreatmentSubsystem::BuildTreatmentsFromPathologyMappingsCache() const
{
    CachedTreatmentsByPathology.Reset();

	TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
	UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);
	
	if (LoadedSubsystemConfig.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreatmentQueueSubsystem] SubsystemConfig not loaded"));
		return;
	}

	UDataTable* MappingTable = LoadedSubsystemConfig[0]->GetTreatmentsFromPathologyMappingTable();
	if (!MappingTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreatmentQueueSubsystem] TreatmentQueueMappingTable is not set"));
		return;
	}

	static const FString Context(TEXT("TreatmentQueueMappings"));
	TArray<FEMRTreatmentForPathologyMapping*> Rows;
	MappingTable->GetAllRows(Context, Rows);

	for (const FEMRTreatmentForPathologyMapping* Row : Rows)
	{
		if (!Row || !Row->Pathology.IsValid())
		{
			continue;
		}

        FGameplayTagContainer& Treatments = CachedTreatmentsByPathology.FindOrAdd(Row->Pathology);
        Treatments.AppendTags(Row->Treatments);
    }
}


void UEMRTreatmentSubsystem::AssignTreatmentWaitingSeat(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    if (UEMRTreatmentWaitingManagerComponent* Manager = FindTreatmentWaitingManager())
    {
        UEMRWaitingSeatComponent* AssignedSeat = nullptr;
        Manager->RequestSeatForPatient(Patient, AssignedSeat);
    }
}

void UEMRTreatmentSubsystem::ReleaseTreatmentWaitingSeat(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    if (UEMRTreatmentWaitingManagerComponent* Manager = FindTreatmentWaitingManager())
    {
        Manager->ReleaseSeatForPatient(Patient);
    }
}

UEMRTreatmentWaitingManagerComponent* UEMRTreatmentSubsystem::FindTreatmentWaitingManager() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AEMRTreatmentWaitingArea> It(World); It; ++It)
    {
        if (AEMRTreatmentWaitingArea* WaitingArea = *It)
        {
            if (UEMRTreatmentWaitingManagerComponent* Manager = WaitingArea->GetManagerComponent())
            {
                return Manager;
            }
        }
    }

    return nullptr;
}

bool UEMRTreatmentSubsystem::ReleaseReceptionSeatIfPresent(AEMRPatient* Patient) const
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


void UEMRTreatmentSubsystem::LoadSubsystemConfig()
{
    UEMRAssetManager::Get().LoadSubsystemConfig(FStreamableDelegate::CreateUObject(this, &ThisClass::OnLoadedSubsystemConfig));
}


void UEMRTreatmentSubsystem::OnLoadedSubsystemConfig()
{
    BuildTreatmentQueueMappingsCache();
    BuildTreatmentsFromPathologyMappingsCache();
    LoadDifficultyTuning();
}


void UEMRTreatmentSubsystem::LoadDifficultyTuning() const
{
    if (CachedDifficultyTuning.IsValid())
    {
        return;
    }

    TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig) || LoadedSubsystemConfig.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] SubsystemConfig not loaded, cannot fetch DifficultyTuning"));
        return;
    }

    CachedDifficultyTuning = LoadedSubsystemConfig[0]->GetDifficultyTuning();

    if (!CachedDifficultyTuning.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TreatmentSubsystem] DifficultyTuning not set in SubsystemConfig"));
    }
}


const UEMRDifficultyTuningData* UEMRTreatmentSubsystem::GetDifficultyTuning() const
{
    LoadDifficultyTuning();
    return CachedDifficultyTuning.Get();
}
