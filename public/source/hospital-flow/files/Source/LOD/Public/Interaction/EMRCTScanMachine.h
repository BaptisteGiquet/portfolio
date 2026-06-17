#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interaction/EMRBaseMachine.h"
#include "EMRCTScanMachine.generated.h"

class UInputMappingContext;
class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;
class UAudioComponent;
class USoundBase;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UAnimMontage;
class UPrimitiveComponent;
class AEMRPlayerCharacter;
class AEMRPatient;
class UWidgetComponent;
class UEMRPatientUIController;
class UEMRTriagePatientCard;
struct FGameplayEventData;
struct FTimerHandle;
class AEMRPlayerController;

UCLASS()
class LOD_API AEMRCTScanMachine : public AEMRBaseMachine
{
    GENERATED_BODY()

public:
    AEMRCTScanMachine();

    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void TryStartExam(AEMRPlayerCharacter* Player);
    void StopExam(AEMRPlayerCharacter* Player, bool bFromCancel);
    void HandleStartButtonPressed(AEMRPlayerCharacter* Player);
    void HandleValidateButtonPressed(AEMRPlayerCharacter* Player);
    int32 GetButtonIndexForComponent(const UPrimitiveComponent* Component) const; // 0=start, 1=validate, INDEX_NONE

    UFUNCTION(BlueprintPure, Category = "EMR|CTScan|State")
    AEMRPlayerCharacter* GetCurrentOperator() const { return CurrentOperator; }

protected:
    virtual void BeginPlay() override;
    virtual bool ShouldReleasePatientOnExamCompleted(FGameplayTag ExamTag) const override;

    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan|Components")
    TObjectPtr<UStaticMeshComponent> TableMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan|Components")
    TObjectPtr<UDecalComponent> LaserZoneDecal = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan|Components")
    TObjectPtr<UStaticMeshComponent> StartButtonMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan|Components")
    TObjectPtr<UStaticMeshComponent> ValidateButtonMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan|Components")
    TObjectPtr<USceneComponent> PatientAttachComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan|Components")
    TObjectPtr<UArrowComponent> OperatorLockPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan|Components")
    TObjectPtr<UArrowComponent> OperatorExitPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan|Components")
    TObjectPtr<UWidgetComponent> PatientCardWidgetComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Input")
    TObjectPtr<UInputMappingContext> ExamInputMappingContext = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Movement", meta = (ClampMin = "0.0"))
    float TableMoveSpeed = 60.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Movement", meta = (ClampMin = "0.0"))
    float TableReturnSpeed = 120.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Movement")
    float TableMinOffset = -80.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Movement")
    float TableMaxOffset = 80.0f;

    UPROPERTY(VisibleAnywhere, Category = "EMR|CTScan|Audio")
    TObjectPtr<UAudioComponent> TableMovementAudioComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Audio")
    TObjectPtr<USoundBase> TableMovementLoopSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Audio", meta = (ClampMin = "0.0"))
    float TableMovementLoopFadeInSeconds = 0.08f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Audio", meta = (ClampMin = "0.0"))
    float TableMovementLoopFadeOutSeconds = 0.15f;

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons")
    FVector ButtonPressOffset = FVector(0.0f, 0.0f, -2.0f);

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons", meta = (ClampMin = "0.0"))
    float ButtonPressInterpSpeed = 18.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons", meta = (ClampMin = "0.0"))
    float ValidatePressHoldSeconds = 0.14f;

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons", meta = (ClampMin = "0.0"))
    float ValidateSuccessDelaySeconds = 0.14f;

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons", meta = (ClampMin = "0.0"))
    float ValidateFailureFlickerDuration = 0.12f;

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons", meta = (ClampMin = "0.0"))
    float ValidateFailurePauseSeconds = 0.5f;

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons", meta = (ClampMin = "1"))
    int32 ValidateFailureFlickerToggleCount = 2;

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons")
    FName ButtonColorParameterName = TEXT("Color");

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons")
    FLinearColor StartEnabledColor = FLinearColor(0.1f, 0.9f, 0.2f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons")
    FLinearColor StartDisabledColor = FLinearColor(0.25f, 0.25f, 0.25f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons")
    FLinearColor ValidateEnabledColor = FLinearColor(0.95f, 0.78f, 0.12f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons")
    FLinearColor ValidateDisabledColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "EMR|CTScan|Buttons")
    FLinearColor ValidateFailureColor = FLinearColor(1.0f, 0.1f, 0.1f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Targets", meta = (ClampMin = "0.0"))
    float TargetSocketToleranceRadius = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Patient")
    FName PatientAttachSocketName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Patient")
    FName PatientMeshComponentName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Patient")
    FName PatientMeshComponentTag = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Patient")
    TObjectPtr<UAnimMontage> PatientEnterTableMontage = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Audio")
    TObjectPtr<USoundBase> SuccessSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|CTScan|Audio")
    TObjectPtr<USoundBase> FailureSound = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentOperator, VisibleAnywhere, Category = "EMR|CTScan|State")
    TObjectPtr<AEMRPlayerCharacter> CurrentOperator = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_AttachedPatient, VisibleAnywhere, Category = "EMR|CTScan|State")
    TObjectPtr<AEMRPatient> AttachedPatient = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_TargetSocket, VisibleAnywhere, Category = "EMR|CTScan|State")
    FName TargetSocketName = NAME_None;

    UPROPERTY(ReplicatedUsing = OnRep_TableState, VisibleAnywhere, Category = "EMR|CTScan|State")
    float TableOffset = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_TableState, VisibleAnywhere, Category = "EMR|CTScan|State")
    bool bTableMoving = false;

    UPROPERTY(ReplicatedUsing = OnRep_TableState, VisibleAnywhere, Category = "EMR|CTScan|State")
    bool bTableReturning = false;

    UPROPERTY(ReplicatedUsing = OnRep_TableState, VisibleAnywhere, Category = "EMR|CTScan|State")
    bool bLaserActive = false;

    UPROPERTY(ReplicatedUsing = OnRep_ButtonState, VisibleAnywhere, Category = "EMR|CTScan|State")
    bool bStartButtonEnabled = true;

    UPROPERTY(ReplicatedUsing = OnRep_ButtonState, VisibleAnywhere, Category = "EMR|CTScan|State")
    bool bValidateButtonEnabled = false;

    UPROPERTY(ReplicatedUsing = OnRep_ButtonState, VisibleAnywhere, Category = "EMR|CTScan|State")
    bool bStartButtonPressed = false;

    UPROPERTY(ReplicatedUsing = OnRep_ButtonState, VisibleAnywhere, Category = "EMR|CTScan|State")
    bool bValidateButtonPressed = false;

    UPROPERTY(ReplicatedUsing = OnRep_ValidateFailurePulseCounter, VisibleAnywhere, Category = "EMR|CTScan|State")
    uint8 ValidateFailurePulseCounter = 0;

    UFUNCTION()
    void OnRep_CurrentOperator();

    UFUNCTION()
    void OnRep_AttachedPatient();

    UFUNCTION()
    void OnRep_TargetSocket();

    UFUNCTION()
    void OnRep_TableState();

    UFUNCTION()
    void OnRep_ButtonState();

    UFUNCTION()
    void OnRep_ValidateFailurePulseCounter();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayPatientEnterMontage(AEMRPatient* Patient);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_StopPatientEnterMontage(AEMRPatient* Patient);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayResultSound(USoundBase* Sound, FVector Location);

private:
    void LockOperator(AEMRPlayerCharacter* Player);
    void UnlockOperator(AEMRPlayerCharacter* Player);
    void TryAttachPatient();
    void DetachPatient(AEMRPatient* Patient);
    void RefreshTargetSocket();
    USkeletalMeshComponent* ResolvePatientMesh() const;
    USkeletalMeshComponent* ResolvePatientMeshFor(const AEMRPatient* Patient) const;
    void UpdateTableMotion(float DeltaSeconds);
    void ApplyTableOffset();
    void CacheTableBaseLocation();
    void ResetExamState(bool bKeepOperator);
    void ResetButtonState();
    bool IsStartButtonInteractable() const;
    bool IsValidateButtonInteractable() const;
    void TriggerValidateFailurePulse();
    void HandleValidatePressRelease();
    void HandleDeferredValidateSuccess();
    void HandleValidateFailureResume();
    void CacheButtonBaseTransforms();
    void EnsureButtonMaterialInstances();
    void UpdateButtons(float DeltaSeconds);
    void ApplyButtonTransforms();
    void ApplyButtonColors();
    bool IsTableMovementAudioActive() const;
    void RefreshTableMovementAudioPlayback();
    void StartValidateFailureFlicker();
    bool IsValidateFailureFlickerVisible() const;
    bool IsLaserOnTarget() const;
    void TriggerResultCue(bool bSuccess);
    void TryCompleteExam();
    bool TrySendExamGameplayEvent(AEMRPlayerCharacter* Operator, const FGameplayEventData& EventData);
    void PlayPatientEnterMontageFor(AEMRPatient* Patient);
    void StopPatientEnterMontageFor(AEMRPatient* Patient);
    void RefreshPatientCardWidget();
    UEMRTriagePatientCard* ResolvePatientCardWidget();

    int32 TableDirection = 1;
    bool bHasCachedBaseLocation = false;
    FVector TableBaseWorldLocation = FVector::ZeroVector;
    TWeakObjectPtr<AEMRPatient> CachedAssignedPatient;
    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPlayerController> CachedOperatorController;
    UPROPERTY(Transient)
    mutable TWeakObjectPtr<USkeletalMeshComponent> CachedTargetMesh;
    bool bPatientOnTable = false;
    bool bCompletionPending = false;
    bool bKeepOperatorAfterCompletion = false;

    TWeakObjectPtr<AEMRPatient> CachedPatientTransformOwner;
    FTransform CachedPatientTransform;
    bool bHasCachedPatientTransform = false;

    UPROPERTY(Transient)
    TObjectPtr<UEMRPatientUIController> PatientCardUIController = nullptr;
    UPROPERTY(Transient)
    TWeakObjectPtr<UEMRTriagePatientCard> CachedPatientCardWidget;
    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPatient> CachedPatientCardPatient;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> StartButtonMID = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ValidateButtonMID = nullptr;

    bool bButtonTransformsCached = false;
    FTransform StartButtonBaseRelativeTransform = FTransform::Identity;
    FTransform ValidateButtonBaseRelativeTransform = FTransform::Identity;
    float StartButtonPressAlpha = 0.0f;
    float ValidateButtonPressAlpha = 0.0f;
    bool bValidateFailureFlickerActive = false;
    float ValidateFailureFlickerElapsed = 0.0f;
    bool bIsTableMovementAudioPlaying = false;
    bool bHasLoggedMissingTableMovementAudioComponent = false;
    bool bHasLoggedMissingTableMovementLoopSound = false;
    int8 LastLoggedTableMovementAudioDesiredState = -1;

    FTimerHandle ValidatePressReleaseTimerHandle;
    FTimerHandle ValidateSuccessTimerHandle;
    FTimerHandle ValidateFailureResumeTimerHandle;
    bool bDeferredValidateSuccess = false;
};
