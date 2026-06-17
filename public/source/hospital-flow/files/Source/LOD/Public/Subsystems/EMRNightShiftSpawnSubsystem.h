#pragma once
#include "CoreMinimal.h"
#include "Data/EMRNightShiftDefinition.h"
#include "EMRNightShiftSpawnSubsystem.generated.h"

class AEMRNightShiftGameState;
class UCurveFloat;
class UEMRDifficultyTuningData;
class AEMRPatientEntryPoint;
class AEMRPatient;
class UEMRPatientData;
class UEMRWaitingRoomManagerComponent;
class UEMRWaitingSeatComponent;


USTRUCT(BlueprintType)
struct FEMRNightShiftSpawnRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    int32 SequenceId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    float RequestTimestamp = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    EEMRNightShiftDifficultyTier RequestedDifficulty = EEMRNightShiftDifficultyTier::Standard;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    FGameplayTagContainer SourceTags;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    bool bIsImmediateRequest = false;

	bool operator<(const FEMRNightShiftSpawnRequest& Other) const
	{
		return SequenceId > Other.SequenceId; // Inverted: heap uses max-heap by default
	}
};


USTRUCT(BlueprintType)
struct FEMRNightShiftSpawnModifierDebug
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    FEMRNightShiftSpawnModifier ModifierDefinition;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    float Accumulator = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    bool bEnabled = false;
};

USTRUCT(BlueprintType)
struct FEMRNightShiftSpawnDebugInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    bool bSpawningEnabled = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    float ElapsedSeconds = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    float BaseAccumulator = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    int32 ActivePatientCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    int32 PendingRequestCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    int32 SpawnSequenceCounter = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    bool bImmediateRequestPending = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    float BaseSpawnRatePerSecond = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    float DifficultySpawnRateMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    float PlayerCountMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    float SpecialEventSpawnRateMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    int32 DeferredAutomaticSpawnCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    int32 WarmReadyPatientCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    float OldestDeferredAutomaticSpawnSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    FEMRNightShiftSpawnProfile ActiveProfile;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    TArray<FEMRNightShiftSpawnModifierDebug> Modifiers;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|NightShift|Spawning")
    TArray<FEMRNightShiftSpawnRequest> PendingRequests;
};


DECLARE_MULTICAST_DELEGATE_OneParam(FNightShiftSpawnRequestNative, const FEMRNightShiftSpawnRequest&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPatientSpawnedNative, AEMRPatient*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNightShiftSpawnRequestBP, const FEMRNightShiftSpawnRequest&, Request);

UCLASS()
class LOD_API UEMRNightShiftSpawnSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    bool StartNightShiftSpawning(const UEMRNightShiftDefinition* Definition);
    void StopNightShiftSpawning();

    UFUNCTION(BlueprintCallable, Category = "EMR|NightShift|Spawning")
    void SetSpawnModifierEnabled(FGameplayTag ModifierTag, bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "EMR|NightShift|Spawning")
    void NotifyActivePatientReleased();

    UFUNCTION(BlueprintCallable, Category = "EMR|NightShift|Spawning")
    void RequestImmediateSpawn(const FGameplayTagContainer& SourceTags = FGameplayTagContainer());

    UFUNCTION(BlueprintCallable, Category = "EMR|NightShift|Spawning")
    void SetSpecialEventSpawnRateMultiplier(float Multiplier);

    UFUNCTION(BlueprintCallable, Category = "EMR|NightShift|Spawning")
    int32 GetPendingRequestCount() const { return PendingSpawnRequests.Num(); }

    UFUNCTION(BlueprintCallable, Category = "EMR|NightShift|Spawning")
    int32 GetActivePatientCount() const { return ActivePatientCount; }

    bool IsSpawning() const { return bSpawningEnabled; }

    FNightShiftSpawnRequestNative& OnSpawnRequestReady() { return SpawnRequestNativeDelegate; }
    FOnPatientSpawnedNative& OnPatientSpawnedNative() { return PatientSpawnedNativeDelegate; }

    UPROPERTY(BlueprintAssignable, Category = "EMR|NightShift|Spawning")
    FNightShiftSpawnRequestBP OnSpawnRequestReady_BP;

    UFUNCTION(BlueprintCallable, Category = "EMR|NightShift|Spawning")
    FEMRNightShiftSpawnDebugInfo GetDebugInfo() const;

private:
    enum class EEMRAutomaticSpawnStage : uint8
    {
        HiddenSpawn = 0,
        ApplyPatientData,
        MeshAnimInit,
        CollisionAIEnable,
        Reveal
    };

    struct FAutomaticSpawnWorkItem
    {
        FEMRNightShiftSpawnRequest SourceRequest;
        TWeakObjectPtr<AEMRPatient> Patient;
        const UEMRPatientData* PatientData = nullptr;
        TWeakObjectPtr<UEMRWaitingSeatComponent> ReservedSeat;
        FTransform SpawnTransform = FTransform::Identity;
        float TimeSinceQueuedSeconds = 0.0f;
        EEMRAutomaticSpawnStage Stage = EEMRAutomaticSpawnStage::HiddenSpawn;
    };

    struct FSpawnModifierState
    {
        FEMRNightShiftSpawnModifier ModifierDefinition;
        TObjectPtr<UCurveFloat> SpawnCurve = nullptr;
        float Accumulator = 0.f;
        bool bEnabled = false;
    };

    void ResetRuntimeState();
    bool EnsureServerContext() const;
    float ApplySpawnRateMultipliers(float BaseRate) const;
    void LoadDifficultyTuningData();
    void RefreshRuntimeTuningValues(bool bForceLog);
    void RefreshInitialAutomaticSpawnDelay();
    int32 GetConnectedPlayerCount() const;
    void GenerateBaseRequests(float DeltaTime);
    void GenerateModifierRequests(float DeltaTime);
    void PumpQueue();
    void ProcessAutomaticSpawnWorkItems(float DeltaTime);
    bool TryAdvanceAutomaticSpawnWorkItem(FAutomaticSpawnWorkItem& WorkItem);
    bool TrySpawnPatient(const FEMRNightShiftSpawnRequest& Request);
    bool TrySpawnPatientImmediate(const FEMRNightShiftSpawnRequest& Request);
    bool TryStartAutomaticSpawnWorkItem(const FEMRNightShiftSpawnRequest& Request);
    void ReleaseAutomaticSpawnWorkItem(FAutomaticSpawnWorkItem& WorkItem, bool bReturnPatientToPool);
    void DispatchMoveToSeatEvent(AEMRPatient& Patient, UEMRWaitingSeatComponent* Seat) const;
    FTransform BuildSpawnTransform() const;
    void EnqueueSpawnRequest(const FGameplayTagContainer& SourceTags, bool bIsImmediateRequest);
    void EnqueueImmediateRequest(const FGameplayTagContainer& SourceTags);
    AEMRNightShiftGameState* GetNightShiftGameState() const;
    UEMRWaitingRoomManagerComponent* GetWaitingRoomManager() const;
    UEMRWaitingSeatComponent* FindAvailableSeat(UEMRWaitingRoomManagerComponent& Manager) const;
    UFUNCTION()
    void HandleSeatReleased(UEMRWaitingSeatComponent* Seat);

    FVector ComputeSpawnOffset() const;
    float GetOldestAutomaticSpawnWorkItemAgeSeconds() const;

    TWeakObjectPtr<const UEMRNightShiftDefinition> ActiveDefinition;
    FEMRNightShiftSpawnProfile ActiveProfile;

    float BaseSpawnRatePerSecond = 0.f;
	
    TArray<FSpawnModifierState> ModifierStates;

    bool bSpawningEnabled = false;
    bool bImmediateRequestPending = false;
    float ElapsedSeconds = 0.f;
    float BaseAccumulator = 0.f;
    int32 ActivePatientCount = 0;
    int32 SpawnSequenceCounter = 0;

    TArray<FEMRNightShiftSpawnRequest> PendingSpawnRequests;

    TArray<FAutomaticSpawnWorkItem> AutomaticSpawnWorkItems;

    UPROPERTY()
    TObjectPtr<const UEMRDifficultyTuningData> CachedDifficultyTuning = nullptr;

    float DifficultySpawnRateMultiplier = 1.0f;
    float PlayerCountMultiplier = 1.0f;
    float SpecialEventSpawnRateMultiplier = 1.0f;
    float InitialAutomaticSpawnDelayRemainingSeconds = 0.0f;
    bool bInitialAutomaticSpawnDelayActive = false;
    bool bInitialAutomaticSpawnRequestIssued = false;
    float TuningRefreshAccumulator = 0.0f;
    bool bHasLoggedMissingDifficultyTuning = false;

    TWeakObjectPtr<const UEMRNightShiftDefinition> DelayAppliedDefinition;
    int32 DelayAppliedCycleIndex = INDEX_NONE;
    int32 DelayAppliedNightShiftIndex = INDEX_NONE;
    TWeakObjectPtr<const UEMRDifficultyTuningData> DelayAppliedDifficultyTuning;

    FNightShiftSpawnRequestNative SpawnRequestNativeDelegate;
    FOnPatientSpawnedNative PatientSpawnedNativeDelegate;

    int32 GetMaxPendingRequestCount() const;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Spawn", meta = (ClampMin = "1"))
    int32 MaxPendingSpawnRequests = 64;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Spawn", meta = (ClampMin = "0.1"))
    float TuningRefreshIntervalSeconds = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Spawn|Automatic", meta = (ClampMin = "0.1"))
    float AutomaticSpawnWorkBudgetMs = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Spawn|Automatic", meta = (ClampMin = "0.0"))
    float AutomaticSpawnMinimumRevealDelaySeconds = 1.0f;
	
    float SpawnOffsetSpacing = 300.f;
	
    int32 SpawnOffsetColumns = 3;

    mutable int32 SpawnOffsetCounter = 0;

    UPROPERTY()
    mutable TWeakObjectPtr<UEMRWaitingRoomManagerComponent> CachedWaitingRoomManager;

    UPROPERTY()
    mutable TWeakObjectPtr<AEMRPatientEntryPoint> CachedEntryPoint;

};
