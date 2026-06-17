#include "GAS/Abilities/EMRGameplayAbility_UseIVBag.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRIntravenousMedicationStand.h"
#include "Shop/EMRItemActor.h"

UEMRGameplayAbility_UseIVBag::UEMRGameplayAbility_UseIVBag()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Event::Intravenous::UseBag;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_UseIVBag::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!HasAuthority(&ActivationInfo) || !TriggerEventData)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRPlayerCharacter* Player = ActorInfo ? Cast<AEMRPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    AEMRIntravenousMedicationStand* Stand = TriggerEventData
    ? const_cast<AEMRIntravenousMedicationStand*>(Cast<AEMRIntravenousMedicationStand>(TriggerEventData->Target))
    : nullptr;
    AEMRItemActor* BagActor = TriggerEventData
    ? const_cast<AEMRItemActor*>(Cast<AEMRItemActor>(TriggerEventData->OptionalObject))
    : nullptr;

    if (!Player || !Stand || !BagActor)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (Stand->TryInstallBag(Player, BagActor))
    {
        TryPlayTriggerItemUseSoundForInstigator(TriggerEventData, ActivationInfo);
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
