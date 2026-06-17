#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "EMRLabAnalyzerMachine.generated.h"

class UInputMappingContext;
class UStaticMeshComponent;
class USceneComponent;
class UPrimitiveComponent;
class UArrowComponent;
class UWidgetComponent;
class UEMRInteractableComponent;
class UMaterialInterface;
class USoundBase;
class AEMRPlayerCharacter;
class AEMRPatient;
class AEMRLabAnalyzerTube;
class AEMRLabAnalyzerTubeSpawnPoint;
class AEMRLabAnalyzerLid;

UENUM()
enum class EEMRLabAnalyzerStartValidationError : uint8
{
    None,
    LidOpen,
    Unbalanced
};

USTRUCT(BlueprintType)
struct FEMRLabAnalyzerTestButtonDef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "EMR", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.PrimitiveComponent"))
FComponentReference ButtonMesh;

    UPROPERTY(EditAnywhere, Category = "EMR", meta = (Categories = "EMR.Abilities.Exam"))
    FGameplayTag TestTag;

    UPROPERTY(EditAnywhere, Category = "EMR")
    FLinearColor ButtonColor = FLinearColor::White;
};

UCLASS()
class LOD_API AEMRLabAnalyzerMachine : public AActor, public IEMRInteractableInterface
{
    GENERATED_BODY()

public:
    AEMRLabAnalyzerMachine();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaSeconds) override;
    virtual FGameplayEventData GetInteractionEventData_Implementation(AActor* InterActor) const override;
    virtual FText GetInteractableDisplayName_Implementation() const override;
    virtual bool IsInteractableEnabled_Implementation() const override;

    void TryStartExam(AEMRPlayerCharacter* Player);
    void StopExam(AEMRPlayerCharacter* Player, bool bFromCancel);
    void HandleStartButtonPressed(AEMRPlayerCharacter* Player);
    void HandleLidTogglePressed(AEMRPlayerCharacter* Player);

    bool TryInsertTube(AEMRLabAnalyzerTube* Tube);
    bool TryTrashTube(AEMRLabAnalyzerTube* Tube, AEMRPlayerCharacter* Player);

    bool IsStartButtonComponent(const UPrimitiveComponent* Component) const;
    bool IsLidComponent(const UPrimitiveComponent* Component) const;
    void PlayTrashAttemptCue(AEMRPlayerCharacter* Player, bool bBalanceTubeAttempt);

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer|State")
    bool IsSpinning() const { return bIsSpinning; }

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer|State")
    bool IsLidClosed() const { return bLidClosed; }

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer|State")
    const FGameplayTagContainer& GetSelectedTests() const { return SelectedTests; }

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer|State")
    AEMRPlayerCharacter* GetCurrentOperator() const { return CurrentOperator; }

    UFUNCTION(BlueprintPure, Category = "EMR|LabAnalyzer|Lid")
    AEMRLabAnalyzerLid* GetLidActor() const { return LidActor; }

    void HandleTubePickedUp(AEMRLabAnalyzerTube* Tube);

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UStaticMeshComponent> MachineMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UStaticMeshComponent> CentrifugeRotorMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UStaticMeshComponent> LidMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UStaticMeshComponent> StartButtonMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UStaticMeshComponent> SeatMesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UArrowComponent> OperatorLockPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UArrowComponent> OperatorExitPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UArrowComponent> SeatPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UArrowComponent> SeatExitPoint = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Seat")
    TObjectPtr<USceneComponent> SeatAttachComponent = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Seat")
    FGameplayTag SeatAnimationTag;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Components")
    TArray<FEMRLabAnalyzerTestButtonDef> TestButtons;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UMaterialInterface> TestButtonMaterial = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Components")
    FName TestButtonColorParameterName = TEXT("Color");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|LabAnalyzer|Components", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent"))
    TArray<FComponentReference> TubeSlotAnchors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EMR|LabAnalyzer|Components", meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.SceneComponent"))
    TArray<FComponentReference> TubeSpawnAnchors;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<USceneComponent> BalanceTubeSpawnPoint = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UEMRInteractableComponent> InteractableComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Input")
    TObjectPtr<UInputMappingContext> ExamInputMappingContext = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Tube")
    TSubclassOf<AEMRLabAnalyzerTube> PatientTubeClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Tube")
    TSubclassOf<AEMRLabAnalyzerTube> BalanceTubeClass;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Tube", meta = (ClampMin = "0.0"))
    float TubeSpawnOccupancyRadius = 20.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Tube")
    FVector TubeSpawnFallbackOffset = FVector(60.0f, 0.0f, 0.0f);

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> UnbalancedSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> CompletedSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Timing", meta = (ClampMin = "0.1"))
    float SpinDuration = 3.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Lid")
    FRotator LidOpenRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Lid")
    FRotator LidClosedRotation = FRotator::ZeroRotator;

    UPROPERTY(EditInstanceOnly, Category = "EMR|LabAnalyzer|Lid")
    TObjectPtr<AEMRLabAnalyzerLid> LidActor = nullptr;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Lid", meta = (ClampMin = "0.0"))
    float LidInterpSpeed = 6.0f;

    UPROPERTY(EditAnywhere, Category = "EMR|LabAnalyzer|Spin", meta = (ClampMin = "0.0"))
    float RotorDegreesPerSecond = 720.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> LidOpenSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> LidCloseSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> TestButtonSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> StartAcceptedSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> StartRejectedSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> RunningSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> TrashPatientTubeSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> TrashBalanceTubeRejectedSound = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "EMR|LabAnalyzer|Components")
    TObjectPtr<UWidgetComponent> StartValidationErrorWidgetComponent = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|UI")
    FText LidOpenValidationErrorText;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|UI")
    FText UnbalancedValidationErrorText;

    UPROPERTY(ReplicatedUsing = OnRep_LidState, VisibleAnywhere, Category = "EMR|LabAnalyzer|State")
    bool bLidClosed = false;

    UPROPERTY(ReplicatedUsing = OnRep_SpinState, VisibleAnywhere, Category = "EMR|LabAnalyzer|State")
    bool bIsSpinning = false;

    UPROPERTY(ReplicatedUsing = OnRep_SelectedTests, VisibleAnywhere, Category = "EMR|LabAnalyzer|State")
    FGameplayTagContainer SelectedTests;

    UPROPERTY(ReplicatedUsing = OnRep_SeatMeshYawOffset, VisibleAnywhere, Category = "EMR|LabAnalyzer|State")
    float SeatMeshYawOffset = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_StartValidationFeedback, VisibleAnywhere, Category = "EMR|LabAnalyzer|State")
    EEMRLabAnalyzerStartValidationError StartValidationError = EEMRLabAnalyzerStartValidationError::None;

    UPROPERTY(ReplicatedUsing = OnRep_StartValidationFeedback, VisibleAnywhere, Category = "EMR|LabAnalyzer|State")
    uint8 StartValidationFeedbackRevision = 0;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Seat", meta = (ClampMin = "0.0"))
    float SeatYawInterpSpeed = 12.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> SeatEnterSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|LabAnalyzer|Audio")
    TObjectPtr<USoundBase> SeatExitSound = nullptr;

    UPROPERTY(Transient)
    float SeatMeshYawOffsetSmoothed = 0.0f;

    UPROPERTY(Transient)
    bool bSeatMeshYawInitialized = false;

    UFUNCTION()
    void OnRep_LidState();

    UFUNCTION()
    void OnRep_SpinState();

    UFUNCTION()
    void OnRep_SelectedTests();

    UFUNCTION()
    void OnRep_CurrentOperator();

    UFUNCTION()
    void OnRep_SeatMeshYawOffset();

    UFUNCTION()
    void OnRep_StartValidationFeedback();

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|LabAnalyzer")
    void HandleLidStateChanged(bool bNowClosed);

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|LabAnalyzer")
    void HandleSpinStateChanged(bool bNowSpinning);

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|LabAnalyzer")
    void HandleSelectedTestsChanged();

private:
    UFUNCTION()
    void HandlePatientQueuedForExam(AEMRPatient* Patient, FGameplayTag ExamTag);

    UFUNCTION()
    void HandleSpawnedTubeDestroyed(AActor* DestroyedActor);

    void CacheSpawnPoint();
    void CacheTubeSpawnAnchors();
    void CacheTubeSlotAnchors();
    USceneComponent* GetTubeSlotAnchor(int32 Index) const;
    FTransform GetSpawnPointTransform(USceneComponent*& OutAnchor) const;
    void SpawnBalanceTube();
    void HandleSpinCompleted();
    void UpdateTubeInteractionLocks();
    bool IsCentrifugeBalanced(bool& bHasPatientTube) const;
    int32 FindAvailableSlotIndex() const;
    void AssignTubeToSlot(AEMRLabAnalyzerTube* Tube, int32 SlotIndex);
    void RemoveTubeFromSlot(AEMRLabAnalyzerTube* Tube);
    void PlayCue(AEMRPlayerCharacter* Player, USoundBase* Sound);
    void ApplyTestButtonColors();
    UStaticMeshComponent* ResolveLidMesh() const;
    void ApplyLidRotation();
    void UpdateLidRotation(float DeltaSeconds);
    void RotateRotor(float DeltaSeconds);
    void SeatOperator(AEMRPlayerCharacter* Player);
    void UnseatOperator(AEMRPlayerCharacter* Player);
    void AttachSeatMeshToPlayer(AEMRPlayerCharacter* Player);
    void DetachSeatMeshToOriginal();
    void UpdateSeatMeshYawFromPlayer(float DeltaSeconds);
    void ApplySeatMeshYawOffset();
    void PlayCueForNearbyPlayers(USoundBase* Sound);
    void SetStartValidationError(EEMRLabAnalyzerStartValidationError NewError);
    void RefreshStartValidationErrorWidget();
    FGameplayTagContainer GetLabTestsForPatient(AEMRPatient* Patient, const FGameplayTag& ExamTag) const;
    void UpdateTickEnabled();

    UPROPERTY(ReplicatedUsing = OnRep_CurrentOperator, VisibleAnywhere, Category = "EMR|LabAnalyzer|State")
    TObjectPtr<AEMRPlayerCharacter> CurrentOperator = nullptr;
    TWeakObjectPtr<AEMRLabAnalyzerTubeSpawnPoint> CachedSpawnPoint;
    TWeakObjectPtr<AEMRLabAnalyzerTube> BalanceTube;

    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<USceneComponent>> CachedTubeSlotAnchors;

    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<USceneComponent>> CachedTubeSpawnAnchors;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AEMRLabAnalyzerTube>> TubeSlots;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AEMRLabAnalyzerTube>> InsertedTubes;

    UPROPERTY(Transient)
    TMap<TWeakObjectPtr<AEMRPatient>, TObjectPtr<AEMRLabAnalyzerTube>> SpawnedTubesByPatient;

    FTimerHandle SpinTimerHandle;
    FRotator TargetLidRotation = FRotator::ZeroRotator;
    bool bIsLidAnimating = false;
    float RotorDebugAccumulator = 0.0f;

    TWeakObjectPtr<USceneComponent> SeatMeshOriginalAttachParent;
    FTransform SeatMeshOriginalRelativeTransform = FTransform::Identity;
    FRotator SeatMeshBaseWorldRotation = FRotator::ZeroRotator;
    float SeatMeshInitialPlayerYaw = 0.0f;
    float SeatMeshInitialControlYaw = 0.0f;
    bool bSeatMeshAttachCached = false;
    bool bSeatMeshFollowYaw = false;
};
