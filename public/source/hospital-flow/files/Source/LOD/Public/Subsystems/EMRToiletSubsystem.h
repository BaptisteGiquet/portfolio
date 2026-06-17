#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayEffectTypes.h"
#include "EMRToiletSubsystem.generated.h"

class AEMRPatient;
class AEMRToiletSlotActor;
class AActor;
class UEMRWaitingRoomManagerComponent;
class UEMRWaitingSeatComponent;
class UEMRToiletConfig;

USTRUCT()
struct FEMRToiletTripState
{
    GENERATED_BODY()

    bool bHasAttempted = false;
    bool bTripActive = false;
    bool bInToilet = false;
    bool bPendingCall = false;

    TWeakObjectPtr<AEMRToiletSlotActor> AssignedSlot;
    FTimerHandle TripFinishTimer;
    FTimerHandle DeferredCallTimer;
};

UCLASS()
class LOD_API UEMRToiletSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    void RegisterSlot(AEMRToiletSlotActor* Slot);
    void UnregisterSlot(AEMRToiletSlotActor* Slot);

    bool IsToiletTripActive(const AEMRPatient* Patient) const;
    bool TryDeferCallForPatient(AEMRPatient* Patient);

    void NotifySlotArrival(AEMRToiletSlotActor* Slot, AEMRPatient* Patient);
    void CleanSlot(AEMRToiletSlotActor* Slot);

private:
    UFUNCTION()
    void HandlePatientDestroyed(AActor* DestroyedActor);

    void ScheduleTripRequestTimer();
    void HandleTripRequestTimer();
    bool TryStartRandomToiletTrip();

    void StartToiletTrip(AEMRPatient* Patient, AEMRToiletSlotActor* Slot);
    void FinishToiletTrip(AEMRToiletSlotActor* Slot);
    void TryProcessDeferredCall(AEMRPatient* Patient);
    void ApplyReputationDrainIfNeeded();
    void ClearReputationDrainIfNeeded();
    void ResolveConfig();
    void TryBindWaitingRoom();
    void DiscoverSlots();
    AEMRToiletSlotActor* FindAvailableSlot() const;
    bool EnsureServerWorld() const;

    TArray<TWeakObjectPtr<AEMRToiletSlotActor>> RegisteredSlots;
    TMap<TWeakObjectPtr<AEMRPatient>, FEMRToiletTripState> TripStates;
    int32 DirtySlotCount = 0;

    FActiveGameplayEffectHandle ReputationDrainHandle;
    TWeakObjectPtr<const UEMRToiletConfig> CachedConfig;
    TWeakObjectPtr<UEMRWaitingRoomManagerComponent> CachedWaitingRoomManager;
    FTimerHandle WaitingRoomRetryTimer;
    FTimerHandle TripRequestTimer;
    bool bPendingTripRequest = false;
};
