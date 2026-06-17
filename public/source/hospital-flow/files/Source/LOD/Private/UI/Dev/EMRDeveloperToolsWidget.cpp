#include "UI/Dev/EMRDeveloperToolsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Characters/Player/EMRPlayerController.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

namespace
{
    UButton* CreateLabeledButton(UWidgetTree& WidgetTree, UVerticalBox& Parent, const TCHAR* ButtonName, const TCHAR* Label)
    {
        UButton* Button = WidgetTree.ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
        if (!Button)
        {
            return nullptr;
        }

        UTextBlock* Text = WidgetTree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (Text)
        {
            Text->SetText(FText::FromString(Label));
            Button->SetContent(Text);
        }

        Parent.AddChildToVerticalBox(Button);
        return Button;
    }
}

void UEMRDeveloperToolsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        EnsureWidgetHierarchy();
    }
    BindNativeWidgetEvents();

    if (CachedOwningEMRPlayerController.IsValid())
    {
        CachedOwningEMRPlayerController->Server_RequestDeveloperToolUpgradeOptions();
    }
}

void UEMRDeveloperToolsWidget::NativeDestruct()
{
    UnbindNativeWidgetEvents();
    CachedOwningEMRPlayerController.Reset();
    Super::NativeDestruct();
}

void UEMRDeveloperToolsWidget::InitializeForController(AEMRPlayerController* InOwningController)
{
    CachedOwningEMRPlayerController = InOwningController;

    if (CachedOwningEMRPlayerController.IsValid())
    {
        SetStatusMessage(TEXT("Refreshing upgrade list..."), true);
        CachedOwningEMRPlayerController->Server_RequestDeveloperToolUpgradeOptions();
    }
}

void UEMRDeveloperToolsWidget::ApplyActionResult(const FEMRDeveloperToolActionResult& Result)
{
    SetStatusMessage(Result.Message, Result.bSuccess);
    BP_OnDeveloperToolActionResultReceived(Result);
}

void UEMRDeveloperToolsWidget::ApplyUpgradeOptions(const TArray<FEMRDeveloperToolUpgradeOption>& InOptions)
{
    UpgradeOptions = InOptions;
    RebuildUpgradeCombo();
    BP_OnDeveloperToolUpgradeOptionsUpdated();

    SetStatusMessage(
        FString::Printf(TEXT("Loaded %d upgrade option(s)."), UpgradeOptions.Num()),
        true);
}

void UEMRDeveloperToolsWidget::RequestDeveloperToolAction(const EEMRDeveloperToolAction Action)
{
    RequestAction(Action);
}

void UEMRDeveloperToolsWidget::RequestDeveloperToolUpgradeList()
{
    HandleRefreshUpgradeListClicked();
}

void UEMRDeveloperToolsWidget::RequestGiveSpecificUpgradeByIndex(const int32 UpgradeIndex)
{
    if (!UpgradeOptions.IsValidIndex(UpgradeIndex))
    {
        SetStatusMessage(TEXT("Invalid upgrade index."), false);
        return;
    }

    RequestAction(EEMRDeveloperToolAction::GiveSpecificUpgrade, UpgradeOptions[UpgradeIndex].UpgradeTag);
}

void UEMRDeveloperToolsWidget::EnsureWidgetHierarchy()
{
    if (!WidgetTree)
    {
        return;
    }

    if (RootScrollBox && RootVerticalBox && StatusText && UpgradeComboBox)
    {
        return;
    }

    if (!RootScrollBox)
    {
        RootScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DevToolsScrollBox"));
    }

    if (!RootVerticalBox)
    {
        RootVerticalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DevToolsRootVerticalBox"));
    }

    if (!StatusText)
    {
        StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DevToolsStatusText"));
        if (StatusText)
        {
            StatusText->SetAutoWrapText(true);
            StatusText->SetText(FText::FromString(TEXT("Developer tools ready.")));
        }
    }

    if (!UpgradeComboBox)
    {
        UpgradeComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("DevToolsUpgradeComboBox"));
    }

    if (RootVerticalBox && RootVerticalBox->GetChildrenCount() == 0)
    {
        if (StatusText)
        {
            RootVerticalBox->AddChildToVerticalBox(StatusText);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonSpawnPatient"), TEXT("Spawn patient")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleSpawnPatientClicked);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonWinNightshift"), TEXT("Win the Nightshift")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleWinNightShiftClicked);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonLoseNightshift"), TEXT("Lose the Nightshift")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleLoseNightShiftClicked);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonEarnRevenue"), TEXT("Earn 1000 TotalRevenue")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleEarnRevenueClicked);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonEarnReputation"), TEXT("Earn 10 Reputation")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleEarnReputationClicked);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonLoseReputation"), TEXT("Lose 10 Reputation")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleLoseReputationClicked);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonSpeedUp"), TEXT("Speed up the game x2")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleSpeedUpClicked);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonSlowDown"), TEXT("Slow down the game x2")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleSlowDownClicked);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonRerollUpgrades"), TEXT("Reroll the upgrades in Hub")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleRerollUpgradesClicked);
        }

        if (UpgradeComboBox)
        {
            RootVerticalBox->AddChildToVerticalBox(UpgradeComboBox);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonRefreshUpgrades"), TEXT("Refresh upgrade list")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleRefreshUpgradeListClicked);
        }

        if (UButton* Button = CreateLabeledButton(*WidgetTree, *RootVerticalBox, TEXT("ButtonGiveSpecificUpgrade"), TEXT("Give selected upgrade")))
        {
            Button->OnClicked.AddDynamic(this, &ThisClass::HandleGiveSpecificUpgradeClicked);
        }
    }

    if (RootScrollBox && RootVerticalBox)
    {
        if (RootScrollBox->GetChildrenCount() == 0)
        {
            RootScrollBox->AddChild(RootVerticalBox);
        }

        if (!WidgetTree->RootWidget)
        {
            WidgetTree->RootWidget = RootScrollBox;
        }
    }
}

void UEMRDeveloperToolsWidget::BindNativeWidgetEvents()
{
    if (Button_SpawnPatient)
    {
        Button_SpawnPatient->OnClicked.RemoveDynamic(this, &ThisClass::HandleSpawnPatientClicked);
        Button_SpawnPatient->OnClicked.AddDynamic(this, &ThisClass::HandleSpawnPatientClicked);
    }

    if (Button_WinNightShift)
    {
        Button_WinNightShift->OnClicked.RemoveDynamic(this, &ThisClass::HandleWinNightShiftClicked);
        Button_WinNightShift->OnClicked.AddDynamic(this, &ThisClass::HandleWinNightShiftClicked);
    }

    if (Button_LoseNightShift)
    {
        Button_LoseNightShift->OnClicked.RemoveDynamic(this, &ThisClass::HandleLoseNightShiftClicked);
        Button_LoseNightShift->OnClicked.AddDynamic(this, &ThisClass::HandleLoseNightShiftClicked);
    }

    if (Button_EarnRevenue1000)
    {
        Button_EarnRevenue1000->OnClicked.RemoveDynamic(this, &ThisClass::HandleEarnRevenueClicked);
        Button_EarnRevenue1000->OnClicked.AddDynamic(this, &ThisClass::HandleEarnRevenueClicked);
    }

    if (Button_EarnReputation10)
    {
        Button_EarnReputation10->OnClicked.RemoveDynamic(this, &ThisClass::HandleEarnReputationClicked);
        Button_EarnReputation10->OnClicked.AddDynamic(this, &ThisClass::HandleEarnReputationClicked);
    }

    if (Button_LoseReputation10)
    {
        Button_LoseReputation10->OnClicked.RemoveDynamic(this, &ThisClass::HandleLoseReputationClicked);
        Button_LoseReputation10->OnClicked.AddDynamic(this, &ThisClass::HandleLoseReputationClicked);
    }

    if (Button_SpeedUpX2)
    {
        Button_SpeedUpX2->OnClicked.RemoveDynamic(this, &ThisClass::HandleSpeedUpClicked);
        Button_SpeedUpX2->OnClicked.AddDynamic(this, &ThisClass::HandleSpeedUpClicked);
    }

    if (Button_SlowDownX2)
    {
        Button_SlowDownX2->OnClicked.RemoveDynamic(this, &ThisClass::HandleSlowDownClicked);
        Button_SlowDownX2->OnClicked.AddDynamic(this, &ThisClass::HandleSlowDownClicked);
    }

    if (Button_RerollHubUpgrades)
    {
        Button_RerollHubUpgrades->OnClicked.RemoveDynamic(this, &ThisClass::HandleRerollUpgradesClicked);
        Button_RerollHubUpgrades->OnClicked.AddDynamic(this, &ThisClass::HandleRerollUpgradesClicked);
    }

    if (Button_RefreshUpgradeList)
    {
        Button_RefreshUpgradeList->OnClicked.RemoveDynamic(this, &ThisClass::HandleRefreshUpgradeListClicked);
        Button_RefreshUpgradeList->OnClicked.AddDynamic(this, &ThisClass::HandleRefreshUpgradeListClicked);
    }

    if (Button_GiveSpecificUpgrade)
    {
        Button_GiveSpecificUpgrade->OnClicked.RemoveDynamic(this, &ThisClass::HandleGiveSpecificUpgradeClicked);
        Button_GiveSpecificUpgrade->OnClicked.AddDynamic(this, &ThisClass::HandleGiveSpecificUpgradeClicked);
    }
}

void UEMRDeveloperToolsWidget::UnbindNativeWidgetEvents()
{
    if (Button_SpawnPatient)
    {
        Button_SpawnPatient->OnClicked.RemoveDynamic(this, &ThisClass::HandleSpawnPatientClicked);
    }

    if (Button_WinNightShift)
    {
        Button_WinNightShift->OnClicked.RemoveDynamic(this, &ThisClass::HandleWinNightShiftClicked);
    }

    if (Button_LoseNightShift)
    {
        Button_LoseNightShift->OnClicked.RemoveDynamic(this, &ThisClass::HandleLoseNightShiftClicked);
    }

    if (Button_EarnRevenue1000)
    {
        Button_EarnRevenue1000->OnClicked.RemoveDynamic(this, &ThisClass::HandleEarnRevenueClicked);
    }

    if (Button_EarnReputation10)
    {
        Button_EarnReputation10->OnClicked.RemoveDynamic(this, &ThisClass::HandleEarnReputationClicked);
    }

    if (Button_LoseReputation10)
    {
        Button_LoseReputation10->OnClicked.RemoveDynamic(this, &ThisClass::HandleLoseReputationClicked);
    }

    if (Button_SpeedUpX2)
    {
        Button_SpeedUpX2->OnClicked.RemoveDynamic(this, &ThisClass::HandleSpeedUpClicked);
    }

    if (Button_SlowDownX2)
    {
        Button_SlowDownX2->OnClicked.RemoveDynamic(this, &ThisClass::HandleSlowDownClicked);
    }

    if (Button_RerollHubUpgrades)
    {
        Button_RerollHubUpgrades->OnClicked.RemoveDynamic(this, &ThisClass::HandleRerollUpgradesClicked);
    }

    if (Button_RefreshUpgradeList)
    {
        Button_RefreshUpgradeList->OnClicked.RemoveDynamic(this, &ThisClass::HandleRefreshUpgradeListClicked);
    }

    if (Button_GiveSpecificUpgrade)
    {
        Button_GiveSpecificUpgrade->OnClicked.RemoveDynamic(this, &ThisClass::HandleGiveSpecificUpgradeClicked);
    }
}

void UEMRDeveloperToolsWidget::RebuildUpgradeCombo()
{
    if (!UpgradeComboBox)
    {
        return;
    }

    UpgradeComboBox->ClearOptions();

    for (const FEMRDeveloperToolUpgradeOption& Option : UpgradeOptions)
    {
        UpgradeComboBox->AddOption(Option.DisplayLabel);
    }

    if (UpgradeOptions.Num() > 0)
    {
        UpgradeComboBox->SetSelectedIndex(0);
    }
}

void UEMRDeveloperToolsWidget::RequestAction(const EEMRDeveloperToolAction Action, const FGameplayTag OptionalUpgradeTag)
{
    AEMRPlayerController* OwningController = CachedOwningEMRPlayerController.Get();
    if (!OwningController)
    {
        SetStatusMessage(TEXT("Missing player controller."), false);
        return;
    }

    FEMRDeveloperToolActionRequest Request;
    Request.Action = Action;
    Request.UpgradeTag = OptionalUpgradeTag;

    OwningController->Server_RequestDeveloperToolAction(Request);
}

void UEMRDeveloperToolsWidget::SetStatusMessage(const FString& InMessage, const bool bWasSuccess)
{
    if (!StatusText)
    {
        return;
    }

    const FString Prefix = bWasSuccess ? TEXT("[OK] ") : TEXT("[ERR] ");
    StatusText->SetText(FText::FromString(Prefix + InMessage));
}

void UEMRDeveloperToolsWidget::HandleSpawnPatientClicked()
{
    RequestAction(EEMRDeveloperToolAction::SpawnPatient);
}

void UEMRDeveloperToolsWidget::HandleWinNightShiftClicked()
{
    RequestAction(EEMRDeveloperToolAction::WinNightShift);
}

void UEMRDeveloperToolsWidget::HandleLoseNightShiftClicked()
{
    RequestAction(EEMRDeveloperToolAction::LoseNightShift);
}

void UEMRDeveloperToolsWidget::HandleEarnRevenueClicked()
{
    RequestAction(EEMRDeveloperToolAction::EarnRevenue1000);
}

void UEMRDeveloperToolsWidget::HandleEarnReputationClicked()
{
    RequestAction(EEMRDeveloperToolAction::EarnReputation10);
}

void UEMRDeveloperToolsWidget::HandleLoseReputationClicked()
{
    RequestAction(EEMRDeveloperToolAction::LoseReputation10);
}

void UEMRDeveloperToolsWidget::HandleSpeedUpClicked()
{
    RequestAction(EEMRDeveloperToolAction::SpeedUpGameX2);
}

void UEMRDeveloperToolsWidget::HandleSlowDownClicked()
{
    RequestAction(EEMRDeveloperToolAction::SlowDownGameX2);
}

void UEMRDeveloperToolsWidget::HandleRerollUpgradesClicked()
{
    RequestAction(EEMRDeveloperToolAction::RerollHubUpgrades);
}

void UEMRDeveloperToolsWidget::HandleGiveSpecificUpgradeClicked()
{
    if (!UpgradeComboBox)
    {
        SetStatusMessage(TEXT("Upgrade selector is not available."), false);
        return;
    }

    const FString SelectedLabel = UpgradeComboBox->GetSelectedOption();
    if (SelectedLabel.IsEmpty())
    {
        SetStatusMessage(TEXT("Select an upgrade first."), false);
        return;
    }

    for (const FEMRDeveloperToolUpgradeOption& Option : UpgradeOptions)
    {
        if (Option.DisplayLabel == SelectedLabel)
        {
            RequestAction(EEMRDeveloperToolAction::GiveSpecificUpgrade, Option.UpgradeTag);
            return;
        }
    }

    SetStatusMessage(TEXT("Selected upgrade is stale. Refresh the list."), false);
}

void UEMRDeveloperToolsWidget::HandleRefreshUpgradeListClicked()
{
    AEMRPlayerController* OwningController = CachedOwningEMRPlayerController.Get();
    if (!OwningController)
    {
        SetStatusMessage(TEXT("Missing player controller."), false);
        return;
    }

    SetStatusMessage(TEXT("Refreshing upgrade list..."), true);
    OwningController->Server_RequestDeveloperToolUpgradeOptions();
}
