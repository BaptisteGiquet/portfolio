#pragma once

#include "CoreMinimal.h"
#include "Interfaces/AGASSCarriedUsablePresentationInterface.h"
#include "GameFramework/Character.h"
#include "AGASSCharacter.generated.h"

class AActor;
class UCameraComponent;
class FLifetimeProperty;
class UAnimMontage;
class USoundBase;
class USkeletalMesh;
class USkeletalMeshComponent;

UCLASS()
class AGASS_API AAGASSCharacter : public ACharacter, public IAGASSCarriedUsablePresentationInterface
{
	GENERATED_BODY()

public:
	AAGASSCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ApplyMoveInput(const FVector2D& MovementInput);
	void ApplyLookInput(const FVector2D& LookInput);
	void StartJumpInput();
	void StopJumpInput();
	void ActivateDeathRagdoll();
	bool IsDeathRagdollActive() const;
	bool HasCarriedUsableItem() const;
	FName GetCarriedUsableAnimationId() const;

	virtual USkeletalMeshComponent* GetAGASSCarriedUsableAttachMesh() const override;
	virtual USkeletalMeshComponent* GetAGASSLocalViewCarriedUsableAttachMesh() const override;
	virtual void SetAGASSCarriedUsablePresentation(AActor* UsableItemActor, FName CarryAnimationId) override;
	virtual void ClearAGASSCarriedUsablePresentation(AActor* UsableItemActor) override;
	virtual void PlayAGASSCarriedUsableUseMontage(UAnimMontage* UseMontage, float PlayRate = 1.f) override;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayFallDeathSound(FVector_NetQuantize SoundLocation);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void NotifyControllerChanged() override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	virtual void PawnClientRestart() override;

private:
	UFUNCTION()
	void OnRep_DeathRagdollActive();

	UFUNCTION()
	void OnRep_CarriedUsablePresentation();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAGASSCarriedUsableUseMontage(UAnimMontage* UseMontage, float PlayRate);

	void ApplyDeathRagdollState();
	void PlayAGASSCarriedUsableUseMontageLocal(UAnimMontage* UseMontage, float PlayRate) const;
	void PlayFallDeathSoundAtLocation(const FVector& SoundLocation) const;
	void RefreshFirstPersonPresentationAssets();
	void RefreshFirstPersonPresentation();
	void RefreshLocalViewSettings();
	void UpdateFirstPersonPresentationAlignment();
	void RefreshDangerousFallSettings();
	void UpdateDangerousFallState(float DeltaSeconds);
	void ResetDangerousFallState();
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

	float DangerousFallSecondsThreshold = 2.f;
	float DangerousFallMinSpeed = 200.f;
	float AccumulatedDangerousFallSeconds = 0.f;
	bool bDangerousFallDeathEnabled = true;
	bool bDangerousFallDeathPending = false;
	bool bDangerousFallDeathTriggered = false;

	UPROPERTY(ReplicatedUsing = OnRep_DeathRagdollActive)
	bool bDeathRagdollActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedUsablePresentation)
	bool bHasCarriedUsableItem = false;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedUsablePresentation)
	FName CarriedUsableAnimationId;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Character")
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Character|FirstPerson")
	TArray<FName> FirstPersonHiddenBones;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Character|FirstPerson")
	TObjectPtr<USkeletalMeshComponent> FirstPersonPresentationMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Character|FirstPerson")
	TSoftObjectPtr<USkeletalMesh> FirstPersonPresentationMeshOverride;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Character|FirstPerson")
	FName FirstPersonPresentationAlignmentBone = TEXT("head");

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Character|FirstPerson")
	FTransform FirstPersonPresentationOffset = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Character|FirstPerson", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float FirstPersonPresentationFieldOfView = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Character|FirstPerson", meta = (ClampMin = "0.001", ClampMax = "1.0"))
	float FirstPersonPresentationScale = 0.6f;

	UPROPERTY(EditAnywhere, Category = "AGASS|Character|Audio")
	TObjectPtr<USoundBase> FallDeathSound;

	TWeakObjectPtr<AActor> PresentedUsableItemActor;
};
