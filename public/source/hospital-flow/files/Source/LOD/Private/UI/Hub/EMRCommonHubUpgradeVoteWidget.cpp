#include "UI/Hub/EMRCommonHubUpgradeVoteWidget.h"

#include "CommonTextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Data/EMRRunUpgradeDefinition.h"
#include "Framework/EMRNightShiftGameState.h"
#include "TimerManager.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"
#include "UI/Frontend/Core/EMRFrontendCommonListView.h"
#include "UI/Hub/EMRCommonHubUpgradeListDataObject.h"
#include "Characters/Player/EMRPlayerController.h"

namespace
{
    constexpr TCHAR HubUpgradeVoteWidgetUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");
}

void UEMRCommonHubUpgradeVoteWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (CommonListView_Upgrades)
    {
        CommonListView_Upgrades->OnItemIsHoveredChanged().AddUObject(this, &ThisClass::OnListViewItemHoveredChanged);
    }
}

void UEMRCommonHubUpgradeVoteWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (CommonListView_Upgrades)
    {
        CommonListView_Upgrades->OnItemSelectionChanged().RemoveAll(this);
        CommonListView_Upgrades->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
    }

    if (CommonButton_Start)
    {
        CommonButton_Start->OnClicked().RemoveAll(this);
        CommonButton_Start->OnClicked().AddUObject(this, &ThisClass::HandleStartClicked);
    }

    RefreshUpgradeOffers();
    BindToGameStateUpdates();
}

void UEMRCommonHubUpgradeVoteWidget::NativeDestruct()
{
    if (CommonListView_Upgrades)
    {
        CommonListView_Upgrades->OnItemSelectionChanged().RemoveAll(this);
    }

    if (CommonButton_Start)
    {
        CommonButton_Start->OnClicked().RemoveAll(this);
    }

    UnbindFromGameStateUpdates();
    UpgradeEntries.Reset();

    Super::NativeDestruct();
}

void UEMRCommonHubUpgradeVoteWidget::NativeOnActivated()
{
    Super::NativeOnActivated();
    RefreshUpgradeOffers();
}

void UEMRCommonHubUpgradeVoteWidget::NativeOnDeactivated()
{
    Super::NativeOnDeactivated();
}

UWidget* UEMRCommonHubUpgradeVoteWidget::NativeGetDesiredFocusTarget() const
{
    if (GetPreferredFocusWidget_Implementation())
    {
        return GetPreferredFocusWidget_Implementation();
    }

    return Super::NativeGetDesiredFocusTarget();
}

UWidget* UEMRCommonHubUpgradeVoteWidget::GetPreferredFocusWidget_Implementation() const
{
    if (!CommonListView_Upgrades)
    {
        return nullptr;
    }

    if (CommonListView_Upgrades->GetNumItems() > 0)
    {
        if (UObject* Item = CommonListView_Upgrades->GetItemAt(0))
        {
            if (UUserWidget* EntryWidget = CommonListView_Upgrades->GetEntryWidgetFromItem(Item))
            {
                return EntryWidget;
            }
        }
    }

    return CommonListView_Upgrades;
}

void UEMRCommonHubUpgradeVoteWidget::RefreshUpgradeOffers()
{
    if (!CommonListView_Upgrades)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s UpgradeVoteWidget refresh skipped: list view missing."), HubUpgradeVoteWidgetUpgradesFlowLogPrefix);
        return;
    }

    CommonListView_Upgrades->ClearListItems();
    UpgradeEntries.Reset();

    const AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.IsValid()
    ? CachedNightShiftGameState.Get()
    : ResolveNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s UpgradeVoteWidget refresh skipped: NightShiftGameState missing."), HubUpgradeVoteWidgetUpgradesFlowLogPrefix);
        UpdateCountdownUI();
        return;
    }

    const FEMRHubUpgradeVoteState& VoteState = RunGS->GetHubUpgradeVoteState();
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s UpgradeVoteWidget refreshing offers OfferCount=%d VoteCountEntries=%d VoteActive=%d WinningIndex=%d"),
        HubUpgradeVoteWidgetUpgradesFlowLogPrefix,
        VoteState.OfferedUpgrades.Num(),
        VoteState.VoteCounts.Num(),
        VoteState.bVoteActive ? 1 : 0,
        VoteState.WinningOfferIndex);
    for (int32 OfferIndex = 0; OfferIndex < VoteState.OfferedUpgrades.Num(); ++OfferIndex)
    {
        UEMRRunUpgradeDefinition* UpgradeDefinition = VoteState.OfferedUpgrades[OfferIndex];
        if (!UpgradeDefinition)
        {
            continue;
        }

        const int32 VoteCount = VoteState.VoteCounts.IsValidIndex(OfferIndex) ? VoteState.VoteCounts[OfferIndex] : 0;
        UEMRCommonHubUpgradeListDataObject* Entry = NewObject<UEMRCommonHubUpgradeListDataObject>(this);
        Entry->Init(UpgradeDefinition, OfferIndex, VoteCount);
        UpgradeEntries.Add(Entry);
        CommonListView_Upgrades->AddItem(Entry);
    }

    for (UEMRCommonHubUpgradeListDataObject* Entry : UpgradeEntries)
    {
        if (Entry && Entry->GetOfferIndex() == LocalSelectedOfferIndex)
        {
            CommonListView_Upgrades->SetSelectedItem(Entry);
            break;
        }
    }

    UpdateCountdownUI();
}

void UEMRCommonHubUpgradeVoteWidget::BindToGameStateUpdates()
{
    UnbindFromGameStateUpdates();

    AEMRNightShiftGameState* RunGS = ResolveNightShiftGameState();
    if (!RunGS)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s UpgradeVoteWidget bind retry: NightShiftGameState not ready."), HubUpgradeVoteWidgetUpgradesFlowLogPrefix);
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(GameStateBindingRetryHandle, this, &ThisClass::BindToGameStateUpdates, 0.2f, false);
        }
        return;
    }

    CachedNightShiftGameState = RunGS;
    VoteStateChangedHandle = RunGS->OnHubUpgradeVoteStateChanged().AddUObject(this, &ThisClass::OnGameStateVoteStateChanged);
    UE_LOG(LogTemp, Warning, TEXT("%s UpgradeVoteWidget bound to game state updates."), HubUpgradeVoteWidgetUpgradesFlowLogPrefix);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(CountdownRefreshHandle, this, &ThisClass::TickCountdown, 0.1f, true);
    }

    OnGameStateVoteStateChanged();
}

void UEMRCommonHubUpgradeVoteWidget::UnbindFromGameStateUpdates()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(GameStateBindingRetryHandle);
        World->GetTimerManager().ClearTimer(CountdownRefreshHandle);
    }

    if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
    {
        if (VoteStateChangedHandle.IsValid())
        {
            RunGS->OnHubUpgradeVoteStateChanged().Remove(VoteStateChangedHandle);
        }
    }

    VoteStateChangedHandle.Reset();
    CachedNightShiftGameState.Reset();
}

AEMRNightShiftGameState* UEMRCommonHubUpgradeVoteWidget::ResolveNightShiftGameState() const
{
    UWorld* World = GetWorld();
    return World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;
}

void UEMRCommonHubUpgradeVoteWidget::HandleSelectionChanged(UObject* SelectedItem)
{
    UEMRCommonHubUpgradeListDataObject* SelectedData = Cast<UEMRCommonHubUpgradeListDataObject>(SelectedItem);
    if (!SelectedData)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s UpgradeVoteWidget selection ignored: selected item data invalid."), HubUpgradeVoteWidgetUpgradesFlowLogPrefix);
        return;
    }

    LocalSelectedOfferIndex = SelectedData->GetOfferIndex();
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s UpgradeVoteWidget local selection changed OfferIndex=%d"),
        HubUpgradeVoteWidgetUpgradesFlowLogPrefix,
        LocalSelectedOfferIndex);

    if (AEMRPlayerController* OwningPC = GetOwningPlayer<AEMRPlayerController>())
    {
        OwningPC->Server_VoteForHubUpgradeByIndex(LocalSelectedOfferIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s UpgradeVoteWidget selection could not send vote: owning player controller missing."), HubUpgradeVoteWidgetUpgradesFlowLogPrefix);
    }
}

void UEMRCommonHubUpgradeVoteWidget::HandleStartClicked()
{
    if (AEMRPlayerController* OwningPC = GetOwningPlayer<AEMRPlayerController>())
    {
        OwningPC->Server_RequestStartHubUpgradeVoteNow();
    }
}

void UEMRCommonHubUpgradeVoteWidget::OnListViewItemHoveredChanged(UObject* InHoveredItem, bool bWasHovered)
{
    if (!InHoveredItem || !CommonListView_Upgrades)
    {
        return;
    }

    UEMRFrontendCommonListEntryWidgetBase* HoveredEntryWidget = CommonListView_Upgrades->GetEntryWidgetFromItem<UEMRFrontendCommonListEntryWidgetBase>(InHoveredItem);
    if (HoveredEntryWidget)
    {
        HoveredEntryWidget->NativeOnListEntryWidgetHovered(bWasHovered);
    }
}

void UEMRCommonHubUpgradeVoteWidget::OnGameStateVoteStateChanged()
{
    UE_LOG(LogTemp, Warning, TEXT("%s UpgradeVoteWidget received replicated vote-state change."), HubUpgradeVoteWidgetUpgradesFlowLogPrefix);
    RefreshUpgradeOffers();
}

void UEMRCommonHubUpgradeVoteWidget::TickCountdown()
{
    UpdateCountdownUI();
}

void UEMRCommonHubUpgradeVoteWidget::UpdateCountdownUI()
{
    const AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.IsValid()
    ? CachedNightShiftGameState.Get()
    : ResolveNightShiftGameState();
    if (!RunGS)
    {
        UpdateStartButtonState();
        return;
    }

    const FEMRHubUpgradeVoteState& VoteState = RunGS->GetHubUpgradeVoteState();
    const float RemainingSeconds = FMath::Max(0.0f, VoteState.VoteEndServerTimeSeconds - RunGS->GetServerWorldTimeSeconds());
    const int32 RemainingSecondsRounded = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));

    if (CommonTextBlock_Countdown)
    {
        CommonTextBlock_Countdown->SetText(FText::FromString(FString::Printf(TEXT("%ds"), RemainingSecondsRounded)));
    }

    if (CommonTextBlock_Status)
    {
        if (VoteState.bVoteActive)
        {
            CommonTextBlock_Status->SetText(FText::FromString(TEXT("Vote in progress")));
        }
        else if (VoteState.OfferedUpgrades.IsValidIndex(VoteState.WinningOfferIndex))
        {
            const UEMRRunUpgradeDefinition* WinningUpgrade = VoteState.OfferedUpgrades[VoteState.WinningOfferIndex];
            const FString WinningName = WinningUpgrade && !WinningUpgrade->DisplayName.IsEmpty()
                ? WinningUpgrade->DisplayName.ToString()
                : TEXT("Upgrade");
            CommonTextBlock_Status->SetText(FText::FromString(FString::Printf(TEXT("Winner: %s"), *WinningName)));
        }
        else
        {
            CommonTextBlock_Status->SetText(FText::FromString(TEXT("Waiting...")));
        }
    }

    BP_OnVoteStateChanged(VoteState.bVoteActive, RemainingSeconds, VoteState.WinningOfferIndex);
    UpdateStartButtonState();
}

void UEMRCommonHubUpgradeVoteWidget::UpdateStartButtonState()
{
    bool bCanStartNow = false;
    int32 SubmittedVoterCount = 0;
    int32 RequiredVoterCount = 0;

    const AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.IsValid()
    ? CachedNightShiftGameState.Get()
    : ResolveNightShiftGameState();
    if (RunGS)
    {
        const FEMRHubUpgradeVoteState& VoteState = RunGS->GetHubUpgradeVoteState();
        bCanStartNow = VoteState.bVoteActive && VoteState.bAllConnectedPlayersVoted;
        SubmittedVoterCount = VoteState.SubmittedVoterCount;
        RequiredVoterCount = VoteState.RequiredVoterCount;
    }

    if (CommonButton_Start)
    {
        CommonButton_Start->SetIsEnabled(bCanStartNow);
    }

    BP_OnStartAvailabilityChanged(bCanStartNow, SubmittedVoterCount, RequiredVoterCount);
}
