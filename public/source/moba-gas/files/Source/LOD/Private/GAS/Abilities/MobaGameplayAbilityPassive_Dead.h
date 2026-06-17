
#pragma once

#include "CoreMinimal.h"
#include "MobaGameplayAbility.h"
#include "MobaGameplayAbilityPassive_Dead.generated.h"


UCLASS()
class LOD_API UMobaGameplayAbilityPassive_Dead : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaGameplayAbilityPassive_Dead();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	bool ShouldOverlappedActorReceiveRewards(const IGenericTeamAgentInterface* VictimTeamInterface, AActor* OverlappedActor) const;

private:
	TArray<AActor*> GetTargetsToReward() const;
	
	void GenerateSphereOverlap(AActor* VictimAvatar, TArray<FOverlapResult>& OverlapResults) const;
	
	void RewardTheKiller(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, AActor* Killer, float& TotalExperience, float& TotalGold);
	void RewardOtherKillParticipants(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, TArray<AActor*>& TargetsToReward, float TotalExperience, float TotalGold);

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reward")
	TSubclassOf<UGameplayEffect> RewardEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reward")
	float RewardAreaRadius = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reward")
	float BaseExperienceReward = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reward")
	float BaseGoldReward = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reward")
	float ExperienceRewardPerVictimExperiencePoint = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reward")
	float GoldRewardPerVictimExperiencePoint = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Reward")
	float KillerRewardPortion = 0.5f;
};
