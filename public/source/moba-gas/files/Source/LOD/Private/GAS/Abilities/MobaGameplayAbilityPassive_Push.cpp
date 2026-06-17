

#include "MobaGameplayAbilityPassive_Push.h"

#include "GAS/MobaGameplayTags.h"


UMobaGameplayAbilityPassive_Push::UMobaGameplayAbilityPassive_Push()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = MobaPushEventTags::Ability_Passive_Push;

	AbilityTriggers.Add(TriggerData);

	ActivationBlockedTags.RemoveTag(MobaStatusTags::Status_Stun);
}


void UMobaGameplayAbilityPassive_Push::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!HasAuthority(&ActivationInfo)) { return; }

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!TriggerEventData) { return; }

	const AActor* Instigator = TriggerEventData->Instigator;
	
	const FGameplayTag TriggerTag = TriggerEventData->EventTag;
	if (!TriggerTag.IsValid()) { return; }
	
	const FVector PushVelocity = LaunchProfilesMap.FindRef(TriggerTag);
	const FVector FinalVelocity = Instigator->GetActorTransform().TransformVector(PushVelocity);
	
	PushSelf(FinalVelocity);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}