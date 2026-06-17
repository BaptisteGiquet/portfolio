#include "UI/DEPRECATEDEMRNightShiftStatusWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/Attributes/EMRTeamAttributeSet.h"

void UDEPRECATEDEMRNightShiftStatusWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CachedNightShiftGameState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
    BindToRunState();
    InitializeFromRunState();
    TryBindToTeamASC();
}


void UDEPRECATEDEMRNightShiftStatusWidget::NativeDestruct()
{
    UnbindFromTeamASC();
    UnbindFromRunState();
    CachedNightShiftGameState.Reset();

    Super::NativeDestruct();
}


void UDEPRECATEDEMRNightShiftStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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
        if (!FMath::IsNearlyEqual(Remaining, CachedRemainingTime))
        {
            CachedRemainingTime = Remaining;
            UpdateTimeDisplay(Remaining);
        }

        const float NewQuota = RunGS->GetCurrentCycleQuota();
        if (!FMath::IsNearlyEqual(NewQuota, CachedQuota))
        {
            CachedQuota = NewQuota;
            UpdateCycleQuota(NewQuota);
        }

        const int32 CurrentCycleIndex = RunGS->GetCurrentCycleIndex();
        if (CachedCycleIndex != CurrentCycleIndex)
        {
            CachedCycleIndex = CurrentCycleIndex;
            UpdateCycleIndexDisplay(CurrentCycleIndex);
        }

        const int32 NightShiftIndex = RunGS->GetNightShiftIndexInCycle();
        if (CachedNightShiftIndex != NightShiftIndex)
        {
            CachedNightShiftIndex = NightShiftIndex;
            UpdateNightShiftIndexDisplay(NightShiftIndex);
        }

        UpdateCycleRevenue(RunGS->GetCurrentCycleRevenue());
    }

    if (!CachedTeamASC.IsValid())
    {
        TryBindToTeamASC();
    }
}


void UDEPRECATEDEMRNightShiftStatusWidget::InitializeFromRunState()
{
    if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
    {
        CachedQuota = RunGS->GetCurrentCycleQuota();
        CachedRemainingTime = RunGS->GetRemainingTimeInNightShift();
        CachedCycleIndex = RunGS->GetCurrentCycleIndex();
        CachedNightShiftIndex = RunGS->GetNightShiftIndexInCycle();

        UE_LOG(
            LogTemp,
            Log,
            TEXT("[NightShiftFlow][UI][StatusWidget] InitializeFromRunState - Cycle=%d NightShift=%d Quota=%.2f Remaining=%.2f"),
            CachedCycleIndex,
            CachedNightShiftIndex,
            CachedQuota,
            CachedRemainingTime);

        UpdateCycleQuota(CachedQuota);
        UpdateTimeDisplay(CachedRemainingTime);
        UpdateCycleRevenue(RunGS->GetCurrentCycleRevenue());
        UpdateCycleIndexDisplay(CachedCycleIndex);
        UpdateNightShiftIndexDisplay(CachedNightShiftIndex);
    }
}


void UDEPRECATEDEMRNightShiftStatusWidget::BindToRunState()
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

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[NightShiftFlow][UI][StatusWidget] Bound to GameState %s (Cycle=%d Quota=%.2f StartRevenue=%.2f)"),
        *CachedNightShiftGameState->GetName(),
        CachedNightShiftGameState->GetCurrentCycleIndex(),
        CachedNightShiftGameState->GetCurrentCycleQuota(),
        CachedNightShiftGameState->GetCurrentCycleStartRevenue());
}


void UDEPRECATEDEMRNightShiftStatusWidget::UnbindFromRunState()
{
    if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
    {
        if (CycleInfoDelegateHandle.IsValid())
        {
            RunGS->OnCycleInfoUpdated().Remove(CycleInfoDelegateHandle);
            CycleInfoDelegateHandle.Reset();
        }
    }
}


void UDEPRECATEDEMRNightShiftStatusWidget::HandleCycleInfoUpdated(int32 CycleIndex, float CycleQuota, float CycleStartRevenue)
{
    UE_LOG(
        LogTemp,
        Log,
        TEXT("[NightShiftFlow][UI][StatusWidget] HandleCycleInfoUpdated - Cycle=%d Quota=%.2f StartRevenue=%.2f"),
        CycleIndex,
        CycleQuota,
        CycleStartRevenue);

    CachedCycleIndex = CycleIndex;
    CachedQuota = CycleQuota;
    UpdateCycleQuota(CycleQuota);
    UpdateCycleIndexDisplay(CycleIndex);
    if (CachedNightShiftGameState.IsValid())
    {
        UpdateCycleRevenue(CachedNightShiftGameState->GetCurrentCycleRevenue());
    }
}


void UDEPRECATEDEMRNightShiftStatusWidget::TryBindToTeamASC()
{
    if (!CachedNightShiftGameState.IsValid())
    {
        return;
    }

    if (UAbilitySystemComponent* TeamASC = CachedNightShiftGameState->GetAbilitySystemComponent())
    {
        CachedTeamASC = TeamASC;

        const float InitialReputation = TeamASC->GetNumericAttribute(UEMRTeamAttributeSet::GetReputationAttribute());

        UpdateCycleRevenue(CachedNightShiftGameState->GetCurrentCycleRevenue());
        UpdateReputation(InitialReputation);

        RevenueDelegateHandle = TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetTotalRevenueAttribute())
            .AddUObject(this, &UDEPRECATEDEMRNightShiftStatusWidget::OnRevenueChanged);

        ReputationDelegateHandle = TeamASC->GetGameplayAttributeValueChangeDelegate(UEMRTeamAttributeSet::GetReputationAttribute())
            .AddUObject(this, &UDEPRECATEDEMRNightShiftStatusWidget::OnReputationChanged);
    }
}


void UDEPRECATEDEMRNightShiftStatusWidget::UnbindFromTeamASC()
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


void UDEPRECATEDEMRNightShiftStatusWidget::UpdateTimeDisplay(float RemainingSeconds)
{
    if (!Text_TimeRemaining)
    {
        return;
    }

    const int32 TotalSeconds = FMath::Max(0, FMath::RoundToInt(RemainingSeconds));
    if (TotalSeconds >= 60)
    {
        const int32 Minutes = TotalSeconds / 60;
        const int32 Seconds = TotalSeconds % 60;

        const FText MinutesText = FText::AsNumber(Minutes);
        const FText SecondsText = FText::FromString(FString::Printf(TEXT("%02d"), Seconds));

        const FText Formatted = FText::Format(
            NSLOCTEXT("NightShiftStatus", "TimeMinutes", "{0}:{1}"),
            MinutesText,
            SecondsText);

        Text_TimeRemaining->SetText(Formatted);
    }
    else
    {
        Text_TimeRemaining->SetText(FText::Format(
            NSLOCTEXT("NightShiftStatus", "TimeSeconds", "{0}s"),
            FText::AsNumber(TotalSeconds)));
    }
}


void UDEPRECATEDEMRNightShiftStatusWidget::UpdateCycleQuota(float NewQuota)
{
    if (!Text_CycleQuota)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[NightShiftFlow][UI][StatusWidget] UpdateCycleQuota -> %.2f"), NewQuota);

    Text_CycleQuota->SetText(FText::AsNumber(FMath::FloorToInt(NewQuota)));
}


void UDEPRECATEDEMRNightShiftStatusWidget::UpdateCycleRevenue(float NewRevenue)
{
    if (!Text_CycleRevenue)
    {
        return;
    }

    Text_CycleRevenue->SetText(FText::AsNumber(FMath::RoundToInt(NewRevenue)));
}


void UDEPRECATEDEMRNightShiftStatusWidget::UpdateReputation(float NewReputation)
{
    if (!Text_Reputation)
    {
        return;
    }

    Text_Reputation->SetText(FText::AsNumber(FMath::RoundToInt(NewReputation)));
}

void UDEPRECATEDEMRNightShiftStatusWidget::UpdateCycleIndexDisplay(int32 CycleIndex)
{
    if (!Text_CycleIndex)
    {
        return;
    }

    Text_CycleIndex->SetText(FText::Format(
        NSLOCTEXT("NightShiftStatus", "CycleIndex", "Cycle {0}"),
        FText::AsNumber(CycleIndex + 1)));
}

void UDEPRECATEDEMRNightShiftStatusWidget::UpdateNightShiftIndexDisplay(int32 NightShiftIndex)
{
    if (!Text_NightShiftIndex)
    {
        return;
    }

    Text_NightShiftIndex->SetText(FText::Format(
        NSLOCTEXT("NightShiftStatus", "NightShiftIndex", "NightShift {0}"),
        FText::AsNumber(NightShiftIndex + 1)));
}


void UDEPRECATEDEMRNightShiftStatusWidget::OnRevenueChanged(const FOnAttributeChangeData& ChangeData)
{
    const float CycleRevenue = ChangeData.NewValue;
    if (CachedNightShiftGameState.IsValid())
    {
        UpdateCycleRevenue(CycleRevenue - CachedNightShiftGameState->GetCurrentCycleStartRevenue());
    }
    else
    {
        UpdateCycleRevenue(CycleRevenue);
    }
}


void UDEPRECATEDEMRNightShiftStatusWidget::OnReputationChanged(const FOnAttributeChangeData& ChangeData)
{
    UpdateReputation(ChangeData.NewValue);
}
