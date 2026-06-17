
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "EMRGameplayAbility.generated.h"

class UAnimInstance;
class AEMRPatient;
class AEMRItemActor;
class USoundBase;
struct FGameplayEventData;

UCLASS()
class UEMRGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UEMRGameplayAbility();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	
	ACharacter* GetAvatarCharacter();
	AEMRItemActor* GetTriggerItemActor(const FGameplayEventData* TriggerEventData) const;
	bool TryConsumeTriggerItemForWaitingPatient(const FGameplayEventData* TriggerEventData, const AEMRPatient* Patient, const FGameplayAbilityActivationInfo& ActivationInfo) const;
	bool TryPlayTriggerItemUseSoundForInstigator(const FGameplayEventData* TriggerEventData, const FGameplayAbilityActivationInfo& ActivationInfo) const;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "EMR|Debug")
	bool bDrawDebug = false;
	
private:
	UPROPERTY()
	TObjectPtr<ACharacter> AvatarCharacter;
};
