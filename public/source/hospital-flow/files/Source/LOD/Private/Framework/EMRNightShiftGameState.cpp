#include "Framework/EMRNightShiftGameState.h"

#include "Data/EMRNightShiftDefinition.h"
#include "GAS/EMRAbilitySystemComponent.h"
#include "GAS/Attributes/EMRTeamAttributeSet.h"
#include "Subsystems/EMRPatientPoolSubsystem.h"
#include "Subsystems/EMRNightShiftSpawnSubsystem.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Net/UnrealNetwork.h"
#include "Curves/CurveFloat.h"
#include "Framework/EMRNightShiftGameMode.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Framework/EMRAssetManager.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
    constexpr TCHAR NightShiftGameStateUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");
}


AEMRNightShiftGameState::AEMRNightShiftGameState()
{
	PrimaryActorTick.bCanEverTick = true;

	TeamASC = CreateDefaultSubobject<UEMRAbilitySystemComponent>(TEXT("TeamASC"));
	TeamASC->SetIsReplicated(true);
	TeamASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	TeamAttributeSet = CreateDefaultSubobject<UEMRTeamAttributeSet>(TEXT("TeamAttributeSet"));
}


void AEMRNightShiftGameState::BeginPlay()
{
    Super::BeginPlay();

    SetActorTickEnabled(HasAuthority());

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[NightShiftFlow][NightShiftGameState] BeginPlay - Authority=%s Remaining=%.2f"),
        HasAuthority() ? TEXT("YES") : TEXT("NO"),
        RemainingTimeInNightShift);

    LoadDifficultyTuning();

    if (!HasAuthority())
    {
        return;
    }

    if (RunPhase == EER_RunPhase::InNightShift && CurrentNightShiftDefinition)
    {
        if (UWorld* World = GetWorld())
        {
            if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
            {
                SpawnSubsystem->StartNightShiftSpawning(CurrentNightShiftDefinition);
            }
        }
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    TeamReputationChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetReputationAttribute())
    .AddUObject(this, &ThisClass::HandleTeamReputationChanged);

    if (!InitTeamEffects.IsEmpty())
    {
        for (const TSubclassOf<UGameplayEffect>& Effect : InitTeamEffects)
        {
			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle EffectSpec = ASC->MakeOutgoingSpec(Effect, 1, ContextHandle);
			ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
		}
	}

	if (NextNightShiftsAvailable.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] BeginPlay - NextNightShiftsAlreadyAvailable=%d"), NextNightShiftsAvailable.Num());
		NextNightShiftsChangedDelegate.Broadcast();
	}
}


void AEMRNightShiftGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (HasAuthority())
    {
        if (GetRunPhase() == EER_RunPhase::InNightShift)
        {
            if (!IsInNightShiftOvertime())
            {
                const float NewRemaining = FMath::Max(RemainingTimeInNightShift - DeltaSeconds, 0.f);
                SetRemainingTimeInNightShift(NewRemaining);
                if (NewRemaining <= 0.f)
                {
                    if (AEMRNightShiftGameMode* RunGameMode = GetWorld()->GetAuthGameMode<AEMRNightShiftGameMode>())
                    {
                        RunGameMode->HandleNightShiftOvertimeStarted();
                    }
                }
            }
            else
            {
                NightShiftOvertimeElapsedSeconds += DeltaSeconds;

                const float NewDrainMultiplier = GetOvertimePatienceDrainMultiplier();
                if (!FMath::IsNearlyEqual(NewDrainMultiplier, CachedPatienceDrainMultiplier))
                {
                    CachedPatienceDrainMultiplier = NewDrainMultiplier;

                    if (UWorld* World = GetWorld())
                    {
                        if (UEMRPatientPoolSubsystem* PoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
                        {
                            PoolSubsystem->ApplyPatienceDrainMultiplier(NewDrainMultiplier);
                        }
                    }
                }
            }
        }

        if (DebugSnapshotInterval > 0.f)
        {
            DebugSnapshotAccumulator += DeltaSeconds;
            if (DebugSnapshotAccumulator >= DebugSnapshotInterval)
            {
                DebugSnapshotAccumulator = 0.f;
                UpdateDebugSnapshots();
            }
        }
    }
}


void AEMRNightShiftGameState::UpdateDebugSnapshots()
{
    if (const UWorld* World = GetWorld())
    {
        if (const UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
        {
            ReplicatedSpawnDebugInfo = SpawnSubsystem->GetDebugInfo();
            bHasReplicatedSpawnDebugInfo = true;
        }
        else
        {
            bHasReplicatedSpawnDebugInfo = false;
            ReplicatedSpawnDebugInfo = FEMRNightShiftSpawnDebugInfo();
        }

        if (const UEMRPatientPoolSubsystem* PoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
        {
            ReplicatedPatientPoolDebugInfo = PoolSubsystem->GetDebugInfo();
            bHasReplicatedPatientPoolDebugInfo = true;
        }
        else
        {
            bHasReplicatedPatientPoolDebugInfo = false;
            ReplicatedPatientPoolDebugInfo = FEMRPatientPoolDebugInfo();
        }
    }
}

void AEMRNightShiftGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, RunPhase);
    DOREPLIFETIME(ThisClass, bRunFailed);
    DOREPLIFETIME(ThisClass, bFinalMissionUnlocked);

    DOREPLIFETIME(ThisClass, CurrentCycleIndex);
    DOREPLIFETIME(ThisClass, NightShiftIndexInCycle);
    DOREPLIFETIME(ThisClass, CurrentCycleQuota);
    DOREPLIFETIME(ThisClass, CurrentCycleStartRevenue);

    DOREPLIFETIME(ThisClass, RemainingTimeInNightShift);
    DOREPLIFETIME(ThisClass, bIsInNightShiftOvertime);
    DOREPLIFETIME(ThisClass, bNightShiftTimeExpired);
    DOREPLIFETIME(ThisClass, bNightShiftFailedByReputation);
    DOREPLIFETIME(ThisClass, bReputationLockedAtZero);
    DOREPLIFETIME(ThisClass, bReputationFrozen);
    DOREPLIFETIME(ThisClass, FrozenReputation);
    DOREPLIFETIME(ThisClass, NightShiftOvertimeElapsedSeconds);
    DOREPLIFETIME(ThisClass, SpecialEventPhase);
    DOREPLIFETIME(ThisClass, ActiveSpecialEventId);
    DOREPLIFETIME(ThisClass, SpecialEventPhaseServerTimestamp);
    DOREPLIFETIME(ThisClass, SpecialEventAlertTitle);
    DOREPLIFETIME(ThisClass, SpecialEventAlertDescription);
    DOREPLIFETIME(ThisClass, SpecialEventFlickerLightTag);
    DOREPLIFETIME(ThisClass, SpecialEventFlickerColor);
    DOREPLIFETIME(ThisClass, SpecialEventLightFlickerRateHz);

    DOREPLIFETIME(ThisClass, CurrentNightShiftDefinition);
    DOREPLIFETIME(ThisClass, NextNightShiftsAvailable);
    DOREPLIFETIME(ThisClass, bNightShiftSelectionLocked);
    DOREPLIFETIME(ThisClass, bNightShiftSelectionCommitted);
    DOREPLIFETIME(ThisClass, HubDecisionStage);
    DOREPLIFETIME(ThisClass, HubUpgradeVoteState);
    DOREPLIFETIME(ThisClass, ActiveRunUpgradeTags);
    DOREPLIFETIME(ThisClass, PendingTravelURL);
    DOREPLIFETIME(ThisClass, SessionInviteCode);

    DOREPLIFETIME(ThisClass, ReplicatedSpawnDebugInfo);
    DOREPLIFETIME(ThisClass, ReplicatedPatientPoolDebugInfo);
    DOREPLIFETIME(ThisClass, bHasReplicatedSpawnDebugInfo);
    DOREPLIFETIME(ThisClass, bHasReplicatedPatientPoolDebugInfo);
    DOREPLIFETIME(ThisClass, LastNightShiftSummary);
    DOREPLIFETIME(ThisClass, LastNightShiftTelemetryRecord);
}


UAbilitySystemComponent* AEMRNightShiftGameState::GetAbilitySystemComponent() const
{
	return TeamASC;
}


float AEMRNightShiftGameState::GetTotalRevenue() const
{
    return TeamAttributeSet ? TeamAttributeSet->GetTotalRevenue() : 0.0f;
}

void AEMRNightShiftGameState::SetTotalRevenue(float NewTotalRevenue)
{
    if (!HasAuthority() || !TeamAttributeSet)
    {
        return;
    }

    TeamAttributeSet->SetTotalRevenue(NewTotalRevenue);
}


float AEMRNightShiftGameState::GetReputation() const
{
    return TeamAttributeSet ? TeamAttributeSet->GetReputation() : 0.0f;
}


float AEMRNightShiftGameState::GetCurrentCycleRevenue() const
{
    const float CycleRevenue = GetTotalRevenue() - CurrentCycleStartRevenue;
    return FMath::Max(0.f, CycleRevenue);
}

TArray<UEMRNightShiftDefinition*> AEMRNightShiftGameState::GetAvailableNextNightShifts() const
{
    TArray<UEMRNightShiftDefinition*> Result;
    Result.Reserve(NextNightShiftsAvailable.Num());

    for (const TObjectPtr<UEMRNightShiftDefinition>& NightShift : NextNightShiftsAvailable)
    {
        Result.Add(NightShift.Get());
    }

    return Result;
}

bool AEMRNightShiftGameState::HasActiveRunUpgradeTag(const FGameplayTag UpgradeTag) const
{
    return GetActiveRunUpgradeStackCount(UpgradeTag) > 0;
}

int32 AEMRNightShiftGameState::GetActiveRunUpgradeStackCount(const FGameplayTag UpgradeTag) const
{
    if (!UpgradeTag.IsValid())
    {
        return 0;
    }

    if (const int32* FoundStack = ActiveRunUpgradeStacks.Find(UpgradeTag))
    {
        return FMath::Max(0, *FoundStack);
    }

    return ActiveRunUpgradeTags.HasTagExact(UpgradeTag) ? 1 : 0;
}



void AEMRNightShiftGameState::SetRunPhase(EER_RunPhase NewPhase)
{
    if (!HasAuthority())
    {
        return;
    }

    const EER_RunPhase PreviousPhase = RunPhase;

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetRunPhase %d -> %d"), static_cast<int32>(RunPhase), static_cast<int32>(NewPhase));
    RunPhase = NewPhase;

    if (PreviousPhase != EER_RunPhase::RunFinished && NewPhase == EER_RunPhase::RunFinished)
    {
        if (UWorld* World = GetWorld())
        {
            if (UEMRPatientPoolSubsystem* PatientPoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
            {
                PatientPoolSubsystem->InitializePool(0);
            }
        }
    }

    if (PreviousPhase == EER_RunPhase::InNightShift && NewPhase != EER_RunPhase::InNightShift)
    {
        SetNightShiftOvertimeActive(false);
        SetSpecialEventPhase(EEMRSpecialEventPhase::None, NAME_None, 0.0f);
        ClearSpecialEventPresentationData();

        if (UWorld* World = GetWorld())
        {
            if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
            {
                SpawnSubsystem->StopNightShiftSpawning();
            }
        }
    }

    if (PreviousPhase != EER_RunPhase::InNightShift && NewPhase == EER_RunPhase::InNightShift && CurrentNightShiftDefinition)
    {
        if (UWorld* World = GetWorld())
        {
            if (UEMRNightShiftSpawnSubsystem* SpawnSubsystem = World->GetSubsystem<UEMRNightShiftSpawnSubsystem>())
            {
                SpawnSubsystem->StartNightShiftSpawning(CurrentNightShiftDefinition);
            }
        }
    }

    // Call on server so listeners react immediately, clients will get the replicated notification.
    OnRep_RunPhase(PreviousPhase);
}

void AEMRNightShiftGameState::SetSessionInviteCode(const FString& InCode)
{
    if (!HasAuthority())
    {
        return;
    }

    if (SessionInviteCode == InCode)
    {
        return;
    }

    SessionInviteCode = InCode;
    OnRep_SessionInviteCode();
}

void AEMRNightShiftGameState::ResetNightShiftSummary()
{
    if (!HasAuthority())
    {
        return;
    }

    LastNightShiftSummary = FEMRNightShiftSummary();
    OnRep_LastNightShiftSummary();
}

void AEMRNightShiftGameState::RegisterPatientPaid()
{
    if (!HasAuthority())
    {
        return;
    }

    ++LastNightShiftSummary.PatientsTreated;
    LastNightShiftSummary.bIsValid = true;
    OnRep_LastNightShiftSummary();
}

void AEMRNightShiftGameState::RegisterPatientLeftWithoutPay()
{
    if (!HasAuthority())
    {
        return;
    }

    ++LastNightShiftSummary.PatientsLeftWithoutPay;
    LastNightShiftSummary.bIsValid = true;
    OnRep_LastNightShiftSummary();
}

void AEMRNightShiftGameState::SetLastNightShiftSummary(const FEMRNightShiftSummary& Summary)
{
    if (!HasAuthority())
    {
        return;
    }

    LastNightShiftSummary = Summary;
    OnRep_LastNightShiftSummary();
}

void AEMRNightShiftGameState::SetLastNightShiftTelemetryRecord(const FEMRNightShiftTelemetryRecord& Record)
{
    if (!HasAuthority())
    {
        return;
    }

    LastNightShiftTelemetryRecord = Record;
    OnRep_LastNightShiftTelemetryRecord();
}


void AEMRNightShiftGameState::SetRunFailed(bool bFailed)
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetRunFailed %d"), bFailed);
	bRunFailed = bFailed;
}


void AEMRNightShiftGameState::SetFinalMissionUnlocked(bool bUnlocked)
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetFinalMissionUnlocked %d"), bUnlocked);
	bFinalMissionUnlocked = bUnlocked;
}


void AEMRNightShiftGameState::SetCurrentCycleInfo(int32 NewCycleIndex, float NewCycleQuota, float NewCycleStartRevenue)
{
    if (!HasAuthority())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetCurrentCycleInfo Cycle=%d Quota=%.2f StartRevenue=%.2f"), NewCycleIndex, NewCycleQuota, NewCycleStartRevenue);

    CurrentCycleIndex        = NewCycleIndex;
    CurrentCycleQuota        = NewCycleQuota;
    CurrentCycleStartRevenue = NewCycleStartRevenue;

    OnRep_CurrentCycleQuota();
}

void AEMRNightShiftGameState::SetNightShiftIndexInCycle(int32 NewNightShiftIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetNightShiftIndexInCycle %d"), NewNightShiftIndex);
	NightShiftIndexInCycle = NewNightShiftIndex;
}


void AEMRNightShiftGameState::SetRemainingTimeInNightShift(float NewRemainingTime)
{
	if (!HasAuthority())
	{
		return;
	}

	RemainingTimeInNightShift = NewRemainingTime;
	
	OnRep_RemainingTimeInNightShift();
}


void AEMRNightShiftGameState::SetCurrentNightShiftDefinition(UEMRNightShiftDefinition* NewNightShift)
{
    if (!HasAuthority())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetCurrentNightShiftDefinition %s"), NewNightShift ? *NewNightShift->GetName() : TEXT("nullptr"));
    CurrentNightShiftDefinition = NewNightShift;
}

void AEMRNightShiftGameState::SetNightShiftOvertimeActive(bool bInOvertime)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bIsInNightShiftOvertime == bInOvertime)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetNightShiftOvertimeActive %d"), bInOvertime);
    bIsInNightShiftOvertime = bInOvertime;
    ResetNightShiftOvertimeProgress();

    OnRep_NightShiftOvertime();
}

void AEMRNightShiftGameState::SetNightShiftTimeExpired(bool bExpired)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bNightShiftTimeExpired == bExpired)
    {
        return;
    }

    bNightShiftTimeExpired = bExpired;
    OnRep_NightShiftTimeExpired();
}

void AEMRNightShiftGameState::SetNightShiftFailedByReputation(bool bFailedByReputation)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bNightShiftFailedByReputation == bFailedByReputation)
    {
        return;
    }

    bNightShiftFailedByReputation = bFailedByReputation;
    OnRep_NightShiftFailedByReputation();
}

void AEMRNightShiftGameState::SetReputationLockedAtZero(bool bLocked)
{
    if (!HasAuthority())
    {
        return;
    }

    const bool bWasLockedAtZero = bReputationLockedAtZero;
    bReputationLockedAtZero = bLocked;

    if (bLocked)
    {
        SetReputationFrozen(true, 0.f);
    }
    else if (bWasLockedAtZero && bReputationFrozen && FMath::IsNearlyZero(FrozenReputation))
    {
        SetReputationFrozen(false, 0.f);
    }
}

void AEMRNightShiftGameState::SetReputationFrozen(const bool bFrozen, const float InFrozenReputation)
{
    if (!HasAuthority())
    {
        return;
    }

    bReputationFrozen = bFrozen;
    FrozenReputation = bFrozen ? FMath::Clamp(InFrozenReputation, 0.f, 100.f) : 0.f;
}

void AEMRNightShiftGameState::ResetNightShiftEndState()
{
    if (!HasAuthority())
    {
        return;
    }

    bNightShiftTimeExpired = false;
    bNightShiftFailedByReputation = false;
    bReputationLockedAtZero = false;
    SetReputationFrozen(false, 0.f);
}

void AEMRNightShiftGameState::SetSpecialEventPhase(const EEMRSpecialEventPhase NewPhase, const FName EventId, const float PhaseServerTimestamp)
{
    if (!HasAuthority())
    {
        return;
    }

    const EEMRSpecialEventPhase PreviousPhase = SpecialEventPhase;
    const bool bPhaseChanged = PreviousPhase != NewPhase;
    const bool bEventChanged = ActiveSpecialEventId != EventId;
    const bool bTimestampChanged = !FMath::IsNearlyEqual(SpecialEventPhaseServerTimestamp, PhaseServerTimestamp);

    if (!bPhaseChanged && !bEventChanged && !bTimestampChanged)
    {
        return;
    }

    SpecialEventPhase = NewPhase;
    ActiveSpecialEventId = EventId;
    SpecialEventPhaseServerTimestamp = PhaseServerTimestamp;

    OnRep_SpecialEventPhase(PreviousPhase);
}

void AEMRNightShiftGameState::SetSpecialEventPresentationData(
    const FText& AlertTitle,
    const FText& AlertDescription,
    const FName FlickerLightTag,
    const FLinearColor& FlickerColor,
    const float LightFlickerRateHz)
{
    if (!HasAuthority())
    {
        return;
    }

    SpecialEventAlertTitle = AlertTitle;
    SpecialEventAlertDescription = AlertDescription;
    SpecialEventFlickerLightTag = FlickerLightTag;
    SpecialEventFlickerColor = FlickerColor;
    SpecialEventLightFlickerRateHz = FMath::Max(LightFlickerRateHz, 0.01f);
}

void AEMRNightShiftGameState::ClearSpecialEventPresentationData()
{
    if (!HasAuthority())
    {
        return;
    }

    SpecialEventAlertTitle = FText::GetEmpty();
    SpecialEventAlertDescription = FText::GetEmpty();
    SpecialEventFlickerLightTag = NAME_None;
    SpecialEventFlickerColor = FLinearColor::Red;
    SpecialEventLightFlickerRateHz = 1.0f;
}

void AEMRNightShiftGameState::ResetNightShiftOvertimeProgress()
{
    NightShiftOvertimeElapsedSeconds = 0.f;
    CachedPatienceDrainMultiplier = GetOvertimePatienceDrainMultiplier();

    if (HasAuthority())
    {
        if (UWorld* World = GetWorld())
        {
            if (UEMRPatientPoolSubsystem* PoolSubsystem = World->GetSubsystem<UEMRPatientPoolSubsystem>())
            {
                PoolSubsystem->ApplyPatienceDrainMultiplier(CachedPatienceDrainMultiplier);
            }
        }
    }
}

float AEMRNightShiftGameState::GetOvertimeDifficultyMultiplier() const
{
    if (!bIsInNightShiftOvertime)
    {
        return 1.f;
    }

    if (const UEMRDifficultyTuningData* DifficultyTuning = GetDifficultyTuning())
    {
        if (const UCurveFloat* OvertimeDifficultyCurve = DifficultyTuning->GetOvertimeDifficultyCurve())
        {
            const float CurveValue = OvertimeDifficultyCurve->GetFloatValue(NightShiftOvertimeElapsedSeconds);
            return FMath::Max(CurveValue, 1.f);
        }
    }

    return 1.f;
}

float AEMRNightShiftGameState::GetOvertimeSpawnRateMultiplier() const
{
    if (!bIsInNightShiftOvertime)
    {
        return 1.f;
    }

    const float DifficultyScalar = GetDifficultySpecificOvertimeScalar(
        [](const UEMRDifficultyTuningData& Tuning, const EEMRNightShiftDifficultyTier Difficulty)
        {
            return Tuning.GetOvertimeSpawnRateScalar(Difficulty);
        });

    return GetOvertimeDifficultyMultiplier() * DifficultyScalar;
}

float AEMRNightShiftGameState::GetOvertimePatienceDrainMultiplier() const
{
    if (!bIsInNightShiftOvertime)
    {
        return 1.f;
    }

    const float DifficultyScalar = GetDifficultySpecificOvertimeScalar(
            [](const UEMRDifficultyTuningData& Tuning, const EEMRNightShiftDifficultyTier Difficulty)
            {
                return Tuning.GetOvertimePatienceDrainScalar(Difficulty);
            });

    return GetOvertimeDifficultyMultiplier() * DifficultyScalar;
}

void AEMRNightShiftGameState::SetAvailableNextNightShifts(const TArray<UEMRNightShiftDefinition*>& NewList)
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetAvailableNextNightShifts Count=%d"), NewList.Num());
	NextNightShiftsAvailable.Reset();
	for (UEMRNightShiftDefinition* NightShiftDef : NewList)
	{
		NextNightShiftsAvailable.Add(NightShiftDef);
	}

    SetNightShiftSelectionCommitted(false);

    OnRep_NextNightShiftsAvailable();
}

void AEMRNightShiftGameState::SetNightShiftSelectionLocked(bool bLocked)
{
    if (!HasAuthority())
    {
        return;
    }

	if (bNightShiftSelectionLocked == bLocked)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetNightShiftSelectionLocked %d"), bLocked);
    bNightShiftSelectionLocked = bLocked;
    OnRep_NightShiftSelectionLocked();
}

void AEMRNightShiftGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        if (TeamReputationChangedHandle.IsValid())
        {
            ASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetReputationAttribute())
                .Remove(TeamReputationChangedHandle);
        }
    }

    TeamReputationChangedHandle.Reset();

    Super::EndPlay(EndPlayReason);
}

void AEMRNightShiftGameState::HandleTeamReputationChanged(const FOnAttributeChangeData& Data)
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftGameState] Team reputation changed: %.2f -> %.2f"), Data.OldValue, Data.NewValue);
}

void AEMRNightShiftGameState::SetNightShiftSelectionCommitted(bool bCommitted)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bNightShiftSelectionCommitted == bCommitted)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] SetNightShiftSelectionCommitted %d"), bCommitted);
    bNightShiftSelectionCommitted = bCommitted;
    OnRep_NightShiftSelectionCommitted();
}

void AEMRNightShiftGameState::SetHubDecisionStage(const EEMRHubDecisionStage NewStage)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s GameState SetHubDecisionStage ignored: no authority."), NightShiftGameStateUpgradesFlowLogPrefix);
        return;
    }

    if (HubDecisionStage == NewStage)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s GameState SetHubDecisionStage no-op stage=%d"),
            NightShiftGameStateUpgradesFlowLogPrefix,
            static_cast<int32>(NewStage));
        return;
    }

    const EEMRHubDecisionStage PreviousStage = HubDecisionStage;
    HubDecisionStage = NewStage;
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s GameState SetHubDecisionStage changed %d -> %d"),
        NightShiftGameStateUpgradesFlowLogPrefix,
        static_cast<int32>(PreviousStage),
        static_cast<int32>(NewStage));
    OnRep_HubDecisionStage(PreviousStage);
}

void AEMRNightShiftGameState::SetHubUpgradeVoteState(const FEMRHubUpgradeVoteState& NewState)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s GameState SetHubUpgradeVoteState ignored: no authority."), NightShiftGameStateUpgradesFlowLogPrefix);
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s GameState SetHubUpgradeVoteState offers=%d votes=%d active=%d winner=%d voteEnd=%.2f"),
        NightShiftGameStateUpgradesFlowLogPrefix,
        NewState.OfferedUpgrades.Num(),
        NewState.VoteCounts.Num(),
        NewState.bVoteActive ? 1 : 0,
        NewState.WinningOfferIndex,
        NewState.VoteEndServerTimeSeconds);
    HubUpgradeVoteState = NewState;
    OnRep_HubUpgradeVoteState();
}

void AEMRNightShiftGameState::SetActiveRunUpgradeTags(const FGameplayTagContainer& NewTags)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s GameState SetActiveRunUpgradeTags ignored: no authority."), NightShiftGameStateUpgradesFlowLogPrefix);
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s GameState SetActiveRunUpgradeTags count=%d tags=%s"),
        NightShiftGameStateUpgradesFlowLogPrefix,
        NewTags.Num(),
        *NewTags.ToStringSimple());
    ActiveRunUpgradeStacks.Reset();
    for (const FGameplayTag& Tag : NewTags)
    {
        if (Tag.IsValid())
        {
            ActiveRunUpgradeStacks.Add(Tag, 1);
        }
    }

    RebuildActiveRunUpgradeTagsFromStacks();
    OnRep_ActiveRunUpgradeStacks();
    OnRep_ActiveRunUpgradeTags();
}

void AEMRNightShiftGameState::SetActiveRunUpgradeStacks(const TMap<FGameplayTag, int32>& NewStacks)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s GameState SetActiveRunUpgradeStacks ignored: no authority."), NightShiftGameStateUpgradesFlowLogPrefix);
        return;
    }

    ActiveRunUpgradeStacks.Reset();
    for (const TPair<FGameplayTag, int32>& StackPair : NewStacks)
    {
        if (StackPair.Key.IsValid() && StackPair.Value > 0)
        {
            ActiveRunUpgradeStacks.Add(StackPair.Key, StackPair.Value);
        }
    }

    RebuildActiveRunUpgradeTagsFromStacks();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s GameState SetActiveRunUpgradeStacks count=%d tags=%s"),
        NightShiftGameStateUpgradesFlowLogPrefix,
        ActiveRunUpgradeStacks.Num(),
        *ActiveRunUpgradeTags.ToStringSimple());
    OnRep_ActiveRunUpgradeStacks();
    OnRep_ActiveRunUpgradeTags();
}

bool AEMRNightShiftGameState::AddActiveRunUpgradeTag(const FGameplayTag UpgradeTag)
{
    if (!HasAuthority() || !UpgradeTag.IsValid())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s GameState AddActiveRunUpgradeTag rejected auth=%d tagValid=%d"),
            NightShiftGameStateUpgradesFlowLogPrefix,
            HasAuthority() ? 1 : 0,
            UpgradeTag.IsValid() ? 1 : 0);
        return false;
    }

    const int32 PreviousStack = GetActiveRunUpgradeStackCount(UpgradeTag);
    const int32 NewStack = AddActiveRunUpgradeStack(UpgradeTag, 1);
    return PreviousStack <= 0 && NewStack > 0;
}

int32 AEMRNightShiftGameState::AddActiveRunUpgradeStack(const FGameplayTag UpgradeTag, const int32 StackDelta)
{
    if (!HasAuthority() || !UpgradeTag.IsValid() || StackDelta <= 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s GameState AddActiveRunUpgradeStack rejected auth=%d tagValid=%d delta=%d"),
            NightShiftGameStateUpgradesFlowLogPrefix,
            HasAuthority() ? 1 : 0,
            UpgradeTag.IsValid() ? 1 : 0,
            StackDelta);
        return 0;
    }

    int32& StackCount = ActiveRunUpgradeStacks.FindOrAdd(UpgradeTag);
    StackCount = FMath::Max(0, StackCount) + StackDelta;
    RebuildActiveRunUpgradeTagsFromStacks();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s GameState AddActiveRunUpgradeStack tag=%s delta=%d newStack=%d"),
        NightShiftGameStateUpgradesFlowLogPrefix,
        *UpgradeTag.ToString(),
        StackDelta,
        StackCount);
    OnRep_ActiveRunUpgradeStacks();
    OnRep_ActiveRunUpgradeTags();
    return StackCount;
}

void AEMRNightShiftGameState::RebuildActiveRunUpgradeTagsFromStacks()
{
    ActiveRunUpgradeTags.Reset();
    for (const TPair<FGameplayTag, int32>& StackPair : ActiveRunUpgradeStacks)
    {
        if (StackPair.Key.IsValid() && StackPair.Value > 0)
        {
            ActiveRunUpgradeTags.AddTag(StackPair.Key);
        }
    }
}

void AEMRNightShiftGameState::SetPendingTravelURL(const FString& NewURL)
{
	if (!HasAuthority())
	{
		return;
	}

	if (PendingTravelURL == NewURL)
	{
		return;
	}

	PendingTravelURL = NewURL;
	OnRep_PendingTravelURL();
}

void AEMRNightShiftGameState::ClearPendingTravelURL()
{
	SetPendingTravelURL(TEXT(""));
}

void AEMRNightShiftGameState::ClearNightShiftSelectionLock()
{
	SetNightShiftSelectionLocked(false);
}


void AEMRNightShiftGameState::OnRep_RunPhase(EER_RunPhase PreviousPhase)
{
    RunPhaseChangedDelegate.Broadcast(RunPhase, PreviousPhase);
}


void AEMRNightShiftGameState::OnRep_CurrentCycleQuota()
{
    UE_LOG(
        LogTemp,
        Log,
        TEXT("[NightShiftFlow][NightShiftGameState] OnRep_CurrentCycleQuota - Cycle=%d Quota=%.2f StartRevenue=%.2f"),
        CurrentCycleIndex,
        CurrentCycleQuota,
        CurrentCycleStartRevenue);

    CycleInfoUpdatedDelegate.Broadcast(CurrentCycleIndex, CurrentCycleQuota, CurrentCycleStartRevenue);
}


void AEMRNightShiftGameState::OnRep_RemainingTimeInNightShift()
{

}

void AEMRNightShiftGameState::OnRep_NightShiftOvertime()
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] OnRep_NightShiftOvertime - Active=%s"), bIsInNightShiftOvertime ? TEXT("true") : TEXT("false"));

    if (bIsInNightShiftOvertime)
    {
        NightShiftOvertimeStartedDelegate.Broadcast();
    }
}

void AEMRNightShiftGameState::OnRep_NightShiftTimeExpired()
{
    if (bNightShiftTimeExpired)
    {
        NightShiftTimeExpiredDelegate.Broadcast();
    }
}

void AEMRNightShiftGameState::OnRep_NightShiftFailedByReputation()
{
    if (bNightShiftFailedByReputation)
    {
        NightShiftFailedByReputationDelegate.Broadcast();
    }
}

void AEMRNightShiftGameState::Multicast_PlayGlobalSound2D_Implementation(USoundBase* Sound)
{
    if (GetNetMode() == NM_DedicatedServer || !Sound)
    {
        return;
    }

    UGameplayStatics::PlaySound2D(this, Sound);
}

void AEMRNightShiftGameState::OnRep_SpecialEventPhase(EEMRSpecialEventPhase PreviousPhase)
{
    (void)PreviousPhase;
    SpecialEventPhaseChangedDelegate.Broadcast(SpecialEventPhase, ActiveSpecialEventId, SpecialEventPhaseServerTimestamp);
}

void AEMRNightShiftGameState::OnRep_NextNightShiftsAvailable()
{
	UE_LOG(LogTemp, Log, TEXT("[NightShiftGameState] OnRep_NextNightShiftsAvailable - %d NightShifts available"), 
	NextNightShiftsAvailable.Num());

	NextNightShiftsChangedDelegate.Broadcast();
}

void AEMRNightShiftGameState::OnRep_NightShiftSelectionLocked()
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftGameState] OnRep_NightShiftSelectionLocked - Locked=%s"), bNightShiftSelectionLocked ? TEXT("true") : TEXT("false"));
    NightShiftSelectionLockedDelegate.Broadcast();
}

void AEMRNightShiftGameState::OnRep_NightShiftSelectionCommitted()
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftGameState] OnRep_NightShiftSelectionCommitted - Committed=%s"), bNightShiftSelectionCommitted ? TEXT("true") : TEXT("false"));
    NightShiftSelectionCommittedDelegate.Broadcast();
}

void AEMRNightShiftGameState::OnRep_HubDecisionStage(const EEMRHubDecisionStage PreviousStage)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s GameState OnRep_HubDecisionStage %d -> %d"),
        NightShiftGameStateUpgradesFlowLogPrefix,
        static_cast<int32>(PreviousStage),
        static_cast<int32>(HubDecisionStage));
    HubDecisionStageChangedDelegate.Broadcast(HubDecisionStage, PreviousStage);
}

void AEMRNightShiftGameState::OnRep_HubUpgradeVoteState()
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s GameState OnRep_HubUpgradeVoteState offers=%d votes=%d active=%d winner=%d voteEnd=%.2f"),
        NightShiftGameStateUpgradesFlowLogPrefix,
        HubUpgradeVoteState.OfferedUpgrades.Num(),
        HubUpgradeVoteState.VoteCounts.Num(),
        HubUpgradeVoteState.bVoteActive ? 1 : 0,
        HubUpgradeVoteState.WinningOfferIndex,
        HubUpgradeVoteState.VoteEndServerTimeSeconds);
    HubUpgradeVoteStateChangedDelegate.Broadcast();
}

void AEMRNightShiftGameState::OnRep_ActiveRunUpgradeTags()
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s GameState OnRep_ActiveRunUpgradeTags count=%d tags=%s"),
        NightShiftGameStateUpgradesFlowLogPrefix,
        ActiveRunUpgradeTags.Num(),
        *ActiveRunUpgradeTags.ToStringSimple());
    ActiveRunUpgradeTagsChangedDelegate.Broadcast();
}

void AEMRNightShiftGameState::OnRep_ActiveRunUpgradeStacks()
{
    FString StackLog;
    for (const TPair<FGameplayTag, int32>& StackPair : ActiveRunUpgradeStacks)
    {
        if (!StackLog.IsEmpty())
        {
            StackLog += TEXT(", ");
        }

        StackLog += FString::Printf(TEXT("%s=%d"), *StackPair.Key.ToString(), StackPair.Value);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s GameState OnRep_ActiveRunUpgradeStacks count=%d stacks=[%s]"),
        NightShiftGameStateUpgradesFlowLogPrefix,
        ActiveRunUpgradeStacks.Num(),
        StackLog.IsEmpty() ? TEXT("<none>") : *StackLog);
}

void AEMRNightShiftGameState::OnRep_PendingTravelURL()
{
    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][NightShiftGameState] OnRep_PendingTravelURL -> %s"), PendingTravelURL.IsEmpty() ? TEXT("<none>") : *PendingTravelURL);
    PendingTravelChangedDelegate.Broadcast();
}

void AEMRNightShiftGameState::OnRep_SessionInviteCode()
{
    SessionInviteCodeChangedDelegate.Broadcast(SessionInviteCode);
}

void AEMRNightShiftGameState::OnRep_LastNightShiftSummary()
{
    NightShiftSummaryChangedDelegate.Broadcast(LastNightShiftSummary);
}

void AEMRNightShiftGameState::OnRep_LastNightShiftTelemetryRecord()
{
    NightShiftTelemetryPublishedDelegate.Broadcast(LastNightShiftTelemetryRecord);
}

const UEMRDifficultyTuningData* AEMRNightShiftGameState::GetDifficultyTuningData() const
{
    return GetDifficultyTuning();
}

float AEMRNightShiftGameState::GetDifficultySpecificOvertimeScalar(const TFunction<float(const UEMRDifficultyTuningData&, EEMRNightShiftDifficultyTier)>& Getter) const
{
    const UEMRDifficultyTuningData* DifficultyTuning = GetDifficultyTuning();

    const EEMRNightShiftDifficultyTier DifficultyTier = CurrentNightShiftDefinition
    ? CurrentNightShiftDefinition->DifficultyTier
    : EEMRNightShiftDifficultyTier::Calm;

    return DifficultyTuning ? Getter(*DifficultyTuning, DifficultyTier) : 1.f;
}

const UEMRDifficultyTuningData* AEMRNightShiftGameState::GetDifficultyTuning() const
{
    LoadDifficultyTuning();
    return CachedDifficultyTuning.Get();
}

void AEMRNightShiftGameState::LoadDifficultyTuning() const
{
    if (CachedDifficultyTuning.IsValid())
    {
        return;
    }

    TArray<const UEMRSubsystemConfig*> Configs;
    if (!UEMRAssetManager::Get().CollectLoadedSubsystemConfig(Configs) || Configs.Num() == 0)
    {
        return;
    }

    if (const UEMRSubsystemConfig* Config = Configs[0])
    {
        CachedDifficultyTuning = Config->GetDifficultyTuning();
    }
}
