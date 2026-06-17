#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Framework/EMRNightShiftGameState.h"
#include "EMRRunProgressionSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEMRRunProgression, Log, Log);

class UEMRNightShiftDefinition;

UENUM()
enum class EEMRHubertOutcomeAnnouncement : uint8
{
    NightShiftWin,
    NightShiftLose,
    CycleWin,
    CycleLose
};

/**
 * Keeps run progression data alive across level travels (Hub <-> NightShift).
 * This is a purely server-side cache used by GameModes to rehydrate GameStates.
 */
USTRUCT(BlueprintType)
struct FEMRRunProgressionState
{
    GENERATED_BODY()

    UPROPERTY()
    EER_RunPhase RunPhase = EER_RunPhase::Hub;

    UPROPERTY()
    bool bRunFailed = false;

    UPROPERTY()
    bool bFinalMissionUnlocked = false;

    UPROPERTY()
    int32 CurrentCycleIndex = 0;

    UPROPERTY()
    int32 NightShiftIndexInCycle = 0;

    UPROPERTY()
    float CurrentCycleQuota = 0.f;

    UPROPERTY()
    float CurrentCycleStartRevenue = 0.f;

    UPROPERTY()
    float CachedTotalRevenue = 0.f;

    UPROPERTY()
    float CachedReputation = 0.f;

    UPROPERTY()
    float NightShiftStartTotalRevenue = 0.f;

    UPROPERTY()
    float RemainingTimeInNightShift = 0.f;

    UPROPERTY()
    bool bIsInNightShiftOvertime = false;

    UPROPERTY()
    FString SessionInviteCode;

    UPROPERTY()
    FEMRNightShiftSummary LastNightShiftSummary;

    UPROPERTY()
    TSoftObjectPtr<UEMRNightShiftDefinition> CurrentNightShiftDefinition;

    UPROPERTY()
    TArray<TSoftObjectPtr<UEMRNightShiftDefinition>> NextNightShiftsAvailable;

    UPROPERTY()
    FGameplayTagContainer ActiveRunUpgradeTags;

    UPROPERTY()
    TMap<FGameplayTag, int32> ActiveRunUpgradeStacks;
};

UCLASS()
class LOD_API UEMRRunProgressionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UEMRRunProgressionSubsystem();

    void CacheFromGameState(const AEMRNightShiftGameState* NightShiftGameState);

    bool ApplyToGameState(AEMRNightShiftGameState* NightShiftGameState) const;

    bool HasCachedState() const { return bHasCachedState; }

    void ResetCachedState();

    void SetPendingNightShiftForTravel(UEMRNightShiftDefinition* NightShift);

    UEMRNightShiftDefinition* GetPendingNightShiftForTravel() const { return PendingNightShiftForTravel.Get(); }

    void ClearPendingNightShiftForTravel();

    /** Retrieve the cached NightShifts offered for the next selection (server-side). */
    void GetCachedNextNightShifts(TArray<UEMRNightShiftDefinition*>& OutNightShifts) const;

    /** Centralized helpers for cycle transitions + diagnostics. */
    void CommitCycleSetup(AEMRNightShiftGameState* NightShiftGameState, int32 CycleIndex, float CycleQuota);
    void LogAuthoritativeState(const TCHAR* Context, const AEMRNightShiftGameState* NightShiftGameState) const;

    /** Capture/reset NightShift start snapshot for budget/revenue rollbacks. */
    void StoreNightShiftStartSnapshot(const AEMRNightShiftGameState* NightShiftGameState);
    void RestoreNightShiftStartEconomy(AEMRNightShiftGameState* NightShiftGameState);
    float GetNightShiftStartTotalRevenue() const { return CachedState.NightShiftStartTotalRevenue; }

    void QueueHubertOutcomeAnnouncement(EEMRHubertOutcomeAnnouncement Announcement);
    void ConsumeHubertOutcomeAnnouncements(TArray<EEMRHubertOutcomeAnnouncement>& OutAnnouncements);
    void ClearHubertOutcomeAnnouncements();

private:
    UEMRNightShiftDefinition* ResolveNightShiftDefinition(const TSoftObjectPtr<UEMRNightShiftDefinition>& NightShiftRef) const;

    UPROPERTY()
    FEMRRunProgressionState CachedState;

    UPROPERTY()
    bool bHasCachedState = false;

    UPROPERTY()
    TWeakObjectPtr<UEMRNightShiftDefinition> PendingNightShiftForTravel;

    UPROPERTY()
    TArray<EEMRHubertOutcomeAnnouncement> PendingHubertOutcomeAnnouncements;

    mutable TMap<TSoftObjectPtr<UEMRNightShiftDefinition>, TWeakObjectPtr<UEMRNightShiftDefinition>> ResolvedNightShiftDefinitions;
};
