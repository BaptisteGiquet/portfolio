#include "GAS/Abilities/EMRGameplayAbility_CTScanMachine.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRCTScanMachine.h"

UEMRGameplayAbility_CTScanMachine::UEMRGameplayAbility_CTScanMachine()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Machine::CTScan;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_CTScanMachine::ActivateAbility(
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
    AEMRCTScanMachine* CTScanMachine = TriggerEventData
    ? const_cast<AEMRCTScanMachine*>(Cast<AEMRCTScanMachine>(TriggerEventData->OptionalObject))
    : nullptr;

    if (!PlayerCharacter || !CTScanMachine)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (CTScanMachine->GetCurrentOperator() == PlayerCharacter)
    {
        CTScanMachine->StopExam(PlayerCharacter, true);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    CTScanMachine->TryStartExam(PlayerCharacter);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
