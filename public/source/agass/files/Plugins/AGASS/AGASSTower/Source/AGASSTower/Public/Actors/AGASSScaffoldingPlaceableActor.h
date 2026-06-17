#pragma once

#include "CoreMinimal.h"
#include "Actors/AGASSUtilityPlaceableActor.h"
#include "AGASSScaffoldingPlaceableActor.generated.h"

class APawn;
class FLifetimeProperty;
class UAGASSScaffoldingBehaviorData;
class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class AGASSTOWER_API AAGASSScaffoldingPlaceableActor : public AAGASSUtilityPlaceableActor
{
	GENERATED_BODY()

public:
	AAGASSScaffoldingPlaceableActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool CanBeClaimedBy(const AController* Controller) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UPrimitiveComponent* GetMovementBaseComponent() const override;
	virtual bool SupportsPlacementPitchRotation() const override;
	virtual FTransform AdjustDesiredPreviewTransform(const FTransform& DesiredTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const override;
	virtual void ConfigureCarryPreviewMesh(UStaticMeshComponent& PreviewMeshComponent) const override;
	virtual void CollapseForSessionEvent(const FVector& WorldImpulse) override;
	void RefreshPlacedVisualState();

protected:
	virtual void GatherPlacementOverlapIgnoredActors(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor, TArray<const AActor*>& OutIgnoredActors) const override;
	virtual bool IsPlacementTransformAllowedByActor(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const override;
	virtual bool ShouldSimulatePhysicsWhenVisible() const override;
	virtual void HandleItemDataChanged() override;
	virtual void HandleCarryStateChanged(bool bNowCarried) override;
	virtual void HandlePlacedAtTransform(const FTransform& ApprovedTransform) override;

private:
	UFUNCTION()
	void OnRep_UseStackedVisualMesh();

	UFUNCTION()
	void OnRep_IsFallingUnsupported();

	UFUNCTION()
	void HandleScaffoldingMeshWake(UPrimitiveComponent* WakingComponent, FName BoneName);

	UFUNCTION()
	void HandleScaffoldingMeshSleep(UPrimitiveComponent* SleepingComponent, FName BoneName);

	bool TryResolveSupportedPlacementTransform(const FTransform& DesiredTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor, FTransform& OutResolvedTransform) const;
	bool TryResolveStackedPlacementTransform(const FTransform& DesiredTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor, FTransform& OutResolvedTransform) const;
	bool TryResolveGroundPlacementTransform(const FTransform& DesiredTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor, FTransform& OutResolvedTransform) const;
	bool FindTargetedSupportScaffolding(const FTransform& DesiredTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor, AAGASSScaffoldingPlaceableActor*& OutSupportScaffolding, FVector& OutSupportHitLocation) const;
	bool IsStableSupportScaffolding(const AAGASSScaffoldingPlaceableActor* CandidateSupport) const;
	bool HasStableScaffoldingOccupyingOrigin(const FVector& CandidateOrigin, const AAGASSScaffoldingPlaceableActor* IgnoredScaffolding) const;
	AAGASSScaffoldingPlaceableActor* FindSupportingScaffoldingByStackAlignment(const FVector& CandidateOrigin) const;
	AAGASSScaffoldingPlaceableActor* FindSupportingScaffoldingAtOrigin(const FVector& CandidateOrigin, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const;
	float GetStackTopWorldZ(const AAGASSScaffoldingPlaceableActor& SupportScaffolding) const;
	bool CanStabilizeUnsupportedRestingPose(AAGASSScaffoldingPlaceableActor*& OutSupportScaffolding) const;
	UStaticMeshComponent* GetActiveWalkableMeshComponent() const;
	bool TryResolveUnsupportedFallLanding(
		const FVector& TraceStart,
		const FVector& TraceEnd,
		FVector& OutResolvedOrigin,
		AAGASSScaffoldingPlaceableActor*& OutSupportScaffolding) const;
	void ApplyScaffoldingVisualMesh();
	void SetUseStackedVisualMesh(bool bNewUseStackedVisualMesh);
	UStaticMesh* ResolveGroundVisualMesh() const;
	UStaticMesh* ResolveStackedVisualMesh() const;
	float GetGroundPlacementTraceDistance() const;
	float GetStackDetectionTraceSlack() const;
	bool ShouldDrawPlacementDebug() const;
	float GetPlacementDebugDrawDuration() const;
	void RefreshScaffoldingBehavior();
	void SetSupportingScaffolding(AAGASSScaffoldingPlaceableActor* NewSupportScaffolding);
	void AddSupportedScaffolding(AAGASSScaffoldingPlaceableActor* SupportedScaffolding);
	void RemoveSupportedScaffolding(AAGASSScaffoldingPlaceableActor* SupportedScaffolding);
	void InvalidateSupportedScaffolding();
	void BeginUnsupportedFall();
	void UpdatePawnCollisionForPhysicsState();

	UPROPERTY(ReplicatedUsing = OnRep_UseStackedVisualMesh)
	bool bUseStackedVisualMesh = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsFallingUnsupported)
	bool bIsFallingUnsupported = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Scaffolding|Placement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> StackTargetVolumeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Scaffolding|Placement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> StackSupportPointComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Scaffolding", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> StackedMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAGASSScaffoldingBehaviorData> ScaffoldingBehavior;

	TWeakObjectPtr<AAGASSScaffoldingPlaceableActor> SupportingScaffolding;
	TArray<TWeakObjectPtr<AAGASSScaffoldingPlaceableActor>> SupportedScaffoldingActors;
	float UnsupportedFallVerticalSpeed = 0.f;
};
