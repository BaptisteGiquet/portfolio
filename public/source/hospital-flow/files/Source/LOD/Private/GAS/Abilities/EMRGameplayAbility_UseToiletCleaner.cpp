#include "GAS/Abilities/EMRGameplayAbility_UseToiletCleaner.h"

#include "Environment/EMRToiletSlotActor.h"
#include "GAS/EMRTags.h"
#include "Shop/EMRItemActor.h"
#include "Shop/EMRItemData.h"
#include "Subsystems/EMRToiletSubsystem.h"

UEMRGameplayAbility_UseToiletCleaner::UEMRGameplayAbility_UseToiletCleaner()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Event::Item::UseToiletCleaner;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_UseToiletCleaner::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!HasAuthority(&ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!TriggerEventData)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    const AEMRToiletSlotActor* Slot = Cast<AEMRToiletSlotActor>(TriggerEventData->Target);
    if (!Slot || !Slot->IsDirty())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (UEMRToiletSubsystem* ToiletSubsystem = GetWorld()->GetSubsystem<UEMRToiletSubsystem>())
    {
        ToiletSubsystem->CleanSlot(const_cast<AEMRToiletSlotActor*>(Slot));
        TryPlayTriggerItemUseSoundForInstigator(TriggerEventData, ActivationInfo);
    }

    if (AEMRItemActor* ItemActor = GetTriggerItemActor(TriggerEventData))
    {
        if (const UEMRItemData* ItemData = ItemActor->GetItemData())
        {
            if (ItemData->IsItemConsumable())
            {
                ItemActor->Destroy();
            }
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
