#include "GAS/Abilities/EMRGameplayAbility_MoveToWaitingSeat.h"

#include "Characters/Patient/EMRPatient.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogGA_MoveToWaitingSeat, Log, All);

UEMRGameplayAbility_MoveToWaitingSeat::UEMRGameplayAbility_MoveToWaitingSeat()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	

	FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Abilities::MoveToSeat;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}


void UEMRGameplayAbility_MoveToWaitingSeat::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UE_LOG(LogGA_MoveToWaitingSeat, Verbose, TEXT("[GA] Activate MoveToWaitingSeat (Handle=%s)"), *Handle.ToString());

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Warning, TEXT("[GA] CommitAbility failed."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!HasAuthority(&ActivationInfo))
    {
        UE_LOG(LogGA_MoveToWaitingSeat, VeryVerbose, TEXT("[GA] Lacking authority, ending on client."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRPatient* Patient = Cast<AEMRPatient>(GetAvatarActorFromActorInfo());
    if (!Patient)
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Warning, TEXT("[GA] Avatar is not a patient."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UWorld* World = Patient->GetWorld();
    if (!World)
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Warning, TEXT("[GA] World missing for patient %s"), *Patient->GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UEMRWaitingRoomManagerComponent* Manager = FindWaitingRoomManager(World);
    if (!Manager)
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Warning, TEXT("[GA] No waiting room manager found in world %s"), *World->GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UEMRWaitingSeatComponent* Seat = const_cast<UEMRWaitingSeatComponent*>(Cast<UEMRWaitingSeatComponent>(TriggerEventData->OptionalObject));
    if (Seat)
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Log, TEXT("[GA] Using provided seat %s for patient %s"), *Seat->GetName(), *Patient->GetName());
    }
    else
    {
        Seat = EnsureSeatReservation(Patient, Manager);
        if (!Seat)
        {
            UE_LOG(LogGA_MoveToWaitingSeat, Warning, TEXT("[GA] Seat reservation failed for patient %s"), *Patient->GetName());
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }
    }

    UE_LOG(LogGA_MoveToWaitingSeat, Log, TEXT("[GA] Seat %s ready for patient %s"), *Seat->GetName(), *Patient->GetName());
    SendNavigationIntent(Patient, Seat);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UEMRWaitingRoomManagerComponent* UEMRGameplayAbility_MoveToWaitingSeat::FindWaitingRoomManager(const UWorld* World) const
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
                UE_LOG(LogGA_MoveToWaitingSeat, VeryVerbose, TEXT("[GA] Found waiting room manager on area %s"), *WaitingArea->GetName());
                return Manager;
            }
        }
    }

    return nullptr;
}

UEMRWaitingSeatComponent* UEMRGameplayAbility_MoveToWaitingSeat::EnsureSeatReservation(AActor* PatientActor, UEMRWaitingRoomManagerComponent* Manager) const
{
    if (!PatientActor || !Manager)
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Warning, TEXT("[GA] EnsureSeatReservation missing patient or manager."));
        return nullptr;
    }

    if (UEMRWaitingSeatComponent* ExistingSeat = Manager->GetSeatForPatient(PatientActor))
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Log, TEXT("[GA] Patient %s already has seat %s"), *PatientActor->GetName(), *ExistingSeat->GetName());
        return ExistingSeat;
    }

    UEMRWaitingSeatComponent* ReservedSeat = nullptr;
    Manager->RequestSeatForPatient(PatientActor, ReservedSeat);

    if (ReservedSeat)
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Log, TEXT("[GA] Reserved seat %s for patient %s"), *ReservedSeat->GetName(), *PatientActor->GetName());
    }
    else
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Warning, TEXT("[GA] No seat available yet for patient %s (queued)."), *PatientActor->GetName());
    }

    return ReservedSeat;
}

void UEMRGameplayAbility_MoveToWaitingSeat::SendNavigationIntent(AEMRPatient* Patient, UEMRWaitingSeatComponent* Seat) const
{
    if (!Patient || !Seat)
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Warning, TEXT("[GA] Cannot send navigation intent: Patient or Seat missing."));
        return;
    }

    AActor* SeatActor = Seat->GetOwner();
    if (UEMRNavigationIntentSubsystem* NavigationSubsystem = Patient->GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
    {
        const FTransform ApproachTransform = Seat->GetApproachTransform();
        const FString TargetKind = Seat->HasApproachPoint() ? TEXT("explicit approach point") : TEXT("derived approach offset");

        UE_LOG(LogGA_MoveToWaitingSeat, Log, TEXT("[GA] Broadcasting MoveToWaitingSeat intent for %s to %s (%s) @ %s"), *Patient->GetName(), *SeatActor->GetName(), *TargetKind, *ApproachTransform.GetLocation().ToString());
        NavigationSubsystem->BroadcastNavigationIntent(Patient, EMRTags::GMS::AI::Navigation::MoveToWaitingSeat, SeatActor, ApproachTransform.GetLocation(), ApproachTransform.GetRotation().Rotator());
    }
    else
    {
        UE_LOG(LogGA_MoveToWaitingSeat, Warning, TEXT("[GA] Navigation subsystem unavailable for patient %s"), *Patient->GetName());
    }
}
