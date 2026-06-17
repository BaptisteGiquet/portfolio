#pragma once

#include "GAS/Abilities/EMRGameplayAbility.h"
#include "EMRGameplayAbility_ToggleWatch.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitInputPress;
class UAnimMontage;
class AEMRPlayerCharacter;

UCLASS()
class LOD_API UEMRGameplayAbility_ToggleWatch : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_ToggleWatch();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

private:
    UFUNCTION()
    void HandleWatchMontageCompleted();

    UFUNCTION()
    void HandleWatchMontageCancelled();

    UFUNCTION()
    void HandleInputPressed(float TimeWaited);

    void BeginWaitForInput();
    void StartWatchMontage(UAnimMontage* MontageToPlay, bool bReverse, bool bReuseCurrentPosition);
    UAnimMontage* ResolveToggleWatchMontage(const AEMRPlayerCharacter* PlayerCharacter) const;
    void SetWatchSocketLookActive(bool bEnabled);

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch")
    TObjectPtr<UAnimMontage> ToggleWatchMontage = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch")
    TObjectPtr<UAnimMontage> ToggleWatchMontageSeated = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch", meta = (ClampMin = "0.05"))
    float ToggleReturnTime = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch", meta = (ClampMin = "1.0"))
    float MaxTogglePlayRate = 6.0f;

    TWeakObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask;
    TWeakObjectPtr<UAnimMontage> ActiveToggleWatchMontage;
    TWeakObjectPtr<UAbilityTask_WaitInputPress> InputPressTask;
};
