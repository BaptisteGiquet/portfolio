#include "UI/AGASSSessionBrowserScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Settings/AGASSFrontendDeveloperSettings.h"
#include "Subsystems/AGASSInviteCodeSubsystem.h"
#include "Subsystems/AGASSSessionSubsystem.h"
#include "UI/AGASSFrontendButtonBase.h"
#include "UI/AGASSSessionBrowserEntryWidget.h"

UAGASSSessionBrowserScreenWidget::UAGASSSessionBrowserScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
	OverrideBackActionDisplayName = NSLOCTEXT("AGASSFrontend", "FrontendBackActionName", "Back");
}

void UAGASSSessionBrowserScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveWidgetByName(Button_Refresh, TEXT("Button_Refresh"));
	ResolveWidgetByName(Button_JoinInviteCode, TEXT("Button_JoinInviteCode"));
	ResolveWidgetByName(TextBox_InviteCode, TEXT("TextBox_InviteCode"));
	ResolveWidgetByName(Scroll_Results, TEXT("Scroll_Results"));
	ResolveWidgetByName(Text_ResultsState, TEXT("Text_ResultsState"));

	if (Button_Refresh != nullptr)
	{
		Button_Refresh->OnClicked().RemoveAll(this);
		Button_Refresh->OnClicked().AddUObject(this, &ThisClass::HandleRefreshClicked);
	}

	if (Button_JoinInviteCode != nullptr)
	{
		Button_JoinInviteCode->OnClicked().RemoveAll(this);
		Button_JoinInviteCode->OnClicked().AddUObject(this, &ThisClass::HandleJoinInviteCodeClicked);
	}

	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SearchResultsChangedHandle = SessionSubsystem->OnSearchResultsChanged().AddUObject(this, &ThisClass::HandleSearchResultsChanged);

		if (SessionSubsystem->GetSearchResults().IsEmpty() && SessionSubsystem->GetSessionFlowState() != EAGASSSessionFlowState::Searching)
		{
			SessionSubsystem->FindSessions();
		}
	}

	RebuildSessionList();
	RefreshEntryPoints();
}

void UAGASSSessionBrowserScreenWidget::NativeDestruct()
{
	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		if (SearchResultsChangedHandle.IsValid())
		{
			SessionSubsystem->OnSearchResultsChanged().Remove(SearchResultsChangedHandle);
			SearchResultsChangedHandle.Reset();
		}
	}

	Super::NativeDestruct();
}

FText UAGASSSessionBrowserScreenWidget::GetTitleText() const
{
	return NSLOCTEXT("AGASSFrontend", "SessionBrowserTitle", "Join Session");
}

void UAGASSSessionBrowserScreenWidget::BuildFallbackScreenContent(UVerticalBox* ContentBox)
{
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "SessionBrowserBody", "Browse active AGASS sessions or join directly by invite code. All entry points still validate the selected map and active mod set before travel."), 16);

	UHorizontalBox* InviteCodeRow = AddHorizontalBox(ContentBox, TEXT("Row_InviteCode"));
	TextBox_InviteCode = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("TextBox_InviteCode"));
	TextBox_InviteCode->SetHintText(NSLOCTEXT("AGASSFrontend", "InviteCodeHint", "Enter invite code"));
	if (UHorizontalBoxSlot* InviteCodeSlot = InviteCodeRow->AddChildToHorizontalBox(TextBox_InviteCode))
	{
		InviteCodeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		InviteCodeSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	Button_JoinInviteCode = CreateMenuButton(NSLOCTEXT("AGASSFrontend", "JoinInviteCodeButton", "Join By Code"), TEXT("Button_JoinInviteCode"));
	InviteCodeRow->AddChildToHorizontalBox(Button_JoinInviteCode);

	Button_Refresh = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "RefreshSessionsButton", "Refresh"), TEXT("Button_Refresh"));
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "AvailableSessionsHeading", "Active Sessions"), 18);
	Scroll_Results = AddScrollBox(ContentBox, 340.f, TEXT("Scroll_Results"));
}

void UAGASSSessionBrowserScreenWidget::RefreshFromSessionState()
{
	Super::RefreshFromSessionState();
	if (Text_Status != nullptr)
	{
		Text_Status->SetVisibility(ESlateVisibility::Collapsed);
	}

	RebuildSessionList();
	RefreshEntryPoints();
}

void UAGASSSessionBrowserScreenWidget::RebuildSessionList()
{
	if (Scroll_Results == nullptr)
	{
		return;
	}

	Scroll_Results->ClearChildren();
	if (Text_ResultsState != nullptr)
	{
		Text_ResultsState->SetVisibility(ESlateVisibility::Collapsed);
	}
	Scroll_Results->SetVisibility(ESlateVisibility::Visible);

	UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (!SessionSubsystem)
	{
		return;
	}

	if (SessionSubsystem->GetSessionFlowState() == EAGASSSessionFlowState::Searching)
	{
		Scroll_Results->SetVisibility(ESlateVisibility::Collapsed);
		if (Text_ResultsState != nullptr)
		{
			Text_ResultsState->SetText(NSLOCTEXT("AGASSFrontend", "SearchingText", "Searching for advertised sessions..."));
			Text_ResultsState->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	const TArray<FAGASSSessionSearchResultData>& SessionResults = SessionSubsystem->GetSearchResults();
	if (SessionResults.IsEmpty())
	{
		Scroll_Results->SetVisibility(ESlateVisibility::Collapsed);
		if (Text_ResultsState != nullptr)
		{
			Text_ResultsState->SetText(NSLOCTEXT("AGASSFrontend", "NoSessionsText", "No active sessions found yet.\nHost a session or refresh to search again."));
			Text_ResultsState->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	const UAGASSFrontendDeveloperSettings* FrontendSettings = UAGASSFrontendDeveloperSettings::Get();
	TSubclassOf<UAGASSSessionBrowserEntryWidget> EntryWidgetClass = FrontendSettings ? FrontendSettings->SessionBrowserEntryWidgetClass.LoadSynchronous() : nullptr;
	if (EntryWidgetClass == nullptr)
	{
		EntryWidgetClass = UAGASSSessionBrowserEntryWidget::StaticClass();
	}

	for (int32 ResultIndex = 0; ResultIndex < SessionResults.Num(); ++ResultIndex)
	{
		UAGASSSessionBrowserEntryWidget* EntryWidget = CreateWidget<UAGASSSessionBrowserEntryWidget>(GetOwningPlayer(), EntryWidgetClass);
		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->InitializeFromResult(ResultIndex, SessionResults[ResultIndex]);
		Scroll_Results->AddChild(EntryWidget);
	}
}

UWidget* UAGASSSessionBrowserScreenWidget::ResolveInitialFocusTarget() const
{
	if (Scroll_Results != nullptr)
	{
		if (const UAGASSSessionBrowserEntryWidget* FirstEntry = Cast<UAGASSSessionBrowserEntryWidget>(Scroll_Results->GetChildAt(0)))
		{
			return FirstEntry->GetPrimaryFocusTarget();
		}
	}

	if (TextBox_InviteCode != nullptr)
	{
		return TextBox_InviteCode;
	}

	if (Button_Refresh != nullptr)
	{
		return Button_Refresh;
	}

	return nullptr;
}

#if WITH_EDITOR
void UAGASSSessionBrowserScreenWidget::GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const
{
	OutRequiredWidgets.Append({
		TEXT("Button_Refresh"),
		TEXT("Button_JoinInviteCode"),
		TEXT("TextBox_InviteCode"),
		TEXT("Scroll_Results"),
		TEXT("Text_ResultsState")
	});
}
#endif

void UAGASSSessionBrowserScreenWidget::HandleSearchResultsChanged()
{
	RebuildSessionList();
}

void UAGASSSessionBrowserScreenWidget::RefreshEntryPoints()
{
}

void UAGASSSessionBrowserScreenWidget::HandleRefreshClicked()
{
	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SessionSubsystem->FindSessions();
	}
}

void UAGASSSessionBrowserScreenWidget::HandleJoinInviteCodeClicked()
{
	if (UAGASSInviteCodeSubsystem* InviteCodeSubsystem = GetInviteCodeSubsystem())
	{
		InviteCodeSubsystem->JoinByInviteCode(TextBox_InviteCode != nullptr ? TextBox_InviteCode->GetText().ToString() : FString());
	}
}
