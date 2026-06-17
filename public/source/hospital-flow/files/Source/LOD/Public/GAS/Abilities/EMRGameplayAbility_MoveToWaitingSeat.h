#pragma once

#include "CoreMinimal.h"
#include "EMRGameplayAbility.h"
#include "EMRGameplayAbility_MoveToWaitingSeat.generated.h"

class UEMRWaitingSeatComponent;
class UEMRWaitingRoomManagerComponent;

/**
 * Simple ability that ensures a patient has a reserved waiting seat and publishes a navigation intent towards it.
 */
UCLASS()
class LOD_API UEMRGameplayAbility_MoveToWaitingSeat : public UEMRGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_MoveToWaitingSeat();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
    UEMRWaitingRoomManagerComponent* FindWaitingRoomManager(const UWorld* World) const;
    UEMRWaitingSeatComponent* EnsureSeatReservation(AActor* PatientActor, UEMRWaitingRoomManagerComponent* Manager) const;
    void SendNavigationIntent(class AEMRPatient* Patient, UEMRWaitingSeatComponent* Seat) const;
};
