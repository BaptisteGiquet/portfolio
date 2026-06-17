#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interaction/EMRBaseMachine.h"
#include "EMRUltrasoundMachine.generated.h"

class UInputMappingContext;
class UArrowComponent;
class USceneComponent;
class UAudioComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UAnimMontage;
class USkeletalMeshComponent;
class UTexture2D;
class UPrimitiveComponent;
class USoundBase;
class UWidgetComponent;
class UEMRPatientUIController;
class UEMRTriagePatientCard;
struct FGameplayEventData;
class AEMRPlayerCharacter;
class AEMRPatient;
class AEMRPlayerController;

UCLASS()
class LOD_API AEMRUltrasoundMachine : public AEMRBaseMachine
{
    GENERATED_BODY()

public:
    AEMRUltrasoundMachine();

    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void TryStartExam(AEMRPlayerCharacter* Player);
    void StopExam(AEMRPlayerCharacter* Player, bool bFromCancel);
    void SetMoveInput(AEMRPlayerCharacter* Player, float InputX, float InputY);
    void SetActiveSlider(AEMRPlayerCharacter* Player, int32 SliderIndex);
    void SetSliderAdjustInput(AEMRPlayerCharacter* Player, float InputValue);
    void HandleCompletionCueFinished(AEMRPlayerCharacter* Operator, bool bCanceled);

    UFUNCTION(BlueprintPure, Category = "EMR|Ultrasound|State")
    AEMRPlayerCharacter* GetCurrentOperator() const { return CurrentOperator; }

    UFUNCTION(BlueprintPure, Category = "EMR|Ultrasound|State")
    FVector GetSliderCurrents() const { return SliderCurrents; }

    UFUNCTION(BlueprintPure, Category = "EMR|Ultrasound|State")
    FVector GetSliderTargets() const { return SliderTargets; }

    UFUNCTION(BlueprintPure, Category = "EMR|Ultrasound|State")
    float GetSliderMinValue() const { return SliderMinValue; }

    UFUNCTION(BlueprintPure, Category = "EMR|Ultrasound|State")
    float GetSliderMaxValue() const { return SliderMaxValue; }

    UFUNCTION(BlueprintPure, Category = "EMR|Ultrasound|State")
    int32 GetActiveSliderIndex() const { return ActiveSliderIndex; }

    int32 GetSliderKnobIndexForComponent(const UPrimitiveComponent* Component) const;

    UFUNCTION(BlueprintPure, Category = "EMR|Ultrasound|Cue")
    bool IsCompletionCueActive() const { return bCompletionCueActive; }

protected:
    virtual void BeginPlay() override;
    virtual bool ShouldReleasePatientOnExamCompleted(FGameplayTag ExamTag) const override;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UArrowComponent> OperatorLockPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UArrowComponent> OperatorExitPoint = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Components")
    TObjectPtr<USceneComponent> PatientAttachComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UStaticMeshComponent> ProbeMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UStaticMeshComponent> MonitorScreenMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UWidgetComponent> PatientCardWidgetComponent = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UStaticMeshComponent> SliderKnob1Mesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UStaticMeshComponent> SliderKnob2Mesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UStaticMeshComponent> SliderKnob3Mesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UStaticMeshComponent> SliderIndicator1Mesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UStaticMeshComponent> SliderIndicator2Mesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Components")
    TObjectPtr<UStaticMeshComponent> SliderIndicator3Mesh = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Input")
    TObjectPtr<UInputMappingContext> ExamInputMappingContext = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe")
    FName ProbeAttachSocketName = TEXT("hand_rSocket");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe")
    FTransform ProbeAttachOffset;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe", meta = (ClampMin = "0.0"))
    float ProbeMoveSpeed = 40.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe")
    FVector2D ProbeLocalBounds = FVector2D(40.0f, 40.0f);

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe")
    float ProbeSurfaceTraceHeight = 30.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe", meta = (ClampMin = "0.0"))
    float ProbeSurfaceOffset = 1.5f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe", meta = (ClampMin = "0.0"))
    float ProbeSurfaceTraceDistance = 80.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe", meta = (ClampMin = "0.0"))
    float ProbeSurfaceSweepRadius = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe", meta = (ClampMin = "0.0"))
    float ProbeSurfacePositionInterpSpeed = 14.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe", meta = (ClampMin = "0.0"))
    float ProbeSurfaceNormalInterpSpeed = 12.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe")
    TEnumAsByte<EAxis::Type> ProbeSurfaceNormalAxis = EAxis::X;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe")
    TEnumAsByte<EAxis::Type> ProbeSurfaceTangentAxis = EAxis::Y;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe")
    FRotator ProbeSurfaceRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Probe")
    bool bClampProbeToLocalBounds = false;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Debug")
    bool bUltrasoundTraceDebug = false;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Targets", meta = (ClampMin = "0.0"))
    float TargetSocketToleranceRadius = 8.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Sliders")
    float SliderMinValue = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Sliders")
    float SliderMaxValue = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Sliders", meta = (ClampMin = "0.0"))
    float SliderAdjustSpeed = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Sliders", meta = (ClampMin = "0.0"))
    float SliderTolerance = 0.05f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Sliders", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SliderTargetMinDistanceFactor = 0.2f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Knobs", meta = (ClampMin = "0.0", ClampMax = "360.0"))
    float KnobRotationMaxDegrees = 300.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Knobs")
    FRotator SliderKnob1BaseRotation = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Knobs")
    FRotator SliderKnob2BaseRotation = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Knobs")
    FRotator SliderKnob3BaseRotation = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Indicators")
    TObjectPtr<UMaterialInterface> SliderIndicatorMaterial = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Indicators")
    FName SliderIndicatorEmissiveParamName = TEXT("EmissiveColor");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Indicators")
    FLinearColor SliderIndicatorOffColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Indicators")
    FLinearColor SliderIndicatorDoneColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Audio")
    TObjectPtr<USoundBase> SliderSelectedSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Audio")
    TObjectPtr<USoundBase> SliderTargetReachedSound = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|Ultrasound|Audio")
    TObjectPtr<UAudioComponent> MovementLoopAudioComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Audio")
    TObjectPtr<USoundBase> MovementLoopSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Audio", meta = (ClampMin = "0.0"))
    float MovementLoopFadeInSeconds = 0.08f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Audio", meta = (ClampMin = "0.0"))
    float MovementLoopFadeOutSeconds = 0.15f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Audio", meta = (ClampMin = "0.0"))
    float MovementLoopHearRadius = 2000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Audio", meta = (ClampMin = "0.0"))
    float MovementLoopStopGraceSeconds = 0.06f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Screen")
    TObjectPtr<UMaterialInterface> UltrasoundScreenMaterial = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Screen")
    FName FlipbookTextureParamName = TEXT("UltrasoundFlipbook");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Screen")
    TObjectPtr<UTexture2D> DefaultUltrasoundScreenTexture = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Screen")
    TArray<TObjectPtr<UTexture2D>> UltrasoundFlipbooks;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Screen")
    FName NoiseParamName = TEXT("NoiseAmount");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Screen", meta = (ClampMin = "0.0"))
    float SliderNoiseInterpSpeed = 6.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Screen")
    FName Slider1ParamName = TEXT("Slider1");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Screen")
    FName Slider2ParamName = TEXT("Slider2");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Screen")
    FName Slider3ParamName = TEXT("Slider3");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Patient")
    FName PatientAttachSocketName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Patient")
    FName PatientMeshComponentName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Patient")
    FName PatientMeshComponentTag = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Patient")
    TObjectPtr<UAnimMontage> PatientEnterTableMontage = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Audio")
    TObjectPtr<USoundBase> TargetFoundSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Audio")
    TObjectPtr<USoundBase> CompletionSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Ultrasound|Cue", meta = (ClampMin = "0.0"))
    float CompletionCueDuration = 1.5f;




    UPROPERTY(ReplicatedUsing = OnRep_CurrentOperator, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    TObjectPtr<AEMRPlayerCharacter> CurrentOperator = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_AttachedPatient, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    TObjectPtr<AEMRPatient> AttachedPatient = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_TargetSocket, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    FName TargetSocketName = NAME_None;

    UPROPERTY(ReplicatedUsing = OnRep_ProbeLocked, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    bool bProbeLocked = false;

    UPROPERTY(ReplicatedUsing = OnRep_ProbeTransform, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    FVector ProbeWorldLocation = FVector::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_ProbeTransform, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    FRotator ProbeWorldRotation = FRotator::ZeroRotator;

    UPROPERTY(ReplicatedUsing = OnRep_Sliders, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    FVector SliderTargets = FVector::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_Sliders, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    FVector SliderCurrents = FVector::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_Sliders, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    float SliderNoiseAmount = 1.0f;

    bool bSliderNoiseSmoothingActive = false;

    UPROPERTY(ReplicatedUsing = OnRep_ActiveSlider, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    int32 ActiveSliderIndex = 0;

    UPROPERTY(ReplicatedUsing = OnRep_ActivePathology, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    FGameplayTag ActivePathologyTag;

    UPROPERTY(ReplicatedUsing = OnRep_ActiveFlipbook, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    TObjectPtr<UTexture2D> ActiveFlipbookTexture = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_MovementLoopActive, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    bool bMovementLoopActive = false;

    UFUNCTION()
    void OnRep_CurrentOperator();

    UFUNCTION()
    void OnRep_AttachedPatient();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayPatientEnterMontage(AEMRPatient* Patient);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_StopPatientEnterMontage(AEMRPatient* Patient);

    UFUNCTION()
    void OnRep_TargetSocket();

    UFUNCTION()
    void OnRep_ProbeLocked();

    UFUNCTION()
    void OnRep_ProbeTransform();

    UFUNCTION()
    void OnRep_Sliders();

    UFUNCTION()
    void OnRep_ActiveSlider();

    UFUNCTION()
    void OnRep_ActivePathology();

    UFUNCTION()
    void OnRep_ActiveFlipbook();

    UFUNCTION()
    void OnRep_MovementLoopActive();

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlaySliderSelectSound();

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlaySliderTargetReachedSound();

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayWorldSoundAtLocation(USoundBase* Sound, FVector_NetQuantize Location);

private:
    bool BeginCompletionCue(AEMRPlayerCharacter* Operator);
    void LockOperator(AEMRPlayerCharacter* Player);
    void UnlockOperator(AEMRPlayerCharacter* Player);
    void AttachProbeToOperator(AEMRPlayerCharacter* Player);
    void DetachProbe();
    void TryAttachPatient();
    void DetachPatient(AEMRPatient* Patient);
    void RefreshTargetSocket();
    UStaticMeshComponent* ResolveSkinProxyComponent();
    bool TraceProbeSurface(const FTransform& AnchorTransform, const FVector& DesiredPlanePoint, FHitResult& OutHit) const;
    FRotator MakeProbeSurfaceRotation(const FTransform& AnchorTransform, const FVector& SurfaceNormal) const;
    void UpdateProbePosition(float DeltaSeconds);
    void ApplyProbeTransform();
    void ValidateProbeTarget();
    void UpdateSliderPhase(float DeltaSeconds);
    void UpdateScreenMaterial();
    void TryCompleteExam();
    void ResetExamState(bool bKeepOperator);
    bool TrySendExamGameplayEvent(AEMRPlayerCharacter* Operator, const FGameplayEventData& EventData);
    USkeletalMeshComponent* ResolvePatientMesh() const;
    USkeletalMeshComponent* ResolvePatientMeshFor(const AEMRPatient* Patient) const;
    void InitializeScreenMaterial();
    float GetSliderValueByIndex(const FVector& Values, int32 Index) const;
    void SetSliderValueByIndex(FVector& Values, int32 Index, float Value) const;
    bool IsSliderLocked(int32 Index) const;
    void UpdateSliderWidgetState();
    void InitializeSliderIndicatorMaterials();
    void UpdateSliderIndicatorState();
    void UpdateKnobRotations();
    UStaticMeshComponent* GetKnobComponentByIndex(int32 Index) const;
    UStaticMeshComponent* GetSliderIndicatorComponentByIndex(int32 Index) const;
    FRotator GetKnobBaseRotationByIndex(int32 Index) const;
    float ComputeSliderNoise() const;
    void ResetSliderTargetReachedState();
    void HandleLocalSliderTargetReachedSounds();
    void PlayLocalSliderSelectSound() const;
    void PlayLocalSliderTargetReachedSound() const;
    void UpdateMovementLoopState(bool bProbeMovedThisTick, float DeltaSeconds);
    void SetMovementLoopActive(bool bNewActive);
    void RefreshMovementLoopAudioPlayback();
    void UpdateOperatorHandTarget();
    void PlayPatientEnterMontageFor(AEMRPatient* Patient);
    void StopPatientEnterMontageFor(AEMRPatient* Patient);
    void HandleAttachedPatientChanged();
    bool TryPlayAttachedPatientMontage();
    void BuildRemainingUltrasoundPathologies();
    bool SelectActivePathology(bool bForceNew);
    void RefreshTargetSocketForActivePathology();
    UTexture2D* SelectRandomFlipbook() const;
    void ResetExamForNextPathology();
    bool TryAdvanceToNextPathology();
    void RandomizeSliderValues();
    float GetMinSliderTargetDistance() const;
    float PickRandomSliderTarget(float StartValue) const;
    void RefreshPatientCardWidget();
    UEMRTriagePatientCard* ResolvePatientCardWidget();

    FVector2D ProbeInputAxis = FVector2D::ZeroVector;
    FVector2D ProbeLocalOffset = FVector2D::ZeroVector;
    float SliderAdjustInput = 0.0f;
    FVector SliderStartValues = FVector::ZeroVector;
    FVector SliderInitialDeltas = FVector::ZeroVector;
    TWeakObjectPtr<UStaticMeshComponent> CachedSkinProxyComponent;
    FVector SmoothedProbeContactPoint = FVector::ZeroVector;
    FVector SmoothedProbeSurfaceNormal = FVector::UpVector;
    FVector LastProbeContactPoint = FVector::ZeroVector;
    FVector LastProbeSurfaceNormal = FVector::UpVector;
    bool bHasProbeSurfaceHit = false;
    bool bHasWarnedMissingProxy = false;
    float MovementLoopIdleElapsedSeconds = 0.0f;

    bool bPatientOnTable = false;
    bool bHasCachedPatientTransform = false;
    bool bCompletionPending = false;
    UPROPERTY(Replicated, VisibleAnywhere, Category = "EMR|Ultrasound|State")
    bool bCompletionCueActive = false;
    TWeakObjectPtr<AEMRPlayerCharacter> CompletionCueOperator;
    FTimerHandle CompletionCueTimer;
    FTransform CachedPatientTransform;
    TWeakObjectPtr<AEMRPatient> CachedPatientTransformOwner;
    TWeakObjectPtr<AEMRPatient> CachedAssignedPatient;
    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPlayerController> CachedOperatorController;
    UPROPERTY(Transient)
    TObjectPtr<UEMRPatientUIController> PatientCardUIController = nullptr;
    UPROPERTY(Transient)
    TWeakObjectPtr<UEMRTriagePatientCard> CachedPatientCardWidget;
    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPatient> CachedPatientCardPatient;
    TWeakObjectPtr<AEMRPlayerCharacter> CachedPreviousOperator;
    mutable TWeakObjectPtr<USkeletalMeshComponent> CachedTargetMesh;
    TObjectPtr<UMaterialInstanceDynamic> MonitorMaterialInstance = nullptr;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> SliderIndicatorMIDs;
    UPROPERTY(Transient)
    TArray<bool> SliderTargetReachedPlayed;
    TArray<FGameplayTag> RemainingUltrasoundPathologies;
    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPatient> LastAttachedPatient;

    UPROPERTY(Transient)
    FTimerHandle AttachedPatientMontageRetryTimer;
};
