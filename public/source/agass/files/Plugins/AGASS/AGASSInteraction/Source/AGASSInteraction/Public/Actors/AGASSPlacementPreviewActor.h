#pragma once

#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "Data/AGASSItemDefinition.h"
#include "GameFramework/Actor.h"
#include "AGASSPlacementPreviewActor.generated.h"

class AAGASSPlaceableItemActor;
class UMaterialInterface;
class UPrimitiveComponent;
class UStaticMesh;
class USceneComponent;

UCLASS()
class AGASSINTERACTION_API AAGASSPlacementPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AAGASSPlacementPreviewActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ApplyLocalPredictedTransform(const FTransform& NewTransform, EAGASSPlacementValidationReason ValidationReason);
	void ClearLocalPrediction();
	void SetReplicatedPreviewState(const FTransform& NewTransform, EAGASSPlacementValidationReason ValidationReason);
	void SetSourceItem(AAGASSPlaceableItemActor* NewSourceItem);
	void InitializeFromPlaceableItem(const AAGASSPlaceableItemActor* PlaceableItem);
	FTransform GetServerPreviewTransform() const;
	UPrimitiveComponent* GetMovementBaseComponent() const
	{
		return PreviewMeshComponent.Get();
	}

	UFUNCTION(BlueprintPure, Category = "AGASS|Interaction")
	bool IsPlacementValid() const;

	UFUNCTION(BlueprintPure, Category = "AGASS|Interaction")
	EAGASSPlacementValidationReason GetPlacementValidationReason() const;

private:
	bool IsLocalPreviewOwner() const;
	FTransform GetActiveTargetTransform() const;
	EAGASSPlacementValidationReason GetActiveValidationReason() const;
	bool ShouldSyncPreviewStaticMeshFromSource() const;
	void ApplyVisualTransform(const FTransform& NewTransform);
	void RefreshVisualFromSourceItem();
	void CacheAuthoredPreviewMaterials();
	void ApplyPreviewBaseMaterials(const UStaticMeshComponent& SourceMeshComponent);
	void ApplyPlacementValidationVisuals(EAGASSPlacementValidationReason NewReason);
	void ApplyPreviewValidationBaseMaterial(UMaterialInterface* ValidationMaterial);
	void ApplyPreviewOverlayMaterial(UMaterialInterface* OverlayMaterial);

	UFUNCTION()
	void OnRep_ServerPreviewState();

	UFUNCTION()
	void OnRep_SourceItem();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "AGASS|Interaction")
	void ReceivePlacementValidationChanged(EAGASSPlacementValidationReason NewReason);

private:
	UPROPERTY(VisibleAnywhere, Category = "AGASS|Interaction")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Interaction")
	TObjectPtr<UStaticMeshComponent> PreviewMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction", meta = (ClampMin = "0.0"))
	float PositionInterpolationSpeed = 14.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction", meta = (ClampMin = "0.0"))
	float RotationInterpolationSpeed = 14.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction", meta = (ClampMin = "0.0"))
	float SnapDistance = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction|Owner", meta = (ClampMin = "0.0"))
	float OwnerLocalPositionInterpolationSpeed = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction|Owner", meta = (ClampMin = "0.0"))
	float OwnerLocalRotationInterpolationSpeed = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction|Owner", meta = (ClampMin = "0.0"))
	float OwnerLocalSnapDistance = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction|Placement")
	TSoftObjectPtr<UMaterialInterface> InvalidOverlayMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction|Placement")
	bool bScalePreviewWhenInvalid = false;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction|Placement", meta = (ClampMin = "0.1"))
	float InvalidPreviewScaleMultiplier = 0.85f;

	UPROPERTY(ReplicatedUsing = OnRep_SourceItem)
	TObjectPtr<AAGASSPlaceableItemActor> SourceItem;

	UPROPERTY(ReplicatedUsing = OnRep_ServerPreviewState)
	FVector_NetQuantize100 ServerPreviewLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_ServerPreviewState)
	FRotator ServerPreviewRotation = FRotator::ZeroRotator;

	UPROPERTY(ReplicatedUsing = OnRep_ServerPreviewState)
	EAGASSPlacementValidationReason ServerPlacementValidationReason = EAGASSPlacementValidationReason::Valid;

	bool bUseLocalPredictedTransform = false;
	bool bHasVisualTransform = false;
	bool bSourceItemVisualsInitialized = false;
	bool bHasCachedAuthoredPreviewMaterials = false;
	FTransform LocalPredictedTransform = FTransform::Identity;
	EAGASSPlacementValidationReason LocalPredictedValidationReason = EAGASSPlacementValidationReason::Valid;
	EAGASSPlacementValidationReason LastAppliedValidationReason = EAGASSPlacementValidationReason::Valid;
	bool bHasAppliedValidationReason = false;
	FVector BasePreviewMeshScale = FVector::OneVector;
	TArray<TObjectPtr<UMaterialInterface>> AuthoredPreviewMaterials;
	TArray<TObjectPtr<UMaterialInterface>> BasePreviewMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LoadedInvalidOverlayMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FallbackPreviewMesh;
};
