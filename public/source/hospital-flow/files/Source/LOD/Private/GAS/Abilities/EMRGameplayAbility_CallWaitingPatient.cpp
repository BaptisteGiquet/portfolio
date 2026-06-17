#include "GAS/Abilities/EMRGameplayAbility_CallWaitingPatient.h"

#include "Characters/Patient/EMRPatient.h"
#include "Environment/EMRWaitingRoomArea.h"
#include "Environment/EMRWaitingRoomManagerComponent.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRBaseMachine.h"
#include "Subsystems/EMRSnackMachineSubsystem.h"
#include "Subsystems/EMRToiletSubsystem.h"
#include "Subsystems/EMRNavigationIntentSubsystem.h"
#include "EngineUtils.h"

UEMRGameplayAbility_CallWaitingPatient::UEMRGameplayAbility_CallWaitingPatient()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Abilities::CallWaitingPatient;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_CallWaitingPatient::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!HasAuthority(&ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    const AEMRPatient* Patient = TriggerEventData ? Cast<AEMRPatient>(TriggerEventData->Target) : nullptr;
    if (!Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_CallWaitingPatient] No patient provided in event payload."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UWorld* World = Patient->GetWorld();
    UEMRWaitingRoomManagerComponent* Manager = World ? FindWaitingRoomManager(World) : nullptr;
    if (!Manager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_CallWaitingPatient] Waiting room manager unavailable."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!IsPatientSeated(*Patient, *Manager))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_CallWaitingPatient] Patient %s is not seated."), *Patient->GetName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (UEMRToiletSubsystem* ToiletSubsystem = World ? World->GetSubsystem<UEMRToiletSubsystem>() : nullptr)
    {
        if (ToiletSubsystem->IsToiletTripActive(Patient))
        {
            ToiletSubsystem->TryDeferCallForPatient(const_cast<AEMRPatient*>(Patient));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }
    }

    if (UEMRSnackMachineSubsystem* SnackMachineSubsystem = World ? World->GetSubsystem<UEMRSnackMachineSubsystem>() : nullptr)
    {
        if (SnackMachineSubsystem->IsSnackTripActive(Patient))
        {
            SnackMachineSubsystem->TryDeferCallForPatient(const_cast<AEMRPatient*>(Patient));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }
    }

    AEMRBaseMachine* ReceptionMachine = FindReceptionMachine(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    if (ReceptionMachine && ReceptionMachine->IsOccupied())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_CallWaitingPatient] Reception is occupied; cannot call another patient."));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    Manager->CallPatient(const_cast<AEMRPatient*>(Patient));

    if (ReceptionMachine)
    {
        ReceptionMachine->SetOccupiedByPatient(const_cast<AEMRPatient*>(Patient));
        SendNavigationIntent(*const_cast<AEMRPatient*>(Patient), *ReceptionMachine);
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UEMRWaitingRoomManagerComponent* UEMRGameplayAbility_CallWaitingPatient::FindWaitingRoomManager(const UWorld* World) const
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

AEMRBaseMachine* UEMRGameplayAbility_CallWaitingPatient::FindReceptionMachine(AActor* ReferenceActor) const
{
    if (!ReferenceActor)
    {
        return nullptr;
    }

    if (const UWorld* World = ReferenceActor->GetWorld())
    {
        if (const UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (const UEMRNavigationIntentSubsystem* NavigationSubsystem = GameInstance->GetSubsystem<UEMRNavigationIntentSubsystem>())
            {
                return NavigationSubsystem->FindClosestMachine(ReferenceActor, EMRTags::Machine::Reception);
            }
        }
    }

    return nullptr;
}

bool UEMRGameplayAbility_CallWaitingPatient::IsPatientSeated(const AEMRPatient& Patient, const UEMRWaitingRoomManagerComponent& Manager) const
{
    const TArray<FEMRWaitingPatientEntry> WaitingPatients = Manager.GetWaitingPatients();
    return WaitingPatients.ContainsByPredicate([&Patient](const FEMRWaitingPatientEntry& Entry)
    {
        return Entry.Patient.Get() == &Patient && Entry.Seat.IsValid();
    });
}

void UEMRGameplayAbility_CallWaitingPatient::SendNavigationIntent(AEMRPatient& Patient, AEMRBaseMachine& ReceptionMachine) const
{
    if (UEMRNavigationIntentSubsystem* NavigationSubsystem = Patient.GetGameInstance()->GetSubsystem<UEMRNavigationIntentSubsystem>())
    {
        const FGameplayTag NavigationTag = NavigationSubsystem->GetNavigationMessageTagForMachine(EMRTags::Machine::Reception);
        NavigationSubsystem->BroadcastNavigationIntent(
            &Patient,
            NavigationTag,
            &ReceptionMachine,
            ReceptionMachine.GetPatientWaitPointLocation(),
            ReceptionMachine.GetPatientWaitPointRotation(),
            EMRTags::Machine::Reception);
    }
}
