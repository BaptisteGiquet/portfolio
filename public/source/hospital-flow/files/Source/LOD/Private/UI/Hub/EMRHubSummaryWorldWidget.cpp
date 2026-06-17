#include "UI/Hub/EMRHubSummaryWorldWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/Attributes/EMRTeamAttributeSet.h"
#include "Subsystems/EMRRunRulesSubsystem.h"

void UEMRHubSummaryWorldWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CachedNightShiftGameState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;

    if (Text_CurrentCycleQuota)
    {
        DefaultQuotaColor = Text_CurrentCycleQuota->GetColorAndOpacity();
    }

    BindToRunState();
    InitializeFromRunState();
    TryBindToTeamASC();
}

void UEMRHubSummaryWorldWidget::NativeDestruct()
{
    UnbindFromTeamASC();
    UnbindFromRunState();
    CachedNightShiftGameState.Reset();

    Super::NativeDestruct();
}

void UEMRHubSummaryWorldWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!CachedNightShiftGameState.IsValid())
    {
        UnbindFromRunState();
        CachedNightShiftGameState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
        BindToRunState();
        InitializeFromRunState();
        TryBindToTeamASC();
    }
    else
    {
        RefreshSummary();
    }

    if (!CachedTeamASC.IsValid())
    {
        TryBindToTeamASC();
    }
}

void UEMRHubSummaryWorldWidget::InitializeFromRunState()
{
    RefreshSummary();
}

void UEMRHubSummaryWorldWidget::BindToRunState()
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

    if (!RunPhaseChangedHandle.IsValid())
    {
        RunPhaseChangedHandle = CachedNightShiftGameState->OnRunPhaseChanged().AddUObject(
            this,
            &ThisClass::HandleRunPhaseChanged);
    }
}

void UEMRHubSummaryWorldWidget::UnbindFromRunState()
{
    if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
    {
        if (CycleInfoDelegateHandle.IsValid())
        {
            RunGS->OnCycleInfoUpdated().Remove(CycleInfoDelegateHandle);
            CycleInfoDelegateHandle.Reset();
        }

        if (RunPhaseChangedHandle.IsValid())
        {
            RunGS->OnRunPhaseChanged().Remove(RunPhaseChangedHandle);
            RunPhaseChangedHandle.Reset();
        }
    }
}

void UEMRHubSummaryWorldWidget::TryBindToTeamASC()
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

    if (CachedTeamASC.Get() == TeamASC && RevenueDelegateHandle.IsValid())
    {
        return;
    }

    UnbindFromTeamASC();
    CachedTeamASC = TeamASC;

    RevenueDelegateHandle = TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetTotalRevenueAttribute())
    .AddUObject(this, &ThisClass::OnTotalRevenueChanged);

    RefreshSummary();
}

void UEMRHubSummaryWorldWidget::UnbindFromTeamASC()
{
    if (UAbilitySystemComponent* TeamASC = CachedTeamASC.Get())
    {
        if (RevenueDelegateHandle.IsValid())
        {
            TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetTotalRevenueAttribute()).Remove(RevenueDelegateHandle);
            RevenueDelegateHandle.Reset();
        }
    }

    CachedTeamASC.Reset();
}

void UEMRHubSummaryWorldWidget::RefreshSummary()
{
    AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get();
    if (!RunGS && GetWorld())
    {
        RunGS = GetWorld()->GetGameState<AEMRNightShiftGameState>();
        CachedNightShiftGameState = RunGS;
    }

    const int32 MaxNightShiftsInCycle = ResolveMaxNightShiftsInCycle();

    const int32 DisplayedCycle = RunGS ? FMath::Max(1, RunGS->GetCurrentCycleIndex() + 1) : 1;
    const int32 DisplayedNextNightShift = RunGS
    ? FMath::Clamp(RunGS->GetNightShiftIndexInCycle() + 1, 1, MaxNightShiftsInCycle)
    : 1;
    const bool bIsRunWon = RunGS && RunGS->GetRunPhase() == EER_RunPhase::MissionFinal && RunGS->IsFinalMissionUnlocked();
    const bool bIsRunLost = RunGS && RunGS->GetRunPhase() == EER_RunPhase::RunFinished && RunGS->HasRunFailed();
    const bool bIsEndOfSession = bIsRunWon || bIsRunLost;
    const int32 DisplayedCycleRevenue = RunGS
    ? FMath::Max(0, FMath::RoundToInt(bIsEndOfSession ? RunGS->GetTotalRevenue() : RunGS->GetCurrentCycleRevenue()))
    : 0;
    const int32 DisplayedCycleQuota = RunGS
    ? FMath::Max(0, FMath::FloorToInt(RunGS->GetCurrentCycleQuota()))
    : 0;
    const bool bHasReachedCycleQuota = RunGS
    ? RunGS->GetTotalRevenue() >= RunGS->GetCurrentCycleQuota()
    : false;
    const bool bQuotaWarning = RunGS
    ? ShouldUseQuotaWarningColor(DisplayedNextNightShift, MaxNightShiftsInCycle, RunGS->GetRunPhase(), bHasReachedCycleQuota)
    : false;

    if (CachedDisplayedCycle != DisplayedCycle)
    {
        CachedDisplayedCycle = DisplayedCycle;
        SetOptionalIntegerText(Text_CurrentCycleNumber, DisplayedCycle);
    }

    if (CachedDisplayedNightShift != DisplayedNextNightShift)
    {
        CachedDisplayedNightShift = DisplayedNextNightShift;
        SetOptionalIntegerText(Text_CurrentNightShiftNumber, DisplayedNextNightShift);
    }

    if (CachedDisplayedMaxNightShift != MaxNightShiftsInCycle)
    {
        CachedDisplayedMaxNightShift = MaxNightShiftsInCycle;
        SetOptionalIntegerText(Text_MaxNightShiftInCycle, MaxNightShiftsInCycle);
    }

    if (CachedDisplayedCycleRevenue != DisplayedCycleRevenue)
    {
        CachedDisplayedCycleRevenue = DisplayedCycleRevenue;
        SetOptionalIntegerText(Text_CurrentCycleRevenue, DisplayedCycleRevenue);
    }

    if (CachedDisplayedCycleQuota != DisplayedCycleQuota)
    {
        CachedDisplayedCycleQuota = DisplayedCycleQuota;
        SetOptionalIntegerText(Text_CurrentCycleQuota, DisplayedCycleQuota);
    }

    if (Text_CurrentCycleQuota && bCachedQuotaWarning != bQuotaWarning)
    {
        bCachedQuotaWarning = bQuotaWarning;
        Text_CurrentCycleQuota->SetColorAndOpacity(bQuotaWarning ? QuotaWarningColor : DefaultQuotaColor);
    }

    SetOptionalTextVisibility(Text_CurrentCycleNumber, bIsEndOfSession ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    SetOptionalTextVisibility(Text_CurrentNightShiftNumber, bIsEndOfSession ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    SetOptionalTextVisibility(Text_MaxNightShiftInCycle, bIsEndOfSession ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    SetOptionalTextVisibility(Text_CurrentCycleRevenue, ESlateVisibility::HitTestInvisible);
    SetOptionalTextVisibility(Text_CurrentCycleQuota, ESlateVisibility::HitTestInvisible);

    ESlateVisibility EndOfSessionResultVisibility = ESlateVisibility::Collapsed;
    if (bIsRunWon)
    {
        EndOfSessionResultVisibility = ESlateVisibility::HitTestInvisible;
        if (Text_EndOfSessionResult)
        {
            Text_EndOfSessionResult->SetText(EndOfSessionWonText);
        }
    }
    else if (bIsRunLost)
    {
        EndOfSessionResultVisibility = ESlateVisibility::HitTestInvisible;
        if (Text_EndOfSessionResult)
        {
            Text_EndOfSessionResult->SetText(EndOfSessionLostText);
        }
    }

    SetOptionalTextVisibility(Text_EndOfSessionResult, EndOfSessionResultVisibility);
    SetOptionalBorderVisibility(Border_EndOfSessionResult, EndOfSessionResultVisibility);
}

int32 UEMRHubSummaryWorldWidget::ResolveMaxNightShiftsInCycle() const
{
    const UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    const UEMRRunRulesSubsystem* RunRulesSubsystem = GameInstance ? GameInstance->GetSubsystem<UEMRRunRulesSubsystem>() : nullptr;
    return FMath::Max(1, RunRulesSubsystem ? RunRulesSubsystem->GetNightShiftsPerCycle() : 1);
}

bool UEMRHubSummaryWorldWidget::ShouldUseQuotaWarningColor(
    const int32 NextNightShiftDisplay,
    const int32 MaxNightShiftsInCycle,
    const EER_RunPhase RunPhase,
    const bool bHasReachedCycleQuota) const
{
    return RunPhase == EER_RunPhase::Hub
    && NextNightShiftDisplay == MaxNightShiftsInCycle
    && !bHasReachedCycleQuota;
}

void UEMRHubSummaryWorldWidget::SetOptionalIntegerText(UTextBlock* TextBlock, const int32 Value) const
{
    if (!TextBlock)
    {
        return;
    }

    TextBlock->SetText(FText::AsNumber(Value));
}

void UEMRHubSummaryWorldWidget::SetOptionalTextVisibility(UTextBlock* TextBlock, ESlateVisibility NewVisibility) const
{
    if (!TextBlock)
    {
        return;
    }

    TextBlock->SetVisibility(NewVisibility);
}

void UEMRHubSummaryWorldWidget::SetOptionalBorderVisibility(UBorder* Border, const ESlateVisibility NewVisibility) const
{
    if (!Border)
    {
        return;
    }

    Border->SetVisibility(NewVisibility);
}

void UEMRHubSummaryWorldWidget::OnTotalRevenueChanged(const FOnAttributeChangeData& ChangeData)
{
    (void)ChangeData;
    RefreshSummary();
}

void UEMRHubSummaryWorldWidget::HandleCycleInfoUpdated(const int32 CycleIndex, const float CycleQuota, const float CycleStartRevenue)
{
    (void)CycleIndex;
    (void)CycleQuota;
    (void)CycleStartRevenue;
    RefreshSummary();
}

void UEMRHubSummaryWorldWidget::HandleRunPhaseChanged(const EER_RunPhase NewPhase, const EER_RunPhase PreviousPhase)
{
    (void)NewPhase;
    (void)PreviousPhase;
    RefreshSummary();
}
