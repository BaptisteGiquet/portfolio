#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"
#include "Engine/NetSerialization.h"
#include "GameplayTagContainer.h"
#include "GAS/EMRGASConfig.h"
#include "Interfaces/EMRSeatedAnimationInterface.h"
#include "LOD/EMRCollisionChannels.h"

#include "EMRPlayerCharacter.generated.h"

UENUM()
enum class EEMRWatchSocketState : uint8
{
	StandingIdle,
	StandingWatch,
	SeatedIdle,
	SeatedWatch
};

USTRUCT(BlueprintType)
struct FEMRWatchSocketConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson")
	FName SocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson")
	FTransform RelativeTransform = FTransform::Identity;
};

class UGameplayAbility;
class UAbilitySystemComponent;
class UEMRPlayerSystemGenerics;
class UEMRInventoryComponent;
class UEMRTeamAttributeSet;
class UEMRCameraAimComponent;
class USceneComponent;
struct FInputActionValue;
struct FGameplayAbilityInputBinds;
enum class EER_RunPhase : uint8;
enum class EEMRSpecialEventPhase : uint8;
enum class EEMRHubDecisionStage : uint8;

class UEMRPatientAttributeSet;
class UEMRAbilitySystemComponent;
class UInputMappingContext;
class UCameraComponent;
class USpringArmComponent;
class USkeletalMeshComponent;
class UInputAction;
class UEMRInteractableDetectionComponent;
class UEMRPlayerMachineInputComponent;
class UEMRPlayerWidgetInteractionComponent;
class UEMRPlayerViewStateComponent;
class UChildActorComponent;
class UWidgetInteractionComponent;
class UWidgetComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UDecalComponent;
class UEMRItemPlacementComponent;
class USoundBase;
class UUserWidget;
struct FTimerHandle;
class AActor;
class AEMRReceptionMachine;
class AEMRXRayMachine;
class AEMRUltrasoundMachine;
class AEMRCTScanMachine;
class AEMRIntravenousMedicationStand;
class AEMRLabAnalyzerMachine;
class AEMRLabAnalyzerTube;
class AEMRItemActor;
class AEMRItemDispenser;
class AEMRHubTabletActor;
class AEMROxygenMachine;
class AEMROxygenMask;
class AEMRToiletCleaner;
class AEMROvertimeStopTerminal;
class AEMRNightShiftGameState;
struct FHitResult;
enum class EEMRItemDispenserButtonType : uint8;




UCLASS()
class AEMRPlayerCharacter : public ACharacter, public IEMRSeatedAnimationInterface
{
    GENERATED_BODY()

public:
    AEMRPlayerCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void PawnClientRestart() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void OnRep_PlayerState() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	bool TryHandleHoveredWidgetInteract(UWidgetComponent* TargetWidget);
	
    UFUNCTION(BlueprintCallable, Category = "EMR|Movement")
    void SetIsSeated(bool bInIsSeated);

    UFUNCTION(BlueprintCallable, Category = "EMR|View")
    void RequestInstantSeatedCameraTransition();

    UFUNCTION(BlueprintCallable, Category = "EMR|Movement")
    void SetSeatedRotateBodyWithCamera(bool bEnable);

    UFUNCTION(BlueprintPure, Category = "EMR|Movement")
    bool IsSeated() const { return bIsSeated; }

    UFUNCTION(BlueprintPure, Category = "EMR|Movement")
    FVector2D GetLastMoveInputAxis() const { return LastMoveInputAxis; }

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EMR|Movement")
    void SetSeatedAnimationTag(const FGameplayTag& InTag);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EMR|Movement")
    FGameplayTag GetSeatedAnimationTag() const;

    virtual void SetSeatedAnimationTag_Implementation(const FGameplayTag& InTag) override;
    virtual FGameplayTag GetSeatedAnimationTag_Implementation() const override;

    UFUNCTION(BlueprintPure, Category = "EMR|View")
    USkeletalMeshComponent* GetPlayerMeshComponentForSeating() const;


    UFUNCTION(BlueprintPure, Category = "EMR|View")
    FVector GetLookTargetLocation() const { return LookTargetLocation; }

	UFUNCTION(BlueprintPure, Category = "EMR|Watch")
	bool IsLookingAtWatch() const { return bLookingAtWatch; }

	UFUNCTION(BlueprintPure, Category = "EMR|Watch")
	bool CanUseWatch() const;

    void HandleToggleWatchAbility();
	void SetWatchSocketLookActive(bool bEnabled);
	void NotifyCarriedItemStateChanged();

    UFUNCTION(Server, Reliable)
    void ServerSetIsSeated(bool bInIsSeated);

    UFUNCTION(Server, Reliable)
    void ServerSetWatchSocketLookActive(bool bInWatchSocketLookActive);

    UFUNCTION(Server, Reliable)
    void ServerSetLookTargetLocation(const FVector_NetQuantize10& InLocation);

    UFUNCTION(Server, Unreliable)
    void ServerSetLookTargetLocationUnreliable(const FVector_NetQuantize10& InLocation);

    UFUNCTION(Server, Reliable)
    void Server_RequestReceptionUnseat(AEMRReceptionMachine* ReceptionMachine);

    UFUNCTION(Server, Reliable)
    void Server_SetXRayMoveInput(AEMRXRayMachine* Machine, float InputX, float InputY);

    UFUNCTION(Server, Reliable)
    void Server_RequestXRayExit(AEMRXRayMachine* Machine);

    UFUNCTION(Server, Reliable)
    void Server_SetUltrasoundMoveInput(AEMRUltrasoundMachine* Machine, float InputX, float InputY);

    UFUNCTION(Server, Reliable)
    void Server_SetUltrasoundActiveSlider(AEMRUltrasoundMachine* Machine, int32 SliderIndex);

    UFUNCTION(Server, Reliable)
    void Server_SetUltrasoundSliderAdjust(AEMRUltrasoundMachine* Machine, float InputValue);

    UFUNCTION(Server, Reliable)
    void Server_RequestUltrasoundExit(AEMRUltrasoundMachine* Machine);

    UFUNCTION(Server, Reliable)
    void Server_NotifyUltrasoundCompletionCueFinished(AEMRUltrasoundMachine* Machine, bool bCanceled);

    UFUNCTION(Server, Reliable)
    void Server_RequestCTScanExit(AEMRCTScanMachine* Machine);

    UFUNCTION(Server, Reliable)
    void Server_HandleCTScanButtonPressed(AEMRCTScanMachine* Machine, int32 ButtonIndex);

    UFUNCTION(Server, Reliable)
    void Server_RequestIVStandExit(AEMRIntravenousMedicationStand* Stand);

    UFUNCTION(Server, Reliable)
    void Server_HandleIVStandConfirm(AEMRIntravenousMedicationStand* Stand);

    UFUNCTION(Server, Reliable)
    void Server_RequestOxygenMachineExit(AEMROxygenMachine* Machine);

    UFUNCTION(Server, Reliable)
    void Server_RequestOxygenMaskReturn(AEMROxygenMask* Mask);

    UFUNCTION(Server, Reliable)
    void Server_RequestToiletCleanerReturn(AEMRToiletCleaner* Cleaner);

    UFUNCTION(Server, Reliable)
    void Server_RequestAnchoredCarryItemReturn(AActor* AnchoredItemActor);

    UFUNCTION(Server, Reliable)
    void Server_RequestLabAnalyzerExit(AEMRLabAnalyzerMachine* Machine);

    UFUNCTION(Server, Reliable)
    void Server_HandleLabAnalyzerStartPressed(AEMRLabAnalyzerMachine* Machine);

    UFUNCTION(Server, Reliable)
    void Server_HandleLabAnalyzerLidPressed(AEMRLabAnalyzerMachine* Machine);

    UFUNCTION(Server, Reliable)
    void Server_HandleItemDispenserButtonPressed(AEMRItemDispenser* Dispenser, EEMRItemDispenserButtonType ButtonType, int32 Digit);

    UFUNCTION(Server, Reliable)
    void Server_HandleOvertimeStopTerminalButtonPressed(AEMROvertimeStopTerminal* Terminal);

    UFUNCTION(Server, Reliable)
    void Server_HandleConfirmInteraction(const FVector_NetQuantize& ViewLocation,
                                         const FVector_NetQuantizeNormal& ViewDirection);

    UFUNCTION(Server, Reliable)
    void Server_PlayWatchSoundForAllPlayers(USoundBase* Sound, FVector_NetQuantize SoundLocation);


    UFUNCTION(Client, Reliable)
    void Client_BeginXRayExam(AEMRXRayMachine* Machine, UInputMappingContext* ExamIMC);

    UFUNCTION(Client, Reliable)
    void Client_EndXRayExam(AEMRXRayMachine* Machine);

    UFUNCTION(Client, Reliable)
    void Client_BeginUltrasoundExam(AEMRUltrasoundMachine* Machine, UInputMappingContext* ExamIMC);

    UFUNCTION(Client, Reliable)
    void Client_EndUltrasoundExam(AEMRUltrasoundMachine* Machine);

    UFUNCTION(Client, Reliable)
    void Client_BeginCTScanExam(AEMRCTScanMachine* Machine, UInputMappingContext* ExamIMC);

    UFUNCTION(Client, Reliable)
    void Client_EndCTScanExam(AEMRCTScanMachine* Machine);

    UFUNCTION(Client, Reliable)
    void Client_BeginIVTreatment(AEMRIntravenousMedicationStand* Stand, UInputMappingContext* ExamIMC);

    UFUNCTION(Client, Reliable)
    void Client_EndIVTreatment(AEMRIntravenousMedicationStand* Stand);

    UFUNCTION(Client, Reliable)
    void Client_BeginOxygenMove(AEMROxygenMachine* Machine);

    UFUNCTION(Client, Reliable)
    void Client_EndOxygenMove(AEMROxygenMachine* Machine);

    UFUNCTION(Client, Reliable)
    void Client_BeginLabAnalyzerExam(AEMRLabAnalyzerMachine* Machine, UInputMappingContext* ExamIMC);

    UFUNCTION(Client, Reliable)
    void Client_EndLabAnalyzerExam(AEMRLabAnalyzerMachine* Machine);

    UFUNCTION(Client, Reliable)
    void Client_PlayWorldSoundAtLocation(USoundBase* Sound, FVector_NetQuantize Location);

    UFUNCTION(Client, Reliable)
    void Client_PlayWatchReputationFailureSound();

    void PlayWorldSoundForAllPlayers(USoundBase* Sound, FVector_NetQuantize Location) const;


    UFUNCTION()
    void OnRep_IsSeated();
    UFUNCTION()
    void OnRep_SeatedAnimationTag();
    UFUNCTION()
    void OnRep_LookingAtWatch();
    UFUNCTION()
    void OnRep_WatchSocketLookActive();

protected:
    UPROPERTY(EditDefaultsOnly, Category = "EMR|Collision")
    float FixedCapsuleRadius = 42.f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Collision")
    float FixedCapsuleHalfHeight = 96.f;

private:
    void RefreshSeatedRenderState();
    UPROPERTY(VisibleAnywhere, Category = "EMR|Essential Variables")
    TWeakObjectPtr<APlayerController> CachedPlayerController;

    UPROPERTY(Transient)
    FVector2D LastMoveInputAxis = FVector2D::ZeroVector;
	

    UPROPERTY(ReplicatedUsing = "OnRep_IsSeated", VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Movement", meta = (AllowPrivateAccess = "true"))
    bool bIsSeated = false;

    UPROPERTY(ReplicatedUsing = "OnRep_SeatedAnimationTag", VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Movement", meta = (AllowPrivateAccess = "true"))
    FGameplayTag SeatedAnimationTag;

    bool bSeatedRotateBodyWithCamera = false;
    bool bSavedUseControllerRotationYaw = false;
    bool bHasSavedSeatedRotation = false;



	UPROPERTY(VisibleAnywhere, Category = "EMR|Interaction")
	TObjectPtr<UEMRInteractableDetectionComponent> InteractableDetectionComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Interaction")
    TObjectPtr<UEMRPlayerMachineInputComponent> PlayerMachineInputComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Interaction|Widgets")
    TObjectPtr<UEMRPlayerWidgetInteractionComponent> PlayerWidgetInteractionComponent = nullptr;

	UFUNCTION()
	void HandleInteractableTargetChanged(AActor* NewTarget);

	TWeakObjectPtr<AActor> PreviousInteractableTarget;
	bool IsGAInteractTargetCurrentlyActiveInteraction(const AActor* Target) const;
	void RefreshCurrentTargetInteractionFeedback();
	
	
	
	/**********************************************************************/
	/*                             Camera                                 */
	/**********************************************************************/



private:

    UPROPERTY(EditDefaultsOnly, Category = "EMR|View")
    TObjectPtr<USpringArmComponent> CameraBoom = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|View")
    TObjectPtr<UCameraComponent> Camera = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|View")
	TObjectPtr<USceneComponent> LookTarget = nullptr;
	
    UPROPERTY(EditDefaultsOnly, Category = "EMR|View")
    FComponentReference PlayerMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|View")
	FComponentReference SeatedCameraMesh;



	UPROPERTY(EditDefaultsOnly, Category = "EMR|View")
	FName SeatedCameraSocketName = TEXT("headSocket");

	UPROPERTY(EditDefaultsOnly, Category = "EMR|View", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float SeatedCameraBlendDuration = 0.25f;

	UPROPERTY()
	FTransform DefaultCameraBoomRelativeTransform;

	UPROPERTY()
	FTransform CameraTransitionStartWorld;

	UPROPERTY()
	FTransform CameraTransitionTargetWorld;

	UPROPERTY()
	float ActiveSeatedCameraBlendDuration = 0.f;

	UPROPERTY()
	float CameraTransitionElapsed = 0.f;

	UPROPERTY()
	bool bCameraTransitionActive = false;

	UPROPERTY()
	bool bCameraTransitionToSeated = false;

    UPROPERTY()
    bool bCameraAttachedToSeat = false;

    UPROPERTY(Transient)
    bool bInstantSeatedCameraTransitionRequested = false;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "EMR|View", meta = (AllowPrivateAccess = "true"))
    FVector LookTargetLocation = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|View", meta = (ClampMin = "0.01"))
    float LookTargetReplicationDistanceThreshold = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|View", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.2"))
    float LookTargetReplicationMinInterval = 0.033f;

    UPROPERTY(VisibleAnywhere, Category = "EMR|View")
    TObjectPtr<UEMRPlayerViewStateComponent> PlayerViewStateComponent = nullptr;

    USkeletalMeshComponent* ResolveSeatedCameraMesh() const;
    USkeletalMeshComponent* ResolvePlayerMeshComponent() const;
    bool CanUseSeatedCameraSocket(const USkeletalMeshComponent* InMesh) const;
    void HandleSeatedCameraStateChanged();
    float ResolveSeatedCameraBlendDuration();
    void UpdateLookTargetLocation(bool bForceReplication);
    void UpdateTickState();
	void StartCameraTransition(bool bToSeated);
	void UpdateCameraTransition(float DeltaSeconds);
	void FinishCameraTransition();
	FTransform GetDefaultCameraBoomWorldTransform() const;
	void CacheDefaultCameraBoomTransform();
    bool ShouldRotateBodyWithCameraForCurrentSeat() const;

	/**********************************************************************/
	/*                           Hub Tablet                               */
	/**********************************************************************/

	UPROPERTY(VisibleAnywhere, Category = "EMR|Hub|Tablet")
	TObjectPtr<UChildActorComponent> HubTabletComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Tablet")
	TSubclassOf<AEMRHubTabletActor> HubTabletClass;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Hub|Tablet")
	FName HubTabletSocketName = TEXT("hand_rSocket");

	UPROPERTY(VisibleAnywhere, Category = "EMR|Hub|Tablet")
	TObjectPtr<UWidgetInteractionComponent> WorldWidgetInteraction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Interaction|Widgets")
	float WorldWidgetInteractionDistance = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Interaction|Widgets")
	TEnumAsByte<ECollisionChannel> WorldWidgetInteractionTraceChannel = ECC_InteractableWidgets;

	
    UPROPERTY(EditDefaultsOnly, Category = "EMR|Carry|FirstPerson")
    TObjectPtr<UStaticMeshComponent> FirstPersonCarriedItemMesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Carry|FirstPerson")
    TObjectPtr<UStaticMeshComponent> FirstPersonCarriedItemFilledOverlayMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Carry|FirstPerson")
	TObjectPtr<UWidgetComponent> FirstPersonCarriedItemWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson")
	TObjectPtr<UStaticMeshComponent> ThirdPersonWatchMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch")
	TObjectPtr<UWidgetComponent> WatchWidgetComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch")
	TSubclassOf<UUserWidget> WatchWidgetClass = nullptr;

	UPROPERTY(EditAnywhere, Category = "EMR|Watch")
	FTransform WatchWidgetTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|Audio")
	TObjectPtr<USoundBase> WatchOvertimeSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|Audio")
	TObjectPtr<USoundBase> WatchSpecialEventAlertSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|Audio")
	TObjectPtr<USoundBase> WatchReputationFailureSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson")
	FName ThirdPersonWatchSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson")
	FTransform ThirdPersonWatchMeshTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson")
	FEMRWatchSocketConfig WatchSocketStandingIdle;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson")
	FEMRWatchSocketConfig WatchSocketStandingLook;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson")
	FEMRWatchSocketConfig WatchSocketSeatedIdle;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson")
	FEMRWatchSocketConfig WatchSocketSeatedLook;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Watch|ThirdPerson", meta = (ClampMin = "0.0"))
	float WatchSocketBlendDuration = 0.15f;


	UPROPERTY(VisibleAnywhere, Category = "EMR|Placement")
	TObjectPtr<UEMRItemPlacementComponent> ItemPlacementComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "EMR|Placement|Preview")
	TObjectPtr<UStaticMeshComponent> PlacementPreviewMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement|Preview")
	TObjectPtr<UMaterialInterface> PlacementPreviewMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement|Preview")
	FName PlacementPreviewColorParam = TEXT("TintColor");

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement|Preview")
	FLinearColor PlacementPreviewValidColor = FLinearColor(0.f, 1.f, 0.f, 0.35f);

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Placement|Preview")
	FLinearColor PlacementPreviewInvalidColor = FLinearColor(1.f, 0.f, 0.f, 0.35f);

    UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.1", ClampMax="5.0", UIMin="0.1", UIMax="5.0"))
    float LookSensitivity = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Carry|FirstPerson", meta = (ClampMin = "0.05"))
    float FirstPersonCarriedItemRefreshInterval = 0.15f;

	/**********************************************************************/
	/*                             Inputs                                 */
	/**********************************************************************/

	
private:
void AddGameplayMappingContext() const;
    void HandleLookInput(const FInputActionValue& InputActionValue);
    void HandleMoveInput(const FInputActionValue& InputActionValue);
    void HandleToggleGameMenuInput(const FInputActionValue& InputActionValue);
    void HandleToggleDeveloperToolsInput(const FInputActionValue& InputActionValue);
    void HandlePushToTalkStarted(const FInputActionValue& InputActionValue);
    void HandlePushToTalkCompleted(const FInputActionValue& InputActionValue);
    void HandleAbilityInput(EAbilityInputID AbilityInputID);
    void HandleXRayMoveLeftStarted(const FInputActionValue& InputActionValue);
    void HandleXRayMoveLeftCompleted(const FInputActionValue& InputActionValue);
    void HandleXRayMoveRightStarted(const FInputActionValue& InputActionValue);
    void HandleXRayMoveRightCompleted(const FInputActionValue& InputActionValue);
    void HandleXRayMoveForwardStarted(const FInputActionValue& InputActionValue);
    void HandleXRayMoveForwardCompleted(const FInputActionValue& InputActionValue);
    void HandleXRayMoveBackwardStarted(const FInputActionValue& InputActionValue);
    void HandleXRayMoveBackwardCompleted(const FInputActionValue& InputActionValue);
    void ResetXRayMoveInput();
    void HandleUltrasoundMoveLeftStarted(const FInputActionValue& InputActionValue);
    void HandleUltrasoundMoveLeftCompleted(const FInputActionValue& InputActionValue);
    void HandleUltrasoundMoveRightStarted(const FInputActionValue& InputActionValue);
    void HandleUltrasoundMoveRightCompleted(const FInputActionValue& InputActionValue);
    void HandleUltrasoundMoveForwardStarted(const FInputActionValue& InputActionValue);
    void HandleUltrasoundMoveForwardCompleted(const FInputActionValue& InputActionValue);
    void HandleUltrasoundMoveBackwardStarted(const FInputActionValue& InputActionValue);
    void HandleUltrasoundMoveBackwardCompleted(const FInputActionValue& InputActionValue);
    void ResetUltrasoundMoveInput();
    void HandleUltrasoundAdjustInputTriggered(const FInputActionValue& InputActionValue);
    void HandleUltrasoundAdjustInputCompleted(const FInputActionValue& InputActionValue);
    void ResetUltrasoundAdjustInput();
   //void LogGrantedAbilities() const;
    void SetInputEnabled(const bool bIsEnabled);

	void InitializeFirstPersonCarriedItemMesh();
	void InitializeThirdPersonWatchMesh();
	void RefreshAbilityActorInfoMesh();
	void RefreshThirdPersonWatchMesh(bool bForce = false);
	void UpdateThirdPersonWatchBlend(float DeltaSeconds);
	EEMRWatchSocketState ResolveWatchSocketState() const;
	FEMRWatchSocketConfig GetWatchSocketConfig(EEMRWatchSocketState State) const;
	bool IsWatchSocketValid(USkeletalMeshComponent* SourceMesh, const FEMRWatchSocketConfig& Config, EEMRWatchSocketState State);
	void BeginWatchSocketBlend(USkeletalMeshComponent* SourceMesh, EEMRWatchSocketState State, const FEMRWatchSocketConfig& Config);
	void AttachThirdPersonWatchMesh(USkeletalMeshComponent* SourceMesh, const FName& SocketName, const FTransform& RelativeTransform);
    void StartCarriedItemRefreshTimer();
    void StopCarriedItemRefreshTimer();
    void RefreshFirstPersonCarriedItem();
    void RefreshFirstPersonCarriedItemFilledOverlay(const AEMRItemActor* CarriedItemActor, bool bShouldShowBaseMesh);
    void ClearFirstPersonCarriedItemFilledOverlay();
	void RefreshFirstPersonCarriedItemWidget(AActor* CarriedActor);
    void RefreshFirstPersonTubeDecals(AEMRLabAnalyzerTube* Tube);
    void BuildFirstPersonTubeDecals(AEMRLabAnalyzerTube* Tube);
    void ClearFirstPersonTubeDecals();
	void UpdatePlacementPreview();
	void SetPlacementPreviewVisible(bool bVisible);
    AActor* FindCarriedActor() const;

	void InitializeHubTabletComponent();
	void BindToHubTabletState();
	void UnbindFromHubTabletState();
	void UpdateHubTabletState();
	void HandleHubTabletRunPhaseChanged(EER_RunPhase NewPhase, EER_RunPhase PreviousPhase);
    void HandleHubTabletPendingTravelChanged();
    void HandleHubTabletSelectionCommittedChanged();
    void HandleHubTabletDecisionStageChanged(EEMRHubDecisionStage NewStage, EEMRHubDecisionStage PreviousStage);
    bool IsGAInteractInput(EAbilityInputID AbilityInputID) const;
    bool TryHandleActiveGAInteractCancel();
    bool TryHandleGAInteractStopInput(EAbilityInputID AbilityInputID);
    bool TryHandleWorldWidgetClick(EAbilityInputID AbilityInputID);
    bool HasWorldWidgetUnderCrosshair(FHitResult& OutHit) const;
    bool TryHandleReceptionCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleXRayCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleUltrasoundCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleCTScanCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleIVStandCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleOxygenMachineCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleLabAnalyzerCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleAnchoredCarryItemCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleToiletCleanerCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleUltrasoundSliderSelect(EAbilityInputID AbilityInputID);
    bool TryHandleCTScanButtonSelect(EAbilityInputID AbilityInputID);
    bool TryHandleIVStandConfirm(EAbilityInputID AbilityInputID);
    bool TryHandleOxygenMaskCancelInput(EAbilityInputID AbilityInputID);
    bool TryHandleItemDispenserButtonSelect(EAbilityInputID AbilityInputID);
    bool TryHandleOvertimeStopTerminalButtonSelect(EAbilityInputID AbilityInputID);
    bool TryHandleLabAnalyzerButtonSelect(EAbilityInputID AbilityInputID);
    bool FindBestItemHit(const FVector& ViewLocation, const FVector& ViewDirection, FHitResult& OutBestHit) const;
    bool FindBestUsableTargetHit(const FVector& ViewLocation, const FVector& ViewDirection, const AActor* CarriedActor, FHitResult& OutBestHit) const;
    bool IsItemActor(const AActor* Actor) const;
	AEMRReceptionMachine* FindSeatedReceptionMachine() const;
    AEMRHubTabletActor* ResolveOwnedHubTablet() const;
	void UpdateHubRotationBehavior(const AEMRNightShiftGameState* RunGS);
	void UpdateWatchAvailability(const AEMRNightShiftGameState* RunGS);
	void HandleWatchOvertimeStarted();
	void PlayWatchOvertimeCue();
	void HandleWatchSpecialEventPhaseChanged(EEMRSpecialEventPhase NewPhase, FName EventId, float PhaseServerTimestamp);
	void PlayWatchSpecialEventAlertCue();
	void SetLookingAtWatch(bool bEnabled);
	void CancelActiveMontagesForSeatedChange();


	
	UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
	TObjectPtr<UInputMappingContext> GameplayInputMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
	TObjectPtr<UInputAction> JumpInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> LookInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> MoveInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> ToggleGameMenuInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> ToggleDeveloperToolsInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> PushToTalkInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> XRayMoveLeftInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> XRayMoveRightInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> XRayMoveForwardInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> XRayMoveBackwardInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> UltrasoundMoveLeftInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> UltrasoundMoveRightInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> UltrasoundMoveForwardInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> UltrasoundMoveBackwardInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TObjectPtr<UInputAction> UltrasoundSliderAdjustInputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Input")
    TMap<EAbilityInputID, TObjectPtr<const UInputAction>> GameplayAbilityInputActions;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRXRayMachine> ActiveXRayMachine;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> ActiveXRayInputMappingContext = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRUltrasoundMachine> ActiveUltrasoundMachine;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> ActiveUltrasoundInputMappingContext = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRCTScanMachine> ActiveCTScanMachine;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> ActiveCTScanInputMappingContext = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRIntravenousMedicationStand> ActiveIVStand;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> ActiveIVInputMappingContext = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMROxygenMachine> ActiveOxygenMachine;
    float CachedLocalOxygenMoveMaxWalkSpeed = 0.0f;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRLabAnalyzerMachine> ActiveLabAnalyzerMachine;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> ActiveLabAnalyzerInputMappingContext = nullptr;

    UPROPERTY()
    FTimerHandle CarriedItemRefreshHandle;

    UPROPERTY()
    TWeakObjectPtr<AActor> CachedCarriedActor;

	UPROPERTY()
	TWeakObjectPtr<AActor> CachedFirstPersonWidgetActor;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FirstPersonCarriedItemFilledOverlayMID = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> FirstPersonCarriedItemFilledOverlayCachedMesh = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> FirstPersonCarriedItemFilledOverlayCachedMaterial = nullptr;


    UPROPERTY(Transient)
    TArray<TObjectPtr<UDecalComponent>> FirstPersonTubeDecals;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> FirstPersonTubeDecalMIDs;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRLabAnalyzerTube> CachedPreviewTube;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PlacementPreviewMID = nullptr;

	bool bPlacementPreviewActive = false;
	UPROPERTY(ReplicatedUsing = "OnRep_LookingAtWatch", VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Watch", meta = (AllowPrivateAccess = "true"))
	bool bLookingAtWatch = false;
	UPROPERTY(ReplicatedUsing = "OnRep_WatchSocketLookActive", VisibleAnywhere, BlueprintReadOnly, Category = "EMR|Watch", meta = (AllowPrivateAccess = "true"))
	bool bWatchSocketLookActive = false;
	bool bWatchSocketBlendActive = false;
	float WatchSocketBlendElapsed = 0.f;
	FTransform WatchSocketBlendStart = FTransform::Identity;
	EEMRWatchSocketState ActiveWatchSocketState = EEMRWatchSocketState::StandingIdle;
	EEMRWatchSocketState PendingWatchSocketState = EEMRWatchSocketState::StandingIdle;
	FName PendingWatchSocketName = NAME_None;
	FTransform PendingWatchSocketTransform = FTransform::Identity;
	bool bHasActiveWatchSocketState = false;
	uint8 MissingWatchSocketWarningMask = 0;
	bool bHasWarnedMissingThirdPersonWatchMesh = false;
	bool bHasWarnedMissingThirdPersonWatchSocket = false;

	UPROPERTY()
	FTimerHandle HubTabletBindRetryHandle;

	FDelegateHandle HubTabletRunPhaseHandle;
	FDelegateHandle HubTabletPendingTravelHandle;
	FDelegateHandle HubTabletSelectionCommittedHandle;
    FDelegateHandle HubTabletDecisionStageHandle;
	FDelegateHandle WatchOvertimeHandle;
	FDelegateHandle WatchSpecialEventPhaseHandle;

	mutable TWeakObjectPtr<AEMRNightShiftGameState> CachedNightShiftGameState;

	UFUNCTION(Server, Reliable)
	void ServerSetLookingAtWatch(bool bInLookingAtWatch);

	

	/**********************************************************************/
	/*                           GAMEPLAY ABILITY                         */
	/**********************************************************************/
	
public:
	void GrantStartupAbilities();

private:
	UPROPERTY()
	mutable TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "EMR|GAS")
	TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|GAS")
	TArray<TSubclassOf<UGameplayAbility>> PassiveAbilities;
};
