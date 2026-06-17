#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EMRMagicBoxSubsystem.generated.h"

class AEMRMagicBoxActor;
class AEMRPatient;
class AActor;
class UEMRDifficultyTuningData;

USTRUCT()
struct FEMRMagicBoxTripState
{
    GENERATED_BODY()

    bool bTripActive = false;
    bool bAtMagicBox = false;
    bool bEnteringBox = false;
    bool bTreatmentStarted = false;
    TWeakObjectPtr<AEMRMagicBoxActor> AssignedMagicBox;
    FTimerHandle EntryTimerHandle;
    FTimerHandle InsideArrivalPollTimerHandle;
    FTimerHandle StartTreatmentTimerHandle;
    FTimerHandle FinishTreatmentTimerHandle;
    FTimerHandle ExitTimerHandle;
};

UCLASS()
class LOD_API UEMRMagicBoxSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    void RegisterMagicBox(AEMRMagicBoxActor* MagicBox);
    void UnregisterMagicBox(AEMRMagicBoxActor* MagicBox);

    bool TryAssignPatient(AEMRPatient* Patient);
    void NotifyMagicBoxArrival(AEMRMagicBoxActor* MagicBox, AEMRPatient* Patient);

private:
    UFUNCTION()
    void HandlePatientDestroyed(AActor* DestroyedActor);

    void HandlePatientEnterMagicBox(AEMRPatient* Patient);
    void PollPatientInsideArrival(AEMRPatient* Patient);
    void StartMagicTreatment(AEMRPatient* Patient);
    void FinishMagicTreatment(AEMRPatient* Patient);
    void FinalizeMagicTreatment(AEMRPatient* Patient);
    AEMRMagicBoxActor* FindAvailableMagicBox() const;
    bool EnsureServerWorld() const;
    void ResolveDifficultyTuning();
    float GetMagicBoxTreatmentDurationSeconds() const;
    void ClearTripTimers(FEMRMagicBoxTripState& State) const;

    TArray<TWeakObjectPtr<AEMRMagicBoxActor>> RegisteredMagicBoxes;
    TMap<TWeakObjectPtr<AEMRPatient>, FEMRMagicBoxTripState> TripStates;
    TWeakObjectPtr<const UEMRDifficultyTuningData> CachedDifficultyTuning;
};
