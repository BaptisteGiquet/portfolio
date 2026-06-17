#pragma once

#include "CoreMinimal.h"
#include "Actors/AGASSCarryableItemActor.h"
#include "Data/AGASSItemDefinition.h"
#include "Data/AGASSPlaceableItemRuntimeData.h"
#include "TimerManager.h"
#include "AGASSPlaceableItemActor.generated.h"

class AController;
class APawn;
class UAGASSPlaceableBehaviorData;
class UAudioComponent;
class UBoxComponent;
class UPrimitiveComponent;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;
struct FHitResult;

UCLASS()
class AGASSTOWER_API AAGASSPlaceableItemActor : public AAGASSCarryableItemActor
{
	GENERATED_BODY()

public:
	AAGASSPlaceableItemActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual EAGASSCarriedItemType GetCarriedItemType() const override;
	virtual bool Claim(AController* Controller) override;
	virtual bool ClaimForImmediateCarry(AController* Controller) override;
	virtual void ReleaseClaim(bool bRestoreTransform);
	virtual void ReleaseFromCarry(const FTransform& ReleaseTransform) override;
	virtual bool PlaceAtTransform(const FTransform& ApprovedTransform);
	virtual bool CanToggleHeldRotationMode() const;
	virtual bool CanAdjustHeldPreviewDistance() const;
	virtual float GetPreferredHeldPreviewDistance() const;

	FVector GetPreviewHalfExtent() const;
	float GetPlacementRotationStepDegrees() const;
	EAGASSPlacementValidationReason EvaluatePlacementTransform(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPreviewActor) const;
	bool RequiresTouchingPlacementSupport() const;
	bool TryResolveTouchingPlacementSnapTransform(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor, FTransform& OutSnappedTransform) const;
	bool HasTouchingPlacementSupport(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const;
	float GetPlacementTouchTolerance() const;

	void SetRuntimeItemData(const FAGASSPlaceableItemRuntimeData& NewRuntimeItemData);
	UAGASSPlaceableBehaviorData* GetBehaviorTuning() const;
	int32 GetPurchaseCost() const;
	int32 GetSellValue() const;
	bool IsHeldHidden() const;
	bool IsPlacementCommitted() const;
	void BeginSpawnFallCollisionSequence();

	UFUNCTION(BlueprintPure, Category = "AGASS|Tower")
	AAGASSPlaceableItemActor* GetSupportingTowerPlaceable() const;

	void GetSupportedTowerPlaceables(TArray<AAGASSPlaceableItemActor*>& OutSupportedPlaceables) const;

	UFUNCTION(BlueprintPure, Category = "AGASS|Tower")
	bool IsPartOfTowerStack() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AGASS|Events")
	virtual void CollapseForSessionEvent(const FVector& WorldImpulse);

	virtual bool SupportsPlacementPitchRotation() const;
	virtual bool ShouldProvideCarryPreviewCollision() const;
	virtual FTransform AdjustDesiredPreviewTransform(const FTransform& DesiredTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const;
	virtual void ConfigureCarryPreviewMesh(UStaticMeshComponent& PreviewMeshComponent) const;
	virtual UPrimitiveComponent* GetMovementBaseComponent() const;
	void TransferCharacterMovementBases(UPrimitiveComponent* FromBaseComponent, UPrimitiveComponent* ToBaseComponent) const;

	const UStaticMeshComponent* GetItemMeshComponent() const
	{
		return ItemMeshComponent;
	}

private:
	UFUNCTION()
	void OnRep_RuntimeItemData();

	UFUNCTION()
	void OnRep_PlacementCommitted();

	UFUNCTION()
	void HandleMeshComponentWake(UPrimitiveComponent* WakingComponent, FName BoneName);

	UFUNCTION()
	void HandleMeshComponentSleep(UPrimitiveComponent* SleepingComponent, FName BoneName);

	UFUNCTION()
	void HandleMeshComponentHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayGroundImpactSound(FVector_NetQuantize SoundLocation, int32 SoundIndex);

	void ApplyHeldHiddenState();
	void ApplyResolvedItemVisuals();
	void ApplyResolvedPhysicsTuning();
	void ApplyVisiblePhysicsState(bool bForceWakeIfSimulating = false);
	void RefreshSpawnFallCollisionBounds();
	void EndSpawnFallCollisionSequence();
	void ResolveFinalPlacedEncroachment();
	void ClearSettledDormancyTimer();
	void ScheduleSettledDormancy();
	void CommitSettledDormancy();
	void ResumeActiveReplication();
	void SetPlacementCommitted(bool bNewCommitted);
	AAGASSPlaceableItemActor* FindSupportingTowerPlaceableAtTransform(
		const FTransform& CandidateTransform,
		const APawn* PlacementOwner,
		const AActor* IgnoredPlacementActor) const;
	void SetSupportingTowerPlaceable(AAGASSPlaceableItemActor* NewSupportPlaceable);
	void AddSupportedTowerPlaceable(AAGASSPlaceableItemActor* SupportedPlaceable);
	void RemoveSupportedTowerPlaceable(AAGASSPlaceableItemActor* SupportedPlaceable);
	void InvalidateSupportedTowerPlaceables();

	UStaticMesh* ResolveWorldStaticMesh() const;
	UAGASSPlaceableBehaviorData* ResolveBehaviorTuning() const;
	FVector GetMeshDerivedHalfExtent() const;
	FVector GetMeshDerivedLocalBoundsCenter() const;
	float GetPlacementValidationInset() const;
	bool ShouldUseSettledDormancy() const;
	float GetSettledDormancyDelaySeconds() const;
	int32 ResolveGroundImpactSoundIndex() const;

protected:
	virtual bool CanBeClaimedByController(const AController* Controller) const;
	virtual void GatherPlacementOverlapIgnoredActors(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor, TArray<const AActor*>& OutIgnoredActors) const;
	virtual bool IsPlacementTransformAllowedByActor(const FTransform& CandidateTransform, const APawn* PlacementOwner, const AActor* IgnoredPlacementActor) const;
	virtual bool ShouldSimulatePhysicsWhenVisible() const;
	virtual bool RequiresTouchingPlacementSupportByDefault() const;
	virtual void HandleItemDefinitionChanged() override;
	virtual void HandleItemDataChanged();
	virtual void HandleCarryStateChanged(bool bNowCarried) override;
	virtual void HandlePlacedAtTransform(const FTransform& ApprovedTransform);

	UStaticMeshComponent* GetMutableItemMeshComponent() const
	{
		return ItemMeshComponent;
	}

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Tower")
	TObjectPtr<UStaticMeshComponent> ItemMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Tower")
	TObjectPtr<UBoxComponent> SpawnFallCollisionComponent;

	UPROPERTY(EditAnywhere, Category = "AGASS|Tower|Audio")
	TArray<TObjectPtr<USoundBase>> GroundImpactSounds;

	UPROPERTY(ReplicatedUsing = OnRep_RuntimeItemData)
	FAGASSPlaceableItemRuntimeData RuntimeItemData;

	UPROPERTY(ReplicatedUsing = OnRep_PlacementCommitted)
	bool bPlacementCommitted = false;

	FTimerHandle SettledDormancyTimerHandle;
	bool bPlacementCommittedBeforeClaim = false;
	bool bSpawnFallCollisionActive = false;
	double LastGroundImpactSoundTimeSeconds = -1.0;

	UPROPERTY(Transient)
	TWeakObjectPtr<AAGASSPlaceableItemActor> SupportingTowerPlaceable;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AAGASSPlaceableItemActor>> SupportedTowerPlaceables;
};
