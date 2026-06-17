#pragma once

#include "CoreMinimal.h"
#include "Data/AGASSItemDefinition.h"
#include "Components/ActorComponent.h"
#include "AGASSInteractionComponent.generated.h"

class AAGASSPlacementPreviewActor;
class AAGASSCarryableItemActor;
class AAGASSPlaceableItemActor;
class AAGASSUsableItemActor;
class AActor;
class APlayerController;
class APawn;
class USoundBase;

UCLASS(ClassGroup = (AGASS), meta = (BlueprintSpawnableComponent))
class AGASSINTERACTION_API UAGASSInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAGASSInteractionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "AGASS|Interaction")
	void TryPickupFocusedItem();

	bool TryQuickInteract();
	bool TryHoldInteract(AActor* TargetActor = nullptr);
	bool ResolveFocusedHoldInteract(AActor*& OutTargetActor, float& OutHoldDurationSeconds) const;
	bool IsFocusedInteractActor(const AActor* TargetActor) const;

	UFUNCTION(BlueprintCallable, Category = "AGASS|Interaction")
	void CancelHeldItem();

	UFUNCTION(BlueprintCallable, Category = "AGASS|Interaction")
	void TryPlaceHeldItem();

	UFUNCTION(BlueprintCallable, Category = "AGASS|Interaction")
	void TogglePlacementRotationMode();

	UFUNCTION(BlueprintPure, Category = "AGASS|Interaction")
	bool IsPlacementRotationModeActive() const;

	bool TryBeginServerHoldFromSpawnedItem(AAGASSCarryableItemActor* SpawnedItem);
	void ForceReleaseHeldItem(bool bRestoreSourceTransform = true);

	void ApplyPlacementRotationInput(const FVector2D& RotationInput);
	void AdjustPreviewDistance(float DistanceInput);

	AAGASSCarryableItemActor* GetHeldItem() const
	{
		return HeldItem;
	}

	AAGASSPlacementPreviewActor* GetPreviewActor() const
	{
		return PreviewActor;
	}

protected:
	UFUNCTION(Server, Reliable)
	void ServerTryPickup(AAGASSCarryableItemActor* TargetItem);

	UFUNCTION(Server, Reliable)
	void ServerInteractActor(AActor* TargetActor);

	UFUNCTION(Server, Reliable)
	void ServerCancelHeldItem();

	UFUNCTION(Server, Reliable)
	void ServerTryHeldPrimaryAction();

	UFUNCTION(Server, Unreliable)
	void ServerUpdatePreview(const FVector_NetQuantize100& PreviewLocation, const FRotator& PreviewRotation);

	UFUNCTION(Server, Reliable)
	void ServerPlaceHeldItem(const FVector_NetQuantize100& PreviewLocation, const FRotator& PreviewRotation);

	UFUNCTION(Client, Reliable)
	void ClientPlayPickupItemSound();

	UFUNCTION(Client, Reliable)
	void ClientPlayPlaceItemSound();

	UFUNCTION()
	void OnRep_HeldItem();

	UFUNCTION()
	void OnRep_PreviewActor(AAGASSPlacementPreviewActor* PreviousPreviewActor);

private:
	bool CanInteractInCurrentWorld() const;
	bool CanServerClaimTarget(const AAGASSCarryableItemActor* TargetItem) const;
	bool CanServerAcceptPreviewLocation(const FVector& PreviewLocation) const;
	EAGASSPlacementValidationReason EvaluatePreviewTransform(const FTransform& CandidateTransform) const;
	void ResetPreviewDistanceState();
	void InitializePreviewStateFromHeldItem();
	float GetPlacementVerticalOffset(const FRotator& PlacementRotation) const;
	bool DoesHeldItemSupportPitchRotation() const;
	FRotator ResolveDesiredPlacementRotation(const FRotator& ViewRotation) const;
	float QuantizePlacementAngle(float RawAngle) const;
	void DeactivatePlacementRotationMode();
	void ResetPlacementRotationState();
	FTransform ResolveHeldUsableDropTransform() const;

	APlayerController* GetOwningPlayerController() const;
	APawn* GetOwningPawn() const;
	AActor* FindFocusedInteractActor() const;
	AAGASSCarryableItemActor* FindFocusedPickupTarget() const;
	float GetRequiredHoldDurationForActor(const AActor* TargetActor) const;
	FTransform CalculateDesiredPreviewTransform() const;
	FVector ConstrainDesiredPreviewLocation(const FVector& DesiredLocation) const;
	bool TryHeldItemPrimaryAction();
	bool IsHeldItemUsingPlacementPrimaryAction() const;
	bool BeginServerHold(AAGASSCarryableItemActor* TargetItem, bool bSkipRangeValidation = false);
	void ProcessServerPlacementRequest(const FTransform& CandidateTransform);
	void FinalizeSuccessfulHeldPrimaryAction(bool bConsumeHeldItem);
	void FinalizeServerPlacement(const FTransform& ApprovedTransform);
	void ReleaseServerHold(bool bRestoreSourceTransform);
	void UpdateServerPreviewTransform(const FTransform& NewTransform, EAGASSPlacementValidationReason ValidationReason);
	AAGASSPlaceableItemActor* GetHeldPlaceableItem() const;
	AAGASSUsableItemActor* GetHeldUsableItem() const;
	void PlayLocalSound2D(USoundBase* Sound) const;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction")
	float PickupTraceDistance = 350.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction")
	float PreviewTraceDistance = 450.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction")
	float PreviewUpdateInterval = 0.033f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction", meta = (ClampMin = "0.0"))
	float PreviewDistanceAdjustmentStep = 35.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction", meta = (ClampMin = "0.0"))
	float MinPreviewDistanceFromPawn = 125.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction")
	float MaxPreviewDistanceFromPawn = 550.f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Interaction")
	TSoftClassPtr<AAGASSPlacementPreviewActor> PreviewActorClass;

	UPROPERTY(EditAnywhere, Category = "AGASS|Interaction|Audio")
	TObjectPtr<USoundBase> PickupItemSound;

	UPROPERTY(EditAnywhere, Category = "AGASS|Interaction|Audio")
	TObjectPtr<USoundBase> PlaceItemSound;

	UPROPERTY(ReplicatedUsing = OnRep_HeldItem)
	TObjectPtr<AAGASSCarryableItemActor> HeldItem;

	UPROPERTY(ReplicatedUsing = OnRep_PreviewActor)
	TObjectPtr<AAGASSPlacementPreviewActor> PreviewActor;

	double NextPreviewSendTimeSeconds = 0.0;
	bool bPlacementRotationModeActive = false;
	bool bHasPlacementRotationOverride = false;
	FRotator ManualPlacementRotation = FRotator::ZeroRotator;
	bool bHasLastLoggedQuantizedPlacementRotation = false;
	FRotator LastLoggedQuantizedPlacementRotation = FRotator::ZeroRotator;
	FRotator LockedPlacementViewRotation = FRotator::ZeroRotator;
	float CurrentPreviewDistanceFromPawn = 0.f;
};
