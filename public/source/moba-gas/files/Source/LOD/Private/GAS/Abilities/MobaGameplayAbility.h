
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GenericTeamAgentInterface.h"
#include "MobaGameplayTypes.h"
#include "MobaGameplayAbility.generated.h"

class UAnimInstance;

UCLASS()
class UMobaGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaGameplayAbility();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;


	TMap<FGameplayTag, FGameplayAbilityProperties> GetAbilityPropertiesMaps() const { return AbilityPropertiesMap; }
	
protected:
	UAnimInstance* GetAvatarAnimInstance() const;
	
	TArray<FHitResult> GetHitResultFromSweepTargetData(const FGameplayAbilityTargetDataHandle& LocationTargetData, const ETeamAttitude::Type TeamAttitude = ETeamAttitude::Hostile, const float SweepRadius = 30.f, const bool bDrawDebug = false, const bool bIgnoreAvatar = true) const;

	void ApplyGameplayEffectToHitResultActor(const FHitResult& InHitResult, const TSubclassOf<UGameplayEffect>& InGameplayEffectClass, const int32 AbilityLevel);
	
	void PushSelf(const FVector& PushVelocity);
	void PushTarget(AActor* Target, const FGameplayTag PushEventTag, const FGameplayEventData EventData);

	void PlayMontageLocally(UAnimMontage* MontageToPlay);
	void StopMontageAfterCurrentSection(UAnimMontage* MontageToStop);

	AActor* GetAimTarget(const float AimDistance, const ETeamAttitude::Type TeamAttitude) const;
	bool IsActorTeamAttitude(const AActor* OtherActor, ETeamAttitude::Type TeamAttitude) const;

	FGenericTeamId GetOwnerTeamID() const;

	ACharacter* GetAvatarCharacter();

	void SendLocalGameplayEvent(const FGameplayTag& EventTag, const FGameplayEventData& EventData);

	UPROPERTY(EditAnywhere, Category = "Ability|Properties")
	TMap<FGameplayTag, FGameplayAbilityProperties> AbilityPropertiesMap;
	
private:
	UPROPERTY()
	TObjectPtr<ACharacter> AvatarCharacter;
};
