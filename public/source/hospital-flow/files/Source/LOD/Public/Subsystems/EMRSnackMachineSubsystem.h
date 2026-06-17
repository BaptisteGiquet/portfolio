#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EMRSnackMachineSubsystem.generated.h"

class AEMRPatient;
class AEMRSnackMachineActor;
class AActor;
class UEMRDifficultyTuningData;
class UEMRWaitingRoomManagerComponent;
struct FEMRSnackMachineUpgradeTuning;

USTRUCT()
struct FEMRSnackTripState
{
    GENERATED_BODY()

    bool bHasAttempted = false;
    bool bTripActive = false;
    bool bAtMachine = false;
    bool bPendingCall = false;

    TWeakObjectPtr<AEMRSnackMachineActor> AssignedMachine;
    FTimerHandle TripFinishTimer;
    FTimerHandle DeferredCallTimer;
};

UCLASS()
class LOD_API UEMRSnackMachineSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    void RegisterMachine(AEMRSnackMachineActor* Machine);
    void UnregisterMachine(AEMRSnackMachineActor* Machine);

    bool IsSnackTripActive(const AEMRPatient* Patient) const;
    bool TryDeferCallForPatient(AEMRPatient* Patient);
    void NotifyMachineArrival(AEMRSnackMachineActor* Machine, AEMRPatient* Patient);

private:
    UFUNCTION()
    void HandlePatientDestroyed(AActor* DestroyedActor);

    UFUNCTION()
    void HandleWaitingPatientsChanged();

    void ScheduleTripRequestTimer();
    void HandleTripRequestTimer();
    bool TryStartRandomSnackTrip();
    void StartSnackTrip(AEMRPatient* Patient, AEMRSnackMachineActor* Machine);
    void FinishSnackTrip(AEMRSnackMachineActor* Machine);
    void TryProcessDeferredCall(AEMRPatient* Patient);
    void ApplySnackRewards(AEMRPatient* Patient) const;
    void SendPatientBackToWaitingSeat(AEMRPatient* Patient) const;
    void ResolveDifficultyTuning();
    void TryBindWaitingRoom();
    void DiscoverMachines();
    AEMRSnackMachineActor* FindAvailableMachine() const;
    bool EnsureServerWorld() const;

    const FEMRSnackMachineUpgradeTuning* GetSnackUpgradeTuning() const;

    TArray<TWeakObjectPtr<AEMRSnackMachineActor>> RegisteredMachines;
    TMap<TWeakObjectPtr<AEMRPatient>, FEMRSnackTripState> TripStates;

    TWeakObjectPtr<const UEMRDifficultyTuningData> CachedDifficultyTuning;
    TWeakObjectPtr<UEMRWaitingRoomManagerComponent> CachedWaitingRoomManager;

    FTimerHandle WaitingRoomRetryTimer;
    FTimerHandle TripRequestTimer;
    bool bPendingTripRequest = false;
};
