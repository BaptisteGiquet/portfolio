
#pragma once

#include "CoreMinimal.h"
#include "MobaGameplayAbility.h"
#include "MobaGameplayAbilityPassive_Push.generated.h"


UCLASS()
class LOD_API UMobaGameplayAbilityPassive_Push : public UMobaGameplayAbility
{
	GENERATED_BODY()

public:
	UMobaGameplayAbilityPassive_Push();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Launch")
	TMap<FGameplayTag, FVector> LaunchProfilesMap;
};
