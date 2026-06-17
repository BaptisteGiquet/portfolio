#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "GameFramework/Actor.h"
#include "EMRIntravenousMedicationStand.generated.h"

class UArrowComponent;
class UInputMappingContext;
class UEMRInteractableComponent;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;
class UCableComponent;
class AEMRPatient;
class AEMRPlayerCharacter;
class AEMRItemActor;
class AEMRTreatmentBed;
class USkeletalMeshComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EEMRIVStandCannulaSide : uint8
{
    Left UMETA(DisplayName = "Left"),
    Right UMETA(DisplayName = "Right"),
};

UCLASS()
class LOD_API AEMRIntravenousMedicationStand : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMRIntravenousMedicationStand();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void TryStartTreatment(AEMRPlayerCharacter* Player);
    void StopTreatment(AEMRPlayerCharacter* Player, bool bFromCancel);
    void HandleConfirmPressed(AEMRPlayerCharacter* Player);
    bool TryInstallBag(AEMRPlayerCharacter* Player, AEMRItemActor* BagActor);

    UFUNCTION(BlueprintPure, Category = "EMR|IV|State")
    AEMRPlayerCharacter* GetCurrentOperator() const { return CurrentOperator; }

    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

protected:
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "EMR|IV|Components")
    TObjectPtr<UStaticMeshComponent> StandMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|IV|Components")
    TObjectPtr<UStaticMeshComponent> CannulaMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|IV|Components")
    TObjectPtr<UStaticMeshComponent> BagMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|IV|Components")
    TObjectPtr<UStaticMeshComponent> BagInteriorMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|IV|Components")
    TObjectPtr<USceneComponent> TubeStartAnchor = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|IV|Components")
    TObjectPtr<UCableComponent> TubeCable = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|IV|Components")
    TObjectPtr<UArrowComponent> OperatorLockPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|IV|Components")
    TObjectPtr<UArrowComponent> OperatorExitPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|IV|Components")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|IV|Input")
    TObjectPtr<UInputMappingContext> TreatmentInputMappingContext = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "EMR|IV|Patient")
    TObjectPtr<AEMRTreatmentBed> AssignedBed = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|IV|Patient")
    FName PatientMeshComponentName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|IV|Patient")
    FName PatientMeshComponentTag = NAME_None;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Patient")
    EEMRIVStandCannulaSide CannulaTargetSide = EEMRIVStandCannulaSide::Left;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Patient")
    FName LeftTargetSocketName = TEXT("iv_socket");

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Patient")
    FName RightTargetSocketName = TEXT("iv_socket");

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Cannula")
    FVector CannulaBounds = FVector(15.0f, 15.0f, 0.0f);

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Cannula", meta = (ClampMin = "0.0"))
    float CannulaMoveSpeed = 15.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Cannula", meta = (ClampMin = "0.0"))
    float TargetToleranceRadius = 8.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Cannula")
    FRotator CannulaRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Cannula")
    FVector CannulaLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Cannula", meta = (ClampMin = "0.0"))
    float MissPauseDuration = 1.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Cannula")
    FName CannulaOpacityParameterName = TEXT("Opacity");

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Cannula", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CannulaReadyOpacity = 1.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Cannula", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CannulaDimOpacity = 0.35f;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Bag")
    FName BagFillParameterName = TEXT("FillAmount");

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Bag", meta = (ClampMin = "0.01"))
    float BagFillNetUpdateInterval = 0.1f;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Tube")
    bool bTubeRequiresCannula = true;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Tube")
    bool bTubeAlwaysVisible = true;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Tube")
    FName TubeEndSocketName = TEXT("cable_socket");

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Tube", meta = (ClampMin = "0.0", UIMin = "0.0"))
    float CableDepleteDurationSeconds = 1.5f;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Debug")
    bool bDebugCannulaTargets = false;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Debug", meta = (ClampMin = "0.01", UIMin = "0.01"))
    float DebugPointSize = 6.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Debug", meta = (ClampMin = "0.1", UIMin = "0.1"))
    float DebugPointDuration = 2.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Audio")
    TObjectPtr<USoundBase> SuccessResultSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Audio")
    TObjectPtr<USoundBase> FailureResultSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Audio")
    TObjectPtr<USoundBase> MissingBagSound = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|IV|Audio")
    TObjectPtr<USoundBase> BagInstalledSound = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentPatient)
    TObjectPtr<AEMRPatient> CurrentPatient = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentOperator)
    TObjectPtr<AEMRPlayerCharacter> CurrentOperator = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_CannulaOffset)
    FVector CannulaOffset = FVector::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_CannulaActive)
    bool bCannulaActive = false;

    UPROPERTY(ReplicatedUsing = OnRep_CannulaAttached)
    bool bCannulaAttached = false;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|IV|Cannula", meta = (ClampMin = "0.01"))
    float CannulaNetUpdateInterval = 0.1f;

    UPROPERTY(Replicated)
    bool bCompletionPending = false;

    UPROPERTY(ReplicatedUsing = OnRep_BagInstalled)
    bool bBagInstalled = false;

    UPROPERTY(ReplicatedUsing = OnRep_BagFill)
    float BagFillNormalized = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_CableFill)
    float CableFillNormalized = 0.0f;

    UPROPERTY()
    FTimerHandle CompletionTimer;

    UPROPERTY()
    FTimerHandle MissPauseTimer;

    UPROPERTY()
    bool bCannulaMovementPaused = false;

    UFUNCTION()
    void OnRep_CurrentPatient();

    UFUNCTION()
    void OnRep_CurrentOperator();

    UFUNCTION()
    void OnRep_CannulaOffset();

    UFUNCTION()
    void OnRep_CannulaActive();

    UFUNCTION()
    void OnRep_CannulaAttached();

    UFUNCTION()
    void OnRep_BagInstalled();

    UFUNCTION()
    void OnRep_BagFill();

    UFUNCTION()
    void OnRep_CableFill();

private:
    bool CanAutoStartTreatmentAfterBagInstall() const;
    void BeginAutomaticTreatmentAfterBagInstall(AEMRPlayerCharacter* InstallingPlayer);
    void PlaceCannulaAutomatically();

    void UpdateCurrentPatient();
    void UpdateCannulaMovement(float DeltaSeconds);
    void UpdateCannulaMovementOnClient(float DeltaSeconds);
    void ApplyCannulaOffset(const FVector& Offset);
    void UpdateCannulaVisibility();
    void UpdateCannulaOpacity(bool bInRange);
    void PickNewCannulaTarget();
    bool IsCannulaWithinTolerance() const;
    bool IsOffsetWithinTolerance(const FVector& Offset) const;
    bool IsCurrentPatientAttachedToBed() const;

    bool DoesPatientNeedTreatment(const AEMRPatient* Patient) const;
    bool IsPatientLeaving(const AEMRPatient* Patient) const;
    bool HasValidSocket(const AEMRPatient* Patient) const;
    FName GetConfiguredTargetSocketName() const;
    bool ResolveTargetSocket(
        const AEMRPatient* Patient,
        USkeletalMeshComponent*& OutPatientMesh,
        FName& OutSocketName) const;
    USkeletalMeshComponent* ResolvePatientMeshFor(const AEMRPatient* Patient) const;
    bool HasFullBag() const;

    void LockOperator(AEMRPlayerCharacter* Player);
    void UnlockOperator(AEMRPlayerCharacter* Player);
    void StartCompletionTimer();
    void HandleCompletionTimer();
    void ClearCompletionTimer();
    void SetCurrentPatientPatienceDrainSuppressed(bool bSuppressed);
    void ClearPatienceDrainSuppression();
    void TriggerResultCue(bool bSuccess);
    void PlayBagCue(USoundBase* Sound, AEMRPlayerCharacter* Player) const;
    void StartBagDepletion(float DurationSeconds);
    void StopBagDepletion();
    void CompleteBagDepletion();
    void StartCableDepletion(float DurationSeconds);
    void StopCableDepletion();
    void CompleteCableDepletion();
    void UpdateCableFillMaterial();
    void FinishTreatment();
    void UpdateBagVisuals();
    void UpdateBagFillMaterial();
    void UpdateTubeCable();
    bool ShouldShowTube() const;
    void DebugDrawCannulaPoints() const;

    void StartMissPause();
    void HandleMissPauseTimer();
    void AttachCannulaToPatient();
    void DetachCannulaFromPatient();

    FVector ServerCannulaOffset = FVector::ZeroVector;
    FVector CannulaTargetOffset = FVector::ZeroVector;
    float CannulaNetUpdateAccumulator = 0.0f;
    bool bHasCannulaTarget = false;
    mutable bool bLoggedMissingSocket = false;
    bool bBagDepleting = false;
    float BagDepleteStartTime = 0.0f;
    float BagDepleteDuration = 0.0f;
    float BagDepleteStartFill = 0.0f;
    float BagFillNetUpdateAccumulator = 0.0f;
    bool bCableDepleting = false;
    float CableDepleteStartTime = 0.0f;
    float CableDepleteDuration = 0.0f;
    float CableDepleteStartFill = 0.0f;
    float CableFillNetUpdateAccumulator = 0.0f;
    float PlannedCableDuration = 0.0f;
    bool bClientCannulaOffsetInitialized = false;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CannulaMID = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BagInteriorMID = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> BagInteriorSourceMaterial = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CableMID = nullptr;

    mutable TWeakObjectPtr<USkeletalMeshComponent> CachedPatientMesh;
    mutable TWeakObjectPtr<const AEMRPatient> CachedPatientMeshOwner;
    TWeakObjectPtr<AEMRPatient> PatienceDrainSuppressedPatient;

};
