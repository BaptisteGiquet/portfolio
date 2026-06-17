#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/EMRTags.h"
#include "EMRGameplayAbility_SelectNightShift.generated.h"

class UEMRNightShiftDefinition;
class AEMRPlayerState;
class AEMRNightShiftGameState;


USTRUCT(BlueprintType)
struct FEMRNightShiftSelectionMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGameplayTag MessageTag;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UEMRNightShiftDefinition> SelectedDefinition = nullptr;

	UPROPERTY(BlueprintReadWrite)
	bool bSelectionLocked = false;
};


USTRUCT()
struct FNighShiftPendingTagHandles
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<AEMRPlayerState> PlayerState = nullptr;

    UPROPERTY()
    TArray<FActiveGameplayEffectHandle> Handles;
};

/**
 * Server-authoritative ability that validates and caches hub NightShift selections.
 */
UCLASS()
class LOD_API UEMRGameplayAbility_SelectNightShift : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_SelectNightShift();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    UEMRNightShiftDefinition* ResolveDefinitionFromOfferIndex(const FGameplayAbilityActorInfo* ActorInfo, int32 OfferIndex) const;

    void CacheSelectionOnServer(const FGameplayAbilityActorInfo* ActorInfo, UEMRNightShiftDefinition* Definition) const;

	bool ValidateSelection(const FGameplayAbilityActorInfo* ActorInfo, UEMRNightShiftDefinition* Definition) const;
	
    void ApplyPendingNightShiftTag(AEMRPlayerState* PlayerState) const;
    void RemovePendingNightShiftTags(AEMRPlayerState* PlayerState) const;
    void ClearAllPendingTags() const;

    UPROPERTY()
    mutable TArray<FNighShiftPendingTagHandles> PendingTagEffects;

    UPROPERTY(EditDefaultsOnly, Category="EMR|Ability|Tags")
    FGameplayTag SelectionLockedTag = EMRTags::GameMode::Hub::PendingNightShift;

    void CancelAbilityWithError(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        FName ErrorTag = NAME_None) const;
};
