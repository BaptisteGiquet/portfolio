#pragma once

#include "CoreMinimal.h"
#include "Actors/AGASSSessionEventActor.h"
#include "Interfaces/AGASSSharedTeamMoneyInterface.h"
#include "AGASSMadCopsSessionEventActor.generated.h"

class AAGASSMadCopsCarActor;
class AAGASSMadCopsOfficerCharacter;
class AAGASSMadCopsRouteAnchorActor;
class AAGASSPlaceableItemActor;
class AController;
class UAGASSMadCopsSessionEventDefinition;

UENUM(BlueprintType)
enum class EAGASSMadCopsEventPhase : uint8
{
	Inactive,
	SirenLeadIn,
	CarArriving,
	OfficerScanning,
	OfficerMovingToStack,
	OfficerDestroyingStack,
	OfficerReturningToCar,
	Leaving,
	Finished
};

USTRUCT(BlueprintType)
struct AGASSTOWER_API FAGASSMadCopsTowerTarget
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AAGASSPlaceableItemActor>> Items;

	UPROPERTY()
	FVector TowerCenter = FVector::ZeroVector;

	UPROPERTY()
	float DistanceSqToOfficer = TNumericLimits<float>::Max();
};

UCLASS(Blueprintable)
class AGASSTOWER_API AAGASSMadCopsSessionEventActor : public AAGASSSessionEventActor
{
	GENERATED_BODY()

public:
	AAGASSMadCopsSessionEventActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleEventActivated_Implementation() override;
	virtual void HandleEventEnded_Implementation() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ApplyDefinitionSettings(const UAGASSMadCopsSessionEventDefinition& Definition);

	UFUNCTION(BlueprintPure, Category = "AGASS|Mad Cops")
	EAGASSMadCopsEventPhase GetMadCopsPhase() const;

	UFUNCTION(BlueprintPure, Category = "AGASS|Mad Cops")
	int32 GetCurrentBribeCost() const;

	UFUNCTION(BlueprintPure, Category = "AGASS|Mad Cops")
	bool CanAcceptBribe() const;

	bool TryAcceptBribe(AController* InteractingController);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "AGASS|Mad Cops")
	void ReceiveMadCopsPhaseChanged(EAGASSMadCopsEventPhase NewPhase);

	UFUNCTION(BlueprintImplementableEvent, Category = "AGASS|Mad Cops")
	void ReceiveMadCopsBribeAccepted(int32 PaidAmount);

	UFUNCTION(BlueprintImplementableEvent, Category = "AGASS|Mad Cops")
	void ReceiveMadCopsStackDestroyed(int32 DestroyedItemCount, FVector ClusterCenter);

private:
	UFUNCTION()
	void OnRep_MadCopsPhase();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastNotifyBribeAccepted(int32 PaidAmount);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastNotifyStackDestroyed(int32 DestroyedItemCount, FVector_NetQuantize ClusterCenter);

	bool TryProjectLocationToGround(FVector& InOutLocation) const;
	AAGASSMadCopsRouteAnchorActor* ResolvePrimaryRouteAnchor() const;
	bool SpawnPoliceCar();
	bool SpawnOfficerCharacter();
	bool BeginOfficerMoveTo(const FVector& GoalLocation, float AcceptanceRadius);
	bool HasOfficerReachedMoveGoal() const;
	void ClearOfficerMoveGoal();
	bool IsCandidatePlacedTowerItem(const AAGASSPlaceableItemActor* Candidate) const;
	bool IsCandidateVisibleFromScan(const AAGASSPlaceableItemActor* Candidate, const FVector& ScanOrigin) const;
	AAGASSPlaceableItemActor* ResolveTowerRoot(AAGASSPlaceableItemActor* Candidate) const;
	bool BuildTowerTargetFromRoot(AAGASSPlaceableItemActor* TowerRoot, const FVector& ScanOrigin, float ScanRadiusSquared, FAGASSMadCopsTowerTarget& OutTowerTarget) const;
	bool ResolveNextTowerTarget(FAGASSMadCopsTowerTarget& OutTowerTarget) const;
	FVector ResolveOfficerApproachLocation(const FAGASSMadCopsTowerTarget& TowerTarget) const;
	void DrawDebugScanVisualization() const;
	void DrawDebugTowerTarget(const FAGASSMadCopsTowerTarget& TowerTarget) const;
	void AdvancePhase(EAGASSMadCopsEventPhase NewPhase);
	void BeginOfficerScan();
	void BeginMoveToTower(const FAGASSMadCopsTowerTarget& NewTowerTarget);
	void DestroyActiveTower();
	void BeginLeaveFlow();
	void UpdateCarTransform(const FTransform& StartTransform, const FTransform& EndTransform, float Alpha) const;
	int32 ComputeBribeCost() const;
	TScriptInterface<IAGASSSharedTeamMoneyInterface> ResolveSharedMoneyProvider() const;
	float GetServerTimeSeconds() const;
	bool HasPhaseElapsed(float DurationSeconds) const;

	UPROPERTY(ReplicatedUsing = OnRep_MadCopsPhase)
	EAGASSMadCopsEventPhase MadCopsPhase = EAGASSMadCopsEventPhase::Inactive;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "AGASS|Mad Cops")
	int32 CurrentBribeCost = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "AGASS|Mad Cops")
	bool bBribeAccepted = false;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "AGASS|Mad Cops")
	TObjectPtr<AAGASSMadCopsOfficerCharacter> OfficerCharacter;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "AGASS|Mad Cops")
	TObjectPtr<AAGASSMadCopsCarActor> PoliceCarActor;

	UPROPERTY(Transient)
	FAGASSMadCopsTowerTarget ActiveTowerTarget;

	FTransform RouteEntryTransform = FTransform::Identity;
	FTransform RouteParkingTransform = FTransform::Identity;
	FTransform RouteScanTransform = FTransform::Identity;
	FTransform RouteExitTransform = FTransform::Identity;
	TSoftClassPtr<AAGASSMadCopsOfficerCharacter> ConfiguredOfficerCharacterClass;
	TSoftClassPtr<AAGASSMadCopsCarActor> ConfiguredPoliceCarActorClass;
	int32 BaseBribeCost = 100;
	int32 BribeCostIncreasePerActivation = 50;
	float ScanRadius = 2500.f;
	float OfficerTowerStandOffDistance = 160.f;
	float OfficerNavigationAcceptanceRadius = 125.f;
	float OfficerMoveTimeoutSeconds = 8.f;
	float CollapseImpulseStrength = 250.f;
	float SirenLeadInSeconds = 2.f;
	float CarArrivalDurationSeconds = 4.f;
	float ScanDurationSeconds = 2.f;
	float DestroyPauseSeconds = 1.f;
	float ReturnToCarDelaySeconds = 0.25f;
	float CarDepartureDurationSeconds = 3.5f;
	FVector OfficerExitLocalOffset = FVector(150.f, 90.f, 0.f);
	float GroundProjectionTraceHeight = 2500.f;
	bool bDrawDebugTowerDetection = false;
	FVector CurrentOfficerMoveGoalLocation = FVector::ZeroVector;
	float CurrentOfficerMoveAcceptanceRadius = 0.f;
	float PhaseStartServerTimeSeconds = 0.f;
	float OfficerMoveStartServerTimeSeconds = 0.f;
	bool bOfficerMoveActive = false;
	bool bReachedScanLocation = false;
};
