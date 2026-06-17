#include "GAS/Abilities/EMRGameplayAbility_UseCarriedObject.h"

#include "GameFramework/Character.h"
#include "Interaction/EMRCarryableComponent.h"


UEMRGameplayAbility_UseCarriedObject::UEMRGameplayAbility_UseCarriedObject()
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}


void UEMRGameplayAbility_UseCarriedObject::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UEMRCarryableComponent* Carryable = GetCarriedComponent(*Character);
	if (!Carryable)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	FGameplayEventData UseObjectEvent;
	if (Carryable->BuildUseObjectEventData(Character, UseObjectEvent))
	{
		SendGameplayEvent(UseObjectEvent.EventTag, UseObjectEvent);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


UEMRCarryableComponent* UEMRGameplayAbility_UseCarriedObject::GetCarriedComponent(const ACharacter& Character) const
{
    TArray<AActor*> AttachedActors;
    Character.GetAttachedActors(AttachedActors, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ false);

    for (AActor* AttachedActor : AttachedActors)
    {
        if (!AttachedActor)
        {
            continue;
        }

        UEMRCarryableComponent* Carryable = AttachedActor->FindComponentByClass<UEMRCarryableComponent>();
        if (!Carryable || !Carryable->IsCarried())
        {
            continue;
        }

        if (AttachedActor->GetAttachParentActor() != &Character)
        {
            continue;
        }

        return Carryable;
    }

    return nullptr;
}
