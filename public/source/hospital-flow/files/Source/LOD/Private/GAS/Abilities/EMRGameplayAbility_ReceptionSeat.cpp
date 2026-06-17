#include "GAS/Abilities/EMRGameplayAbility_ReceptionSeat.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRReceptionMachine.h"

UEMRGameplayAbility_ReceptionSeat::UEMRGameplayAbility_ReceptionSeat()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Machine::Reception;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_ReceptionSeat::ActivateAbility(
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
    AEMRReceptionMachine* ReceptionMachine = const_cast<AEMRReceptionMachine*>(Cast<AEMRReceptionMachine>(TriggerEventData->OptionalObject));
    if (!PlayerCharacter || !ReceptionMachine)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (PlayerCharacter->IsSeated())
    {
        if (ReceptionMachine->GetSeatedPlayer() == PlayerCharacter)
        {
            ReceptionMachine->UnseatPlayer(PlayerCharacter);
        }

        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!ReceptionMachine->GetSeatedPlayer())
    {
        ReceptionMachine->SeatPlayer(PlayerCharacter);
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
