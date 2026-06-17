#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interaction/EMRBaseMachine.h"
#include "TimerManager.h"
#include "EMRXRayMachine.generated.h"

class UInputMappingContext;
class UAnimMontage;
class UArrowComponent;
class USceneComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UCanvasRenderTarget2D;
class UAudioComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USoundBase;
class UTexture2D;
class UCanvas;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UPrimitiveComponent;
class UWidgetComponent;
class UEMRPatientUIController;
class UEMRTriagePatientCard;
struct FGameplayEventData;
struct FHitResult;
class AEMRPlayerCharacter;
class AEMRPlayerController;
class AEMRPatient;

UCLASS()
class LOD_API AEMRXRayMachine : public AEMRBaseMachine
{
    GENERATED_BODY()

public:
    AEMRXRayMachine();

    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void TryStartExam(AEMRPlayerCharacter* Player);
    void StopExam(AEMRPlayerCharacter* Player, bool bFromCancel);
    void SetMoveInput(AEMRPlayerCharacter* Player, float InputX, float InputY);

    UFUNCTION(BlueprintPure, Category = "EMR|XRay|State")
    AEMRPlayerCharacter* GetCurrentOperator() const { return CurrentOperator; }

protected:
    virtual void BeginPlay() override;
    virtual bool ShouldReleasePatientOnExamCompleted(FGameplayTag ExamTag) const override;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Components")
    TObjectPtr<UStaticMeshComponent> XAxisMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Components")
    TObjectPtr<UStaticMeshComponent> YAxisMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Components")
    TObjectPtr<UStaticMeshComponent> SeatMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Components")
    TObjectPtr<UArrowComponent> OperatorLockPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Components")
    TObjectPtr<UArrowComponent> OperatorExitPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Components")
    TObjectPtr<UArrowComponent> SeatPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Components")
    TObjectPtr<UArrowComponent> SeatExitPoint = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|XRay|Components")
    TObjectPtr<USceneComponent> SeatAttachComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Input")
    TObjectPtr<UInputMappingContext> ExamInputMappingContext = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Monitor")
    TObjectPtr<USceneComponent> MonitorCaptureRoot = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Monitor")
    TObjectPtr<USceneCaptureComponent2D> MonitorCapture = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor")
    TObjectPtr<UTextureRenderTarget2D> MonitorRenderTarget = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor")
    TObjectPtr<UCanvasRenderTarget2D> MonitorOverlayRenderTarget = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor")
    TObjectPtr<UMaterialInterface> MonitorScreenMaterial = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Monitor")
    TObjectPtr<UStaticMeshComponent> MonitorScreenMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Components")
    TObjectPtr<UWidgetComponent> PatientCardWidgetComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor")
    FName MonitorSceneTextureParamName = TEXT("SceneTexture");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor")
    FName MonitorOverlayTextureParamName = TEXT("OverlayTexture");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor", meta = (ClampMin = "1"))
    int32 MonitorRenderTargetSize = 512;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor", meta = (ClampMin = "1.0"))
    float MonitorOrthoWidth = 200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor", meta = (ClampMin = "0.0"))
    float MonitorAimDotSize = 8.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor", meta = (ClampMin = "0.0"))
    float MonitorFoundDotSize = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor", meta = (ClampMin = "0.0"))
    float MonitorOverlayRefreshIntervalSeconds = 0.1f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor")
    FLinearColor MonitorAimColor = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor")
    FLinearColor MonitorFoundColor = FLinearColor::Green;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Monitor")
    TObjectPtr<UTexture2D> MonitorDotTexture = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Movement", meta = (ClampMin = "0.0"))
    float XMoveSpeed = 80.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Movement", meta = (ClampMin = "0.0"))
    float YMoveSpeed = 80.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Movement")
    float XMinOffset = -50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Movement")
    float XMaxOffset = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Movement")
    float YMinOffset = -50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Movement")
    float YMaxOffset = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Targets", meta = (ClampMin = "0.0"))
    float TargetSocketToleranceRadius = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Targets", meta = (ClampMin = "0.0"))
    float PatientAttachDistance = 80.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Patient")
    TObjectPtr<USceneComponent> PatientAttachComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Patient")
    FName PatientAttachSocketName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Patient")
    FName PatientMeshComponentName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Patient")
    FName PatientMeshComponentTag = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Patient")
    TObjectPtr<UAnimMontage> PatientEnterTableMontage = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Audio")
    TObjectPtr<USoundBase> TargetFoundSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Cue", meta = (ClampMin = "0.0"))
    float TargetFoundCueDuration = 0.75f;

    UPROPERTY(VisibleAnywhere, Category = "EMR|XRay|Audio")
    TObjectPtr<UAudioComponent> MovementLoopAudioComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Audio")
    TObjectPtr<USoundBase> MovementLoopSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Audio")
    TObjectPtr<USoundBase> SeatEnterSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Audio")
    TObjectPtr<USoundBase> SeatExitSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Audio", meta = (ClampMin = "0.0"))
    float MovementLoopFadeInSeconds = 0.08f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Audio", meta = (ClampMin = "0.0"))
    float MovementLoopFadeOutSeconds = 0.15f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Audio", meta = (ClampMin = "0.0"))
    float MovementLoopHearRadius = 2000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Audio", meta = (ClampMin = "0.0"))
    float MovementLoopStopGraceSeconds = 0.06f;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentOperator, VisibleAnywhere, Category = "EMR|XRay|State")
    TObjectPtr<AEMRPlayerCharacter> CurrentOperator = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|XRay|Seat")
    FGameplayTag SeatAnimationTag;

    UPROPERTY(ReplicatedUsing = OnRep_XAxisOffset, VisibleAnywhere, Category = "EMR|XRay|State")
    float XAxisOffset = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_YAxisOffset, VisibleAnywhere, Category = "EMR|XRay|State")
    float YAxisOffset = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_FoundSocketNames, VisibleAnywhere, Category = "EMR|XRay|State")
    TArray<FName> FoundSocketNamesReplicated;

    UPROPERTY(ReplicatedUsing = OnRep_AttachedPatient, VisibleAnywhere, Category = "EMR|XRay|State")
    TObjectPtr<AEMRPatient> AttachedPatient = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_MovementLoopActive, VisibleAnywhere, Category = "EMR|XRay|State")
    bool bMovementLoopActive = false;

    UFUNCTION()
    void OnRep_CurrentOperator();

    UFUNCTION()
    void OnRep_XAxisOffset();

    UFUNCTION()
    void OnRep_YAxisOffset();

    UFUNCTION()
    void OnRep_FoundSocketNames();

    UFUNCTION()
    void OnRep_AttachedPatient();

    UFUNCTION()
    void OnRep_MovementLoopActive();

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayPatientEnterMontage(AEMRPatient* Patient);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_StopPatientEnterMontage(AEMRPatient* Patient);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayWorldSoundAtLocation(USoundBase* Sound, FVector_NetQuantize Location);

private:
    void SeatOperator(AEMRPlayerCharacter* Player);
    void UnseatOperator(AEMRPlayerCharacter* Player);
    void AttachSeatMeshToPlayer(AEMRPlayerCharacter* Player);
    void DetachSeatMeshToOriginal();
    void UpdateSeatMeshYawFromPlayer(float DeltaSeconds);
    void ApplySeatMeshYawOffset();
    UFUNCTION()
    void OnRep_SeatMeshYawOffset();

    void UpdateHeadOffsets(float DeltaSeconds);
    void ApplyAxisOffsets();
    void TryAttachPatient();
    void DetachPatient(AEMRPatient* Patient);
    void RefreshRequiredSockets();
    void ValidateTargetSockets();
    void HandleTargetFound(const FName& SocketName, const FVector& SocketLocation);
    void HandleCueFinished();
    bool TrySendExamGameplayEvent(AEMRPlayerCharacter* Operator, const FGameplayEventData& EventData);
    void TryCompleteExam();
    void ResetExamState(bool bKeepOperator);
    void PlayPatientEnterMontageFor(AEMRPatient* Patient);
    void StopPatientEnterMontageFor(AEMRPatient* Patient);
    USkeletalMeshComponent* ResolvePatientMesh() const;
    USkeletalMeshComponent* ResolvePatientMeshFor(const AEMRPatient* Patient) const;
    void HandleAttachedPatientChanged();
    bool TryPlayAttachedPatientMontage();
    void CacheAxisBaseLocations();
    void UpdateMovementLoopState(bool bHeadMovedThisTick, float DeltaSeconds);
    void SetMovementLoopActive(bool bNewActive);
    void RefreshMovementLoopAudioPlayback();
    void InitializeMonitorCapture();
    void RefreshMonitorCaptureTargets();
    void UpdateMonitorOverlay(bool bForce = false);
    void RefreshMonitorCaptureActivity();
    bool ShouldRefreshMonitorOverlayContinuously() const;
    FVector2D ComputeMonitorUV(const FVector& WorldLocation) const;
    void DrawMonitorDot(UCanvas* Canvas, const FVector2D& UV, float Size, const FLinearColor& Color, int32 Width, int32 Height) const;
    void RefreshPatientCardWidget();
    UEMRTriagePatientCard* ResolvePatientCardWidget();
    UFUNCTION()
    void HandleMonitorOverlayUpdate(UCanvas* Canvas, int32 Width, int32 Height);

    UFUNCTION()
    void OnDetectionRadiusBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnDetectionRadiusEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPatient> CachedAssignedPatient;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPlayerController> CachedOperatorController;

    UPROPERTY(Transient)
    TArray<FName> RequiredSocketNames;

    UPROPERTY(Transient)
    TSet<FName> FoundSocketNames;

    UPROPERTY(Transient)
    TArray<FTimerHandle> ActiveCueTimers;

    UPROPERTY(Transient)
    FVector2D MoveInputAxis = FVector2D::ZeroVector;

    UPROPERTY(Transient)
    mutable TWeakObjectPtr<USkeletalMeshComponent> CachedTargetMesh;

    UPROPERTY(Transient)
    bool bPatientInDetectionRadius = false;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPatient> CachedPatientTransformOwner;

    UPROPERTY(Transient)
    FTransform CachedPatientTransform;

    UPROPERTY(Transient)
    bool bHasCachedPatientTransform = false;

    UPROPERTY(Transient)
    FVector XAxisBaseWorldLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    FVector YAxisBaseWorldLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    bool bAxisBaseCached = false;

    UPROPERTY(Transient)
    float MovementLoopIdleElapsedSeconds = 0.0f;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> SeatMeshOriginalAttachParent = nullptr;

    UPROPERTY(Transient)
    FTransform SeatMeshOriginalRelativeTransform;

    UPROPERTY(Transient)
    bool bSeatMeshAttachCached = false;

    UPROPERTY(Transient)
    bool bSeatMeshFollowYaw = false;

    UPROPERTY(Transient)
    FRotator SeatMeshBaseWorldRotation = FRotator::ZeroRotator;

    UPROPERTY(Transient)
    float SeatMeshInitialPlayerYaw = 0.f;

    UPROPERTY(Transient)
    float SeatMeshInitialControlYaw = 0.f;

    UPROPERTY(ReplicatedUsing = "OnRep_SeatMeshYawOffset")
    float SeatMeshYawOffset = 0.f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|XRay|Seat", meta = (ClampMin = "0.0"))
    float SeatYawInterpSpeed = 12.0f;

    UPROPERTY(Transient)
    float SeatMeshYawOffsetSmoothed = 0.f;

    UPROPERTY(Transient)
    bool bSeatMeshYawInitialized = false;

    UPROPERTY(Transient)
    int32 PendingCueCount = 0;

    UPROPERTY(Transient)
    bool bPatientOnTable = false;

    UPROPERTY(Transient)
    bool bCompletionPending = false;

    UPROPERTY(Transient)
    bool bKeepOperatorAfterCompletion = false;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> MonitorScreenMID = nullptr;

    UPROPERTY(Transient)
    float MonitorOverlayRefreshAccumulatorSeconds = 0.0f;

    UPROPERTY(Transient)
    bool bWasRefreshingMonitorOverlay = false;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPatient> LastAttachedPatient;

    UPROPERTY(Transient)
    FTimerHandle AttachedPatientMontageRetryTimer;

    UPROPERTY(Transient)
    TObjectPtr<UEMRPatientUIController> PatientCardUIController = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<UEMRTriagePatientCard> CachedPatientCardWidget;

    UPROPERTY(Transient)
    TWeakObjectPtr<AEMRPatient> CachedPatientCardPatient;
};
