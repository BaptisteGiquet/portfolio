
#include "GAS/Abilities/EMRGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Characters/Patient/EMRPatient.h"
#include "GameFramework/Character.h"
#include "Shop/EMRItemActor.h"
#include "Shop/EMRItemData.h"
#include "Sound/SoundBase.h"


UEMRGameplayAbility::UEMRGameplayAbility()
{

}


bool UEMRGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const FGameplayAbilitySpec* AbilitySpec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	if (AbilitySpec && AbilitySpec->Level <= 0)
	{
		return false;
	}
	
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}



ACharacter* UEMRGameplayAbility::GetAvatarCharacter()
{
	if (!AvatarCharacter)
	{
		AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	}

	return AvatarCharacter;
}

AEMRItemActor* UEMRGameplayAbility::GetTriggerItemActor(const FGameplayEventData* TriggerEventData) const
{
	if (!TriggerEventData)
	{
		return nullptr;
	}

	if (AEMRItemActor* ItemActor = const_cast<AEMRItemActor*>(Cast<AEMRItemActor>(TriggerEventData->OptionalObject)))
	{
		return ItemActor;
	}

	return const_cast<AEMRItemActor*>(Cast<AEMRItemActor>(TriggerEventData->Target));
}

bool UEMRGameplayAbility::TryConsumeTriggerItemForWaitingPatient(const FGameplayEventData* TriggerEventData, const AEMRPatient* Patient, const FGameplayAbilityActivationInfo& ActivationInfo) const
{
	if (!HasAuthority(&ActivationInfo) || !Patient || !Patient->CanReceiveTreatment())
	{
		return false;
	}

	AEMRItemActor* ItemActor = GetTriggerItemActor(TriggerEventData);
	if (!ItemActor)
	{
		return false;
	}

	const UEMRItemData* ItemData = ItemActor->GetItemData();
	if (!ItemData || !ItemData->IsItemConsumable())
	{
		return false;
	}

	return ItemActor->Destroy();
}

bool UEMRGameplayAbility::TryPlayTriggerItemUseSoundForInstigator(
	const FGameplayEventData* TriggerEventData,
	const FGameplayAbilityActivationInfo& ActivationInfo) const
{
	if (!HasAuthority(&ActivationInfo))
	{
		return false;
	}

	const AEMRItemActor* ItemActor = GetTriggerItemActor(TriggerEventData);
	if (!ItemActor)
	{
		return false;
	}

	const UEMRItemData* ItemData = ItemActor->GetItemData();
	if (!ItemData)
	{
		return false;
	}

	USoundBase* UseSound = ItemData->GetUseInteractionSound();
	if (!UseSound)
	{
		return false;
	}

	AEMRPlayerCharacter* InstigatorCharacter = TriggerEventData
	? const_cast<AEMRPlayerCharacter*>(Cast<AEMRPlayerCharacter>(TriggerEventData->Instigator))
	: nullptr;
	if (!InstigatorCharacter)
	{
		InstigatorCharacter = Cast<AEMRPlayerCharacter>(GetAvatarActorFromActorInfo());
	}

	if (!InstigatorCharacter)
	{
		return false;
	}

	InstigatorCharacter->PlayWorldSoundForAllPlayers(UseSound, InstigatorCharacter->GetActorLocation());
	return true;
}
