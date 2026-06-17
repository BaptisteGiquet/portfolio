#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "EMRGameplayAbility_CallWaitingPatient.generated.h"

class UEMRWaitingRoomManagerComponent;
class AEMRBaseMachine;
class AEMRPatient;

/**
 * Server-only ability that validates and processes calls to bring a seated patient to reception.
 */
UCLASS()
class LOD_API UEMRGameplayAbility_CallWaitingPatient : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UEMRGameplayAbility_CallWaitingPatient();

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
    UEMRWaitingRoomManagerComponent* FindWaitingRoomManager(const UWorld* World) const;
    AEMRBaseMachine* FindReceptionMachine(AActor* ReferenceActor) const;
    bool IsPatientSeated(const AEMRPatient& Patient, const UEMRWaitingRoomManagerComponent& Manager) const;
    void SendNavigationIntent(AEMRPatient& Patient, AEMRBaseMachine& ReceptionMachine) const;
};

