#include "GAS/Abilities/EMRGameplayAbility_IntravenousMedicationStand.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRIntravenousMedicationStand.h"

UEMRGameplayAbility_IntravenousMedicationStand::UEMRGameplayAbility_IntravenousMedicationStand()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Machine::IntravenousMedication;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_IntravenousMedicationStand::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!HasAuthority(&ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRPlayerCharacter* PlayerCharacter = ActorInfo ? Cast<AEMRPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    AEMRIntravenousMedicationStand* Stand = TriggerEventData
    ? const_cast<AEMRIntravenousMedicationStand*>(Cast<AEMRIntravenousMedicationStand>(TriggerEventData->OptionalObject))
    : nullptr;

    if (!PlayerCharacter || !Stand)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    constexpr bool bEnableLegacyManualIVStandInteraction = false;
    if (!bEnableLegacyManualIVStandInteraction)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (Stand->GetCurrentOperator() == PlayerCharacter)
    {
        Stand->StopTreatment(PlayerCharacter, true);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    Stand->TryStartTreatment(PlayerCharacter);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
