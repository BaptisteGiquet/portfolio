

#include "MobaGameplayAbility_BlackHole.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS/MobaTargetActor_PickGroundTarget.h"
#include "GAS/Abilities/MobaTargetActor_BlackHole.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GAS/MobaGameplayTags.h"


void UMobaGameplayAbility_BlackHole::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	PlayCastBlackHoleMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("PlayCastBlackHole"), BlackHoleTargetingMontage);
	PlayCastBlackHoleMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
	PlayCastBlackHoleMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
	PlayCastBlackHoleMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
	PlayCastBlackHoleMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
	PlayCastBlackHoleMontageTask->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitPlacementDataTask = UAbilityTask_WaitTargetData::WaitTargetData(this, TEXT("WaitPlacementData"), EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	WaitPlacementDataTask->ValidData.AddDynamic(this, &ThisClass::PlaceBlackHole);
	WaitPlacementDataTask->Cancelled.AddDynamic(this, &ThisClass::PlacementCanceled);
	WaitPlacementDataTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitPlacementDataTask->BeginSpawningActor(this, TargetActorClass, TargetActor);

	AMobaTargetActor_PickGroundTarget* GroundPickTargetActor = Cast<AMobaTargetActor_PickGroundTarget>(TargetActor);
	if (GroundPickTargetActor)
	{
		GroundPickTargetActor->SetShouldDrawDebug(bShouldDrawDebug);
		GroundPickTargetActor->SetTargetingAreaRadius(BlackHoleAreaRadius);
		GroundPickTargetActor->SetTargetingMaxRange(TargetingMaxRange);
	}

	WaitPlacementDataTask->FinishSpawningActor(this, TargetActor);
	AddAimEffect();
}

void UMobaGameplayAbility_BlackHole::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemoveAimEffect();
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UMobaGameplayAbility_BlackHole::OnMontageEnded()
{
	UE_LOG(LogTemp, Warning, TEXT("OnMontageEnded"))
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


void UMobaGameplayAbility_BlackHole::PlaceBlackHole(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	RemoveAimEffect();
	
	if (PlayCastBlackHoleMontageTask)
	{
		PlayCastBlackHoleMontageTask->OnBlendOut.RemoveAll(this);
		PlayCastBlackHoleMontageTask->OnCancelled.RemoveAll(this);
		PlayCastBlackHoleMontageTask->OnInterrupted.RemoveAll(this);
		PlayCastBlackHoleMontageTask->OnCompleted.RemoveAll(this);
	}

	if (HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo) && HoldBlackHoleMontage)
	{
		UAbilityTask_PlayMontageAndWait* PlayHoldBlackHoleMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("PlayHoldBlackHoleMontage"), HoldBlackHoleMontage);
		PlayHoldBlackHoleMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayHoldBlackHoleMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayHoldBlackHoleMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayHoldBlackHoleMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayHoldBlackHoleMontageTask->ReadyForActivation();
	}
	
	CachedTargetDataHandle = TargetDataHandle;
	
	UAbilityTask_WaitDelay* WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, 0.1f);
	WaitDelayTask->OnFinish.AddDynamic(this, &ThisClass::SpawnBlackHoleTargetActor);
	WaitDelayTask->ReadyForActivation();
}


void UMobaGameplayAbility_BlackHole::PlacementCanceled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}


void UMobaGameplayAbility_BlackHole::OnValidData(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	UE_LOG(LogTemp, Warning, TEXT("OnValidData"))
	if (!FinalBlowMontage) { return; }
	
	if (HasAuthority(&CurrentActivationInfo))
	{
		TArray<AActor*> TargetedActors = UAbilitySystemBlueprintLibrary::GetAllActorsFromTargetData(TargetDataHandle);
		
		for (AActor* Target : TargetedActors)
		{
			const FGameplayAbilityProperties& Properties = AbilityPropertiesMap.FindRef(MobaPhaseBlackHoleTags::Ability_Phase_BlackHole_Damage);
	
			for (const TSubclassOf<UGameplayEffect>& DamageEffect : Properties.DamageEffects)
			{
				const FGameplayEffectSpecHandle GameplayEffectSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
		
				ApplyGameplayEffectSpecToTarget(GetCurrentAbilitySpecHandle(), CurrentActorInfo, CurrentActivationInfo, GameplayEffectSpecHandle, TargetDataHandle);
			}

			FGameplayEventData EventData;
			EventData.Target = Target;
			EventData.TargetData = TargetDataHandle;
			EventData.Instigator = GetAvatarActorFromActorInfo();
			PushTarget(Target, MobaPushEventTags::Ability_Passive_Push_BlackHole, EventData);	
		}
		

		UAbilityTask_PlayMontageAndWait* PlayBlowMontageAndWaitTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("PlayBlowMontageAndWait"), FinalBlowMontage);
		PlayBlowMontageAndWaitTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayBlowMontageAndWaitTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayBlowMontageAndWaitTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayBlowMontageAndWaitTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
		PlayBlowMontageAndWaitTask->ReadyForActivation();
	}
	else
	{
		PlayMontageLocally(FinalBlowMontage);	
	}
}


void UMobaGameplayAbility_BlackHole::OnCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	UE_LOG(LogTemp, Warning, TEXT("OnCancelled"))
}


void UMobaGameplayAbility_BlackHole::SpawnBlackHoleTargetActor()
{
	BlackHoleTargetingTask = UAbilityTask_WaitTargetData::WaitTargetData(this, TEXT("BlackHoleBlow"), EGameplayTargetingConfirmation::UserConfirmed, BlackHoleTargetActorClass);
	BlackHoleTargetingTask->ValidData.AddDynamic(this, &ThisClass::OnValidData);
	BlackHoleTargetingTask->Cancelled.AddDynamic(this, &ThisClass::OnCancelled);
	BlackHoleTargetingTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	BlackHoleTargetingTask->BeginSpawningActor(this, BlackHoleTargetActorClass, TargetActor);
	
	AMobaTargetActor_BlackHole* BlackHoleTargetActor = Cast<AMobaTargetActor_BlackHole>(TargetActor);
	if (BlackHoleTargetActor)
	{
		BlackHoleTargetActor->ConfigureBlackHole(BlackHoleAreaRadius, BlackHolePullSpeed, BlackHoleDuration, GetOwnerTeamID());
	}
	
	BlackHoleTargetingTask->FinishSpawningActor(this, TargetActor);
	
	if (BlackHoleTargetActor)
	{
		BlackHoleTargetActor->SetActorLocation(UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(CachedTargetDataHandle, 1).ImpactPoint);
	}
}


void UMobaGameplayAbility_BlackHole::AddAimEffect()
{
	if (AimEffectClass)
	{
		AimEffectHandle = ApplyGameplayEffectToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, AimEffectClass.GetDefaultObject(), GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));	
	}
}


void UMobaGameplayAbility_BlackHole::RemoveAimEffect()
{
	if (AimEffectHandle.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveActiveGameplayEffect(AimEffectHandle);
	}
}

