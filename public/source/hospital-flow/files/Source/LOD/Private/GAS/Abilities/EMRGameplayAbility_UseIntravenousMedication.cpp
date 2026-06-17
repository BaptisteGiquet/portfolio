#include "GAS/Abilities/EMRGameplayAbility_UseIntravenousMedication.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Characters/Patient/EMRPatient.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRTreatmentSubsystem.h"


UEMRGameplayAbility_UseIntravenousMedication::UEMRGameplayAbility_UseIntravenousMedication()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = EMRTags::Abilities::Treatment::IntravenousMedication;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}


void UEMRGameplayAbility_UseIntravenousMedication::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    InteractionTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EMRTags::Abilities::Interact, nullptr, /*OnlyTriggerOnce=*/false, /*OnlyMatchExact=*/true);

    if (!InteractionTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
	
    InteractionTask->EventReceived.AddDynamic(this, &UEMRGameplayAbility_UseIntravenousMedication::OnPatientInteractionReceived);
    InteractionTask->ReadyForActivation();
}


void UEMRGameplayAbility_UseIntravenousMedication::OnPatientInteractionReceived(FGameplayEventData Payload)
{
    if (!IsActive())
    {
        return;
    }

    if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
    {
        if (!HasAuthorityOrPredictionKey(ActorInfo, &CurrentActivationInfo))
        {
            EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, false);
            return;
        }

        AEMRPatient* Patient = const_cast<AEMRPatient*>(Cast<AEMRPatient>(Payload.Target));
        if (!Patient)
        {
            return;
        }

        if (TryCompleteTreatment(*Patient))
        {
            EndAbility(CurrentSpecHandle, ActorInfo, CurrentActivationInfo, true, false);
        }
    }
}


bool UEMRGameplayAbility_UseIntravenousMedication::TryCompleteTreatment(AEMRPatient& Patient) const
{
    if (!Patient.CanReceiveTreatment())
    {
        return false;
    }

    if (!GetWorld() || !GetWorld()->GetGameInstance())
    {
        return false;
    }

    UEMRTreatmentSubsystem* TreatmentSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>();
    if (!TreatmentSubsystem)
    {
        return false;
    }

    const FGameplayTag TreatmentTag = EMRTags::Abilities::Treatment::IntravenousMedication;
    const FGameplayTagContainer PatientTreatmentsNeeded = TreatmentSubsystem->GetTreatmentsFromPatient(&Patient);

    if (!PatientTreatmentsNeeded.HasTagExact(TreatmentTag))
    {
        return false;
    }

    if (HasAuthority(&CurrentActivationInfo))
    {
        TreatmentSubsystem->CompleteTreatmentForPatient(&Patient, TreatmentTag, true);
    }

    return true;
}
