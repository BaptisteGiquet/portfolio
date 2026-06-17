
#include "GAS/Abilities/MobaGameplayAbility_Combo.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/MobaGameplayTags.h"
#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"



UMobaGameplayAbility_Combo::UMobaGameplayAbility_Combo()
{
	// Set Ability.BasicAttack tag to this Ability, And block ability that has the same tag, prevent spamming the same ability
	FGameplayTagContainer CurrentAbilityTags = GetAssetTags();
	CurrentAbilityTags.AddTag(MobaGameplayAbilityPressedTags::Ability_BasicAttack_Pressed);
	SetAssetTags(CurrentAbilityTags);
	
	BlockAbilitiesWithTag.AddTag(MobaGameplayAbilityPressedTags::Ability_BasicAttack_Pressed);
}




void UMobaGameplayAbility_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// On server and Owningclient
	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		// Will check if we can use the ability (think about commit before push)
		if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}

		PlayComboMontageAndWaitTask();
		WaitComboChangeTask();
		WaitSameInputPressTask();
	}

	// Only on server
	if (HasAuthority(&ActivationInfo))
	{
		UAbilityTask_WaitGameplayEvent* WaitTargetsPositionsTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MobaCrunchDamageEventTags::Ability_Crunch_Combo_Damage);
		WaitTargetsPositionsTask->EventReceived.AddDynamic(this, &UMobaGameplayAbility_Combo::DoDamage);
		WaitTargetsPositionsTask->ReadyForActivation();
	}
}




void UMobaGameplayAbility_Combo::PlayComboMontageAndWaitTask()
{
	UAbilityTask_PlayMontageAndWait* PlayComboMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("ComboMontage"), ComboMontage);
	PlayComboMontageTask->OnBlendOut.AddDynamic(this, &UMobaGameplayAbility_Combo::K2_EndAbility);
	PlayComboMontageTask->OnCancelled.AddDynamic(this, &UMobaGameplayAbility_Combo::K2_EndAbility);
	PlayComboMontageTask->OnInterrupted.AddDynamic(this, &UMobaGameplayAbility_Combo::K2_EndAbility);
	PlayComboMontageTask->OnCompleted.AddDynamic(this, &UMobaGameplayAbility_Combo::K2_EndAbility);
	PlayComboMontageTask->ReadyForActivation();
}




void UMobaGameplayAbility_Combo::WaitComboChangeTask()
{
	// Wait for a GameplayEvent to come (Here notify from AM_Combo)
	UAbilityTask_WaitGameplayEvent* WaitComboChangeTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MobaAbilityCrunchComboChangeTags::Ability_Crunch_Combo_Change, nullptr, false, false);
	WaitComboChangeTask->EventReceived.AddDynamic(this, &UMobaGameplayAbility_Combo::OnComboChangeGameplayEventReceived);
	WaitComboChangeTask->ReadyForActivation();
}




void UMobaGameplayAbility_Combo::WaitSameInputPressTask()
{
	// This task will wait until the same input as the ability is pressed again
	UAbilityTask_WaitInputPress* WaitSameInputPressTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitSameInputPressTask->OnPress.AddDynamic(this, &UMobaGameplayAbility_Combo::HandleNewInputPress);
	WaitSameInputPressTask->ReadyForActivation();
}




void UMobaGameplayAbility_Combo::TryCommitCombo()
{
	if (NextComboName == NAME_None)
	{
		return;
	}
	
	UAnimInstance* AvatarAnimInstance = GetAvatarAnimInstance();
	if (!AvatarAnimInstance)
	{
		return;
	}

	AvatarAnimInstance->Montage_SetNextSection(AvatarAnimInstance->Montage_GetCurrentSection(ComboMontage), NextComboName, ComboMontage);
}




TSubclassOf<UGameplayEffect> UMobaGameplayAbility_Combo::GetGameplayEffectForCurrentCombo()
{
	const UAnimInstance* AvatarAnimInstance = GetAvatarAnimInstance();
	if (AvatarAnimInstance && ComboEffectMap.Num() > 0)
	{
		const FName CurrentComboSection = AvatarAnimInstance->Montage_GetCurrentSection(ComboMontage);
		if (CurrentComboSection == NAME_None) { return DefaultEffect; }
		
		TSubclassOf<UGameplayEffect>* GameplayEffect = ComboEffectMap.Find(CurrentComboSection);
		if (GameplayEffect)
		{
			return *GameplayEffect;	
		}
	}
	return DefaultEffect;
}




void UMobaGameplayAbility_Combo::DoDamage(FGameplayEventData EventData)
{
	const TSubclassOf<UGameplayEffect> GameplayEffectClass = GetGameplayEffectForCurrentCombo();
	if (!GameplayEffectClass) { return; }
	
	EventData.Instigator = GetAvatarActorFromActorInfo();
	
	const int32 HitResultCount = UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(EventData.TargetData);
	
	for (int32 HitResultIndex = 0; HitResultIndex < HitResultCount; ++HitResultIndex)
	{
		FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(EventData.TargetData, HitResultIndex);
		const AActor* HitActor = HitResult.GetActor();
		if (!HitActor) { continue; }

		ApplyGameplayEffectToHitResultActor(HitResult, GameplayEffectClass, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
	}
}




void UMobaGameplayAbility_Combo::HandleNewInputPress(float TimeWaited)
{
	WaitSameInputPressTask();
	TryCommitCombo();
}




void UMobaGameplayAbility_Combo::OnComboChangeGameplayEventReceived(FGameplayEventData Payload)
{
	// We get the precise Event Tag received from the notify
	const FGameplayTag& EventTag = Payload.EventTag;
	
	if (EventTag.MatchesTagExact(MobaAbilityCrunchComboChangeTags::Ability_Crunch_Combo_Change_End))
	{
		NextComboName = NAME_None;
		return;
	}
	TArray<FName> EventTagNames;
	UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, EventTagNames);
	NextComboName = EventTagNames.Last();
}
