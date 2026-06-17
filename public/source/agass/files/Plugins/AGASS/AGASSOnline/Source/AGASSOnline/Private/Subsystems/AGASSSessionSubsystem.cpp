#include "Subsystems/AGASSSessionSubsystem.h"

#include "Components/AGASSSessionInfoComponent.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/AGASSOnlineDeveloperSettings.h"
#include "Subsystems/AGASSModsSubsystem.h"
#include "Subsystems/AGASSInviteCodeSubsystem.h"
#include "Subsystems/AGASSGameplayEventSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSSessionFlow, Log, All);

namespace AGASSSessionSettingKeys
{
	static const FName InviteCode(TEXT("AGASS_INVITE_CODE"));
	static const FName BuildKey(TEXT("AGASS_BUILD"));
	static const FName SelectedMapId(TEXT("AGASS_SELECTED_MAP_ID"));
	static const FName SelectedMapDisplayName(TEXT("AGASS_SELECTED_MAP_NAME"));
	static const FName ActiveModIds(TEXT("AGASS_ACTIVE_MOD_IDS"));
	static const FName ContentCompatibilityHash(TEXT("AGASS_CONTENT_COMPAT"));
}

namespace AGASSSessionTravelOptionKeys
{
	static const TCHAR* GameplaySession = TEXT("AGASS_GAMEPLAY_SESSION");
	static const TCHAR* TowerSession = TEXT("AGASS_TOWER_SESSION");
	static const TCHAR* BuildKey = TEXT("AGASS_BUILD_KEY");
	static const TCHAR* ContentCompatibility = TEXT("AGASS_CONTENT_COMPAT");
	static const TCHAR* SelectedMapId = TEXT("AGASS_SELECTED_MAP_ID");
}

namespace
{
	constexpr float HostCreateSessionDiagnosticsIntervalSeconds = 1.0f;
	constexpr float HostCreateSessionWatchdogSeconds = 10.0f;

	const TCHAR* LexToString(const EAGASSSessionFlowState FlowState)
	{
		switch (FlowState)
		{
		case EAGASSSessionFlowState::Idle:
			return TEXT("Idle");
		case EAGASSSessionFlowState::Hosting:
			return TEXT("Hosting");
		case EAGASSSessionFlowState::Searching:
			return TEXT("Searching");
		case EAGASSSessionFlowState::Joining:
			return TEXT("Joining");
		case EAGASSSessionFlowState::InSession:
			return TEXT("InSession");
		case EAGASSSessionFlowState::ReturningToFrontend:
			return TEXT("ReturningToFrontend");
		default:
			return TEXT("Unknown");
		}
	}

	FString GetCleanWorldPackageName(const UWorld* World)
	{
		return World ? UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) : FString();
	}

	FString NormalizeFrontendFailureMessage(const FString& FailureMessage)
	{
		if (FailureMessage.Contains(TEXT("AGASS_RunClosed")))
		{
			return NSLOCTEXT("AGASSOnline", "RunClosedStatus", "That run is already finishing. Start a new session or join a different active one.").ToString();
		}

		if (FailureMessage.Contains(TEXT("AGASS_BuildMismatch")))
		{
			return NSLOCTEXT("AGASSOnline", "BuildMismatchStatus", "This client does not match the host's current build compatibility key.").ToString();
		}

		if (FailureMessage.Contains(TEXT("AGASS_ContentMismatch")))
		{
			return NSLOCTEXT("AGASSOnline", "ContentMismatchStatus", "The selected map or active mods do not match this session. Return to the frontend, adjust the active mods, and try again.").ToString();
		}

		return FailureMessage;
	}

	FString DescribeJoinResult(const EOnJoinSessionCompleteResult::Type Result)
	{
		switch (Result)
		{
		case EOnJoinSessionCompleteResult::Success:
			return TEXT("Join succeeded.");
		case EOnJoinSessionCompleteResult::SessionIsFull:
			return TEXT("The selected session is full.");
		case EOnJoinSessionCompleteResult::SessionDoesNotExist:
			return TEXT("The selected session no longer exists.");
		case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
			return TEXT("Could not resolve the host address for the selected session.");
		case EOnJoinSessionCompleteResult::AlreadyInSession:
			return TEXT("Already in a session.");
		case EOnJoinSessionCompleteResult::UnknownError:
		default:
			return TEXT("Joining the session failed.");
		}
	}
}

UAGASSSessionSubsystem::UAGASSSessionSubsystem()
	: FlowState(EAGASSSessionFlowState::Idle)
	, PendingAction(EPendingSessionAction::None)
	, PendingSearchResultIndex(INDEX_NONE)
	, PendingSessionUpdateAction(EAGASSSessionUpdateAction::None)
{
}

void UAGASSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency(UAGASSModsSubsystem::StaticClass());

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);

	if (UAGASSModsSubsystem* ModsSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSModsSubsystem>() : nullptr)
	{
		ModsSelectionChangedHandle = ModsSubsystem->OnSelectionChanged().AddUObject(this, &ThisClass::HandleModsSelectionChanged);
	}

	SetStatusMessage(FString::Printf(TEXT("Online subsystem: %s"), *GetOnlineSubsystemName().ToString()));
}

void UAGASSSessionSubsystem::Deinitialize()
{
	ClearSessionInfoComponentBinding();

	if (const IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		if (UpdateSessionCompleteHandle.IsValid())
		{
			SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
			UpdateSessionCompleteHandle.Reset();
		}
	}

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	if (UAGASSModsSubsystem* ModsSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSModsSubsystem>() : nullptr)
	{
		if (ModsSelectionChangedHandle.IsValid())
		{
			ModsSubsystem->OnSelectionChanged().Remove(ModsSelectionChangedHandle);
			ModsSelectionChangedHandle.Reset();
		}
	}

	if (GEngine)
	{
		if (NetworkFailureHandle.IsValid())
		{
			GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
			NetworkFailureHandle.Reset();
		}

		if (TravelFailureHandle.IsValid())
		{
			GEngine->OnTravelFailure().Remove(TravelFailureHandle);
			TravelFailureHandle.Reset();
		}
	}

	Super::Deinitialize();
}

void UAGASSSessionSubsystem::HostSession()
{
	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("HostSession requested. world=%s flow=%s busy=%s namedSessionActive=%s onlineSubsystem=%s"),
		*GetNameSafe(GetWorld()),
		LexToString(FlowState),
		IsBusy() ? TEXT("true") : TEXT("false"),
		IsNamedSessionActive() ? TEXT("true") : TEXT("false"),
		*GetOnlineSubsystemName().ToString());

	if (IsBusy())
	{
		UE_LOG(LogAGASSSessionFlow, Warning, TEXT("HostSession ignored because the session flow is busy. flow=%s status=\"%s\""),
			LexToString(FlowState),
			*StatusMessage);
		return;
	}

	PendingAction = EPendingSessionAction::Host;
	PendingSearchResultIndex = INDEX_NONE;
	PendingInviteCode.Reset();
	PendingFrontendReason.Reset();

	if (IsNamedSessionActive())
	{
		UE_LOG(LogAGASSSessionFlow, Display, TEXT("HostSession found an existing named session. Destroying it before hosting a new one."));
		DestroyNamedSessionForPendingAction();
		return;
	}

	UE_LOG(LogAGASSSessionFlow, Display, TEXT("HostSession has no existing named session. Continuing directly to ExecutePendingActionAfterDestroy."));
	ExecutePendingActionAfterDestroy();
}

void UAGASSSessionSubsystem::FindSessions()
{
	if (FlowState == EAGASSSessionFlowState::Hosting || FlowState == EAGASSSessionFlowState::Joining || FlowState == EAGASSSessionFlowState::ReturningToFrontend)
	{
		return;
	}

	PendingInviteCode.Reset();
	BeginFindSessions();
}

void UAGASSSessionSubsystem::JoinSearchResult(const int32 SearchResultIndex)
{
	if (IsBusy())
	{
		return;
	}

	PendingAction = EPendingSessionAction::JoinSearchResult;
	PendingSearchResultIndex = SearchResultIndex;
	PendingInviteCode.Reset();
	PendingFrontendReason.Reset();

	if (IsNamedSessionActive())
	{
		DestroyNamedSessionForPendingAction();
		return;
	}

	ExecutePendingActionAfterDestroy();
}

void UAGASSSessionSubsystem::JoinResolvedSearchResult(const FOnlineSessionSearchResult& SearchResult, const FString& JoinStatusMessage)
{
	if (IsBusy() || !SearchResult.IsValid())
	{
		return;
	}

	PendingAction = EPendingSessionAction::JoinResolvedSearchResult;
	PendingSearchResultIndex = INDEX_NONE;
	PendingResolvedSearchResult = SearchResult;
	PendingJoinStatusMessage = JoinStatusMessage;
	PendingInviteCode.Reset();
	PendingFrontendReason.Reset();

	if (IsNamedSessionActive())
	{
		DestroyNamedSessionForPendingAction();
		return;
	}

	ExecutePendingActionAfterDestroy();
}

void UAGASSSessionSubsystem::JoinInviteCode(const FString& InviteCode)
{
	if (IsBusy())
	{
		return;
	}

	UAGASSInviteCodeSubsystem* InviteCodeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSInviteCodeSubsystem>() : nullptr;
	const FString NormalizedInviteCode = InviteCodeSubsystem ? InviteCodeSubsystem->NormalizeInviteCode(InviteCode) : InviteCode;
	if (NormalizedInviteCode.IsEmpty())
	{
		SetStatusMessage(TEXT("Enter a valid invite code."));
		SetFlowState(EAGASSSessionFlowState::Idle);
		return;
	}

	PendingAction = EPendingSessionAction::ResolveInviteCode;
	PendingSearchResultIndex = INDEX_NONE;
	PendingInviteCode = NormalizedInviteCode;
	PendingFrontendReason.Reset();

	if (IsNamedSessionActive())
	{
		DestroyNamedSessionForPendingAction();
		return;
	}

	ExecutePendingActionAfterDestroy();
}

void UAGASSSessionSubsystem::ReturnToFrontend(const FString& Reason, const bool bDestroySession)
{
	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("ReturnToFrontend reason=\"%s\" destroySession=%d hasNamedSession=%d world=%s"),
		*Reason,
		bDestroySession ? 1 : 0,
		IsNamedSessionActive() ? 1 : 0,
		*GetNameSafe(GetWorld()));

	PendingFrontendReason = Reason;

	if (bDestroySession && IsNamedSessionActive())
	{
		PendingAction = EPendingSessionAction::ReturnToFrontend;
		DestroyNamedSessionForPendingAction();
		return;
	}

	CompleteFrontendReturn(Reason);
}

void UAGASSSessionSubsystem::SetHostedSessionJoinInProgressEnabled(const bool bAllowJoinInProgress)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogAGASSSessionFlow, Warning, TEXT("SetHostedSessionJoinInProgressEnabled(%d) failed because SessionInterface is invalid."), bAllowJoinInProgress ? 1 : 0);
		return;
	}

	FNamedOnlineSession* const NamedSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (NamedSession == nullptr)
	{
		UE_LOG(LogAGASSSessionFlow, Warning, TEXT("SetHostedSessionJoinInProgressEnabled(%d) failed because named session does not exist."), bAllowJoinInProgress ? 1 : 0);
		return;
	}

	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	const bool bShouldAdvertise = bAllowJoinInProgress;
	const bool bNeedsUpdate = UpdatedSettings.bAllowJoinInProgress != bAllowJoinInProgress
		|| UpdatedSettings.bAllowJoinViaPresence != bAllowJoinInProgress
		|| UpdatedSettings.bShouldAdvertise != bShouldAdvertise;
	if (!bNeedsUpdate)
	{
		UE_LOG(LogAGASSSessionFlow, Display, TEXT("SetHostedSessionJoinInProgressEnabled(%d) skipped because settings already match."), bAllowJoinInProgress ? 1 : 0);
		return;
	}

	if (PendingSessionUpdateAction != EAGASSSessionUpdateAction::None)
	{
		UE_LOG(LogAGASSSessionFlow, Warning, TEXT("SetHostedSessionJoinInProgressEnabled(%d) skipped because another session update is already in flight."), bAllowJoinInProgress ? 1 : 0);
		return;
	}

	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("Updating hosted session join-in-progress to %d. previousAdvertise=%d previousJoinInProgress=%d inviteCode=\"%s\""),
		bAllowJoinInProgress ? 1 : 0,
		UpdatedSettings.bShouldAdvertise ? 1 : 0,
		UpdatedSettings.bAllowJoinInProgress ? 1 : 0,
		*CurrentInviteCode);

	UpdatedSettings.bAllowJoinInProgress = bAllowJoinInProgress;
	UpdatedSettings.bAllowJoinViaPresence = bAllowJoinInProgress;
	UpdatedSettings.bShouldAdvertise = bShouldAdvertise;

	if (bAllowJoinInProgress)
	{
		UpdatedSettings.Set(AGASSSessionSettingKeys::InviteCode, CurrentInviteCode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}
	else
	{
		UpdatedSettings.Set(AGASSSessionSettingKeys::InviteCode, FString(), EOnlineDataAdvertisementType::DontAdvertise);
	}

	if (UpdateSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
		UpdateSessionCompleteHandle.Reset();
	}

	PendingSessionUpdateAction = EAGASSSessionUpdateAction::ToggleJoinInProgress;
	UpdateSessionCompleteHandle = SessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleUpdateSessionComplete));

	if (!SessionInterface->UpdateSession(NAME_GameSession, UpdatedSettings, true))
	{
		PendingSessionUpdateAction = EAGASSSessionUpdateAction::None;
		UE_LOG(LogAGASSSessionFlow, Warning, TEXT("UpdateSession submission failed while toggling join-in-progress."));
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
		UpdateSessionCompleteHandle.Reset();
	}
}

bool UAGASSSessionSubsystem::CanRegenerateInviteCode() const
{
	return HasNamedSession()
		&& FlowState == EAGASSSessionFlowState::InSession
		&& !CurrentInviteCode.IsEmpty()
		&& !IsInviteCodeRegenerationInProgress();
}

bool UAGASSSessionSubsystem::IsInviteCodeRegenerationInProgress() const
{
	if (const UAGASSSessionInfoComponent* SessionInfoComponent = GetSessionInfoComponent())
	{
		return SessionInfoComponent->IsInviteCodeRegenerationInProgress();
	}

	return PendingSessionUpdateAction == EAGASSSessionUpdateAction::RegenerateInviteCode;
}

bool UAGASSSessionSubsystem::RequestHostedSessionInviteCodeRegeneration()
{
	if (GetWorld() == nullptr || GetWorld()->GetNetMode() == NM_Client)
	{
		return false;
	}

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	UAGASSInviteCodeSubsystem* InviteCodeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSInviteCodeSubsystem>() : nullptr;
	if (!SessionInterface.IsValid() || InviteCodeSubsystem == nullptr)
	{
		return false;
	}

	FNamedOnlineSession* const NamedSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (NamedSession == nullptr || !CanRegenerateInviteCode())
	{
		return false;
	}

	FString NewInviteCode = InviteCodeSubsystem->GenerateInviteCode();
	if (NewInviteCode.IsEmpty())
	{
		return false;
	}

	if (NewInviteCode.Equals(CurrentInviteCode, ESearchCase::CaseSensitive))
	{
		NewInviteCode.Append(TEXT("1"));
	}

	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	UpdatedSettings.Set(AGASSSessionSettingKeys::InviteCode, NewInviteCode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (UpdateSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
		UpdateSessionCompleteHandle.Reset();
	}

	PendingSessionUpdateAction = EAGASSSessionUpdateAction::RegenerateInviteCode;
	PendingRegeneratedInviteCode = NewInviteCode;
	if (UAGASSSessionInfoComponent* SessionInfoComponent = GetSessionInfoComponent())
	{
		SessionInfoComponent->SetInviteCodeRegenerationInProgress(true);
	}

	SetStatusMessage(FString::Printf(TEXT("Updating invite code to %s..."), *NewInviteCode));
	UpdateSessionCompleteHandle = SessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleUpdateSessionComplete));

	if (!SessionInterface->UpdateSession(NAME_GameSession, UpdatedSettings, true))
	{
		if (UAGASSSessionInfoComponent* SessionInfoComponent = GetSessionInfoComponent())
		{
			SessionInfoComponent->SetInviteCodeRegenerationInProgress(false);
		}

		PendingSessionUpdateAction = EAGASSSessionUpdateAction::None;
		PendingRegeneratedInviteCode.Reset();
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
		UpdateSessionCompleteHandle.Reset();
		SetStatusMessage(FString::Printf(TEXT("Invite code remains %s."), *CurrentInviteCode));
		return false;
	}

	return true;
}

const TArray<FAGASSSessionSearchResultData>& UAGASSSessionSubsystem::GetSearchResults() const
{
	return SearchResults;
}

EAGASSSessionFlowState UAGASSSessionSubsystem::GetSessionFlowState() const
{
	return FlowState;
}

const FString& UAGASSSessionSubsystem::GetStatusMessage() const
{
	return StatusMessage;
}

const FString& UAGASSSessionSubsystem::GetCurrentInviteCode() const
{
	return CurrentInviteCode;
}

bool UAGASSSessionSubsystem::IsBusy() const
{
	return FlowState == EAGASSSessionFlowState::Hosting
		|| FlowState == EAGASSSessionFlowState::Joining
		|| FlowState == EAGASSSessionFlowState::ReturningToFrontend;
}

bool UAGASSSessionSubsystem::HasNamedSession() const
{
	return IsNamedSessionActive();
}

FName UAGASSSessionSubsystem::GetOnlineSubsystemName() const
{
	if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		return OnlineSubsystem->GetSubsystemName();
	}

	return NAME_None;
}

FAGASSSessionStateChangedEvent& UAGASSSessionSubsystem::OnSessionStateChanged()
{
	return SessionStateChangedEvent;
}

FAGASSSessionSearchResultsChangedEvent& UAGASSSessionSubsystem::OnSearchResultsChanged()
{
	return SearchResultsChangedEvent;
}

IOnlineSessionPtr UAGASSSessionSubsystem::GetSessionInterface() const
{
	return Online::GetSessionInterface(GetWorld());
}

bool UAGASSSessionSubsystem::ShouldUseLanSessions() const
{
	const UAGASSOnlineDeveloperSettings* Settings = UAGASSOnlineDeveloperSettings::Get();
	const FName SubsystemName = GetOnlineSubsystemName();
	return Settings && (Settings->bPreferLanSessions || SubsystemName == NULL_SUBSYSTEM || SubsystemName.IsNone());
}

bool UAGASSSessionSubsystem::ShouldUseLobbies() const
{
	const UAGASSOnlineDeveloperSettings* Settings = UAGASSOnlineDeveloperSettings::Get();
	return Settings && Settings->bUseLobbiesIfAvailable && GetOnlineSubsystemName() != NULL_SUBSYSTEM && !GetOnlineSubsystemName().IsNone();
}

FString UAGASSSessionSubsystem::GetFrontendMapPath() const
{
	const UAGASSOnlineDeveloperSettings* Settings = UAGASSOnlineDeveloperSettings::Get();
	return Settings ? Settings->FrontendMap.ToSoftObjectPath().GetLongPackageName() : FString();
}

bool UAGASSSessionSubsystem::IsGameplaySessionWorld(const UWorld* World) const
{
	if (World == nullptr || !World->IsGameWorld())
	{
		return false;
	}

	if (World->URL.HasOption(AGASSSessionTravelOptionKeys::GameplaySession)
		|| World->URL.HasOption(AGASSSessionTravelOptionKeys::TowerSession))
	{
		return true;
	}

	if (const UAGASSModsSubsystem* ModsSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSModsSubsystem>() : nullptr)
	{
		return ModsSubsystem->IsGameplaySessionWorld(*World);
	}

	return false;
}

bool UAGASSSessionSubsystem::IsFrontendWorld(const UWorld* World) const
{
	return !GetFrontendMapPath().IsEmpty() && GetCleanWorldPackageName(World) == GetFrontendMapPath();
}

bool UAGASSSessionSubsystem::IsNamedSessionActive() const
{
	if (const IOnlineSessionPtr SessionInterface = GetSessionInterface())
	{
		return SessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
	}

	return false;
}

int32 UAGASSSessionSubsystem::GetLocalUserNum() const
{
	return 0;
}

void UAGASSSessionSubsystem::BeginHostSession()
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	const UAGASSOnlineDeveloperSettings* Settings = UAGASSOnlineDeveloperSettings::Get();
	UAGASSInviteCodeSubsystem* InviteCodeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSInviteCodeSubsystem>() : nullptr;
	UAGASSModsSubsystem* ModsSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSModsSubsystem>() : nullptr;
	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("BeginHostSession. world=%s sessionInterfaceValid=%s settingsValid=%s inviteCodeSubsystemValid=%s modsSubsystemValid=%s"),
		*GetNameSafe(GetWorld()),
		SessionInterface.IsValid() ? TEXT("true") : TEXT("false"),
		Settings != nullptr ? TEXT("true") : TEXT("false"),
		InviteCodeSubsystem != nullptr ? TEXT("true") : TEXT("false"),
		ModsSubsystem != nullptr ? TEXT("true") : TEXT("false"));

	if (!SessionInterface.IsValid() || !Settings || !InviteCodeSubsystem || !ModsSubsystem)
	{
		HandleFailure(TEXT("The online session interface is unavailable."), false, false);
		return;
	}

	FAGASSResolvedContentSelection HostedSelection;
	FString SelectionFailureMessage;
	if (!ModsSubsystem->TryBuildHostedSelection(HostedSelection, SelectionFailureMessage) || !HostedSelection.bIsValid)
	{
		UE_LOG(
			LogAGASSSessionFlow,
			Warning,
			TEXT("BeginHostSession failed to resolve a valid hosted selection. failure=\"%s\" isValid=%s"),
			*SelectionFailureMessage,
			HostedSelection.bIsValid ? TEXT("true") : TEXT("false"));
		HandleFailure(
			SelectionFailureMessage.IsEmpty()
				? NSLOCTEXT("AGASSOnline", "HostSelectionUnavailable", "Select a valid map and active mod set before hosting.").ToString()
				: SelectionFailureMessage,
			false,
			false);
		return;
	}

	CurrentInviteCode = InviteCodeSubsystem->GenerateInviteCode();
	PendingHostedTravelMapPath = HostedSelection.SelectedMap.WorldPackagePath;
	PendingHostedTravelOptions = BuildConnectionTravelOptions(HostedSelection.ContentCompatibilityHash);
	if (!HostedSelection.SelectedMap.MapId.IsEmpty())
	{
		PendingHostedTravelOptions += FString::Printf(TEXT("?%s=%s"), AGASSSessionTravelOptionKeys::SelectedMapId, *HostedSelection.SelectedMap.MapId);
	}

	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("BeginHostSession resolved selection. mapId=\"%s\" mapLabel=\"%s\" mapPath=\"%s\" activeMods=\"%s\" contentHash=\"%s\" travelOptions=\"%s\""),
		*HostedSelection.SelectedMap.MapId,
		*HostedSelection.SelectedMap.GetDisplayLabel(),
		*HostedSelection.SelectedMap.WorldPackagePath,
		*HostedSelection.ActiveModIdsCsv,
		*HostedSelection.ContentCompatibilityHash,
		*PendingHostedTravelOptions);

	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = Settings->MaxPublicConnections;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bIsLANMatch = ShouldUseLanSessions();
	SessionSettings.bUseLobbiesIfAvailable = ShouldUseLobbies();
	SessionSettings.Set(SETTING_MAPNAME, HostedSelection.SelectedMap.WorldPackagePath, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(AGASSSessionSettingKeys::InviteCode, CurrentInviteCode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(AGASSSessionSettingKeys::BuildKey, Settings->BuildCompatibilityKey, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(AGASSSessionSettingKeys::SelectedMapId, HostedSelection.SelectedMap.MapId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(
		AGASSSessionSettingKeys::SelectedMapDisplayName,
		HostedSelection.SelectedMap.GetDisplayLabel(),
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(
		AGASSSessionSettingKeys::ActiveModIds,
		HostedSelection.ActiveModIdsCsv,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(
		AGASSSessionSettingKeys::ContentCompatibilityHash,
		HostedSelection.ContentCompatibilityHash,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("BeginHostSession session settings. publicConnections=%d shouldAdvertise=%s allowJoinInProgress=%s usePresence=%s isLAN=%s useLobbies=%s inviteCode=\"%s\" buildKey=\"%s\""),
		SessionSettings.NumPublicConnections,
		SessionSettings.bShouldAdvertise ? TEXT("true") : TEXT("false"),
		SessionSettings.bAllowJoinInProgress ? TEXT("true") : TEXT("false"),
		SessionSettings.bUsesPresence ? TEXT("true") : TEXT("false"),
		SessionSettings.bIsLANMatch ? TEXT("true") : TEXT("false"),
		SessionSettings.bUseLobbiesIfAvailable ? TEXT("true") : TEXT("false"),
		*CurrentInviteCode,
		*Settings->BuildCompatibilityKey);

	SetStatusMessage(FString::Printf(TEXT("Creating session. Invite code: %s"), *CurrentInviteCode));
	SetFlowState(EAGASSSessionFlowState::Hosting);
	if (UAGASSSessionInfoComponent* SessionInfoComponent = GetSessionInfoComponent())
	{
		SessionInfoComponent->SetInviteCode(CurrentInviteCode);
		SessionInfoComponent->SetInviteCodeRegenerationInProgress(false);
	}

	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));

	const bool bCreateSubmitted = SessionInterface->CreateSession(GetLocalUserNum(), NAME_GameSession, SessionSettings);
	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("BeginHostSession submitted CreateSession. success=%s localUserNum=%d sessionName=%s"),
		bCreateSubmitted ? TEXT("true") : TEXT("false"),
		GetLocalUserNum(),
		TEXT("GameSession"));

	if (!bCreateSubmitted)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		CreateSessionCompleteHandle.Reset();
		HandleFailure(TEXT("Failed to submit the host session request."), false, false);
		return;
	}

	StartHostCreateSessionWatchdog();
	StartHostCreateSessionDiagnostics();
}

void UAGASSSessionSubsystem::BeginFindSessions()
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	const UAGASSOnlineDeveloperSettings* Settings = UAGASSOnlineDeveloperSettings::Get();
	if (!SessionInterface.IsValid() || !Settings)
	{
		HandleFailure(TEXT("The online session interface is unavailable."), false, false);
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	const bool bSearchingForSpecificInviteCode = !PendingInviteCode.IsEmpty();
	SessionSearch->MaxSearchResults =
		bSearchingForSpecificInviteCode && ShouldUseLobbies()
			? FMath::Max(Settings->SessionSearchMaxResults, 128)
			: Settings->SessionSearchMaxResults;
	SessionSearch->bIsLanQuery = ShouldUseLanSessions();
	if (!Settings->BuildCompatibilityKey.IsEmpty())
	{
		SessionSearch->QuerySettings.Set(
			AGASSSessionSettingKeys::BuildKey,
			Settings->BuildCompatibilityKey,
			EOnlineComparisonOp::Equals);
	}

	if (bSearchingForSpecificInviteCode)
	{
		SessionSearch->QuerySettings.Set(
			AGASSSessionSettingKeys::InviteCode,
			PendingInviteCode,
			EOnlineComparisonOp::Equals);
	}

	if (ShouldUseLobbies())
	{
		SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	SetStatusMessage(
		bSearchingForSpecificInviteCode
			? FString::Printf(TEXT("Searching for invite code %s..."), *PendingInviteCode)
			: TEXT("Searching for active sessions..."));
	SetFlowState(EAGASSSessionFlowState::Searching);

	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));

	if (!SessionInterface->FindSessions(GetLocalUserNum(), SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		FindSessionsCompleteHandle.Reset();
		HandleFailure(TEXT("Failed to start the session search."), false, false);
	}
}

void UAGASSSessionSubsystem::BeginJoinSearchResult(const int32 SearchResultIndex)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid() || !NativeSearchResults.IsValidIndex(SearchResultIndex))
	{
		HandleFailure(TEXT("The requested session result is no longer valid."), false, false);
		return;
	}

	BeginJoinResolvedSearchResult(NativeSearchResults[SearchResultIndex], TEXT("Joining session..."));
}

void UAGASSSessionSubsystem::BeginJoinResolvedSearchResult(const FOnlineSessionSearchResult& SearchResult, const FString& JoinStatusMessage)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid() || !SearchResult.IsValid())
	{
		HandleFailure(TEXT("The requested session result is no longer valid."), false, false);
		return;
	}

	FString JoinFailureMessage;
	FString JoinContentCompatibilityHash;
	if (!EvaluateJoinCompatibility(SearchResult, JoinFailureMessage, JoinContentCompatibilityHash))
	{
		HandleFailure(
			JoinFailureMessage.IsEmpty()
				? NSLOCTEXT("AGASSOnline", "JoinCompatibilityFailure", "This session does not match the current frontend map/mod selection.").ToString()
				: JoinFailureMessage,
			!IsFrontendWorld(GetWorld()),
			false);
		return;
	}

	FString InviteCode;
	SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::InviteCode, InviteCode);

	CurrentInviteCode = InviteCode;
	PendingJoinTravelOptions = BuildConnectionTravelOptions(JoinContentCompatibilityHash);
	SetStatusMessage(JoinStatusMessage.IsEmpty() ? TEXT("Joining session...") : JoinStatusMessage);
	SetFlowState(EAGASSSessionFlowState::Joining);

	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));

	if (!SessionInterface->JoinSession(GetLocalUserNum(), NAME_GameSession, SearchResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		JoinSessionCompleteHandle.Reset();
		HandleFailure(TEXT("Failed to submit the join request."), false, false);
	}
}

void UAGASSSessionSubsystem::DestroyNamedSessionForPendingAction()
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("DestroyNamedSessionForPendingAction. pendingAction=%d sessionInterfaceValid=%s namedSessionActive=%s"),
		static_cast<int32>(PendingAction),
		SessionInterface.IsValid() ? TEXT("true") : TEXT("false"),
		(SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession) != nullptr) ? TEXT("true") : TEXT("false"));

	if (!SessionInterface.IsValid() || SessionInterface->GetNamedSession(NAME_GameSession) == nullptr)
	{
		UE_LOG(LogAGASSSessionFlow, Display, TEXT("DestroyNamedSessionForPendingAction skipped because no named session exists."));
		ExecutePendingActionAfterDestroy();
		return;
	}

	SetStatusMessage(TEXT("Destroying the previous session..."));

	DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));

	const bool bDestroySubmitted = SessionInterface->DestroySession(NAME_GameSession);
	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("DestroyNamedSessionForPendingAction submitted DestroySession. success=%s sessionName=%s"),
		bDestroySubmitted ? TEXT("true") : TEXT("false"),
		TEXT("GameSession"));

	if (!bDestroySubmitted)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		DestroySessionCompleteHandle.Reset();
		HandleFailure(TEXT("Failed to destroy the previous session."), true, false);
	}
}

void UAGASSSessionSubsystem::ExecutePendingActionAfterDestroy()
{
	const EPendingSessionAction Action = PendingAction;
	const int32 SearchResultIndex = PendingSearchResultIndex;
	const FOnlineSessionSearchResult ResolvedSearchResult = PendingResolvedSearchResult;
	const FString JoinStatusMessage = PendingJoinStatusMessage;
	const FString InviteCodeToResolve = PendingInviteCode;
	const FString FrontendReason = PendingFrontendReason;

	ResetPendingAction();

	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("ExecutePendingActionAfterDestroy. action=%d searchResultIndex=%d inviteCode=\"%s\" frontendReason=\"%s\" joinStatus=\"%s\""),
		static_cast<int32>(Action),
		SearchResultIndex,
		*InviteCodeToResolve,
		*FrontendReason,
		*JoinStatusMessage);

	switch (Action)
	{
	case EPendingSessionAction::Host:
		BeginHostSession();
		break;
	case EPendingSessionAction::JoinSearchResult:
		BeginJoinSearchResult(SearchResultIndex);
		break;
	case EPendingSessionAction::JoinResolvedSearchResult:
		BeginJoinResolvedSearchResult(ResolvedSearchResult, JoinStatusMessage);
		break;
	case EPendingSessionAction::ResolveInviteCode:
		ResolveAndJoinInviteCode(InviteCodeToResolve);
		break;
	case EPendingSessionAction::ReturnToFrontend:
		CompleteFrontendReturn(FrontendReason);
		break;
	case EPendingSessionAction::None:
	default:
		break;
	}
}

void UAGASSSessionSubsystem::ResolveAndJoinInviteCode(const FString& NormalizedInviteCode)
{
	PendingInviteCode = NormalizedInviteCode;
	BeginFindSessions();
}

void UAGASSSessionSubsystem::TravelToHostedMap()
{
	if (PendingHostedTravelMapPath.IsEmpty())
	{
		HandleFailure(TEXT("No selected gameplay session map is configured for hosting."), false, true);
		return;
	}

	SetStatusMessage(FString::Printf(TEXT("Traveling to the gameplay map. Invite code: %s"), *CurrentInviteCode));

	FString TravelOptions = PendingHostedTravelOptions;
	if (!TravelOptions.Contains(TEXT("listen")))
	{
		TravelOptions = TravelOptions.IsEmpty()
			? TEXT("listen")
			: FString::Printf(TEXT("listen%s"), *TravelOptions);
	}

	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("TravelToHostedMap issuing OpenLevel. mapPath=\"%s\" travelOptions=\"%s\" world=%s"),
		*PendingHostedTravelMapPath,
		*TravelOptions,
		*GetNameSafe(GetWorld()));

	UGameplayStatics::OpenLevel(GetGameInstance(), FName(*PendingHostedTravelMapPath), true, *TravelOptions);
}

void UAGASSSessionSubsystem::CompleteFrontendReturn(const FString& Reason)
{
	ClearHostCreateSessionWatchdog();

	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("CompleteFrontendReturn reason=\"%s\" world=%s frontendMap=\"%s\""),
		*Reason,
		*GetNameSafe(GetWorld()),
		*GetFrontendMapPath());

	ClearSessionInfoComponentBinding();
	CurrentInviteCode.Reset();
	PendingRegeneratedInviteCode.Reset();
	PendingSessionUpdateAction = EAGASSSessionUpdateAction::None;
	PendingHostedTravelMapPath.Reset();
	PendingHostedTravelOptions.Reset();
	PendingJoinTravelOptions.Reset();
	NativeSearchResults.Reset();
	SearchResults.Reset();
	SearchResultsChangedEvent.Broadcast();

	SetStatusMessage(Reason);
	SetFlowState(EAGASSSessionFlowState::ReturningToFrontend);

	const FString FrontendMapPath = GetFrontendMapPath();
	if (!FrontendMapPath.IsEmpty() && !IsFrontendWorld(GetWorld()))
	{
		UGameplayStatics::OpenLevel(GetGameInstance(), FName(*FrontendMapPath));
	}

	SetFlowState(EAGASSSessionFlowState::Idle);
}

void UAGASSSessionSubsystem::CacheSearchResults()
{
	NativeSearchResults.Reset();
	SearchResults.Reset();

	const UAGASSOnlineDeveloperSettings* Settings = UAGASSOnlineDeveloperSettings::Get();
	const FString ExpectedBuildKey = Settings ? Settings->BuildCompatibilityKey : FString();

	if (!SessionSearch.IsValid())
	{
		SearchResultsChangedEvent.Broadcast();
		return;
	}

	for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
	{
		if (!SearchResult.IsValid())
		{
			continue;
		}

		FString ResultBuildKey;
		if (!ExpectedBuildKey.IsEmpty()
			&& SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::BuildKey, ResultBuildKey)
			&& !ResultBuildKey.Equals(ExpectedBuildKey, ESearchCase::IgnoreCase))
		{
			continue;
		}

		FString MapPath;
		FString MapId;
		FString MapDisplayName;
		FString InviteCode;
		FString ActiveModIdsCsv;
		SearchResult.Session.SessionSettings.Get(SETTING_MAPNAME, MapPath);
		SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::SelectedMapId, MapId);
		SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::SelectedMapDisplayName, MapDisplayName);
		SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::InviteCode, InviteCode);
		SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::ActiveModIds, ActiveModIdsCsv);

		FString CompatibilityFailureMessage;
		FString ContentCompatibilityHash;
		const bool bCompatibleWithLocalSetup = EvaluateJoinCompatibility(SearchResult, CompatibilityFailureMessage, ContentCompatibilityHash);
		TArray<FString> ActiveModIds;
		ActiveModIdsCsv.ParseIntoArray(ActiveModIds, TEXT(","), true);

		FAGASSSessionSearchResultData SessionData;
		SessionData.SessionId = SearchResult.GetSessionIdStr();
		SessionData.HostName = SearchResult.Session.OwningUserName;
		SessionData.MapPath = MapPath;
		SessionData.MapId = MapId;
		SessionData.MapDisplayName = MapDisplayName.IsEmpty() ? FPackageName::GetShortName(MapPath) : MapDisplayName;
		SessionData.InviteCode = InviteCode;
		SessionData.ActiveModIdsCsv = ActiveModIdsCsv;
		SessionData.ActiveModCount = ActiveModIds.Num();
		SessionData.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
		SessionData.CurrentPlayers = SessionData.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
		SessionData.bIsLANMatch = SearchResult.Session.SessionSettings.bIsLANMatch;
		SessionData.bIsCompatibleWithLocalSetup = bCompatibleWithLocalSetup;
		SessionData.JoinDisabledReason = CompatibilityFailureMessage;
		SessionData.bIsJoinable = SearchResult.Session.NumOpenPublicConnections > 0 && bCompatibleWithLocalSetup;
		if (SearchResult.Session.NumOpenPublicConnections <= 0 && SessionData.JoinDisabledReason.IsEmpty())
		{
			SessionData.JoinDisabledReason = NSLOCTEXT("AGASSOnline", "SearchResultFull", "The selected session is already full.").ToString();
		}

		NativeSearchResults.Add(SearchResult);
		SearchResults.Add(MoveTemp(SessionData));
	}

	SearchResultsChangedEvent.Broadcast();
}

void UAGASSSessionSubsystem::RebuildCachedSearchResultCompatibility()
{
	if (NativeSearchResults.Num() != SearchResults.Num())
	{
		CacheSearchResults();
		return;
	}

	for (int32 ResultIndex = 0; ResultIndex < NativeSearchResults.Num(); ++ResultIndex)
	{
		FString CompatibilityFailureMessage;
		FString ContentCompatibilityHash;
		const bool bCompatible = EvaluateJoinCompatibility(NativeSearchResults[ResultIndex], CompatibilityFailureMessage, ContentCompatibilityHash);
		SearchResults[ResultIndex].bIsCompatibleWithLocalSetup = bCompatible;
		SearchResults[ResultIndex].JoinDisabledReason = CompatibilityFailureMessage;
		SearchResults[ResultIndex].bIsJoinable =
			NativeSearchResults[ResultIndex].Session.NumOpenPublicConnections > 0 && bCompatible;

		if (NativeSearchResults[ResultIndex].Session.NumOpenPublicConnections <= 0
			&& SearchResults[ResultIndex].JoinDisabledReason.IsEmpty())
		{
			SearchResults[ResultIndex].JoinDisabledReason = NSLOCTEXT("AGASSOnline", "SearchResultFullCached", "The selected session is already full.").ToString();
		}
	}

	SearchResultsChangedEvent.Broadcast();
}

FString UAGASSSessionSubsystem::BuildConnectionTravelOptions(const FString& ContentCompatibilityHash) const
{
	FString TravelOptions = FString::Printf(
		TEXT("?%s=1?%s=1"),
		AGASSSessionTravelOptionKeys::GameplaySession,
		AGASSSessionTravelOptionKeys::TowerSession);

	if (const UAGASSOnlineDeveloperSettings* Settings = UAGASSOnlineDeveloperSettings::Get())
	{
		if (!Settings->BuildCompatibilityKey.IsEmpty())
		{
			TravelOptions += FString::Printf(TEXT("?%s=%s"), AGASSSessionTravelOptionKeys::BuildKey, *Settings->BuildCompatibilityKey);
		}
	}

	if (!ContentCompatibilityHash.IsEmpty())
	{
		TravelOptions += FString::Printf(TEXT("?%s=%s"), AGASSSessionTravelOptionKeys::ContentCompatibility, *ContentCompatibilityHash);
	}

	return TravelOptions;
}

bool UAGASSSessionSubsystem::EvaluateJoinCompatibility(
	const FOnlineSessionSearchResult& SearchResult,
	FString& OutFailureMessage,
	FString& OutContentCompatibilityHash) const
{
	OutFailureMessage.Reset();
	OutContentCompatibilityHash.Reset();

	FString RemoteMapId;
	FString RemoteMapDisplayName;
	FString RemoteActiveModIdsCsv;
	FString RemoteContentCompatibilityHash;
	SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::SelectedMapId, RemoteMapId);
	SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::SelectedMapDisplayName, RemoteMapDisplayName);
	SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::ActiveModIds, RemoteActiveModIdsCsv);
	SearchResult.Session.SessionSettings.Get(AGASSSessionSettingKeys::ContentCompatibilityHash, RemoteContentCompatibilityHash);

	UAGASSModsSubsystem* ModsSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSModsSubsystem>() : nullptr;
	if (ModsSubsystem == nullptr)
	{
		OutFailureMessage = NSLOCTEXT("AGASSOnline", "MissingModsSubsystem", "The mods subsystem is unavailable for content compatibility checks.").ToString();
		return false;
	}

	FAGASSJoinCompatibilityResult CompatibilityResult;
	if (!ModsSubsystem->EvaluateJoinCompatibility(
		RemoteMapId,
		RemoteMapDisplayName,
		RemoteActiveModIdsCsv,
		RemoteContentCompatibilityHash,
		CompatibilityResult))
	{
		OutFailureMessage = CompatibilityResult.FriendlyFailureMessage;
		return false;
	}

	OutContentCompatibilityHash = CompatibilityResult.LocalContentCompatibilityHash;
	return true;
}

void UAGASSSessionSubsystem::HandleModsSelectionChanged()
{
	RebuildCachedSearchResultCompatibility();
}

void UAGASSSessionSubsystem::SetFlowState(const EAGASSSessionFlowState NewState)
{
	if (FlowState == NewState)
	{
		UE_LOG(LogAGASSSessionFlow, Display, TEXT("SetFlowState broadcast without change. state=%s world=%s status=\"%s\""),
			LexToString(FlowState),
			*GetNameSafe(GetWorld()),
			*StatusMessage);
		SessionStateChangedEvent.Broadcast();
		return;
	}

	UE_LOG(LogAGASSSessionFlow, Display, TEXT("SetFlowState %s -> %s world=%s status=\"%s\""),
		LexToString(FlowState),
		LexToString(NewState),
		*GetNameSafe(GetWorld()),
		*StatusMessage);
	FlowState = NewState;
	SessionStateChangedEvent.Broadcast();
}

void UAGASSSessionSubsystem::SetStatusMessage(const FString& NewStatusMessage)
{
	UE_LOG(LogAGASSSessionFlow, Display, TEXT("SetStatusMessage \"%s\" -> \"%s\" world=%s flow=%s"),
		*StatusMessage,
		*NewStatusMessage,
		*GetNameSafe(GetWorld()),
		LexToString(FlowState));
	StatusMessage = NewStatusMessage;
	SessionStateChangedEvent.Broadcast();
}

void UAGASSSessionSubsystem::ResetPendingAction()
{
	PendingAction = EPendingSessionAction::None;
	PendingSearchResultIndex = INDEX_NONE;
	PendingResolvedSearchResult = FOnlineSessionSearchResult();
	PendingJoinStatusMessage.Reset();
	PendingInviteCode.Reset();
	PendingFrontendReason.Reset();
	PendingJoinTravelOptions.Reset();
}

void UAGASSSessionSubsystem::StartHostCreateSessionDiagnostics()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HostCreateSessionDiagnosticsHandle);
		World->GetTimerManager().SetTimer(
			HostCreateSessionDiagnosticsHandle,
			this,
			&ThisClass::LogHostCreateSessionProgress,
			HostCreateSessionDiagnosticsIntervalSeconds,
			true);
		UE_LOG(
			LogAGASSSessionFlow,
			Display,
			TEXT("StartHostCreateSessionDiagnostics intervalSeconds=%.2f world=%s"),
			HostCreateSessionDiagnosticsIntervalSeconds,
			*GetNameSafe(World));
	}
}

void UAGASSSessionSubsystem::ClearHostCreateSessionDiagnostics()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HostCreateSessionDiagnosticsHandle);
	}

	HostCreateSessionDiagnosticsHandle.Invalidate();
}

void UAGASSSessionSubsystem::LogHostCreateSessionProgress()
{
	if (FlowState != EAGASSSessionFlowState::Hosting)
	{
		ClearHostCreateSessionDiagnostics();
		return;
	}

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	const FNamedOnlineSession* const NamedSession = SessionInterface.IsValid() ? SessionInterface->GetNamedSession(NAME_GameSession) : nullptr;
	const int32 SessionStateValue = NamedSession != nullptr ? static_cast<int32>(NamedSession->SessionState) : -1;
	const FString OwningUserId = (NamedSession != nullptr && NamedSession->OwningUserId.IsValid())
		? NamedSession->OwningUserId->ToString()
		: FString();

	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("LogHostCreateSessionProgress flow=%s status=\"%s\" sessionInterfaceValid=%s namedSessionActive=%s sessionState=%d owningUser=\"%s\" world=%s"),
		LexToString(FlowState),
		*StatusMessage,
		SessionInterface.IsValid() ? TEXT("true") : TEXT("false"),
		NamedSession != nullptr ? TEXT("true") : TEXT("false"),
		SessionStateValue,
		OwningUserId.IsEmpty() ? TEXT("<none>") : *OwningUserId,
		*GetNameSafe(GetWorld()));
}

void UAGASSSessionSubsystem::StartHostCreateSessionWatchdog()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HostCreateSessionWatchdogHandle);
		World->GetTimerManager().SetTimer(
			HostCreateSessionWatchdogHandle,
			this,
			&ThisClass::HandleHostCreateSessionWatchdogExpired,
			HostCreateSessionWatchdogSeconds,
			false);
		UE_LOG(LogAGASSSessionFlow, Display, TEXT("StartHostCreateSessionWatchdog timeoutSeconds=%.2f world=%s"), HostCreateSessionWatchdogSeconds, *GetNameSafe(World));
	}
}

void UAGASSSessionSubsystem::ClearHostCreateSessionWatchdog()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HostCreateSessionWatchdogHandle);
	}

	HostCreateSessionWatchdogHandle.Invalidate();
}

void UAGASSSessionSubsystem::HandleHostCreateSessionWatchdogExpired()
{
	if (FlowState != EAGASSSessionFlowState::Hosting)
	{
		UE_LOG(
			LogAGASSSessionFlow,
			Display,
			TEXT("HandleHostCreateSessionWatchdogExpired ignored because flow already advanced to %s."),
			LexToString(FlowState));
		ClearHostCreateSessionDiagnostics();
		ClearHostCreateSessionWatchdog();
		return;
	}

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	const FNamedOnlineSession* const NamedSession = SessionInterface.IsValid() ? SessionInterface->GetNamedSession(NAME_GameSession) : nullptr;
	const int32 SessionStateValue = NamedSession != nullptr ? static_cast<int32>(NamedSession->SessionState) : -1;

	UE_LOG(
		LogAGASSSessionFlow,
		Warning,
		TEXT("HandleHostCreateSessionWatchdogExpired after %.2f seconds. flow=%s status=\"%s\" sessionInterfaceValid=%s namedSessionActive=%s sessionState=%d world=%s"),
		HostCreateSessionWatchdogSeconds,
		LexToString(FlowState),
		*StatusMessage,
		SessionInterface.IsValid() ? TEXT("true") : TEXT("false"),
		NamedSession != nullptr ? TEXT("true") : TEXT("false"),
		SessionStateValue,
		*GetNameSafe(GetWorld()));

	ClearHostCreateSessionDiagnostics();
	SetStatusMessage(TEXT("Steam session creation timed out before the create callback completed."));
	HandleFailure(TEXT("Steam session creation timed out before the create callback completed."), false, true);
}

void UAGASSSessionSubsystem::BindSessionInfoComponentForCurrentWorld()
{
	UAGASSSessionInfoComponent* SessionInfoComponent = GetSessionInfoComponent();
	if (SessionInfoComponent == nullptr)
	{
		QueueBindSessionInfoComponentForCurrentWorld();
		return;
	}

	if (BoundSessionInfoComponent.Get() == SessionInfoComponent)
	{
		RefreshInviteCodeFromSessionInfo();
		return;
	}

	ClearSessionInfoComponentBinding();
	BoundSessionInfoComponent = SessionInfoComponent;
	SessionInfoChangedHandle = SessionInfoComponent->OnSessionInfoChanged().AddUObject(this, &ThisClass::HandleSessionInfoChanged);
	RefreshInviteCodeFromSessionInfo();
}

void UAGASSSessionSubsystem::QueueBindSessionInfoComponentForCurrentWorld()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SessionInfoBindRetryHandle, this, &ThisClass::BindSessionInfoComponentForCurrentWorld, 0.01f, false);
	}
}

void UAGASSSessionSubsystem::ClearSessionInfoComponentBinding()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SessionInfoBindRetryHandle);
	}
	SessionInfoBindRetryHandle.Invalidate();

	if (UAGASSSessionInfoComponent* SessionInfoComponent = BoundSessionInfoComponent.Get())
	{
		if (SessionInfoChangedHandle.IsValid())
		{
			SessionInfoComponent->OnSessionInfoChanged().Remove(SessionInfoChangedHandle);
			SessionInfoChangedHandle.Reset();
		}
	}

	BoundSessionInfoComponent.Reset();
}

UAGASSSessionInfoComponent* UAGASSSessionSubsystem::GetSessionInfoComponent() const
{
	if (const UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->FindComponentByClass<UAGASSSessionInfoComponent>();
		}
	}

	return nullptr;
}

void UAGASSSessionSubsystem::RefreshInviteCodeFromSessionInfo()
{
	if (const UAGASSSessionInfoComponent* SessionInfoComponent = BoundSessionInfoComponent.Get())
	{
		if (!SessionInfoComponent->GetInviteCode().IsEmpty() || CurrentInviteCode.IsEmpty())
		{
			CurrentInviteCode = SessionInfoComponent->GetInviteCode();
		}

		SessionStateChangedEvent.Broadcast();
	}
}

void UAGASSSessionSubsystem::HandleSessionInfoChanged()
{
	RefreshInviteCodeFromSessionInfo();
}

void UAGASSSessionSubsystem::HandleFailure(const FString& FailureMessage, const bool bForceFrontendTravel, const bool bDestroySession)
{
	ClearHostCreateSessionDiagnostics();
	ClearHostCreateSessionWatchdog();

	const FString NormalizedFailureMessage = NormalizeFrontendFailureMessage(FailureMessage);
	UE_LOG(
		LogAGASSSessionFlow,
		Warning,
		TEXT("HandleFailure failure=\"%s\" normalized=\"%s\" forceFrontend=%d destroySession=%d hasNamedSession=%d pendingAction=%d world=%s"),
		*FailureMessage,
		*NormalizedFailureMessage,
		bForceFrontendTravel ? 1 : 0,
		bDestroySession ? 1 : 0,
		IsNamedSessionActive() ? 1 : 0,
		static_cast<int32>(PendingAction),
		*GetNameSafe(GetWorld()));

	if (bDestroySession && IsNamedSessionActive() && PendingAction != EPendingSessionAction::ReturnToFrontend)
	{
		ReturnToFrontend(NormalizedFailureMessage, true);
		return;
	}

	if (bForceFrontendTravel && !IsFrontendWorld(GetWorld()))
	{
		ReturnToFrontend(NormalizedFailureMessage, bDestroySession);
		return;
	}

	SetStatusMessage(NormalizedFailureMessage);
	SetFlowState(EAGASSSessionFlowState::Idle);
}

void UAGASSSessionSubsystem::HandleCreateSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	ClearHostCreateSessionDiagnostics();
	ClearHostCreateSessionWatchdog();

	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("HandleCreateSessionComplete. session=%s success=%s interfaceValid=%s namedSessionActive=%s flow=%s"),
		*SessionName.ToString(),
		bWasSuccessful ? TEXT("true") : TEXT("false"),
		SessionInterface.IsValid() ? TEXT("true") : TEXT("false"),
		IsNamedSessionActive() ? TEXT("true") : TEXT("false"),
		LexToString(FlowState));

	if (SessionInterface.IsValid() && CreateSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		CreateSessionCompleteHandle.Reset();
	}

	if (!bWasSuccessful || SessionName != NAME_GameSession)
	{
		HandleFailure(TEXT("Creating the session failed."), false, false);
		return;
	}

	StartSessionCompleteHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(
		FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleStartSessionComplete));

	const bool bStartSubmitted = SessionInterface->StartSession(NAME_GameSession);
	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("HandleCreateSessionComplete submitted StartSession. success=%s session=%s"),
		bStartSubmitted ? TEXT("true") : TEXT("false"),
		TEXT("GameSession"));

	if (!bStartSubmitted)
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);
		StartSessionCompleteHandle.Reset();
		HandleFailure(TEXT("Starting the hosted session failed."), false, true);
	}
}

void UAGASSSessionSubsystem::HandleStartSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	ClearHostCreateSessionDiagnostics();
	ClearHostCreateSessionWatchdog();
	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("HandleStartSessionComplete. session=%s success=%s interfaceValid=%s flow=%s mapPath=\"%s\""),
		*SessionName.ToString(),
		bWasSuccessful ? TEXT("true") : TEXT("false"),
		SessionInterface.IsValid() ? TEXT("true") : TEXT("false"),
		LexToString(FlowState),
		*PendingHostedTravelMapPath);

	if (SessionInterface.IsValid() && StartSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);
		StartSessionCompleteHandle.Reset();
	}

	if (!bWasSuccessful || SessionName != NAME_GameSession)
	{
		HandleFailure(TEXT("The hosted session could not be started."), false, true);
		return;
	}

	UE_LOG(LogAGASSSessionFlow, Display, TEXT("HandleStartSessionComplete succeeded. Proceeding to TravelToHostedMap."));
	TravelToHostedMap();
}

void UAGASSSessionSubsystem::HandleFindSessionsComplete(const bool bWasSuccessful)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid() && FindSessionsCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		FindSessionsCompleteHandle.Reset();
	}

	if (!bWasSuccessful)
	{
		HandleFailure(TEXT("Searching for sessions failed."), false, false);
		return;
	}

	CacheSearchResults();

	if (!PendingInviteCode.IsEmpty())
	{
		const FString InviteCodeToResolve = PendingInviteCode;
		PendingInviteCode.Reset();

		for (int32 ResultIndex = 0; ResultIndex < SearchResults.Num(); ++ResultIndex)
		{
			if (SearchResults[ResultIndex].InviteCode.Equals(InviteCodeToResolve, ESearchCase::IgnoreCase))
			{
				BeginJoinSearchResult(ResultIndex);
				return;
			}
		}

		HandleFailure(FString::Printf(TEXT("No session is advertising invite code %s."), *InviteCodeToResolve), false, false);
		return;
	}

	SetStatusMessage(FString::Printf(TEXT("Found %d advertised session(s)."), SearchResults.Num()));
	SetFlowState(EAGASSSessionFlowState::Idle);
}

void UAGASSSessionSubsystem::HandleJoinSessionComplete(const FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid() && JoinSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		JoinSessionCompleteHandle.Reset();
	}

	if (!SessionInterface.IsValid() || SessionName != NAME_GameSession || Result != EOnJoinSessionCompleteResult::Success)
	{
		PendingJoinTravelOptions.Reset();
		UE_LOG(LogAGASSSessionFlow, Warning, TEXT("HandleJoinSessionComplete failed. session=%s result=%d"), *SessionName.ToString(), static_cast<int32>(Result));
		HandleFailure(DescribeJoinResult(Result), false, false);
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		PendingJoinTravelOptions.Reset();
		HandleFailure(TEXT("Joining succeeded, but the host address could not be resolved."), false, true);
		return;
	}

	if (!PendingJoinTravelOptions.IsEmpty())
	{
		ConnectString += PendingJoinTravelOptions;
		PendingJoinTravelOptions.Reset();
	}

	APlayerController* LocalPlayerController = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!LocalPlayerController)
	{
		HandleFailure(TEXT("No local player controller is available for client travel."), false, true);
		return;
	}

	SetStatusMessage(TEXT("Joining the gameplay session..."));
	UE_LOG(LogAGASSSessionFlow, Display, TEXT("HandleJoinSessionComplete success. connectString=\"%s\""), *ConnectString);
	LocalPlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}

void UAGASSSessionSubsystem::HandleUpdateSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid() && UpdateSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteHandle);
		UpdateSessionCompleteHandle.Reset();
	}

	const EAGASSSessionUpdateAction CompletedAction = PendingSessionUpdateAction;
	const FString RegeneratedInviteCode = PendingRegeneratedInviteCode;
	PendingSessionUpdateAction = EAGASSSessionUpdateAction::None;
	PendingRegeneratedInviteCode.Reset();

	if (!bWasSuccessful || SessionName != NAME_GameSession)
	{
		UE_LOG(LogAGASSSessionFlow, Warning, TEXT("HandleUpdateSessionComplete failed. session=%s success=%d"), *SessionName.ToString(), bWasSuccessful ? 1 : 0);
		if (CompletedAction == EAGASSSessionUpdateAction::RegenerateInviteCode)
		{
			if (UAGASSSessionInfoComponent* SessionInfoComponent = GetSessionInfoComponent())
			{
				SessionInfoComponent->SetInviteCodeRegenerationInProgress(false);
			}

			SetStatusMessage(FString::Printf(TEXT("Invite code remains %s."), *CurrentInviteCode));
		}
		return;
	}

	if (CompletedAction == EAGASSSessionUpdateAction::RegenerateInviteCode)
	{
		CurrentInviteCode = RegeneratedInviteCode;
		if (UAGASSSessionInfoComponent* SessionInfoComponent = GetSessionInfoComponent())
		{
			SessionInfoComponent->SetInviteCode(RegeneratedInviteCode);
			SessionInfoComponent->SetInviteCodeRegenerationInProgress(false);
		}

		SetStatusMessage(FString::Printf(TEXT("Invite code updated to %s."), *RegeneratedInviteCode));
	}

	UE_LOG(LogAGASSSessionFlow, Display, TEXT("HandleUpdateSessionComplete success. session=%s"), *SessionName.ToString());
}

void UAGASSSessionSubsystem::HandleDestroySessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	ClearHostCreateSessionWatchdog();

	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("HandleDestroySessionComplete. session=%s success=%s interfaceValid=%s pendingAction=%d flow=%s"),
		*SessionName.ToString(),
		bWasSuccessful ? TEXT("true") : TEXT("false"),
		SessionInterface.IsValid() ? TEXT("true") : TEXT("false"),
		static_cast<int32>(PendingAction),
		LexToString(FlowState));

	if (SessionInterface.IsValid() && DestroySessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		DestroySessionCompleteHandle.Reset();
	}

	if (!bWasSuccessful || SessionName != NAME_GameSession)
	{
		const bool bWasFrontendReturn = PendingAction == EPendingSessionAction::ReturnToFrontend;
		ResetPendingAction();
		if (UAGASSSessionInfoComponent* SessionInfoComponent = GetSessionInfoComponent())
		{
			SessionInfoComponent->SetInviteCodeRegenerationInProgress(false);
		}

		PendingSessionUpdateAction = EAGASSSessionUpdateAction::None;
		PendingRegeneratedInviteCode.Reset();
		HandleFailure(TEXT("Destroying the active session failed."), bWasFrontendReturn, false);
		return;
	}

	ClearSessionInfoComponentBinding();
	ExecutePendingActionAfterDestroy();
}

void UAGASSSessionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	ClearSessionInfoComponentBinding();

	if (!LoadedWorld || !LoadedWorld->IsGameWorld())
	{
		return;
	}

	const FString LoadedPackageName = GetCleanWorldPackageName(LoadedWorld);
	const bool bIsGameplaySessionWorld = IsGameplaySessionWorld(LoadedWorld);
	UE_LOG(
		LogAGASSSessionFlow,
		Display,
		TEXT("HandlePostLoadMap loadedPackage=%s currentFlowState=%d currentInviteCode=\"%s\""),
		*LoadedPackageName,
		static_cast<int32>(FlowState),
		*CurrentInviteCode);
	if (bIsGameplaySessionWorld)
	{
		ClearHostCreateSessionDiagnostics();
		ClearHostCreateSessionWatchdog();

		if (LoadedWorld->GetNetMode() != NM_Client)
		{
			if (UAGASSSessionInfoComponent* SessionInfoComponent = GetSessionInfoComponent())
			{
				SessionInfoComponent->SetInviteCode(CurrentInviteCode);
				SessionInfoComponent->SetInviteCodeRegenerationInProgress(false);
			}

			BindSessionInfoComponentForCurrentWorld();
		}
		else
		{
			BindSessionInfoComponentForCurrentWorld();
			RefreshInviteCodeFromSessionInfo();
		}

		SetStatusMessage(FString::Printf(TEXT("Joined the selected gameplay session. Invite code: %s"), *CurrentInviteCode));
		SetFlowState(EAGASSSessionFlowState::InSession);
		UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(this, AGASSGameplayEventNames::SessionEntered(), 1, 0.f, true);
		return;
	}

	if (LoadedPackageName == GetFrontendMapPath() && FlowState == EAGASSSessionFlowState::ReturningToFrontend)
	{
		SetFlowState(EAGASSSessionFlowState::Idle);
	}
}

void UAGASSSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, const ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	const FString FailureMessage = FString::Printf(TEXT("Network failure (%d): %s"), static_cast<int32>(FailureType), *ErrorString);
	HandleFailure(FailureMessage, !IsFrontendWorld(World), true);
}

void UAGASSSessionSubsystem::HandleTravelFailure(UWorld* World, const ETravelFailure::Type FailureType, const FString& ErrorString)
{
	const FString FailureMessage = FString::Printf(TEXT("Travel failure (%d): %s"), static_cast<int32>(FailureType), *ErrorString);
	HandleFailure(FailureMessage, !IsFrontendWorld(World), true);
}
