#include "GAS/Abilities/EMRGameplayAbility_UltrasoundMachine.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRUltrasoundMachine.h"

UEMRGameplayAbility_UltrasoundMachine::UEMRGameplayAbility_UltrasoundMachine()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Machine::Ultrasound;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_UltrasoundMachine::ActivateAbility(
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
    AEMRUltrasoundMachine* UltrasoundMachine = TriggerEventData
    ? const_cast<AEMRUltrasoundMachine*>(Cast<AEMRUltrasoundMachine>(TriggerEventData->OptionalObject))
    : nullptr;

    if (!PlayerCharacter || !UltrasoundMachine)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (UltrasoundMachine->GetCurrentOperator() == PlayerCharacter)
    {
        UltrasoundMachine->StopExam(PlayerCharacter, true);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UltrasoundMachine->TryStartExam(PlayerCharacter);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
