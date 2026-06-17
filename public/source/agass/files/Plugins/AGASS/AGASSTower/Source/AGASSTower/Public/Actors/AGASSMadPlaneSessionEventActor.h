#pragma once

#include "CoreMinimal.h"
#include "Actors/AGASSSessionEventActor.h"
#include "AGASSMadPlaneSessionEventActor.generated.h"

class AAGASSMadPlaneAircraftActor;
class AAGASSPlaceableItemActor;
class AController;
class FLifetimeProperty;
class UAGASSMadPlaneSessionEventDefinition;

UENUM(BlueprintType)
enum class EAGASSMadPlaneEventPhase : uint8
{
	Inactive,
	LeadIn,
	Approaching,
	Diverted,
	Impacted,
	Leaving,
	Finished
};

UCLASS(Blueprintable)
class AGASSTOWER_API AAGASSMadPlaneSessionEventActor : public AAGASSSessionEventActor
{
	GENERATED_BODY()

public:
	AAGASSMadPlaneSessionEventActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleEventActivated_Implementation() override;
	virtual void HandleEventEnded_Implementation() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ApplyDefinitionSettings(const UAGASSMadPlaneSessionEventDefinition& Definition);
	bool TryAcceptFumigeneSignal(AController* UsingController);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "AGASS|Mad Plane")
	void ReceiveMadPlanePhaseChanged(EAGASSMadPlaneEventPhase NewPhase);

	UFUNCTION(BlueprintImplementableEvent, Category = "AGASS|Mad Plane")
	void ReceiveMadPlaneTargetResolved(FVector TowerCenter, FVector TowerTopLocation, float TowerHeightMeters);

	UFUNCTION(BlueprintImplementableEvent, Category = "AGASS|Mad Plane")
	void ReceiveMadPlaneDiverted(FVector SignalLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "AGASS|Mad Plane")
	void ReceiveMadPlaneImpact(int32 CollapsedItemCount, FVector ImpactLocation);

private:
	UFUNCTION()
	void OnRep_MadPlanePhase();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastNotifyMadPlaneDiverted(FVector_NetQuantize SignalLocation);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastNotifyMadPlaneImpact(int32 CollapsedItemCount, FVector_NetQuantize ImpactLocation);

	bool ResolveTargetTower();
	bool ResolveFlightPath();
	bool SpawnAircraftActor();
	bool IsControllerAtValidSignalPosition(const AController& UsingController) const;
	FVector BuildImpactPoint() const;
	FVector BuildPassThroughExitPoint() const;
	void ResolveAffectedItemsForImpact(TArray<AAGASSPlaceableItemActor*>& OutAffectedItems) const;
	void PerformImpactCollapse();
	void AdvancePhase(EAGASSMadPlaneEventPhase NewPhase);
	void StartFlightSegment(EAGASSMadPlaneEventPhase NewPhase, const FVector& StartLocation, const FVector& EndLocation, float DurationSeconds);
	void StartLeaveFlight(bool bWasDiverted);
	bool UpdateFlightSegment() const;
	float GetServerTimeSeconds() const;
	bool HasPhaseElapsed(float DurationSeconds) const;

	UPROPERTY(ReplicatedUsing = OnRep_MadPlanePhase)
	EAGASSMadPlaneEventPhase MadPlanePhase = EAGASSMadPlaneEventPhase::Inactive;

	UPROPERTY(Replicated)
	TObjectPtr<AAGASSMadPlaneAircraftActor> AircraftActor = nullptr;

	UPROPERTY(Replicated)
	FVector_NetQuantize TargetTowerCenter = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FVector_NetQuantize TargetTowerTopLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	float TargetTowerHeightMeters = 0.f;

	UPROPERTY(Replicated)
	bool bPlaneDiverted = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AAGASSPlaceableItemActor>> TargetTowerItems;

	FTransform ApproachStartTransform = FTransform::Identity;
	FTransform DivertExitTransform = FTransform::Identity;
	FTransform FlightSegmentStartTransform = FTransform::Identity;
	FTransform FlightSegmentEndTransform = FTransform::Identity;
	TSoftClassPtr<AAGASSMadPlaneAircraftActor> ConfiguredAircraftActorClass;
	float LeadInSeconds = 2.f;
	float ApproachDurationSeconds = 6.f;
	float FlightDistanceFromTower = 5000.f;
	float RedirectDurationSeconds = 3.f;
	float PostImpactPauseSeconds = 0.5f;
	float ImpactCollapseRadius = 450.f;
	float CollapseImpulseStrength = 325.f;
	float SignalHorizontalTolerance = 250.f;
	float SignalVerticalToleranceBelowTop = 220.f;
	float MinimumTargetTowerHeightMeters = 0.f;
	float PhaseStartServerTimeSeconds = 0.f;
	float CurrentFlightDurationSeconds = 0.f;
};
