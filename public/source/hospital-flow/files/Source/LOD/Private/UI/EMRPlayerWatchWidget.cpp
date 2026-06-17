#include "UI/EMRPlayerWatchWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/Attributes/EMRTeamAttributeSet.h"
#include "Subsystems/EMRRunRulesSubsystem.h"

void UEMRPlayerWatchWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CachedNightShiftGameState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;

    if (Text_TimeHours)
    {
        DefaultHourColor = Text_TimeHours->GetColorAndOpacity();
    }

    if (Text_Reputation)
    {
        DefaultReputationColor = Text_Reputation->GetColorAndOpacity();
    }
    BindToRunState();
    InitializeFromRunState();
    TryBindToTeamASC();
}

void UEMRPlayerWatchWidget::NativeDestruct()
{
    UnbindFromTeamASC();
    UnbindFromRunState();
    CachedNightShiftGameState.Reset();

    Super::NativeDestruct();
}

void UEMRPlayerWatchWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!CachedNightShiftGameState.IsValid())
    {
        UnbindFromRunState();
        CachedNightShiftGameState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
        BindToRunState();
        InitializeFromRunState();
        TryBindToTeamASC();
        return;
    }

    if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
    {
        const float Remaining = RunGS->GetRemainingTimeInNightShift();
        float DurationSeconds = CachedNightShiftDuration;
        if (const UWorld* World = GetWorld())
        {
            if (const UGameInstance* GameInstance = World->GetGameInstance())
            {
                if (const UEMRRunRulesSubsystem* RunRulesSubsystem = GameInstance->GetSubsystem<UEMRRunRulesSubsystem>())
                {
                    DurationSeconds = RunRulesSubsystem->GetNightShiftDurationSeconds();
                }
            }
        }
        if (DurationSeconds <= 0.f)
        {
            DurationSeconds = 600.f;
        }

        const bool bDurationChanged = !FMath::IsNearlyEqual(DurationSeconds, CachedNightShiftDuration);
        if (bDurationChanged)
        {
            CachedNightShiftDuration = DurationSeconds;
        }

        if (!FMath::IsNearlyEqual(Remaining, CachedRemainingTime) || bDurationChanged)
        {
            CachedRemainingTime = Remaining;
            UpdateWatchTime(Remaining, CachedNightShiftDuration);
        }

        UpdateSpecialEventAlertState();
        UpdateOvertimeState(RunGS->IsInNightShiftOvertime());
    }

    if (!CachedTeamASC.IsValid())
    {
        TryBindToTeamASC();
    }
}

void UEMRPlayerWatchWidget::InitializeFromRunState()
{
    if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
    {
        CachedNightShiftDuration = 600.f;
        if (const UWorld* World = GetWorld())
        {
            if (const UGameInstance* GameInstance = World->GetGameInstance())
            {
                if (const UEMRRunRulesSubsystem* RunRulesSubsystem = GameInstance->GetSubsystem<UEMRRunRulesSubsystem>())
                {
                    CachedNightShiftDuration = RunRulesSubsystem->GetNightShiftDurationSeconds();
                }
            }
        }
        CachedNightShiftDuration = FMath::Max(CachedNightShiftDuration, 1.0f);

        CachedRemainingTime = RunGS->GetRemainingTimeInNightShift();
        UpdateWatchTime(CachedRemainingTime, CachedNightShiftDuration);

        UpdateCycleQuota(RunGS->GetCurrentCycleQuota());
        UpdateTotalRevenue(RunGS->GetTotalRevenue());
        UpdateReputation(RunGS->GetReputation());
        UpdateSpecialEventAlertState();
        UpdateOvertimeState(RunGS->IsInNightShiftOvertime());
    }
}

void UEMRPlayerWatchWidget::BindToRunState()
{
    if (!CachedNightShiftGameState.IsValid())
    {
        return;
    }

    if (!CycleInfoDelegateHandle.IsValid())
    {
        CycleInfoDelegateHandle = CachedNightShiftGameState->OnCycleInfoUpdated().AddUObject(
            this,
            &ThisClass::HandleCycleInfoUpdated);
    }

    if (!OvertimeDelegateHandle.IsValid())
    {
        OvertimeDelegateHandle = CachedNightShiftGameState->OnNightShiftOvertimeStarted().AddUObject(
            this,
            &ThisClass::HandleOvertimeStarted);
    }

    if (!SpecialEventPhaseDelegateHandle.IsValid())
    {
        SpecialEventPhaseDelegateHandle = CachedNightShiftGameState->OnSpecialEventPhaseChanged().AddUObject(
            this,
            &ThisClass::HandleSpecialEventPhaseChanged);
    }
}

void UEMRPlayerWatchWidget::UnbindFromRunState()
{
    if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
    {
        if (CycleInfoDelegateHandle.IsValid())
        {
            RunGS->OnCycleInfoUpdated().Remove(CycleInfoDelegateHandle);
            CycleInfoDelegateHandle.Reset();
        }

        if (OvertimeDelegateHandle.IsValid())
        {
            RunGS->OnNightShiftOvertimeStarted().Remove(OvertimeDelegateHandle);
            OvertimeDelegateHandle.Reset();
        }

        if (SpecialEventPhaseDelegateHandle.IsValid())
        {
            RunGS->OnSpecialEventPhaseChanged().Remove(SpecialEventPhaseDelegateHandle);
            SpecialEventPhaseDelegateHandle.Reset();
        }
    }
}

void UEMRPlayerWatchWidget::TryBindToTeamASC()
{
    if (!CachedNightShiftGameState.IsValid())
    {
        return;
    }

    UAbilitySystemComponent* TeamASC = CachedNightShiftGameState->GetAbilitySystemComponent();
    if (!TeamASC)
    {
        return;
    }

    CachedTeamASC = TeamASC;

    UpdateTotalRevenue(CachedNightShiftGameState->GetTotalRevenue());
    UpdateReputation(CachedNightShiftGameState->GetReputation());

    RevenueDelegateHandle = TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetTotalRevenueAttribute())
    .AddUObject(this, &ThisClass::OnTotalRevenueChanged);

    ReputationDelegateHandle = TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetReputationAttribute())
    .AddUObject(this, &ThisClass::OnReputationChanged);
}

void UEMRPlayerWatchWidget::UnbindFromTeamASC()
{
    if (UAbilitySystemComponent* TeamASC = CachedTeamASC.Get())
    {
        if (RevenueDelegateHandle.IsValid())
        {
            TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetTotalRevenueAttribute()).Remove(RevenueDelegateHandle);
            RevenueDelegateHandle.Reset();
        }

        if (ReputationDelegateHandle.IsValid())
        {
            TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetReputationAttribute()).Remove(ReputationDelegateHandle);
            ReputationDelegateHandle.Reset();
        }
    }

    CachedTeamASC.Reset();
}

void UEMRPlayerWatchWidget::UpdateWatchTime(float RemainingSeconds, float TotalDurationSeconds)
{
    if (bSpecialEventAlertActive)
    {
        return;
    }

    if (!Text_TimeHours)
    {
        return;
    }

    const float ClampedDuration = FMath::Max(TotalDurationSeconds, 1.f);
    const float ClampedRemaining = FMath::Clamp(RemainingSeconds, 0.f, ClampedDuration);
    const float ElapsedSeconds = ClampedDuration - ClampedRemaining;
    const float NightSeconds = (ElapsedSeconds / ClampedDuration) * (12.f * 3600.f);

    const int32 TotalSeconds = (FMath::FloorToInt((20.f * 3600.f) + NightSeconds)) % (24 * 3600);
    const int32 Hour24 = TotalSeconds / 3600;
    const int32 Hour12 = (Hour24 % 12) == 0 ? 12 : (Hour24 % 12);
    const bool bIsPM = Hour24 >= 12;

    const bool bForcePeriodUpdate = (CachedHour == INDEX_NONE);

    if (CachedHour != Hour12)
    {
        CachedHour = Hour12;
        Text_TimeHours->SetText(FText::FromString(FString::Printf(TEXT("%02d"), Hour12)));
    }

    if (Text_TimeMinutes)
    {
        Text_TimeMinutes->SetText(FText::GetEmpty());
    }

    if (Text_TimePeriod && (bForcePeriodUpdate || bCachedIsPM != bIsPM))
    {
        bCachedIsPM = bIsPM;
        Text_TimePeriod->SetText(FText::FromString(bIsPM ? TEXT("PM") : TEXT("AM")));
    }
}

void UEMRPlayerWatchWidget::UpdateTotalRevenue(float NewRevenue)
{
    if (bSpecialEventAlertActive)
    {
        return;
    }

    if (!Text_TotalRevenue)
    {
        return;
    }

    Text_TotalRevenue->SetText(FText::AsNumber(FMath::RoundToInt(NewRevenue)));
}

void UEMRPlayerWatchWidget::UpdateCycleQuota(float NewQuota)
{
    if (bSpecialEventAlertActive)
    {
        return;
    }

    if (!Text_CycleQuota)
    {
        return;
    }

    Text_CycleQuota->SetText(FText::AsNumber(FMath::FloorToInt(NewQuota)));
}

void UEMRPlayerWatchWidget::UpdateReputation(float NewReputation)
{
    if (bSpecialEventAlertActive)
    {
        return;
    }

    if (!Text_Reputation)
    {
        return;
    }

    Text_Reputation->SetText(FText::AsNumber(FMath::RoundToInt(NewReputation)));
    Text_Reputation->SetColorAndOpacity(NewReputation <= 0.f ? ReputationZeroColor : DefaultReputationColor);
}

void UEMRPlayerWatchWidget::UpdateOvertimeState(bool bInOvertime)
{
    if (bOvertimeActive == bInOvertime)
    {
        return;
    }

    bOvertimeActive = bInOvertime;

    if (Text_TimeHours)
    {
        Text_TimeHours->SetColorAndOpacity(bOvertimeActive ? OvertimeHourColor : DefaultHourColor);
    }
}

void UEMRPlayerWatchWidget::HandleCycleInfoUpdated(int32 CycleIndex, float CycleQuota, float CycleStartRevenue)
{
    UpdateCycleQuota(CycleQuota);
}

void UEMRPlayerWatchWidget::OnTotalRevenueChanged(const FOnAttributeChangeData& ChangeData)
{
    UpdateTotalRevenue(ChangeData.NewValue);
}

void UEMRPlayerWatchWidget::OnReputationChanged(const FOnAttributeChangeData& ChangeData)
{
    UpdateReputation(ChangeData.NewValue);
}

void UEMRPlayerWatchWidget::HandleOvertimeStarted()
{
    UpdateOvertimeState(true);
}

void UEMRPlayerWatchWidget::HandleSpecialEventPhaseChanged(
    EEMRSpecialEventPhase NewPhase,
    FName EventId,
    float PhaseServerTimestamp)
{
    (void)NewPhase;
    (void)EventId;
    (void)PhaseServerTimestamp;
    UpdateSpecialEventAlertState();
}

void UEMRPlayerWatchWidget::UpdateSpecialEventAlertState()
{
    AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get();
    if (!RunGS)
    {
        return;
    }

    const EEMRSpecialEventPhase Phase = RunGS->GetSpecialEventPhase();
    const bool bShouldShowAlert = Phase == EEMRSpecialEventPhase::Alert;
    const bool bHasDedicatedAlertFields = Text_AlertTitle || Text_AlertDescription;

    if (bShouldShowAlert)
    {
        const FText AlertTitle = !RunGS->GetSpecialEventAlertTitle().IsEmpty()
            ? RunGS->GetSpecialEventAlertTitle()
            : FText::FromString(TEXT("Alert"));
        const FText AlertDescription = RunGS->GetSpecialEventAlertDescription();

        bSpecialEventAlertActive = true;

        if (Text_AlertTitle)
        {
            Text_AlertTitle->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            Text_AlertTitle->SetText(AlertTitle);
        }

        if (Text_AlertDescription)
        {
            Text_AlertDescription->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            Text_AlertDescription->SetText(AlertDescription);
        }

        if (bHasDedicatedAlertFields)
        {
            SetDefaultWatchDataVisible(false);
            return;
        }

        if (Text_TimeHours)
        {
            Text_TimeHours->SetText(FText::FromString(TEXT("ALERT")));
        }

        if (Text_TimeMinutes)
        {
            Text_TimeMinutes->SetText(FText::GetEmpty());
        }

        if (Text_TimePeriod)
        {
            Text_TimePeriod->SetText(FText::GetEmpty());
        }

        if (Text_TotalRevenue)
        {
            Text_TotalRevenue->SetText(AlertTitle);
        }

        if (Text_CycleQuota)
        {
            Text_CycleQuota->SetText(AlertDescription);
        }

        if (Text_Reputation)
        {
            Text_Reputation->SetText(FText::GetEmpty());
        }

        SetDefaultWatchDataVisible(true);
        return;
    }

    if (Text_AlertTitle)
    {
        Text_AlertTitle->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (Text_AlertDescription)
    {
        Text_AlertDescription->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (!bSpecialEventAlertActive)
    {
        return;
    }

    bSpecialEventAlertActive = false;
    SetDefaultWatchDataVisible(true);
    InitializeFromRunState();
}

void UEMRPlayerWatchWidget::SetDefaultWatchDataVisible(const bool bVisible)
{
    const ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;

    if (Box_DefaultWatchData)
    {
        Box_DefaultWatchData->SetVisibility(TargetVisibility);
        return;
    }

    if (Text_TimeHours)
    {
        Text_TimeHours->SetVisibility(TargetVisibility);
    }

    if (Text_TimeMinutes)
    {
        Text_TimeMinutes->SetVisibility(TargetVisibility);
    }

    if (Text_TimePeriod)
    {
        Text_TimePeriod->SetVisibility(TargetVisibility);
    }

    if (Text_TotalRevenue)
    {
        Text_TotalRevenue->SetVisibility(TargetVisibility);
    }

    if (Text_CycleQuota)
    {
        Text_CycleQuota->SetVisibility(TargetVisibility);
    }

    if (Text_Reputation)
    {
        Text_Reputation->SetVisibility(TargetVisibility);
    }
}
