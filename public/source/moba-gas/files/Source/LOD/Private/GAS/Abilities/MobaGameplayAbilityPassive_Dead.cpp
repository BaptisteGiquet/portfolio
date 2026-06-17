

#include "MobaGameplayAbilityPassive_Dead.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "GAS/MobaGameplayTags.h"
#include "GAS/MobaAbilitySystemStatics.h"
#include "GAS/Attributes/MobaAttributeSet_Hero.h"

UMobaGameplayAbilityPassive_Dead::UMobaGameplayAbilityPassive_Dead()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = MobaStatusTags::Status_Dead;

	AbilityTriggers.Add(TriggerData);

	ActivationBlockedTags.RemoveTag(MobaStatusTags::Status_Dead);
	ActivationBlockedTags.RemoveTag(MobaStatusTags::Status_Stun);
}



void UMobaGameplayAbilityPassive_Dead::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!HasAuthority(&ActivationInfo)) { return; }
	
	
	AActor* Killer = TriggerEventData->ContextHandle.GetEffectCauser();
	if (!Killer || !UMobaAbilitySystemStatics::IsHero(Killer))
	{
		Killer = nullptr;
	}

	// Early out: nothing to reward (no nearby targets and no valid killer)
	TArray<AActor*> TargetsToReward = GetTargetsToReward();
	if (TargetsToReward.Num() == 0 && Killer == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	

	bool bHasExperienceAttribute = false;
	const float VictimExperience = GetAbilitySystemComponentFromActorInfo_Ensured()->GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetExperienceAttribute(), bHasExperienceAttribute);
	float TotalExperience = 0.f;
	float TotalGold = 0.f;

	TotalExperience = BaseExperienceReward + ExperienceRewardPerVictimExperiencePoint * VictimExperience;
	TotalGold = BaseGoldReward + GoldRewardPerVictimExperiencePoint * VictimExperience;


	RewardTheKiller(Handle, ActorInfo, ActivationInfo, Killer, TotalExperience, TotalGold);
	RewardOtherKillParticipants(Handle, ActorInfo, ActivationInfo, TargetsToReward, TotalExperience, TotalGold);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}




TArray<AActor*> UMobaGameplayAbilityPassive_Dead::GetTargetsToReward() const
{
	TSet<AActor*> TargetsToReward;
	
	AActor* VictimAvatar = GetAvatarActorFromActorInfo();
	if (!VictimAvatar || !GetWorld()) { return TargetsToReward.Array(); }

	const IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(VictimAvatar);
	if (!OwnerTeamInterface) { return TargetsToReward.Array(); }

	
	TArray<FOverlapResult> OverlapResults;
	GenerateSphereOverlap(VictimAvatar, OverlapResults);
	
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* OverlappedActor = OverlapResult.GetActor();
		
		if (ShouldOverlappedActorReceiveRewards(OwnerTeamInterface, OverlappedActor))
		{
			TargetsToReward.Add(OverlappedActor);
		}
	}
	
	return TargetsToReward.Array();
}



void UMobaGameplayAbilityPassive_Dead::GenerateSphereOverlap(AActor* VictimAvatar, TArray<FOverlapResult>& OverlapResults) const
{
	const FVector StartLocation = VictimAvatar->GetActorLocation();

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(RewardAreaRadius);

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(VictimAvatar);

	GetWorld()->OverlapMultiByObjectType(OverlapResults, StartLocation, FQuat::Identity, ObjectQueryParams, CollisionShape, CollisionParams);
}



bool UMobaGameplayAbilityPassive_Dead::ShouldOverlappedActorReceiveRewards(const IGenericTeamAgentInterface* VictimTeamInterface, AActor* OverlappedActor) const
{
	if (!OverlappedActor) { return false; }
	
	const ETeamAttitude::Type VictimAttitudeTowardsTarget = VictimTeamInterface->GetTeamAttitudeTowards(*OverlappedActor);
	if (VictimAttitudeTowardsTarget != ETeamAttitude::Hostile) { return false; }
	
	return UMobaAbilitySystemStatics::IsHero(OverlappedActor);
}



void UMobaGameplayAbilityPassive_Dead::RewardTheKiller(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, AActor* Killer, float& TotalExperience, float& TotalGold)
{
	if (Killer && RewardEffect)
	{
		const float KillerExperienceReward = TotalExperience * KillerRewardPortion;
		const float KillerGoldReward = TotalGold * KillerRewardPortion;
		
		const FGameplayEffectSpecHandle RewardEffectSpecHandle = MakeOutgoingGameplayEffectSpec(RewardEffect, GetAbilityLevel());
		RewardEffectSpecHandle.Data->SetSetByCallerMagnitude(MobaDataTags::Data_Attribute_Experience, KillerExperienceReward);
		RewardEffectSpecHandle.Data->SetSetByCallerMagnitude(MobaDataTags::Data_Attribute_Gold, KillerGoldReward);

		const FGameplayAbilityTargetDataHandle KillerData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Killer);
		ApplyGameplayEffectSpecToTarget(Handle, ActorInfo, ActivationInfo, RewardEffectSpecHandle, KillerData);

		TotalExperience -= KillerExperienceReward;
		TotalGold -= KillerGoldReward;
	}
}



void UMobaGameplayAbilityPassive_Dead::RewardOtherKillParticipants(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, TArray<AActor*>& TargetsToReward, float TotalExperience, float TotalGold)
{
	if (!RewardEffect) { return; }
	
	const float ExperienceRewardPerTarget = TotalExperience / TargetsToReward.Num();
	const float GoldRewardPerTarget = TotalGold / TargetsToReward.Num();

	const float TargetActorExperienceReward = ExperienceRewardPerTarget;
	const float TargetActorGoldReward = GoldRewardPerTarget;

	const FGameplayEffectSpecHandle RewardEffectSpecHandle = MakeOutgoingGameplayEffectSpec(RewardEffect, GetAbilityLevel());
	RewardEffectSpecHandle.Data->SetSetByCallerMagnitude(MobaDataTags::Data_Attribute_Experience, TargetActorExperienceReward);
	RewardEffectSpecHandle.Data->SetSetByCallerMagnitude(MobaDataTags::Data_Attribute_Gold, TargetActorGoldReward);

	const FGameplayAbilityTargetDataHandle RewardedTargetsData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActorArray(TargetsToReward, true);
	ApplyGameplayEffectSpecToTarget(Handle, ActorInfo, ActivationInfo, RewardEffectSpecHandle, RewardedTargetsData);
}