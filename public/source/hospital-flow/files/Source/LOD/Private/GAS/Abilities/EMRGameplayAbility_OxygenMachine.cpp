#include "GAS/Abilities/EMRGameplayAbility_OxygenMachine.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMROxygenMachine.h"

UEMRGameplayAbility_OxygenMachine::UEMRGameplayAbility_OxygenMachine()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Machine::Oxygen;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_OxygenMachine::ActivateAbility(
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
    AEMROxygenMachine* Machine = TriggerEventData
    ? const_cast<AEMROxygenMachine*>(Cast<AEMROxygenMachine>(TriggerEventData->OptionalObject))
    : nullptr;

    if (!PlayerCharacter || !Machine)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    Machine->ToggleMove(PlayerCharacter);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
