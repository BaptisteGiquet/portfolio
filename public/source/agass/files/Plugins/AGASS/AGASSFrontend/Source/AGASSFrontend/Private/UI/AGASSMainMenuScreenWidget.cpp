#include "UI/AGASSMainMenuScreenWidget.h"

#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Settings/AGASSFrontendDeveloperSettings.h"
#include "Settings/AGASSSettingsLocal.h"
#include "Subsystems/AGASSModsSubsystem.h"
#include "Subsystems/AGASSSessionSubsystem.h"
#include "Subsystems/AGASSSteamPlatformSubsystem.h"
#include "Subsystems/AGASSUIManagerSubsystem.h"
#include "UI/AGASSFriendTimedRunEntryWidget.h"
#include "UI/AGASSFrontendButtonBase.h"

namespace
{
	FText FormatTimedRunMilliseconds(const int32 Milliseconds)
	{
		const int32 ClampedMilliseconds = FMath::Max(Milliseconds, 0);
		const int32 Minutes = ClampedMilliseconds / 60000;
		const int32 Seconds = (ClampedMilliseconds / 1000) % 60;
		const int32 RemainingMilliseconds = ClampedMilliseconds % 1000;
		return FText::FromString(FString::Printf(TEXT("%d:%02d.%03d"), Minutes, Seconds, RemainingMilliseconds));
	}

	FText BuildActiveModsSummary(const TArray<FString>& ActiveModIds)
	{
		TArray<FString> FilteredIds;
		for (const FString& ActiveModId : ActiveModIds)
		{
			const FString TrimmedId = ActiveModId.TrimStartAndEnd();
			if (!TrimmedId.IsEmpty())
			{
				FilteredIds.Add(TrimmedId);
			}
		}

		if (FilteredIds.IsEmpty())
		{
			return NSLOCTEXT("AGASSFrontend", "ActiveModsNone", "No active mods");
		}

		if (FilteredIds.Num() == 1)
		{
			return FText::FromString(FilteredIds[0]);
		}

		if (FilteredIds.Num() == 2)
		{
			return FText::FromString(FString::Printf(TEXT("%s, %s"), *FilteredIds[0], *FilteredIds[1]));
		}

		return FText::Format(
			NSLOCTEXT("AGASSFrontend", "MainMenuActiveModsSummaryCompact", "{0}, {1}, +{2}"),
			FText::FromString(FilteredIds[0]),
			FText::FromString(FilteredIds[1]),
			FText::AsNumber(FilteredIds.Num() - 2));
	}
}

FText UAGASSMainMenuScreenWidget::GetTitleText() const
{
	return NSLOCTEXT("AGASSFrontend", "MainMenuTitle", "AGASS");
}

void UAGASSMainMenuScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveWidgetByName(Button_Host, TEXT("Button_Host"));
	ResolveWidgetByName(Button_JoinSession, TEXT("Button_JoinSession"));
	ResolveWidgetByName(Button_ModsAndMaps, TEXT("Button_ModsAndMaps"));
	ResolveWidgetByName(Button_Options, TEXT("Button_Options"));
	ResolveWidgetByName(Button_Credits, TEXT("Button_Credits"));
	ResolveWidgetByName(Button_Quit, TEXT("Button_Quit"));
	ResolveWidgetByName(Text_SelectedMapSummary, TEXT("Text_SelectedMapSummary"));
	ResolveWidgetByName(Text_ActiveModsSummary, TEXT("Text_ActiveModsSummary"));
	ResolveWidgetByName(Text_PersonalBest, TEXT("Text_PersonalBest"));
	ResolveWidgetByName(Text_FriendTimesStatus, TEXT("Text_FriendTimesStatus"));
	ResolveWidgetByName(Scroll_FriendTimes, TEXT("Scroll_FriendTimes"));

	if (Button_Host != nullptr)
	{
		Button_Host->OnClicked().RemoveAll(this);
		Button_Host->OnClicked().AddUObject(this, &ThisClass::HandleHostClicked);
	}

	if (Button_JoinSession != nullptr)
	{
		Button_JoinSession->OnClicked().RemoveAll(this);
		Button_JoinSession->OnClicked().AddUObject(this, &ThisClass::HandleJoinSessionClicked);
	}

	if (Button_ModsAndMaps != nullptr)
	{
		Button_ModsAndMaps->OnClicked().RemoveAll(this);
		Button_ModsAndMaps->OnClicked().AddUObject(this, &ThisClass::HandleModsAndMapsClicked);
	}

	if (Button_Credits != nullptr)
	{
		Button_Credits->OnClicked().RemoveAll(this);
		Button_Credits->OnClicked().AddUObject(this, &ThisClass::HandleCreditsClicked);
	}

	if (Button_Options != nullptr)
	{
		Button_Options->OnClicked().RemoveAll(this);
		Button_Options->OnClicked().AddUObject(this, &ThisClass::HandleOptionsClicked);
	}

	if (Button_Quit != nullptr)
	{
		Button_Quit->OnClicked().RemoveAll(this);
		Button_Quit->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);
	}

	if (UAGASSModsSubsystem* const ModsSubsystem = GetModsSubsystem())
	{
		ModsSelectionChangedHandle = ModsSubsystem->OnSelectionChanged().AddUObject(this, &ThisClass::HandleModsSelectionChanged);
	}

	if (UAGASSSteamPlatformSubsystem* const SteamPlatformSubsystem = GetSteamPlatformSubsystem())
	{
		TimedRunFriendsUpdatedHandle = SteamPlatformSubsystem->OnTimedRunFriendsUpdated().AddUObject(this, &ThisClass::HandleTimedRunFriendsUpdated);
	}

	RefreshTimedRunSummary();
}

void UAGASSMainMenuScreenWidget::NativeDestruct()
{
	if (UAGASSModsSubsystem* const ModsSubsystem = GetModsSubsystem())
	{
		if (ModsSelectionChangedHandle.IsValid())
		{
			ModsSubsystem->OnSelectionChanged().Remove(ModsSelectionChangedHandle);
			ModsSelectionChangedHandle.Reset();
		}
	}

	if (UAGASSSteamPlatformSubsystem* const SteamPlatformSubsystem = GetSteamPlatformSubsystem())
	{
		if (TimedRunFriendsUpdatedHandle.IsValid())
		{
			SteamPlatformSubsystem->OnTimedRunFriendsUpdated().Remove(TimedRunFriendsUpdatedHandle);
			TimedRunFriendsUpdatedHandle.Reset();
		}
	}

	Super::NativeDestruct();
}

void UAGASSMainMenuScreenWidget::BuildFallbackScreenContent(UVerticalBox* ContentBox)
{
	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "MainMenuIntro", "Hosting travels directly into the selected AGASS gameplay map. Joining, mod selection, options, and credits now route through a persistent CommonUI shell instead of rebuilding the root layout for each menu change."), 16);
	AddSpacer(ContentBox, 8.f);

	Text_SelectedMapSummary = AddBodyText(ContentBox, FText::GetEmpty(), 15, TEXT("Text_SelectedMapSummary"));
	Text_ActiveModsSummary = AddBodyText(ContentBox, FText::GetEmpty(), 15, TEXT("Text_ActiveModsSummary"));
	Text_PersonalBest = AddBodyText(ContentBox, FText::GetEmpty(), 15, TEXT("Text_PersonalBest"));
	Text_FriendTimesStatus = AddBodyText(ContentBox, FText::GetEmpty(), 14, TEXT("Text_FriendTimesStatus"));
	Scroll_FriendTimes = AddScrollBox(ContentBox, 220.f, TEXT("Scroll_FriendTimes"));
	AddSpacer(ContentBox, 10.f);

	Button_Host = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "HostSessionButton", "Host Session"), TEXT("Button_Host"));
	Button_JoinSession = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "JoinSessionButton", "Join Session"), TEXT("Button_JoinSession"));
	Button_ModsAndMaps = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "ModsAndMapsButton", "Mods"), TEXT("Button_ModsAndMaps"));
	Button_Options = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "OptionsButton", "Options"), TEXT("Button_Options"));
	Button_Credits = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "CreditsButton", "Credits"), TEXT("Button_Credits"));
	Button_Quit = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "QuitButton", "Quit"), TEXT("Button_Quit"));
}

void UAGASSMainMenuScreenWidget::RefreshFromSessionState()
{
	Super::RefreshFromSessionState();

	const UAGASSModsSubsystem* ModsSubsystem = GetModsSubsystem();
	const FAGASSAvailableMapInfo* SelectedMap = ModsSubsystem ? ModsSubsystem->GetSelectedMapInfo() : nullptr;

	if (Text_SelectedMapSummary)
	{
		Text_SelectedMapSummary->SetText(FText::FromString(SelectedMap != nullptr
			? SelectedMap->GetDisplayLabel()
			: NSLOCTEXT("AGASSFrontend", "SelectedMapFallback", "Unconfigured").ToString()));
	}

	if (Text_ActiveModsSummary)
	{
		const TArray<FString> ActiveModIds = ModsSubsystem ? ModsSubsystem->GetActiveModIds() : TArray<FString>();
		Text_ActiveModsSummary->SetText(BuildActiveModsSummary(ActiveModIds));
	}

	RefreshTimedRunSummary();
}

UWidget* UAGASSMainMenuScreenWidget::ResolveInitialFocusTarget() const
{
	return Button_Host;
}

#if WITH_EDITOR
void UAGASSMainMenuScreenWidget::GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const
{
	OutRequiredWidgets.Append({
		TEXT("Button_Host"),
		TEXT("Button_JoinSession"),
		TEXT("Button_ModsAndMaps"),
		TEXT("Button_Options"),
		TEXT("Button_Credits"),
		TEXT("Button_Quit"),
		TEXT("Text_SelectedMapSummary"),
		TEXT("Text_ActiveModsSummary"),
		TEXT("Text_PersonalBest"),
		TEXT("Text_FriendTimesStatus"),
		TEXT("Scroll_FriendTimes")
	});
}
#endif

void UAGASSMainMenuScreenWidget::HandleHostClicked()
{
	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SessionSubsystem->HostSession();
	}
}

void UAGASSMainMenuScreenWidget::HandleJoinSessionClicked()
{
	if (UAGASSUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->ShowSessionBrowser(GetOwningLocalPlayer());
	}
}

void UAGASSMainMenuScreenWidget::HandleModsAndMapsClicked()
{
	if (UAGASSUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->ShowModsAndMapsScreen(GetOwningLocalPlayer());
	}
}

void UAGASSMainMenuScreenWidget::HandleOptionsClicked()
{
	if (UAGASSUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->ShowOptionsScreen(GetOwningLocalPlayer());
	}
}

void UAGASSMainMenuScreenWidget::HandleCreditsClicked()
{
	if (UAGASSUIManagerSubsystem* UIManager = GetUIManager())
	{
		UIManager->ShowCreditsScreen(GetOwningLocalPlayer());
	}
}

void UAGASSMainMenuScreenWidget::HandleQuitClicked()
{
	FPlatformMisc::RequestExit(false);
}

void UAGASSMainMenuScreenWidget::HandleModsSelectionChanged()
{
	RefreshTimedRunSummary();
}

void UAGASSMainMenuScreenWidget::HandleTimedRunFriendsUpdated(const FString& LeaderboardKey)
{
	RefreshTimedRunSummary();
}

void UAGASSMainMenuScreenWidget::RefreshTimedRunSummary()
{
	const UAGASSModsSubsystem* const ModsSubsystem = GetModsSubsystem();
	UAGASSSteamPlatformSubsystem* const SteamPlatformSubsystem = GetSteamPlatformSubsystem();
	const UAGASSSettingsLocal* const LocalSettings = UAGASSSettingsLocal::Get();

	FAGASSResolvedContentSelection HostedSelection;
	FString FailureMessage;
	const bool bHasHostedSelection = ModsSubsystem != nullptr
		&& ModsSubsystem->TryBuildHostedSelection(HostedSelection, FailureMessage)
		&& HostedSelection.bIsValid;
	const FString LeaderboardKey = bHasHostedSelection ? HostedSelection.BuildTimedRunLeaderboardKey() : FString();

	if (Text_PersonalBest != nullptr)
	{
		const int32 PersonalBestMilliseconds = (LocalSettings != nullptr && !LeaderboardKey.IsEmpty())
			? LocalSettings->GetTimedRunBestMilliseconds(LeaderboardKey)
			: 0;
		Text_PersonalBest->SetText(PersonalBestMilliseconds > 0
			? FText::Format(
				NSLOCTEXT("AGASSFrontend", "MainMenuPersonalBest", "Personal Best: {0}"),
				FormatTimedRunMilliseconds(PersonalBestMilliseconds))
			: NSLOCTEXT("AGASSFrontend", "MainMenuPersonalBestNone", "Personal Best: No recorded time yet"));
	}

	if (Scroll_FriendTimes != nullptr)
	{
		Scroll_FriendTimes->ClearChildren();
	}

	if (Text_FriendTimesStatus == nullptr)
	{
		return;
	}

	if (!bHasHostedSelection || LeaderboardKey.IsEmpty())
	{
		Text_FriendTimesStatus->SetText(NSLOCTEXT("AGASSFrontend", "FriendTimesNoSelection", "Steam Friends: Select a valid map to compare times."));
		return;
	}

	if (SteamPlatformSubsystem == nullptr || !SteamPlatformSubsystem->IsSteamPlatformActive())
	{
		Text_FriendTimesStatus->SetText(NSLOCTEXT("AGASSFrontend", "FriendTimesUnavailable", "Steam Friends: Unavailable in the current platform state."));
		return;
	}

	EAGASSSteamTimedRunFriendsQueryState QueryState = SteamPlatformSubsystem->GetTimedRunFriendsQueryState(LeaderboardKey);
	if (QueryState == EAGASSSteamTimedRunFriendsQueryState::Idle)
	{
		SteamPlatformSubsystem->QueryTimedRunFriends(LeaderboardKey);
		QueryState = SteamPlatformSubsystem->GetTimedRunFriendsQueryState(LeaderboardKey);
	}

	const TArray<FAGASSSteamTimedRunFriendEntry>* const CachedEntries = SteamPlatformSubsystem->FindCachedTimedRunFriends(LeaderboardKey);
	if (QueryState == EAGASSSteamTimedRunFriendsQueryState::Ready && CachedEntries != nullptr && !CachedEntries->IsEmpty())
	{
		Text_FriendTimesStatus->SetText(FText::Format(
			NSLOCTEXT("AGASSFrontend", "FriendTimesReady", "Steam Friends: {0} result(s)"),
			FText::AsNumber(CachedEntries->Num())));

		if (Scroll_FriendTimes != nullptr)
		{
			const UAGASSFrontendDeveloperSettings* const FrontendSettings = UAGASSFrontendDeveloperSettings::Get();
			TSubclassOf<UAGASSFriendTimedRunEntryWidget> EntryWidgetClass = FrontendSettings != nullptr
				? FrontendSettings->FriendTimedRunEntryWidgetClass.LoadSynchronous()
				: UAGASSFriendTimedRunEntryWidget::StaticClass();
			if (!EntryWidgetClass)
			{
				EntryWidgetClass = UAGASSFriendTimedRunEntryWidget::StaticClass();
			}

			for (int32 EntryIndex = 0; EntryIndex < CachedEntries->Num(); ++EntryIndex)
			{
				const FAGASSSteamTimedRunFriendEntry& Entry = (*CachedEntries)[EntryIndex];
				if (UAGASSFriendTimedRunEntryWidget* const EntryWidget = CreateWidget<UAGASSFriendTimedRunEntryWidget>(GetOwningPlayer(), EntryWidgetClass))
				{
					EntryWidget->InitializeEntry(EntryIndex + 1, Entry.FriendDisplayName, Entry.TimeMilliseconds);
					Scroll_FriendTimes->AddChild(EntryWidget);
				}
			}
		}

		return;
	}

	switch (QueryState)
	{
	case EAGASSSteamTimedRunFriendsQueryState::Idle:
		Text_FriendTimesStatus->SetText(NSLOCTEXT("AGASSFrontend", "FriendTimesIdle", "Steam Friends: Waiting to query times."));
		break;

	case EAGASSSteamTimedRunFriendsQueryState::Querying:
		Text_FriendTimesStatus->SetText(NSLOCTEXT("AGASSFrontend", "FriendTimesLoading", "Steam Friends: Loading times..."));
		break;

	case EAGASSSteamTimedRunFriendsQueryState::Ready:
		Text_FriendTimesStatus->SetText(NSLOCTEXT("AGASSFrontend", "FriendTimesEmpty", "Steam Friends: No recorded friend times yet."));
		break;

	case EAGASSSteamTimedRunFriendsQueryState::Failed:
		Text_FriendTimesStatus->SetText(NSLOCTEXT("AGASSFrontend", "FriendTimesFailed", "Steam Friends: Failed to load friend times."));
		break;

	case EAGASSSteamTimedRunFriendsQueryState::Unavailable:
		Text_FriendTimesStatus->SetText(NSLOCTEXT("AGASSFrontend", "FriendTimesUnavailable", "Steam Friends: Unavailable in the current platform state."));
		break;

	default:
		Text_FriendTimesStatus->SetText(NSLOCTEXT("AGASSFrontend", "FriendTimesIdle", "Steam Friends: Waiting to query times."));
		break;
	}
}

UAGASSSteamPlatformSubsystem* UAGASSMainMenuScreenWidget::GetSteamPlatformSubsystem() const
{
	UGameInstance* const GameInstance = GetGameInstance();
	return GameInstance != nullptr ? GameInstance->GetSubsystem<UAGASSSteamPlatformSubsystem>() : nullptr;
}
