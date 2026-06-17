#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "AGASSPlayerController.generated.h"

class UAGASSInteractionComponent;
class UAGASSSettingsLocal;
class UInputAction;
class UInputMappingContext;
class USoundBase;
class AActor;
struct FInputActionValue;

UCLASS()
class AGASS_API AAGASSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAGASSPlayerController();

	UAGASSInteractionComponent* GetInteractionComponent() const
	{
		return InteractionComponent;
	}

	UFUNCTION(Client, Reliable)
	void ClientReturnToFrontendForClosedRun(const FText& ReturnReason);

	UFUNCTION(Client, Reliable)
	void ClientPlayObjectiveReachedSound();

	UFUNCTION(BlueprintCallable)
	void RequestSessionInviteCodeRegeneration();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void SetPawn(APawn* InPawn) override;

private:
	bool ShouldUseEnhancedInput() const;
	void ApplyGameplayInputMappingContext();
	void RemoveGameplayInputMappingContext();
	void QueueApplyGameplayViewportInputConfig();
	void ApplyGameplayViewportInputConfig();
	void BindLegacyInput();
	UAGASSSettingsLocal* GetLocalSettings() const;
	bool IsUsingGamepadLookInput() const;
	FVector2D ApplyLookSettings(const FVector2D& RawInput) const;
	FVector2D ApplyPlacementRotationSettings(const FVector2D& RawInput) const;
	float ApplyPlacementDistanceScale(float RawInput) const;
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void HandleJumpStarted(const FInputActionValue& Value);
	void HandleJumpCompleted(const FInputActionValue& Value);
	void HandleInteractStarted();
	void HandleInteractReleased();
	void HandleInteractHoldCompleted();
	void ClearPendingInteractHold();
	void HandleCancelCarryPressed();
	void HandleTogglePlacementRotationPressed();
	void HandleToggleGameMenuPressed();
	void HandleAdjustPlacementDistanceInput(const FInputActionValue& Value);
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void AdjustPlacementDistance(float Value);
	void StartJump();
	void StopJump();
	void PlayLocalSound2D(USoundBase* Sound) const;

	UFUNCTION(Server, Reliable)
	void ServerRequestRegenerateInviteCode();

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input")
	TSoftObjectPtr<UInputMappingContext> GameplayInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input", meta = (ClampMin = "0"))
	int32 GameplayInputMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input")
	TSoftObjectPtr<UInputAction> MoveInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input")
	TSoftObjectPtr<UInputAction> LookInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input")
	TSoftObjectPtr<UInputAction> JumpInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input")
	TSoftObjectPtr<UInputAction> InteractInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input")
	TSoftObjectPtr<UInputAction> CancelCarryInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input")
	TSoftObjectPtr<UInputAction> RotatePlacementInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input")
	TSoftObjectPtr<UInputAction> AdjustPlacementDistanceInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "AGASS|Input")
	TSoftObjectPtr<UInputAction> ToggleGameMenuInputAction;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Interaction")
	TObjectPtr<UAGASSInteractionComponent> InteractionComponent;

	UPROPERTY(EditAnywhere, Category = "AGASS|Audio")
	TObjectPtr<USoundBase> ObjectiveReachedSound;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> AppliedGameplayInputMappingContext;

	FTimerHandle InteractHoldTimerHandle;
	TWeakObjectPtr<AActor> PendingHoldInteractActor;
	bool bInteractInputHeld = false;
	bool bPendingHoldInteractionConsumed = false;
};
