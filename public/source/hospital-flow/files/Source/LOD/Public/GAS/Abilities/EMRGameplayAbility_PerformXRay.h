
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "EMRGameplayAbility_PerformXRay.generated.h"

class AEMRPatient;

/**
 * Gameplay Ability pour valider et compléter un examen XRay
 * Conditions: Puzzle terminé + Patient assigné + Machine disponible
 */

UCLASS()
class LOD_API UEMRGameplayAbility_PerformXRay : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_PerformXRay();
    void LogAbilityTriggers(const TCHAR* Context) const;

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay", meta = (Categories = "EMR.Abilities.Exam"))
    FGameplayTag XRayExamTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay")
    float CompletionAnimationDuration = 1.0f;

	void ApplyEffectToPatient(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* PatientASC, const TSubclassOf<UGameplayEffect>& EffectClass) const;
};
