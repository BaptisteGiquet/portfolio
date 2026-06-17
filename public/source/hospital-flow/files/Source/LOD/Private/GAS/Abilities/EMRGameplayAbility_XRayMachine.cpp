#include "GAS/Abilities/EMRGameplayAbility_XRayMachine.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRXRayMachine.h"

UEMRGameplayAbility_XRayMachine::UEMRGameplayAbility_XRayMachine()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Machine::XRay;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_XRayMachine::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UE_LOG(LogTemp, Log, TEXT("[XRayAbility] ActivateAbility: %s"), *GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr));

    if (!HasAuthority(&ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayAbility] ActivateAbility: no authority"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRPlayerCharacter* PlayerCharacter = ActorInfo ? Cast<AEMRPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    AEMRXRayMachine* XRayMachine = TriggerEventData
    ? const_cast<AEMRXRayMachine*>(Cast<AEMRXRayMachine>(TriggerEventData->OptionalObject))
    : nullptr;

    if (!PlayerCharacter || !XRayMachine)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayAbility] ActivateAbility: missing player or machine (player=%s, machine=%s)"),
            *GetNameSafe(PlayerCharacter), *GetNameSafe(XRayMachine));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (XRayMachine->GetCurrentOperator() == PlayerCharacter)
    {
        UE_LOG(LogTemp, Log, TEXT("[XRayAbility] ActivateAbility: operator toggling off %s"), *GetNameSafe(XRayMachine));
        XRayMachine->StopExam(PlayerCharacter, true);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[XRayAbility] ActivateAbility: start exam on %s"), *GetNameSafe(XRayMachine));
    XRayMachine->TryStartExam(PlayerCharacter);

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
