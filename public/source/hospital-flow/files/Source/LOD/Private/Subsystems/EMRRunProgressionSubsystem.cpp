#include "Subsystems/EMRRunProgressionSubsystem.h"

#include "Data/EMRNightShiftDefinition.h"

DEFINE_LOG_CATEGORY(LogEMRRunProgression);

namespace
{
    constexpr TCHAR RunProgressionSubsystemUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");
}

UEMRRunProgressionSubsystem::UEMRRunProgressionSubsystem()
{
}

void UEMRRunProgressionSubsystem::CacheFromGameState(const AEMRNightShiftGameState* NightShiftGameState)
{
    if (!NightShiftGameState || !NightShiftGameState->HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunProgression.CacheFromGameState skip GameState=%s auth=%d"),
            *GetNameSafe(NightShiftGameState),
            NightShiftGameState && NightShiftGameState->HasAuthority() ? 1 : 0);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[RunPhaseDebug] CacheFromGameState - GameState=%s Phase=%d Cycle=%d NightShiftIndex=%d"),
        *GetNameSafe(NightShiftGameState),
        static_cast<int32>(NightShiftGameState->GetRunPhase()),
        NightShiftGameState->GetCurrentCycleIndex(),
        NightShiftGameState->GetNightShiftIndexInCycle());

    CachedState.RunPhase = NightShiftGameState->GetRunPhase();
    CachedState.bRunFailed = NightShiftGameState->HasRunFailed();
    CachedState.bFinalMissionUnlocked = NightShiftGameState->IsFinalMissionUnlocked();
    CachedState.CurrentCycleIndex = NightShiftGameState->GetCurrentCycleIndex();
    CachedState.NightShiftIndexInCycle = NightShiftGameState->GetNightShiftIndexInCycle();
    CachedState.CurrentCycleQuota = NightShiftGameState->GetCurrentCycleQuota();
    CachedState.CurrentCycleStartRevenue = NightShiftGameState->GetCurrentCycleStartRevenue();
    CachedState.CachedTotalRevenue = NightShiftGameState->GetTotalRevenue();
    CachedState.CachedReputation = NightShiftGameState->GetReputation();
    CachedState.RemainingTimeInNightShift = NightShiftGameState->GetRemainingTimeInNightShift();
    CachedState.bIsInNightShiftOvertime = NightShiftGameState->IsInNightShiftOvertime();
    CachedState.SessionInviteCode = NightShiftGameState->GetSessionInviteCode();

    CachedState.LastNightShiftSummary = NightShiftGameState->GetLastNightShiftSummary();

    CachedState.CurrentNightShiftDefinition = NightShiftGameState->GetCurrentNightShiftDefinition();
    if (!CachedState.CurrentNightShiftDefinition.IsNull())
    {
        ResolvedNightShiftDefinitions.Add(CachedState.CurrentNightShiftDefinition, CachedState.CurrentNightShiftDefinition.Get());
    }

    CachedState.NextNightShiftsAvailable.Reset();
    for (UEMRNightShiftDefinition* NightShift : NightShiftGameState->GetAvailableNextNightShifts())
    {
        CachedState.NextNightShiftsAvailable.Add(NightShift);
        if (NightShift)
        {
            ResolvedNightShiftDefinitions.Add(NightShift, NightShift);
        }
    }

    CachedState.ActiveRunUpgradeTags = NightShiftGameState->GetActiveRunUpgradeTags();
    CachedState.ActiveRunUpgradeStacks = NightShiftGameState->GetActiveRunUpgradeStacks();
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s RunProgression CacheFromGameState captured active upgrade tags count=%d tags=%s stackCount=%d"),
        RunProgressionSubsystemUpgradesFlowLogPrefix,
        CachedState.ActiveRunUpgradeTags.Num(),
        *CachedState.ActiveRunUpgradeTags.ToStringSimple(),
        CachedState.ActiveRunUpgradeStacks.Num());

    bHasCachedState = true;
    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunProgression.CacheFromGameState cached phase=%d failed=%d finalUnlocked=%d cycle=%d nightShift=%d revenue=%.2f quota=%.2f"),
        static_cast<int32>(CachedState.RunPhase),
        CachedState.bRunFailed ? 1 : 0,
        CachedState.bFinalMissionUnlocked ? 1 : 0,
        CachedState.CurrentCycleIndex,
        CachedState.NightShiftIndexInCycle,
        CachedState.CachedTotalRevenue,
        CachedState.CurrentCycleQuota);

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][Subsystem] CacheFromGameState - Phase=%d, Cycle=%d, NightShift=%d"),
        static_cast<int32>(CachedState.RunPhase), CachedState.CurrentCycleIndex, CachedState.NightShiftIndexInCycle);
}


bool UEMRRunProgressionSubsystem::ApplyToGameState(AEMRNightShiftGameState* NightShiftGameState) const
{
    if (!NightShiftGameState || !NightShiftGameState->HasAuthority() || !bHasCachedState)
    {
        UE_LOG(LogTemp, Warning, TEXT("[RunPhaseDebug] ApplyToGameState skipped - GameState=%s HasAuthority=%s HasCachedState=%s"),
            *GetNameSafe(NightShiftGameState),
            NightShiftGameState && NightShiftGameState->HasAuthority() ? TEXT("true") : TEXT("false"),
            bHasCachedState ? TEXT("true") : TEXT("false"));
        UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunProgression.ApplyToGameState skip GameState=%s auth=%d hasCache=%d"),
            *GetNameSafe(NightShiftGameState),
            NightShiftGameState && NightShiftGameState->HasAuthority() ? 1 : 0,
            bHasCachedState ? 1 : 0);
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunProgression.ApplyToGameState begin GameState=%s phase=%d failed=%d finalUnlocked=%d cycle=%d nightShift=%d revenue=%.2f quota=%.2f"),
        *GetNameSafe(NightShiftGameState),
        static_cast<int32>(CachedState.RunPhase),
        CachedState.bRunFailed ? 1 : 0,
        CachedState.bFinalMissionUnlocked ? 1 : 0,
        CachedState.CurrentCycleIndex,
        CachedState.NightShiftIndexInCycle,
        CachedState.CachedTotalRevenue,
        CachedState.CurrentCycleQuota);

    UE_LOG(LogTemp, Log, TEXT("[RunPhaseDebug] ApplyToGameState start - GameState=%s CachedPhase=%d"),
        *GetNameSafe(NightShiftGameState),
        static_cast<int32>(CachedState.RunPhase));

    NightShiftGameState->SetRunPhase(CachedState.RunPhase);
    NightShiftGameState->SetRunFailed(CachedState.bRunFailed);
    NightShiftGameState->SetFinalMissionUnlocked(CachedState.bFinalMissionUnlocked);

    NightShiftGameState->SetCurrentCycleInfo(CachedState.CurrentCycleIndex, CachedState.CurrentCycleQuota, CachedState.CurrentCycleStartRevenue);
    NightShiftGameState->SetNightShiftIndexInCycle(CachedState.NightShiftIndexInCycle);
    NightShiftGameState->SetTotalRevenue(CachedState.CachedTotalRevenue);
    NightShiftGameState->SetRemainingTimeInNightShift(CachedState.RemainingTimeInNightShift);
    NightShiftGameState->SetNightShiftOvertimeActive(CachedState.bIsInNightShiftOvertime);
    NightShiftGameState->SetSessionInviteCode(CachedState.SessionInviteCode);

    NightShiftGameState->SetLastNightShiftSummary(CachedState.LastNightShiftSummary);

    UEMRNightShiftDefinition* CurrentNightShift = ResolveNightShiftDefinition(CachedState.CurrentNightShiftDefinition);
    if (!CurrentNightShift && !CachedState.CurrentNightShiftDefinition.IsNull())
    {
        UE_LOG(
            LogEMRRunProgression,
            Warning,
            TEXT("[ApplyToGameState] Failed to resolve CurrentNightShiftDefinition from '%s'"),
            *CachedState.CurrentNightShiftDefinition.ToSoftObjectPath().ToString());
    }
    NightShiftGameState->SetCurrentNightShiftDefinition(CurrentNightShift);

    TArray<UEMRNightShiftDefinition*> NextNightShifts;
    for (const TSoftObjectPtr<UEMRNightShiftDefinition>& Entry : CachedState.NextNightShiftsAvailable)
    {
        if (UEMRNightShiftDefinition* Loaded = ResolveNightShiftDefinition(Entry))
        {
            NextNightShifts.Add(Loaded);
        }
        else if (!Entry.IsNull())
        {
            UE_LOG(
                LogEMRRunProgression,
                Warning,
                TEXT("[ApplyToGameState] Failed to resolve offered NightShift from '%s'"),
                *Entry.ToSoftObjectPath().ToString());
        }
    }
    NightShiftGameState->SetAvailableNextNightShifts(NextNightShifts);
    if (CachedState.ActiveRunUpgradeStacks.IsEmpty() && CachedState.ActiveRunUpgradeTags.Num() > 0)
    {
        TMap<FGameplayTag, int32> FallbackStacks;
        for (const FGameplayTag& Tag : CachedState.ActiveRunUpgradeTags)
        {
            if (Tag.IsValid())
            {
                FallbackStacks.Add(Tag, 1);
            }
        }
        NightShiftGameState->SetActiveRunUpgradeStacks(FallbackStacks);
    }
    else
    {
        NightShiftGameState->SetActiveRunUpgradeStacks(CachedState.ActiveRunUpgradeStacks);
    }
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s RunProgression ApplyToGameState restored active upgrade tags count=%d tags=%s stackCount=%d"),
        RunProgressionSubsystemUpgradesFlowLogPrefix,
        CachedState.ActiveRunUpgradeTags.Num(),
        *CachedState.ActiveRunUpgradeTags.ToStringSimple(),
        CachedState.ActiveRunUpgradeStacks.Num());

    UE_LOG(
        LogEMRRunProgression,
        Log,
        TEXT("[ApplyToGameState] Restored Current=%s Offers=%d/%d"),
        *GetNameSafe(CurrentNightShift),
        NextNightShifts.Num(),
        CachedState.NextNightShiftsAvailable.Num());

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][Subsystem] ApplyToGameState - Restoring Phase=%d"), static_cast<int32>(CachedState.RunPhase));

    return true;
}


void UEMRRunProgressionSubsystem::ResetCachedState()
{
    UE_LOG(LogTemp, Warning, TEXT("[EndSessionLobbyTrace] RunProgression.ResetCachedState hadCache=%d phase=%d failed=%d finalUnlocked=%d cycle=%d nightShift=%d"),
        bHasCachedState ? 1 : 0,
        static_cast<int32>(CachedState.RunPhase),
        CachedState.bRunFailed ? 1 : 0,
        CachedState.bFinalMissionUnlocked ? 1 : 0,
        CachedState.CurrentCycleIndex,
        CachedState.NightShiftIndexInCycle);

    CachedState = FEMRRunProgressionState();
    bHasCachedState = false;
    PendingNightShiftForTravel.Reset();
    PendingHubertOutcomeAnnouncements.Reset();
    ResolvedNightShiftDefinitions.Reset();
}


void UEMRRunProgressionSubsystem::SetPendingNightShiftForTravel(UEMRNightShiftDefinition* NightShift)
{
    PendingNightShiftForTravel = NightShift;

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][Subsystem] PendingNightShift set to %s"), NightShift ? *NightShift->GetName() : TEXT("nullptr"));
}


void UEMRRunProgressionSubsystem::ClearPendingNightShiftForTravel()
{
    PendingNightShiftForTravel.Reset();
}


void UEMRRunProgressionSubsystem::GetCachedNextNightShifts(TArray<UEMRNightShiftDefinition*>& OutNightShifts) const
{
    OutNightShifts.Reset();

    if (!bHasCachedState)
    {
        return;
    }

    for (const TSoftObjectPtr<UEMRNightShiftDefinition>& Entry : CachedState.NextNightShiftsAvailable)
    {
        if (UEMRNightShiftDefinition* Loaded = ResolveNightShiftDefinition(Entry))
        {
            OutNightShifts.Add(Loaded);
        }
    }
}

UEMRNightShiftDefinition* UEMRRunProgressionSubsystem::ResolveNightShiftDefinition(const TSoftObjectPtr<UEMRNightShiftDefinition>& NightShiftRef) const
{
    if (NightShiftRef.IsNull())
    {
        return nullptr;
    }

    const FString NightShiftPath = NightShiftRef.ToSoftObjectPath().ToString();

    if (TWeakObjectPtr<UEMRNightShiftDefinition>* CachedPtr = ResolvedNightShiftDefinitions.Find(NightShiftRef))
    {
        if (CachedPtr->IsValid())
        {
            return CachedPtr->Get();
        }

        UE_LOG(
            LogEMRRunProgression,
            Verbose,
            TEXT("[ResolveNightShiftDefinition] Cached pointer invalid for '%s', reloading."),
            *NightShiftPath);
    }

    UEMRNightShiftDefinition* Loaded = NightShiftRef.LoadSynchronous();
    ResolvedNightShiftDefinitions.Add(NightShiftRef, Loaded);

    if (!Loaded)
    {
        UE_LOG(
            LogEMRRunProgression,
            Warning,
            TEXT("[ResolveNightShiftDefinition] LoadSynchronous failed for '%s'"),
            *NightShiftPath);
    }
    else
    {
        UE_LOG(
            LogEMRRunProgression,
            Verbose,
            TEXT("[ResolveNightShiftDefinition] Resolved '%s' -> %s"),
            *NightShiftPath,
            *Loaded->GetName());
    }

    return Loaded;
}


void UEMRRunProgressionSubsystem::CommitCycleSetup(AEMRNightShiftGameState* NightShiftGameState, int32 CycleIndex, float CycleQuota)
{
    if (!NightShiftGameState || !NightShiftGameState->HasAuthority())
    {
        return;
    }

    NightShiftGameState->SetCurrentCycleInfo(CycleIndex, CycleQuota, NightShiftGameState->GetTotalRevenue());
    NightShiftGameState->SetNightShiftIndexInCycle(0);

    CacheFromGameState(NightShiftGameState);
    CachedState.NightShiftStartTotalRevenue = NightShiftGameState->GetTotalRevenue();

    LogAuthoritativeState(TEXT("CommitCycleSetup"), NightShiftGameState);
}


void UEMRRunProgressionSubsystem::LogAuthoritativeState(const TCHAR* Context, const AEMRNightShiftGameState* NightShiftGameState) const
{
    if (!NightShiftGameState)
    {
        return;
    }

    UE_LOG(
        LogEMRRunProgression,
        Log,
        TEXT("[%s] Phase=%d Cycle=%d NightShift=%d Revenue=%.2f StartRevenue=%.2f Quota=%.2f"),
        Context ? Context : TEXT("RunProgression"),
        static_cast<int32>(NightShiftGameState->GetRunPhase()),
        NightShiftGameState->GetCurrentCycleIndex(),
        NightShiftGameState->GetNightShiftIndexInCycle(),
        NightShiftGameState->GetTotalRevenue(),
        NightShiftGameState->GetCurrentCycleStartRevenue(),
        NightShiftGameState->GetCurrentCycleQuota());
}


void UEMRRunProgressionSubsystem::StoreNightShiftStartSnapshot(const AEMRNightShiftGameState* NightShiftGameState)
{
    if (!NightShiftGameState || !NightShiftGameState->HasAuthority())
    {
        return;
    }

    CachedState.NightShiftStartTotalRevenue = NightShiftGameState->GetTotalRevenue();
}


void UEMRRunProgressionSubsystem::RestoreNightShiftStartEconomy(AEMRNightShiftGameState* NightShiftGameState)
{
    if (!NightShiftGameState || !NightShiftGameState->HasAuthority())
    {
        return;
    }

    NightShiftGameState->SetTotalRevenue(CachedState.NightShiftStartTotalRevenue);

    CacheFromGameState(NightShiftGameState);
}

void UEMRRunProgressionSubsystem::QueueHubertOutcomeAnnouncement(const EEMRHubertOutcomeAnnouncement Announcement)
{
    PendingHubertOutcomeAnnouncements.Add(Announcement);
    UE_LOG(LogTemp, Log, TEXT("[Hubert] Queue outcome announcement type=%d pending=%d"), static_cast<int32>(Announcement), PendingHubertOutcomeAnnouncements.Num());
}

void UEMRRunProgressionSubsystem::ConsumeHubertOutcomeAnnouncements(TArray<EEMRHubertOutcomeAnnouncement>& OutAnnouncements)
{
    OutAnnouncements = PendingHubertOutcomeAnnouncements;
    PendingHubertOutcomeAnnouncements.Reset();
}

void UEMRRunProgressionSubsystem::ClearHubertOutcomeAnnouncements()
{
    PendingHubertOutcomeAnnouncements.Reset();
}
