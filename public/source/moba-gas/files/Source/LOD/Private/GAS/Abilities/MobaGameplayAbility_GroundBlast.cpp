

#include "MobaGameplayAbility_GroundBlast.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GAS/MobaTargetActor_PickGroundTarget.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "GAS/MobaGameplayTags.h"



UMobaGameplayAbility_GroundBlast::UMobaGameplayAbility_GroundBlast()
{
	ActivationOwnedTags.AddTag(MobaAbilityAimingEventTags::Ability_Aiming_Crunch_GroundBlast);
	BlockAbilitiesWithTag.AddTag(MobaGameplayAbilityPressedTags::Ability_BasicAttack_Pressed);
}




void UMobaGameplayAbility_GroundBlast::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		PlayGroundBlastMontageTask();
		WaitGroundBlastTargetDataTask();
	}
}




void UMobaGameplayAbility_GroundBlast::PlayGroundBlastMontageTask()
{
	UAbilityTask_PlayMontageAndWait* PlayGroundBlastMontageAndWaitTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("GroundBlastTargetingMontage"), GroundBlastTargetingMontage);
	PlayGroundBlastMontageAndWaitTask->OnBlendOut.AddDynamic(this, &ThisClass::K2_EndAbility);
	PlayGroundBlastMontageAndWaitTask->OnCancelled.AddDynamic(this, &ThisClass::K2_EndAbility);
	PlayGroundBlastMontageAndWaitTask->OnInterrupted.AddDynamic(this, &ThisClass::K2_EndAbility);
	PlayGroundBlastMontageAndWaitTask->OnCompleted.AddDynamic(this, &ThisClass::K2_EndAbility);
	PlayGroundBlastMontageAndWaitTask->ReadyForActivation();
}




void UMobaGameplayAbility_GroundBlast::WaitGroundBlastTargetDataTask()
{
	UAbilityTask_WaitTargetData* WaitGroundBlastTargetDataTask = UAbilityTask_WaitTargetData::WaitTargetData(this, TEXT("GroundBlastTargetData"), EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	WaitGroundBlastTargetDataTask->ValidData.AddDynamic(this, &ThisClass::OnTargetConfirmed);
	WaitGroundBlastTargetDataTask->Cancelled.AddDynamic(this, &ThisClass::OnTargetCanceled);
	WaitGroundBlastTargetDataTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActorSpawned = nullptr;
	WaitGroundBlastTargetDataTask->BeginSpawningActor(this, TargetActorClass, TargetActorSpawned);
	SetupTargetActorVariables(TargetActorSpawned);
	WaitGroundBlastTargetDataTask->FinishSpawningActor(this, TargetActorSpawned);
}




void UMobaGameplayAbility_GroundBlast::SetupTargetActorVariables(AGameplayAbilityTargetActor* TargetActorSpawned)
{
	AMobaTargetActor_PickGroundTarget* PickGroundTargetActor = Cast<AMobaTargetActor_PickGroundTarget>(TargetActorSpawned);
	if (PickGroundTargetActor)
	{
		PickGroundTargetActor->SetShouldDrawDebug(bShouldDrawDebug);
		PickGroundTargetActor->SetTargetingAreaRadius(TargetingAreaRadius);
		PickGroundTargetActor->SetTargetingMaxRange(TargetingMaxRange);
		PickGroundTargetActor->SetShouldTargetFriendly(bShouldTargetFriendly);
		PickGroundTargetActor->SetShouldTargetHostile(bShouldTargetHostile);
	}
}




void UMobaGameplayAbility_GroundBlast::OnTargetConfirmed(const FGameplayAbilityTargetDataHandle& TargetData)
{
	if (!CommitAbility(GetCurrentAbilitySpecHandle(), CurrentActorInfo, CurrentActivationInfo)) { return; }

	
	TArray<AActor*> TargetedActors = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(TargetData);

	if (HasAuthority(&CurrentActivationInfo))
	{
		for (AActor* Target : TargetedActors)
		{
			HandlePushingTargets(TargetData, Target);

			const FGameplayAbilityProperties& Properties = AbilityPropertiesMap.FindRef(MobaCrunchDamageEventTags::Ability_Crunch_GroundBlast_Damage);
		
			for (const TSubclassOf<UGameplayEffect>& DamageEffect : Properties.DamageEffects)
			{
				const FGameplayEffectSpecHandle GameplayEffectSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
			
				ApplyGameplayEffectSpecToTarget(GetCurrentAbilitySpecHandle(), CurrentActorInfo, CurrentActivationInfo, GameplayEffectSpecHandle, TargetData);
			}
		}
	}

	
	
	FGameplayCueParameters BlastGameplayCueParameters;
	const int32 HitLocationIndex = 1;
	BlastGameplayCueParameters.Location = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetData, HitLocationIndex).ImpactPoint;
	BlastGameplayCueParameters.RawMagnitude = TargetingAreaRadius;

	if (!BlastGameplayCueTag.IsValid()) return;
	
	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(BlastGameplayCueTag, BlastGameplayCueParameters);
	
	UAnimInstance* AvatarAnimInstance = GetAvatarAnimInstance();
	if (AvatarAnimInstance)
	{
		AvatarAnimInstance->Montage_Play(GroundBlastCastingMontage);	
	}
	
	K2_EndAbility();
}




void UMobaGameplayAbility_GroundBlast::HandlePushingTargets(const FGameplayAbilityTargetDataHandle& TargetData, AActor* Target)
{
	FGameplayEventData EventData;
	EventData.Target = Target;
	EventData.TargetData = TargetData;
	EventData.Instigator = GetAvatarActorFromActorInfo();
	PushTarget(Target, MobaPushEventTags::Ability_Passive_Push_GroundBlast, EventData);
}




void UMobaGameplayAbility_GroundBlast::OnTargetCanceled(const FGameplayAbilityTargetDataHandle& TargetData)
{
	K2_EndAbility();
}
