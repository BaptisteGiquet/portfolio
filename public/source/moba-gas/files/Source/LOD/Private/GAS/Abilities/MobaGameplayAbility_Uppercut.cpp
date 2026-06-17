

#include "MobaGameplayAbility_Uppercut.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitConfirmCancel.h"
#include "MobaGameplayTypes.h"
#include "GAS/MobaGameplayTags.h"


UMobaGameplayAbility_Uppercut::UMobaGameplayAbility_Uppercut()
{
	FGameplayTagContainer CurrentAbilityTags = GetAssetTags();
	CurrentAbilityTags.AddTag(MobaGameplayAbilityPressedTags::Ability_AbilityOne_Pressed);
	SetAssetTags(CurrentAbilityTags);

	BlockAbilitiesWithTag.AddTag(MobaGameplayAbilityPressedTags::Ability_AbilityOne_Pressed);
	BlockAbilitiesWithTag.AddTag(MobaGameplayAbilityPressedTags::Ability_BasicAttack_Pressed);
}




void UMobaGameplayAbility_Uppercut::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;	
		}

		PlayUppercutMontageAndWaitTask();
		WaitUppercutPhaseChangeTask();
	}

	if (HasAuthority(&ActivationInfo))
	{
		WaitTargetsPositionsForUppercutLaunchTask();
	}
}




void UMobaGameplayAbility_Uppercut::WaitTargetsPositionsForUppercutLaunchTask()
{
	UAbilityTask_WaitGameplayEvent* WaitTargetsPositionsTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MobaCrunchDamageEventTags::Ability_Crunch_Uppercut_Damage, nullptr, false, false);
	WaitTargetsPositionsTask->EventReceived.AddDynamic(this, &UMobaGameplayAbility_Uppercut::ApplyAllGameplayEffectsToTarget);
	WaitTargetsPositionsTask->ReadyForActivation();
}




void UMobaGameplayAbility_Uppercut::PlayUppercutMontageAndWaitTask()
{
	UAbilityTask_PlayMontageAndWait* PlayUppercutMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("UppercutMontage"), UppercutMontage);
	PlayUppercutMontageTask->OnBlendOut.AddDynamic(this, &UMobaGameplayAbility_Uppercut::K2_EndAbility);
	PlayUppercutMontageTask->OnCancelled.AddDynamic(this, &UMobaGameplayAbility_Uppercut::K2_EndAbility);
	PlayUppercutMontageTask->OnInterrupted.AddDynamic(this, &UMobaGameplayAbility_Uppercut::K2_EndAbility);
	PlayUppercutMontageTask->OnCompleted.AddDynamic(this, &UMobaGameplayAbility_Uppercut::K2_EndAbility);
	PlayUppercutMontageTask->ReadyForActivation();
}




void UMobaGameplayAbility_Uppercut::WaitUppercutPhaseChangeTask()
{
	UAbilityTask_WaitGameplayEvent* WaitUppercutChangeTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MobaAbilityCrunchUppercutChangeTags::Ability_Crunch_Uppercut_Change, nullptr, false, false);
	WaitUppercutChangeTask->EventReceived.AddDynamic(this, &UMobaGameplayAbility_Uppercut::OnUppercutPhaseChangedGameplayEventReceived);
	WaitUppercutChangeTask->ReadyForActivation();
}




void UMobaGameplayAbility_Uppercut::OnUppercutPhaseChangedGameplayEventReceived(FGameplayEventData EventData)
{
	const FGameplayTag EventTag = EventData.EventTag;

	if (EventTag.MatchesTagExact(MobaAbilityCrunchUppercutChangeTags::Ability_Crunch_Uppercut_Change_End))
	{
		NextUppercutPhase = NAME_None;
		return;
	}
	TArray<FName> EventTagNames;
	UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, EventTagNames);
	NextUppercutPhase = EventTagNames.Last();

	StartClicConfirmWindow();
}





void UMobaGameplayAbility_Uppercut::StartClicConfirmWindow()
{
	UAbilityTask_WaitConfirmCancel* WaitBasicAttackPressedTask = UAbilityTask_WaitConfirmCancel::WaitConfirmCancel(this);
	WaitBasicAttackPressedTask->OnConfirm.AddDynamic(this, &UMobaGameplayAbility_Uppercut::HandleBasicAttackPressed);
	WaitBasicAttackPressedTask->ReadyForActivation();
}




void UMobaGameplayAbility_Uppercut::HandleBasicAttackPressed()
{
	if (NextUppercutPhase == NAME_None) { return; }
	
	UAnimInstance* AvatarAnimInstance = GetAvatarAnimInstance();
	if (!AvatarAnimInstance) { return; }
	
	AvatarAnimInstance->Montage_SetNextSection(AvatarAnimInstance->Montage_GetCurrentSection(UppercutMontage), NextUppercutPhase, UppercutMontage);
}




void UMobaGameplayAbility_Uppercut::ApplyAllGameplayEffectsToTarget(FGameplayEventData EventData)
{
	if (AbilityPropertiesMap.Num() == 0) { return; }
	
	FGameplayTag PushSelfEventTag = AbilityPropertiesMap.FindRef(EventData.EventTag).PushSelfEventTag;
	if (!PushSelfEventTag.IsValid()) { return; }
	
	PushTarget(GetAvatarActorFromActorInfo(), PushSelfEventTag, EventData);

	
	FGameplayTag PushTargetEventTag = AbilityPropertiesMap.FindRef(EventData.EventTag).PushTargetEventTag;
	if (!PushTargetEventTag.IsValid()) { return; }
	
	const int32 HitResultCount = UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(EventData.TargetData);
	
	for (int32 HitResultIndex = 0; HitResultIndex < HitResultCount; ++HitResultIndex)
	{
		FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(EventData.TargetData, HitResultIndex);
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor) { continue; }

		FGameplayEventData PushEventData;
		PushEventData.Instigator = GetAvatarActorFromActorInfo();
		PushEventData.EventTag = PushTargetEventTag;
		PushEventData.Target = HitActor;
		PushEventData.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);
		
		PushTarget(HitActor, PushTargetEventTag, PushEventData);
		
		
		for (TSubclassOf<UGameplayEffect> GameplayEffectToApply : AbilityPropertiesMap.FindRef(EventData.EventTag).DamageEffects)
		{
			ApplyGameplayEffectToHitResultActor(HitResult, GameplayEffectToApply, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));	
		}
	}
}

