#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_PlaceItem.generated.h"

class AEMRItemActor;

UCLASS()
class UEMRGameplayAbility_PlaceItem : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_PlaceItem();
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;



private:
    bool TryGetCarriedItem(const FGameplayAbilityActorInfo* ActorInfo, AEMRItemActor*& OutItem) const;
};
