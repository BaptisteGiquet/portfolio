

#include "MobaGameplayAbility_Laser.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitCancel.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GAS/MobaGameplayTags.h"
#include "GAS/Abilities/MobaTargetActor_Line.h"
#include "GAS/Attributes/MobaAttributeSet.h"



void UMobaGameplayAbility_Laser::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !LaserMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayLaserMontageAndWait = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, LaserMontage);
		PlayLaserMontageAndWait->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayLaserMontageAndWait->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayLaserMontageAndWait->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayLaserMontageAndWait->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayLaserMontageAndWait->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitLaserShootEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MobaPhaseLaserTags::Ability_Phase_Laser_Fire);
		WaitLaserShootEvent->EventReceived.AddDynamic(this, &ThisClass::FireLaser);
		WaitLaserShootEvent->ReadyForActivation();

		UAbilityTask_WaitCancel* WaitLaserCancel = UAbilityTask_WaitCancel::WaitCancel(this);
		WaitLaserCancel->OnCancel.AddDynamic(this, &ThisClass::OnLaserCanceled);
		WaitLaserCancel->ReadyForActivation();
	}
}


void UMobaGameplayAbility_Laser::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (OwnerASC && OngoingManaCostEffectHandle.IsValid())
	{
		OwnerASC->RemoveActiveGameplayEffect(OngoingManaCostEffectHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UMobaGameplayAbility_Laser::FireLaser(FGameplayEventData EventData)
{
	if (HasAuthority(&CurrentActivationInfo))
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(OngoingManaCostEffectClass, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
		OngoingManaCostEffectHandle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);

		UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
		if (OwnerASC)
		{
			OwnerASC->GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet::GetManaAttribute()).AddUObject(this, &ThisClass::ManaUpdated);
		}	
	}

	UAbilityTask_WaitTargetData* WaitDamageTarget = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::CustomMulti, LaserTargetActorClass);
	WaitDamageTarget->ValidData.AddDynamic(this, &ThisClass::TargetReceived);
	WaitDamageTarget->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitDamageTarget->BeginSpawningActor(this, LaserTargetActorClass, TargetActor);
	

	AMobaTargetActor_Line* LineTargetActor = Cast<AMobaTargetActor_Line>(TargetActor);
	if (LineTargetActor)
	{
		LineTargetActor->ConfigureTargetSetting(TargetRange, CylinderDetectionRadius, TargetingFrequency, GetOwnerTeamID(), bShouldDrawDebug);
	}

	WaitDamageTarget->FinishSpawningActor(this, TargetActor);
	
	if (LineTargetActor)
	{
		LineTargetActor->AttachToComponent(GetOwningComponentFromActorInfo(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetActorAttachSocketName);
	}
}


void UMobaGameplayAbility_Laser::TargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (!HasAuthority(&CurrentActivationInfo)) { return; }

	const FGameplayAbilityProperties& Properties = AbilityPropertiesMap.FindRef(MobaPhaseLaserTags::Ability_Phase_Laser_Damage);
	for (const TSubclassOf<UGameplayEffect>& DamageEffect : Properties.DamageEffects)
	{
		ApplyGameplayEffectToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, TargetDataHandle, DamageEffect, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));	
	}

	TArray<AActor*> TargetedActors = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(TargetDataHandle);

	for (AActor* Target : TargetedActors)
	{
		FGameplayEventData EventData;
		EventData.Target = Target;
		EventData.TargetData = TargetDataHandle;
		EventData.Instigator = GetAvatarActorFromActorInfo();
		PushTarget(Target, MobaPushEventTags::Ability_Passive_Push_Laser, EventData);	
	}
}


void UMobaGameplayAbility_Laser::ManaUpdated(const FOnAttributeChangeData& ManaChangeData)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (OwnerASC && !OwnerASC->CanApplyAttributeModifiers(OngoingManaCostEffectClass.GetDefaultObject(), GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo), MakeEffectContext(CurrentSpecHandle, CurrentActorInfo)))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}


void UMobaGameplayAbility_Laser::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


void UMobaGameplayAbility_Laser::OnLaserCanceled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}





