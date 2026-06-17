#include "GAS/Abilities/EMRGameplayAbility_LabAnalyzerMachine.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "EMRLabAnalyzerLog.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRLabAnalyzerMachine.h"

UEMRGameplayAbility_LabAnalyzerMachine::UEMRGameplayAbility_LabAnalyzerMachine()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Machine::LabAnalyzer;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_LabAnalyzerMachine::ActivateAbility(
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
    AEMRLabAnalyzerMachine* Machine = TriggerEventData
    ? const_cast<AEMRLabAnalyzerMachine*>(Cast<AEMRLabAnalyzerMachine>(TriggerEventData->OptionalObject))
    : nullptr;

    if (!PlayerCharacter || !Machine)
    {
        EMR_LAB_LOG(Log, "LabAnalyzerMachine ability missing player or machine");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (Machine->GetCurrentOperator() == PlayerCharacter)
    {
        Machine->StopExam(PlayerCharacter, true);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    Machine->TryStartExam(PlayerCharacter);
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    return;
}
