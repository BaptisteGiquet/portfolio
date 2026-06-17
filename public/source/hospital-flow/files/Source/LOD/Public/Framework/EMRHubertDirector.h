#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Characters/NPC/EMRHubertCharacter.h"
#include "EMRHubertDirector.generated.h"

class AEMRHospitalExit;
class AEMRHubertCharacter;
class AEMRPlayerCharacter;
class APlayerController;
class USkeletalMeshComponent;
class USoundBase;
enum class EEMRHubertOutcomeAnnouncement : uint8;

UCLASS()
class LOD_API AEMRHubertDirector : public AActor
{
    GENERATED_BODY()

public:
    AEMRHubertDirector();

    virtual void Tick(float DeltaSeconds) override;

    void StartHubRuntime();
    void StartNightShiftRuntime();
    void StopHubertRuntime();

    void QueueOutcomeAnnouncements(const TArray<EEMRHubertOutcomeAnnouncement>& Announcements);
    void NotifyPlayerExitedHospital(AEMRPlayerCharacter* Escaper);

private:
    struct FHubertExitBinding
    {
        TWeakObjectPtr<AEMRHospitalExit> ExitActor;
        FDelegateHandle DelegateHandle;
    };

    void EnterState(EEMRHubertState NewState);
    void UpdateHubRuntime(float DeltaSeconds);
    void UpdateNightRuntime(float DeltaSeconds);
    void UpdateNightPatrolState();
    void UpdateWindowWatchState();
    void UpdateChaseEscaperState();
    void UpdateGrabEscaperState();
    void UpdateCarryEscaperState();
    void UpdateThrowEscaperState();
    void UpdateReturnEscaperState();

    AEMRHubertCharacter* EnsureHubertCharacter(bool bHubContext);
    bool MoveHubertToLocation(const FVector& TargetLocation, float AcceptanceRadius);
    void StopHubertMovement();
    void ApplyHubertMoveSpeed(float NewMoveSpeed) const;
    bool HasReachedLocation(const FVector& TargetLocation, float Tolerance) const;

    void BindHospitalExits();
    void UnbindHospitalExits();
    void HandleHospitalExitOverlap(AEMRPlayerCharacter* Escaper);

    void ScheduleNextWindowWatch();
    AActor* SelectRandomPatrolPoint() const;
    AActor* SelectRandomWindowWatchPoint() const;
    bool CanAnyPlayerSeeHubert() const;
    bool IsPlayerViewingHubert(const APlayerController* PlayerController) const;
    bool AreAllHubPlayersLoaded() const;

    bool CanAcceptEscaperEvent(const AEMRPlayerCharacter* Escaper) const;
    void TryStartNextEscaperChase();
    void ClearInvalidEscaperQueue();
    USkeletalMeshComponent* ResolveEscaperMeshForCarry(const AEMRPlayerCharacter* Escaper) const;
    FName ResolveEscaperThrowPhysicsBone(const USkeletalMeshComponent* EscaperMesh) const;
    void ApplyEscaperGrabCarryOverrides(AEMRPlayerCharacter* Escaper, AEMRHubertCharacter* SpawnedHubert);
    void RestoreEscaperGrabCarryOverrides(AEMRPlayerCharacter* Escaper, AEMRHubertCharacter* SpawnedHubert);
    void ExecuteEscaperEnforcement(AEMRPlayerCharacter* Escaper);
    bool BeginEscaperPhysicalThrow(AEMRPlayerCharacter* Escaper);
    bool IsEscaperVelocityBelowThreshold(const AEMRPlayerCharacter* Escaper) const;
    void FinalizeEscaperPhysicalThrow(AEMRPlayerCharacter* Escaper, bool bTeleportFallback);
    void ResetEscaperPhysicalThrowRuntime();
    void ReturnEscaperInside(AEMRPlayerCharacter* Escaper);
    void ApplyEscapeRevenuePenalty(AEMRPlayerCharacter* Escaper);
    void TryPlayPendingOutcomeAnnouncement();

    USoundBase* ResolveVoiceLineFromArray(const TArray<TSoftObjectPtr<USoundBase>>& VoiceLines, const TCHAR* Context) const;
    USoundBase* ResolveVoiceLineForOutcome(EEMRHubertOutcomeAnnouncement AnnouncementType) const;

private:
    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Spawn")
    TSubclassOf<AEMRHubertCharacter> HubertCharacterClass;

    UPROPERTY(EditInstanceOnly, Category = "EMR|Hubert|Spawn")
    TObjectPtr<AActor> HubSpawnAnchor = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "EMR|Hubert|Spawn")
    TObjectPtr<AActor> NightShiftSpawnAnchor = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "EMR|Hubert|Escape")
    TObjectPtr<AActor> ReturnInsideAnchor = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "EMR|Hubert|Escape")
    TObjectPtr<AActor> ThrowEscaperAnchor = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "EMR|Hubert|Navigation")
    TArray<TObjectPtr<AActor>> PatrolRoutePoints;

    UPROPERTY(EditInstanceOnly, Category = "EMR|Hubert|Navigation")
    TArray<TObjectPtr<AActor>> WindowWatchPoints;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Movement", meta = (ClampMin = "0.0"))
    float PatrolMoveSpeed = 180.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Movement", meta = (ClampMin = "0.0"))
    float ChaseMoveSpeed = 420.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Movement", meta = (ClampMin = "0.0"))
    float CarryMoveSpeed = 300.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Movement", meta = (ClampMin = "0.0"))
    float PatrolArrivalDistance = 120.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|WindowWatch", meta = (ClampMin = "0.0"))
    float WindowWatchMinDelaySeconds = 8.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|WindowWatch", meta = (ClampMin = "0.0"))
    float WindowWatchMaxDelaySeconds = 18.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|WindowWatch", meta = (ClampMin = "1.0", ClampMax = "179.0"))
    float PlayerSeeFOVDegrees = 75.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|WindowWatch", meta = (ClampMin = "0.0"))
    float PlayerSeeMaxDistance = 3000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float EscapeGrabDistance = 160.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float CarryArrivalDistance = 140.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float GrabAnimationDurationSeconds = 0.7f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ThrowAnimationDurationSeconds = 0.6f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ThrowImpulseStrength = 1400.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape")
    float ThrowUpwardImpulse = 220.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape")
    FName ThrowRagdollStartBone = TEXT("pelvis");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ThrowLowVelocityThreshold = 80.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ThrowLowVelocityWindowSeconds = 0.2f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ThrowMinFlightTimeBeforeSettleSeconds = 0.2f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ThrowMinLaunchSpeed = 120.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ThrowRecoveryDelaySeconds = 0.45f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ThrowFallbackTimeoutSeconds = 2.5f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ThrowSuccessDistanceFromReturnAnchor = 900.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape")
    FName GrabAttachSocketName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape")
    FName EscaperCarryMeshComponentTag = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape")
    FName EscaperCarryMeshComponentName = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape")
    FName EscaperCarryRagdollStartBone = TEXT("spine_01");

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EscaperCarryRagdollBlendWeight = 0.2f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float EscapePenaltyRevenueAmount = 250.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float PerPlayerPenaltyCooldownSeconds = 12.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float GlobalPenaltyDebounceSeconds = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Escape", meta = (ClampMin = "0.0"))
    float ChasePathRefreshIntervalSeconds = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Hub", meta = (ClampMin = "0.0"))
    float HubAllPlayersLoadedTimeoutSeconds = 12.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Hub", meta = (ClampMin = "0.0"))
    float HubOutcomeAnnouncementSpacingSeconds = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Audio")
    TArray<TSoftObjectPtr<USoundBase>> NightShiftWinVoiceLines;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Audio")
    TArray<TSoftObjectPtr<USoundBase>> NightShiftLoseVoiceLines;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Audio")
    TArray<TSoftObjectPtr<USoundBase>> CycleWinVoiceLines;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Audio")
    TArray<TSoftObjectPtr<USoundBase>> CycleLoseVoiceLines;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Hubert|Audio")
    TArray<TSoftObjectPtr<USoundBase>> EscapeVoiceLines;

private:
    TWeakObjectPtr<AEMRHubertCharacter> HubertCharacter;
    TArray<FHubertExitBinding> ExitBindings;
    TArray<EEMRHubertOutcomeAnnouncement> PendingOutcomeAnnouncements;
    TArray<TWeakObjectPtr<AEMRPlayerCharacter>> PendingEscaperQueue;
    TMap<TWeakObjectPtr<AEMRPlayerCharacter>, double> PerPlayerPenaltyTimestampSeconds;

    TWeakObjectPtr<AEMRPlayerCharacter> ActiveEscaper;
    TWeakObjectPtr<AEMRPlayerCharacter> EscaperWithCarryOverrides;
    TWeakObjectPtr<USkeletalMeshComponent> EscaperCarryRagdollMesh;
    FName EscaperCarryResolvedRagdollBone = NAME_None;
    TWeakObjectPtr<AActor> ActivePatrolPoint;
    TWeakObjectPtr<AActor> ActiveWindowPoint;

    EEMRHubertState CurrentState = EEMRHubertState::HubDriver;

    bool bHubRuntimeActive = false;
    bool bNightRuntimeActive = false;
    bool bHubLoadTimeoutWarningLogged = false;
    bool bEscaperCarryOverridesSaved = false;
    bool bEscaperCarryRagdollApplied = false;
    bool bEscaperCarryMeshCollisionSaved = false;
    bool bEscaperCarryMeshBlendPhysicsSaved = false;
    bool bEscaperCarryActorCollisionEnabled = true;
    TEnumAsByte<ECollisionEnabled::Type> EscaperCarryCapsuleCollisionMode = ECollisionEnabled::QueryAndPhysics;
    TEnumAsByte<ECollisionEnabled::Type> EscaperCarryMeshCollisionMode = ECollisionEnabled::NoCollision;

    double NextWindowWatchServerTime = 0.0;
    double LastGlobalPenaltyTimestampSeconds = -1.0;
    double LastChaseRefreshTimestampSeconds = 0.0;
    double EscaperActionEndTimestampSeconds = 0.0;
    double EscaperThrowStartTimestampSeconds = 0.0;
    double EscaperLowVelocityWindowStartTimestampSeconds = 0.0;
    double EscaperSettledTimestampSeconds = 0.0;
    double EscaperNextThrowDebugLogTimestampSeconds = 0.0;
    double HubRuntimeStartTimestampSeconds = 0.0;
    double NextOutcomeAnnouncementTimestampSeconds = 0.0;
};
