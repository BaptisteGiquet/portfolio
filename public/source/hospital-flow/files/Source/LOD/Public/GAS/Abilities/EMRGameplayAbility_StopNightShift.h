#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "EMRGameplayAbility_StopNightShift.generated.h"

class AEMRNightShiftGameMode;

/**
 * Server-only ability triggered by a gameplay event to end the current NightShift while in overtime.
 */
UCLASS()
class LOD_API UEMRGameplayAbility_StopNightShift : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_StopNightShift();

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

private:
    AEMRNightShiftGameMode* GetNightShiftGameMode() const;
    bool IsNightShiftInOvertime(const AEMRNightShiftGameMode& GameMode) const;
};
