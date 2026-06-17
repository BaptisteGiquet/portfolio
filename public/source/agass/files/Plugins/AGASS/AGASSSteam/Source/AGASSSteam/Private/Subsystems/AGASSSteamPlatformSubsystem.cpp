#include "Subsystems/AGASSSteamPlatformSubsystem.h"

#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
#include "Settings/AGASSSteamDeveloperSettings.h"
#include "Subsystems/AGASSModsSubsystem.h"
#include "Subsystems/AGASSSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"

#ifndef WITH_STEAMWORKS
#define WITH_STEAMWORKS 0
#endif

#if WITH_STEAMWORKS
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAGASSSteam, Log, All);

namespace
{
	double QuantizeStatValue(const EAGASSSteamStatValueType ValueType, const double RawValue)
	{
		return ValueType == EAGASSSteamStatValueType::Integer
			? static_cast<double>(FMath::RoundToInt(static_cast<float>(RawValue)))
			: RawValue;
	}

	bool IsSteamOverlayEnabled()
	{
#if WITH_STEAMWORKS
		return SteamUtils() != nullptr && SteamUtils()->IsOverlayEnabled();
#else
		return false;
#endif
	}

	const FString& GetTimedRunStatName()
	{
		static const FString StatName(TEXT("TimeMs"));
		return StatName;
	}
}

#if WITH_STEAMWORKS
class FAGASSSteamStatsCallbackBridge
{
public:
	explicit FAGASSSteamStatsCallbackBridge(UAGASSSteamPlatformSubsystem& InOwner)
		: Owner(InOwner)
		, UserStatsStoredCallback()
		, UserAchievementStoredCallback()
	{
		UserStatsStoredCallback.Register(this, &FAGASSSteamStatsCallbackBridge::HandleUserStatsStored);
		UserAchievementStoredCallback.Register(this, &FAGASSSteamStatsCallbackBridge::HandleUserAchievementStored);
	}

	~FAGASSSteamStatsCallbackBridge()
	{
		UserStatsStoredCallback.Unregister();
		UserAchievementStoredCallback.Unregister();
	}

private:
	void HandleUserStatsStored(UserStatsStored_t* CallbackData)
	{
		if (CallbackData == nullptr)
		{
			Owner.HandleSteamStatsStored(false, static_cast<int32>(k_EResultFail));
			return;
		}

		const CGameID CurrentGameId(SteamUtils() != nullptr ? SteamUtils()->GetAppID() : 0);
		const bool bMatchesCurrentGame = CallbackData->m_nGameID == CurrentGameId.ToUint64();
		Owner.HandleSteamStatsStored(bMatchesCurrentGame && CallbackData->m_eResult == k_EResultOK, static_cast<int32>(CallbackData->m_eResult));
	}

	void HandleUserAchievementStored(UserAchievementStored_t* CallbackData)
	{
		if (CallbackData == nullptr)
		{
			return;
		}

		const CGameID CurrentGameId(SteamUtils() != nullptr ? SteamUtils()->GetAppID() : 0);
		if (CallbackData->m_nGameID != CurrentGameId.ToUint64())
		{
			return;
		}

		Owner.HandleSteamAchievementStored(UTF8_TO_TCHAR(CallbackData->m_rgchAchievementName));
	}

private:
	UAGASSSteamPlatformSubsystem& Owner;

	CCallbackManual<FAGASSSteamStatsCallbackBridge, UserStatsStored_t> UserStatsStoredCallback;
	CCallbackManual<FAGASSSteamStatsCallbackBridge, UserAchievementStored_t> UserAchievementStoredCallback;
};
#endif

UAGASSSteamPlatformSubsystem::~UAGASSSteamPlatformSubsystem()
{
	delete SteamStatsCallbackBridge;
	SteamStatsCallbackBridge = nullptr;
}

void UAGASSSteamPlatformSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RefreshSteamDefinitionsFromSettings();
	UE_LOG(
		LogAGASSSteam,
		Display,
		TEXT("Initialize sessionSubsystem=%s onlineSubsystem=%s overlayEnabled=%d"),
		*GetNameSafe(GetSessionSubsystem()),
		GetSessionSubsystem() != nullptr ? *GetSessionSubsystem()->GetOnlineSubsystemName().ToString() : TEXT("None"),
		IsSteamOverlayEnabled());

	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SessionStateChangedHandle = SessionSubsystem->OnSessionStateChanged().AddUObject(this, &ThisClass::HandleSessionStateChanged);
	}

	if (UAGASSGameplayEventSubsystem* GameplayEventSubsystem = GetGameplayEventSubsystem())
	{
		GameplayEventReceivedHandle = GameplayEventSubsystem->OnGameplayEventReceived().AddUObject(this, &ThisClass::HandleGameplayEventReceived);
	}

	BindSessionInterfaceDelegates();
	BindExternalUIDelegates();
	StatsTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::TickStats), 1.0f);

	HandleSessionStateChanged();
}

void UAGASSSteamPlatformSubsystem::Deinitialize()
{
	StopSessionPlaytimeTracking(true);
	FlushDirtyStats(true);

	if (StatsTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(StatsTickerHandle);
		StatsTickerHandle.Reset();
	}

	if (UAGASSGameplayEventSubsystem* GameplayEventSubsystem = GetGameplayEventSubsystem())
	{
		if (GameplayEventReceivedHandle.IsValid())
		{
			GameplayEventSubsystem->OnGameplayEventReceived().Remove(GameplayEventReceivedHandle);
			GameplayEventReceivedHandle.Reset();
		}
	}

	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		if (SessionStateChangedHandle.IsValid())
		{
			SessionSubsystem->OnSessionStateChanged().Remove(SessionStateChangedHandle);
			SessionStateChangedHandle.Reset();
		}
	}

	UnbindSessionInterfaceDelegates();
	UnbindExternalUIDelegates();
	delete SteamStatsCallbackBridge;
	SteamStatsCallbackBridge = nullptr;
	InFlightTimedRunLeaderboardLookupHandle = 0;
	InFlightTimedRunLeaderboardLookupKey.Reset();
	InFlightTimedRunLeaderboardReadHandle = 0;
	InFlightTimedRunLeaderboardReadLeaderboardHandle = 0;
	InFlightTimedRunLeaderboardReadKey.Reset();
	InFlightTimedRunSubmitLookupHandle = 0;
	InFlightTimedRunSubmitKey.Reset();
	InFlightTimedRunSubmitMilliseconds = 0;
	InFlightTimedRunSubmitUploadHandle = 0;
	InFlightTimedRunSubmitLeaderboardHandle = 0;

	Super::Deinitialize();
}

bool UAGASSSteamPlatformSubsystem::IsSteamPlatformActive() const
{
	const UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (SessionSubsystem == nullptr || SessionSubsystem->GetOnlineSubsystemName() != STEAM_SUBSYSTEM)
	{
		return false;
	}

#if WITH_STEAMWORKS
	return SteamUser() != nullptr && SteamFriends() != nullptr && SteamUserStats() != nullptr;
#else
	return false;
#endif
}

bool UAGASSSteamPlatformSubsystem::CanShowFriendsUI() const
{
	if (!IsSteamPlatformActive())
	{
		return false;
	}

	if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		return OnlineSubsystem->GetExternalUIInterface().IsValid();
	}

	return false;
}

bool UAGASSSteamPlatformSubsystem::ShowFriendsUI()
{
	if (!CanShowFriendsUI())
	{
		return false;
	}

	if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineExternalUIPtr ExternalUI = OnlineSubsystem->GetExternalUIInterface())
		{
			return ExternalUI->ShowFriendsUI(0);
		}
	}

	return false;
}

bool UAGASSSteamPlatformSubsystem::CanShowInviteUI() const
{
	const bool bSteamPlatformActive = IsSteamPlatformActive();
	if (!bSteamPlatformActive)
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("CanShowInviteUI failed because Steam platform is not active."));
		return false;
	}

	const UAGASSSessionSubsystem* const SessionSubsystem = GetSessionSubsystem();
	if (SessionSubsystem == nullptr || !SessionSubsystem->HasNamedSession())
	{
		UE_LOG(
			LogAGASSSteam,
			Warning,
			TEXT("CanShowInviteUI failed because session state is not ready. sessionSubsystem=%s hasNamedSession=%d"),
			*GetNameSafe(SessionSubsystem),
			SessionSubsystem != nullptr && SessionSubsystem->HasNamedSession());
		return false;
	}

	uint64 LobbyIdValue = 0;
	const bool bResolvedLobbyId = TryResolveNamedSessionLobbyId(LobbyIdValue);
	UE_LOG(
		LogAGASSSteam,
		Display,
		TEXT("CanShowInviteUI hasNamedSession=%d resolvedLobbyId=%d overlayEnabled=%d"),
		SessionSubsystem->HasNamedSession(),
		bResolvedLobbyId,
		IsSteamOverlayEnabled());
	return bResolvedLobbyId && IsSteamOverlayEnabled();
}

bool UAGASSSteamPlatformSubsystem::ShowInviteUI()
{
	if (!CanShowInviteUI())
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("ShowInviteUI aborted because CanShowInviteUI returned false."));
		return false;
	}

	uint64 LobbyIdValue = 0;
	if (!TryResolveNamedSessionLobbyId(LobbyIdValue))
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("ShowInviteUI failed because the current named session does not resolve to a valid Steam lobby."));
		return false;
	}

#if WITH_STEAMWORKS
	const CSteamID LobbyId(LobbyIdValue);
	SteamFriends()->ActivateGameOverlayInviteDialog(LobbyId);
	UE_LOG(
		LogAGASSSteam,
		Display,
		TEXT("ShowInviteUI opened Steam lobby invite dialog lobbyId=%llu overlayEnabled=%d"),
		LobbyIdValue,
		IsSteamOverlayEnabled());
	return true;
#else
	UE_LOG(LogAGASSSteam, Warning, TEXT("ShowInviteUI failed because Steamworks is unavailable."));
	return false;
#endif
}

IOnlineSessionPtr UAGASSSteamPlatformSubsystem::GetSessionInterface() const
{
	return Online::GetSessionInterface(GetWorld());
}

IOnlineLeaderboardsPtr UAGASSSteamPlatformSubsystem::GetLeaderboardsInterface() const
{
	return Online::GetLeaderboardsInterface(GetWorld());
}

IOnlineIdentityPtr UAGASSSteamPlatformSubsystem::GetIdentityInterface() const
{
	return Online::GetIdentityInterface(GetWorld());
}

bool UAGASSSteamPlatformSubsystem::TryResolveNamedSessionLobbyId(uint64& OutLobbyIdValue) const
{
#if WITH_STEAMWORKS
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	const FNamedOnlineSession* const NamedSession = SessionInterface.IsValid() ? SessionInterface->GetNamedSession(NAME_GameSession) : nullptr;
	if (NamedSession == nullptr || !NamedSession->SessionInfo.IsValid())
	{
		return false;
	}

	const FString SessionIdString = NamedSession->SessionInfo->GetSessionId().ToString();
	if (SessionIdString.IsEmpty())
	{
		return false;
	}

	const uint64 SessionIdValue = FCString::Strtoui64(*SessionIdString, nullptr, 10);
	const CSteamID SessionLobbyId(SessionIdValue);
	if (!SessionLobbyId.IsValid() || !SessionLobbyId.IsLobby())
	{
		UE_LOG(
			LogAGASSSteam,
			Warning,
			TEXT("TryResolveNamedSessionLobbyId rejected sessionId=%s usesPresence=%d useLobbiesIfAvailable=%d"),
			*SessionIdString,
			NamedSession->SessionSettings.bUsesPresence ? 1 : 0,
			NamedSession->SessionSettings.bUseLobbiesIfAvailable ? 1 : 0);
		return false;
	}

	OutLobbyIdValue = SessionLobbyId.ConvertToUint64();
	return true;
#else
	return false;
#endif
}

UAGASSSessionSubsystem* UAGASSSteamPlatformSubsystem::GetSessionSubsystem() const
{
	return GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UAGASSSessionSubsystem>() : nullptr;
}

UAGASSGameplayEventSubsystem* UAGASSSteamPlatformSubsystem::GetGameplayEventSubsystem() const
{
	return GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UAGASSGameplayEventSubsystem>() : nullptr;
}

const UAGASSSteamDeveloperSettings* UAGASSSteamPlatformSubsystem::GetSteamSettings() const
{
	return UAGASSSteamDeveloperSettings::Get();
}

void UAGASSSteamPlatformSubsystem::RefreshSteamDefinitionsFromSettings()
{
	StatDefinitionsByEvent.Reset();
	StatDefinitionsByName.Reset();
	AchievementDefinitions.Reset();

	const UAGASSSteamDeveloperSettings* Settings = GetSteamSettings();
	if (Settings == nullptr)
	{
		return;
	}

	for (const FAGASSSteamStatDefinition& StatDefinition : Settings->StatDefinitions)
	{
		if (StatDefinition.SourceEventName.IsNone() || StatDefinition.SteamStatName.IsEmpty())
		{
			continue;
		}

		StatDefinitionsByEvent.FindOrAdd(StatDefinition.SourceEventName).Add(StatDefinition);
		StatDefinitionsByName.Add(StatDefinition.SteamStatName, StatDefinition);
	}

	if (Settings->PlaytimeTracking.bAccumulateSessionMinutesToSteamStat && !Settings->PlaytimeTracking.SteamStatName.IsEmpty())
	{
		FAGASSSteamStatDefinition PlaytimeStatDefinition;
		PlaytimeStatDefinition.SteamStatName = Settings->PlaytimeTracking.SteamStatName;
		PlaytimeStatDefinition.ValueType = EAGASSSteamStatValueType::Float;
		PlaytimeStatDefinition.Aggregation = EAGASSSteamStatAggregation::Add;
		PlaytimeStatDefinition.ValueSource = EAGASSSteamEventValueSource::FloatValue;
		StatDefinitionsByName.Add(PlaytimeStatDefinition.SteamStatName, PlaytimeStatDefinition);
	}

	for (const FAGASSSteamAchievementDefinition& AchievementDefinition : Settings->AchievementDefinitions)
	{
		if (!AchievementDefinition.SteamAchievementId.IsEmpty())
		{
			AchievementDefinitions.Add(AchievementDefinition);
		}
	}
}

void UAGASSSteamPlatformSubsystem::BindSessionInterfaceDelegates()
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid() || SessionInviteAcceptedHandle.IsValid())
	{
		return;
	}

	SessionInviteAcceptedHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
		FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::HandleSessionUserInviteAccepted));
}

void UAGASSSteamPlatformSubsystem::UnbindSessionInterfaceDelegates()
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid() || !SessionInviteAcceptedHandle.IsValid())
	{
		return;
	}

	SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionInviteAcceptedHandle);
	SessionInviteAcceptedHandle.Reset();
}

void UAGASSSteamPlatformSubsystem::BindExternalUIDelegates()
{
	if (ExternalUIChangeHandle.IsValid())
	{
		return;
	}

	if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineExternalUIPtr ExternalUI = OnlineSubsystem->GetExternalUIInterface())
		{
			ExternalUIChangeHandle = ExternalUI->AddOnExternalUIChangeDelegate_Handle(
				FOnExternalUIChangeDelegate::CreateUObject(this, &ThisClass::HandleExternalUIChange));
			UE_LOG(
				LogAGASSSteam,
				Display,
				TEXT("Bound external UI change delegate subsystem=%s handleValid=%d"),
				*OnlineSubsystem->GetSubsystemName().ToString(),
				ExternalUIChangeHandle.IsValid());
		}
		else
		{
			UE_LOG(LogAGASSSteam, Warning, TEXT("BindExternalUIDelegates could not find an external UI interface."));
		}
	}
	else
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("BindExternalUIDelegates could not find an online subsystem."));
	}
}

void UAGASSSteamPlatformSubsystem::UnbindExternalUIDelegates()
{
	if (!ExternalUIChangeHandle.IsValid())
	{
		return;
	}

	if (IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld()))
	{
		if (const IOnlineExternalUIPtr ExternalUI = OnlineSubsystem->GetExternalUIInterface())
		{
			ExternalUI->ClearOnExternalUIChangeDelegate_Handle(ExternalUIChangeHandle);
		}
	}

	ExternalUIChangeHandle.Reset();
}

void UAGASSSteamPlatformSubsystem::HandleExternalUIChange(bool bIsOpening)
{
	UE_LOG(
		LogAGASSSteam,
		Display,
		TEXT("External UI change opening=%d overlayEnabled=%d world=%s"),
		bIsOpening,
		IsSteamOverlayEnabled(),
		*GetNameSafe(GetWorld()));
}

void UAGASSSteamPlatformSubsystem::HandleSessionStateChanged()
{
	const UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem();
	if (SessionSubsystem == nullptr)
	{
		return;
	}

	if (!IsSteamPlatformActive())
	{
		StopSessionPlaytimeTracking(false);
		return;
	}

	EnsureSteamStatsRequested();

	if (SessionSubsystem->GetSessionFlowState() == EAGASSSessionFlowState::InSession)
	{
		StartSessionPlaytimeTracking();
		return;
	}

	StopSessionPlaytimeTracking(true);
	FlushDirtyStats(true);
}

void UAGASSSteamPlatformSubsystem::HandleGameplayEventReceived(const FAGASSGameplayEvent& GameplayEvent)
{
	if (!IsSteamPlatformActive())
	{
		return;
	}

	if (GameplayEvent.EventName == AGASSGameplayEventNames::RunCompleted()
		&& GameplayEvent.bFlagValue
		&& GameplayEvent.IntValue > 0)
	{
		UE_LOG(
			LogAGASSSteam,
			Display,
			TEXT("HandleGameplayEventReceived observed completed run. victory=%d timeMs=%d durationSeconds=%.3f"),
			GameplayEvent.bFlagValue ? 1 : 0,
			GameplayEvent.IntValue,
			GameplayEvent.FloatValue);

		if (const UGameInstance* const GameInstance = GetGameInstance())
		{
			if (const UAGASSModsSubsystem* const ModsSubsystem = GameInstance->GetSubsystem<UAGASSModsSubsystem>())
			{
				FAGASSResolvedContentSelection HostedSelection;
				FString FailureMessage;
				if (ModsSubsystem->TryBuildHostedSelection(HostedSelection, FailureMessage) && HostedSelection.bIsValid)
				{
					const FString LeaderboardKey = HostedSelection.BuildTimedRunLeaderboardKey();
					UE_LOG(
						LogAGASSSteam,
						Display,
						TEXT("HandleGameplayEventReceived resolved timed run leaderboard key=%s mapId=%s hash=%s"),
						*LeaderboardKey,
						*HostedSelection.SelectedMap.MapId,
						*HostedSelection.ContentCompatibilityHash);
					SubmitTimedRunResult(LeaderboardKey, GameplayEvent.IntValue);
				}
				else
				{
					UE_LOG(
						LogAGASSSteam,
						Warning,
						TEXT("HandleGameplayEventReceived could not resolve hosted selection for timed run submission. failure=\"%s\""),
						*FailureMessage);
				}
			}
		}
	}

	if (!bSteamStatsReady && !EnsureSteamStatsRequested())
	{
		QueuedGameplayEvents.Add(GameplayEvent);
		return;
	}

	ApplyGameplayEvent(GameplayEvent);
}

bool UAGASSSteamPlatformSubsystem::SubmitTimedRunResult(const FString& LeaderboardKey, const int32 TimeMilliseconds)
{
	const bool bSteamPlatformActive = IsSteamPlatformActive();
	if (!bSteamPlatformActive || LeaderboardKey.IsEmpty() || TimeMilliseconds <= 0)
	{
		UE_LOG(
			LogAGASSSteam,
			Warning,
			TEXT("SubmitTimedRunResult rejected. steamActive=%d leaderboardKey=\"%s\" timeMs=%d"),
			bSteamPlatformActive ? 1 : 0,
			*LeaderboardKey,
			TimeMilliseconds);
		return false;
	}

#if WITH_STEAMWORKS
	if (SteamUserStats() == nullptr || SteamUtils() == nullptr)
	{
		UE_LOG(
			LogAGASSSteam,
			Warning,
			TEXT("SubmitTimedRunResult missing Steam interfaces. hasUserStats=%d hasUtils=%d leaderboardKey=%s"),
			SteamUserStats() != nullptr ? 1 : 0,
			SteamUtils() != nullptr ? 1 : 0,
			*LeaderboardKey);
		return false;
	}

	if (InFlightTimedRunSubmitLookupHandle != k_uAPICallInvalid || InFlightTimedRunSubmitUploadHandle != k_uAPICallInvalid)
	{
		UE_LOG(
			LogAGASSSteam,
			Warning,
			TEXT("SubmitTimedRunResult ignored because another timed run submission is already in flight. currentKey=%s requestedKey=%s"),
			*InFlightTimedRunSubmitKey,
			*LeaderboardKey);
		return false;
	}

	UE_LOG(
		LogAGASSSteam,
		Display,
		TEXT("SubmitTimedRunResult starting direct Steam upload. leaderboardKey=%s timeMs=%d localUserId=%llu"),
		*LeaderboardKey,
		TimeMilliseconds,
		static_cast<unsigned long long>(SteamUser()->GetSteamID().ConvertToUint64()));

	InFlightTimedRunSubmitKey = LeaderboardKey;
	InFlightTimedRunSubmitMilliseconds = TimeMilliseconds;
	InFlightTimedRunSubmitLookupHandle = SteamUserStats()->FindOrCreateLeaderboard(
		TCHAR_TO_UTF8(*LeaderboardKey),
		k_ELeaderboardSortMethodAscending,
		k_ELeaderboardDisplayTypeTimeMilliSeconds);
	if (InFlightTimedRunSubmitLookupHandle == k_uAPICallInvalid)
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("SubmitTimedRunResult failed to start FindOrCreateLeaderboard. leaderboardKey=%s"), *LeaderboardKey);
		InFlightTimedRunSubmitKey.Reset();
		InFlightTimedRunSubmitMilliseconds = 0;
		return false;
	}
	return true;
#else
	return false;
#endif
}

bool UAGASSSteamPlatformSubsystem::QueryTimedRunFriends(const FString& LeaderboardKey)
{
	if (LeaderboardKey.IsEmpty())
	{
		return false;
	}

	if (!IsSteamPlatformActive())
	{
		TimedRunFriendsQueryStates.Add(LeaderboardKey, EAGASSSteamTimedRunFriendsQueryState::Unavailable);
		TimedRunFriendsUpdatedEvent.Broadcast(LeaderboardKey);
		return false;
	}

	if (!InFlightTimedRunLeaderboardReadKey.IsEmpty())
	{
		return InFlightTimedRunLeaderboardReadKey.Equals(LeaderboardKey, ESearchCase::IgnoreCase);
	}

	if (!InFlightTimedRunLeaderboardLookupKey.IsEmpty())
	{
		return InFlightTimedRunLeaderboardLookupKey.Equals(LeaderboardKey, ESearchCase::IgnoreCase);
	}

#if WITH_STEAMWORKS
	if (SteamUserStats() == nullptr)
	{
		TimedRunFriendsQueryStates.Add(LeaderboardKey, EAGASSSteamTimedRunFriendsQueryState::Unavailable);
		TimedRunFriendsUpdatedEvent.Broadcast(LeaderboardKey);
		return false;
	}

	TimedRunFriendsQueryStates.Add(LeaderboardKey, EAGASSSteamTimedRunFriendsQueryState::Querying);
	InFlightTimedRunLeaderboardLookupKey = LeaderboardKey;
	InFlightTimedRunLeaderboardLookupHandle = SteamUserStats()->FindLeaderboard(TCHAR_TO_UTF8(*LeaderboardKey));
	if (InFlightTimedRunLeaderboardLookupHandle == k_uAPICallInvalid)
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("QueryTimedRunFriends failed to start leaderboard lookup. key=%s"), *LeaderboardKey);
		InFlightTimedRunLeaderboardLookupKey.Reset();
		InFlightTimedRunLeaderboardLookupHandle = k_uAPICallInvalid;
		TimedRunFriendsQueryStates.Add(LeaderboardKey, EAGASSSteamTimedRunFriendsQueryState::Failed);
		TimedRunFriendsUpdatedEvent.Broadcast(LeaderboardKey);
		return false;
	}

	UE_LOG(LogAGASSSteam, Display, TEXT("QueryTimedRunFriends started leaderboard lookup. key=%s"), *LeaderboardKey);
	return true;
#else
	TimedRunFriendsQueryStates.Add(LeaderboardKey, EAGASSSteamTimedRunFriendsQueryState::Unavailable);
	TimedRunFriendsUpdatedEvent.Broadcast(LeaderboardKey);
	return false;
#endif
}

const TArray<FAGASSSteamTimedRunFriendEntry>* UAGASSSteamPlatformSubsystem::FindCachedTimedRunFriends(const FString& LeaderboardKey) const
{
	return TimedRunFriendsCache.Find(LeaderboardKey);
}

EAGASSSteamTimedRunFriendsQueryState UAGASSSteamPlatformSubsystem::GetTimedRunFriendsQueryState(const FString& LeaderboardKey) const
{
	if (const EAGASSSteamTimedRunFriendsQueryState* const QueryState = TimedRunFriendsQueryStates.Find(LeaderboardKey))
	{
		return *QueryState;
	}

	return EAGASSSteamTimedRunFriendsQueryState::Idle;
}

bool UAGASSSteamPlatformSubsystem::StartTimedRunFriendsRead(const FString& LeaderboardKey, const uint64 LeaderboardHandle)
{
#if WITH_STEAMWORKS
	if (SteamUserStats() == nullptr)
	{
		CompleteTimedRunFriendsQuery(LeaderboardKey, EAGASSSteamTimedRunFriendsQueryState::Unavailable, true);
		return false;
	}

	UE_LOG(
		LogAGASSSteam,
		Display,
		TEXT("StartTimedRunFriendsRead starting direct Steam friends download. key=%s leaderboardHandle=%llu"),
		*LeaderboardKey,
		static_cast<unsigned long long>(LeaderboardHandle));

	InFlightTimedRunLeaderboardReadKey = LeaderboardKey;
	InFlightTimedRunLeaderboardReadLeaderboardHandle = LeaderboardHandle;
	InFlightTimedRunLeaderboardReadHandle = SteamUserStats()->DownloadLeaderboardEntries(
		static_cast<SteamLeaderboard_t>(LeaderboardHandle),
		k_ELeaderboardDataRequestFriends,
		0,
		0);
	if (InFlightTimedRunLeaderboardReadHandle == k_uAPICallInvalid)
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("StartTimedRunFriendsRead failed to start direct Steam friends download. key=%s"), *LeaderboardKey);
		InFlightTimedRunLeaderboardReadKey.Reset();
		InFlightTimedRunLeaderboardReadLeaderboardHandle = 0;
		InFlightTimedRunLeaderboardReadHandle = 0;
		CompleteTimedRunFriendsQuery(LeaderboardKey, EAGASSSteamTimedRunFriendsQueryState::Failed, true);
		return false;
	}
	return true;
#else
	return false;
#endif
}

void UAGASSSteamPlatformSubsystem::TickTimedRunFriendsQuery()
{
#if WITH_STEAMWORKS
	if (InFlightTimedRunLeaderboardLookupHandle == k_uAPICallInvalid || InFlightTimedRunLeaderboardLookupKey.IsEmpty())
	{
		return;
	}

	ISteamUtils* const SteamUtilsInterface = SteamUtils();
	if (SteamUtilsInterface == nullptr)
	{
		return;
	}

	bool bFailedCall = false;
	if (!SteamUtilsInterface->IsAPICallCompleted(InFlightTimedRunLeaderboardLookupHandle, &bFailedCall))
	{
		return;
	}

	LeaderboardFindResult_t CallbackResults;
	FMemory::Memzero(CallbackResults);

	bool bFailedResult = false;
	const bool bSuccessCallResult = SteamUtilsInterface->GetAPICallResult(
		InFlightTimedRunLeaderboardLookupHandle,
		&CallbackResults,
		sizeof(CallbackResults),
		CallbackResults.k_iCallback,
		&bFailedResult);

	const FString CompletedKey = InFlightTimedRunLeaderboardLookupKey;
	InFlightTimedRunLeaderboardLookupHandle = k_uAPICallInvalid;
	InFlightTimedRunLeaderboardLookupKey.Reset();

	const bool bLeaderboardFound =
		bSuccessCallResult &&
		!bFailedCall &&
		!bFailedResult &&
		CallbackResults.m_bLeaderboardFound != 0 &&
		CallbackResults.m_hSteamLeaderboard != -1;

	UE_LOG(
		LogAGASSSteam,
		Display,
		TEXT("TickTimedRunFriendsQuery completed lookup. key=%s found=%d failedCall=%d failedResult=%d callResult=%d handle=%lld"),
		*CompletedKey,
		bLeaderboardFound ? 1 : 0,
		bFailedCall ? 1 : 0,
		bFailedResult ? 1 : 0,
		bSuccessCallResult ? 1 : 0,
		static_cast<long long>(CallbackResults.m_hSteamLeaderboard));

	if (!bLeaderboardFound)
	{
		CompleteTimedRunFriendsQuery(CompletedKey, EAGASSSteamTimedRunFriendsQueryState::Ready, true);
		return;
	}

	StartTimedRunFriendsRead(CompletedKey, static_cast<uint64>(CallbackResults.m_hSteamLeaderboard));
#endif
}

void UAGASSSteamPlatformSubsystem::TickTimedRunFriendsRead()
{
#if WITH_STEAMWORKS
	if (InFlightTimedRunLeaderboardReadHandle == k_uAPICallInvalid || InFlightTimedRunLeaderboardReadKey.IsEmpty())
	{
		return;
	}

	ISteamUtils* const SteamUtilsInterface = SteamUtils();
	ISteamUserStats* const SteamUserStatsInterface = SteamUserStats();
	if (SteamUtilsInterface == nullptr || SteamUserStatsInterface == nullptr)
	{
		return;
	}

	bool bFailedCall = false;
	if (!SteamUtilsInterface->IsAPICallCompleted(InFlightTimedRunLeaderboardReadHandle, &bFailedCall))
	{
		return;
	}

	LeaderboardScoresDownloaded_t CallbackResults;
	FMemory::Memzero(CallbackResults);

	bool bFailedResult = false;
	const bool bSuccessCallResult = SteamUtilsInterface->GetAPICallResult(
		InFlightTimedRunLeaderboardReadHandle,
		&CallbackResults,
		sizeof(CallbackResults),
		CallbackResults.k_iCallback,
		&bFailedResult);

	const FString CompletedKey = InFlightTimedRunLeaderboardReadKey;
	const uint64 CompletedLeaderboardHandle = InFlightTimedRunLeaderboardReadLeaderboardHandle;
	InFlightTimedRunLeaderboardReadHandle = k_uAPICallInvalid;
	InFlightTimedRunLeaderboardReadLeaderboardHandle = 0;
	InFlightTimedRunLeaderboardReadKey.Reset();

	const bool bDownloadSucceeded =
		bSuccessCallResult &&
		!bFailedCall &&
		!bFailedResult &&
		CallbackResults.m_hSteamLeaderboard != -1 &&
		CallbackResults.m_hSteamLeaderboardEntries != -1;

	UE_LOG(
		LogAGASSSteam,
		Display,
		TEXT("TickTimedRunFriendsRead completed download. key=%s success=%d failedCall=%d failedResult=%d callResult=%d entryCount=%d leaderboardHandle=%llu"),
		*CompletedKey,
		bDownloadSucceeded ? 1 : 0,
		bFailedCall ? 1 : 0,
		bFailedResult ? 1 : 0,
		bSuccessCallResult ? 1 : 0,
		CallbackResults.m_cEntryCount,
		static_cast<unsigned long long>(CompletedLeaderboardHandle));

	TArray<FAGASSSteamTimedRunFriendEntry>& CachedEntries = TimedRunFriendsCache.FindOrAdd(CompletedKey);
	CachedEntries.Reset();

	if (!bDownloadSucceeded)
	{
		CompleteTimedRunFriendsQuery(CompletedKey, EAGASSSteamTimedRunFriendsQueryState::Failed, false);
		return;
	}

	const uint64 LocalSteamId = SteamUser() != nullptr ? SteamUser()->GetSteamID().ConvertToUint64() : 0;
	for (int32 EntryIndex = 0; EntryIndex < CallbackResults.m_cEntryCount; ++EntryIndex)
	{
		LeaderboardEntry_t LeaderboardEntry;
		FMemory::Memzero(LeaderboardEntry);
		if (!SteamUserStatsInterface->GetDownloadedLeaderboardEntry(
			CallbackResults.m_hSteamLeaderboardEntries,
			EntryIndex,
			&LeaderboardEntry,
			nullptr,
			0))
		{
			continue;
		}

		const uint64 FriendSteamId = LeaderboardEntry.m_steamIDUser.ConvertToUint64();
		if (FriendSteamId == LocalSteamId)
		{
			continue;
		}

		FAGASSSteamTimedRunFriendEntry& Entry = CachedEntries.AddDefaulted_GetRef();
		Entry.FriendId = LexToString(FriendSteamId);
		Entry.FriendDisplayName = UTF8_TO_TCHAR(SteamFriends()->GetFriendPersonaName(LeaderboardEntry.m_steamIDUser));
		Entry.TimeMilliseconds = LeaderboardEntry.m_nScore;

		UE_LOG(
			LogAGASSSteam,
			Display,
			TEXT("TickTimedRunFriendsRead row playerId=%s nick=%s rank=%d timeMs=%d"),
			*Entry.FriendId,
			*Entry.FriendDisplayName,
			LeaderboardEntry.m_nGlobalRank,
			Entry.TimeMilliseconds);
	}

	CachedEntries.Sort([](const FAGASSSteamTimedRunFriendEntry& Left, const FAGASSSteamTimedRunFriendEntry& Right)
	{
		return Left.TimeMilliseconds < Right.TimeMilliseconds;
	});

	CompleteTimedRunFriendsQuery(CompletedKey, EAGASSSteamTimedRunFriendsQueryState::Ready, false);
#endif
}

void UAGASSSteamPlatformSubsystem::TickTimedRunSubmission()
{
#if WITH_STEAMWORKS
	ISteamUtils* const SteamUtilsInterface = SteamUtils();
	ISteamUserStats* const SteamUserStatsInterface = SteamUserStats();
	if (SteamUtilsInterface == nullptr || SteamUserStatsInterface == nullptr)
	{
		return;
	}

	if (InFlightTimedRunSubmitLookupHandle != k_uAPICallInvalid)
	{
		bool bFailedCall = false;
		if (!SteamUtilsInterface->IsAPICallCompleted(InFlightTimedRunSubmitLookupHandle, &bFailedCall))
		{
			return;
		}

		LeaderboardFindResult_t CallbackResults;
		FMemory::Memzero(CallbackResults);

		bool bFailedResult = false;
		const bool bSuccessCallResult = SteamUtilsInterface->GetAPICallResult(
			InFlightTimedRunSubmitLookupHandle,
			&CallbackResults,
			sizeof(CallbackResults),
			CallbackResults.k_iCallback,
			&bFailedResult);

		InFlightTimedRunSubmitLookupHandle = k_uAPICallInvalid;

		const bool bLeaderboardFound =
			bSuccessCallResult &&
			!bFailedCall &&
			!bFailedResult &&
			CallbackResults.m_hSteamLeaderboard != -1;

		UE_LOG(
			LogAGASSSteam,
			Display,
			TEXT("TickTimedRunSubmission completed leaderboard lookup. key=%s found=%d failedCall=%d failedResult=%d callResult=%d handle=%lld"),
			*InFlightTimedRunSubmitKey,
			bLeaderboardFound ? 1 : 0,
			bFailedCall ? 1 : 0,
			bFailedResult ? 1 : 0,
			bSuccessCallResult ? 1 : 0,
			static_cast<long long>(CallbackResults.m_hSteamLeaderboard));

		if (!bLeaderboardFound)
		{
			UE_LOG(LogAGASSSteam, Warning, TEXT("TickTimedRunSubmission could not resolve leaderboard for upload. key=%s"), *InFlightTimedRunSubmitKey);
			InFlightTimedRunSubmitKey.Reset();
			InFlightTimedRunSubmitMilliseconds = 0;
			return;
		}

		InFlightTimedRunSubmitLeaderboardHandle = static_cast<uint64>(CallbackResults.m_hSteamLeaderboard);
		InFlightTimedRunSubmitUploadHandle = SteamUserStatsInterface->UploadLeaderboardScore(
			static_cast<SteamLeaderboard_t>(InFlightTimedRunSubmitLeaderboardHandle),
			k_ELeaderboardUploadScoreMethodKeepBest,
			InFlightTimedRunSubmitMilliseconds,
			nullptr,
			0);
		if (InFlightTimedRunSubmitUploadHandle == k_uAPICallInvalid)
		{
			UE_LOG(LogAGASSSteam, Warning, TEXT("TickTimedRunSubmission failed to start score upload. key=%s timeMs=%d"), *InFlightTimedRunSubmitKey, InFlightTimedRunSubmitMilliseconds);
			InFlightTimedRunSubmitKey.Reset();
			InFlightTimedRunSubmitMilliseconds = 0;
			InFlightTimedRunSubmitLeaderboardHandle = 0;
		}
		return;
	}

	if (InFlightTimedRunSubmitUploadHandle == k_uAPICallInvalid)
	{
		return;
	}

	bool bFailedCall = false;
	if (!SteamUtilsInterface->IsAPICallCompleted(InFlightTimedRunSubmitUploadHandle, &bFailedCall))
	{
		return;
	}

	LeaderboardScoreUploaded_t CallbackResults;
	FMemory::Memzero(CallbackResults);

	bool bFailedResult = false;
	const bool bSuccessCallResult = SteamUtilsInterface->GetAPICallResult(
		InFlightTimedRunSubmitUploadHandle,
		&CallbackResults,
		sizeof(CallbackResults),
		CallbackResults.k_iCallback,
		&bFailedResult);

	const FString CompletedKey = InFlightTimedRunSubmitKey;
	const int32 CompletedMilliseconds = InFlightTimedRunSubmitMilliseconds;
	InFlightTimedRunSubmitUploadHandle = k_uAPICallInvalid;
	InFlightTimedRunSubmitLeaderboardHandle = 0;
	InFlightTimedRunSubmitKey.Reset();
	InFlightTimedRunSubmitMilliseconds = 0;

	const bool bUploadSucceeded =
		bSuccessCallResult &&
		!bFailedCall &&
		!bFailedResult &&
		CallbackResults.m_bSuccess != 0;

	UE_LOG(
		LogAGASSSteam,
		Display,
		TEXT("TickTimedRunSubmission completed score upload. key=%s success=%d failedCall=%d failedResult=%d callResult=%d score=%d changed=%d newRank=%d previousRank=%d"),
		*CompletedKey,
		bUploadSucceeded ? 1 : 0,
		bFailedCall ? 1 : 0,
		bFailedResult ? 1 : 0,
		bSuccessCallResult ? 1 : 0,
		CompletedMilliseconds,
		CallbackResults.m_bScoreChanged ? 1 : 0,
		CallbackResults.m_nGlobalRankNew,
		CallbackResults.m_nGlobalRankPrevious);
#endif
}

void UAGASSSteamPlatformSubsystem::CompleteTimedRunFriendsQuery(
	const FString& LeaderboardKey,
	const EAGASSSteamTimedRunFriendsQueryState QueryState,
	const bool bResetCachedEntries)
{
	if (bResetCachedEntries)
	{
		TArray<FAGASSSteamTimedRunFriendEntry>& CachedEntries = TimedRunFriendsCache.FindOrAdd(LeaderboardKey);
		CachedEntries.Reset();
	}

	TimedRunFriendsQueryStates.Add(LeaderboardKey, QueryState);
	TimedRunFriendsUpdatedEvent.Broadcast(LeaderboardKey);
}

void UAGASSSteamPlatformSubsystem::HandleSessionUserInviteAccepted(
	const bool bWasSuccessful,
	const int32 ControllerId,
	FUniqueNetIdPtr UserId,
	const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("Steam invite acceptance failed. controllerId=%d userId=%s"), ControllerId, UserId.IsValid() ? *UserId->ToString() : TEXT("None"));
		return;
	}

	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SessionSubsystem->JoinResolvedSearchResult(InviteResult, NSLOCTEXT("AGASSSteam", "JoiningSteamInviteStatus", "Joining Steam invite...").ToString());
	}
}

bool UAGASSSteamPlatformSubsystem::TickStats(float DeltaTime)
{
	if (!IsSteamPlatformActive())
	{
		return true;
	}

	TickTimedRunFriendsQuery();
	TickTimedRunFriendsRead();
	TickTimedRunSubmission();
	EnsureSteamStatsRequested();
	AddPendingPlaytimeStatIfNeeded(false);
	FlushDirtyStats(false);

	return true;
}

bool UAGASSSteamPlatformSubsystem::EnsureSteamStatsRequested()
{
	if (!IsSteamPlatformActive())
	{
		return false;
	}

	if (bSteamStatsReady)
	{
		return true;
	}

#if WITH_STEAMWORKS
	if (SteamStatsCallbackBridge == nullptr)
	{
		SteamStatsCallbackBridge = new FAGASSSteamStatsCallbackBridge(*this);
	}

	if (SteamUserStats() == nullptr)
	{
		return false;
	}

	if (!bSteamStatsRequestSubmitted)
	{
		bSteamStatsRequestSubmitted = true;
		HandleSteamStatsReady(true);
	}

	return bSteamStatsReady;
#else
	return false;
#endif
}

void UAGASSSteamPlatformSubsystem::HandleSteamStatsReady(const bool bWasSuccessful)
{
	bSteamStatsRequestSubmitted = bWasSuccessful;
	bSteamStatsReady = bWasSuccessful;

	if (!bWasSuccessful)
	{
		bSteamStatsRequestSubmitted = false;
		UE_LOG(LogAGASSSteam, Warning, TEXT("Steam user stats request failed."));
		return;
	}

#if WITH_STEAMWORKS
	TrackedStatValues.Reset();
	UnlockedAchievementIds.Reset();

	if (ISteamUserStats* const SteamUserStatsInterface = SteamUserStats())
	{
		for (const TPair<FString, FAGASSSteamStatDefinition>& Pair : StatDefinitionsByName)
		{
			double Value = 0.0;
			if (Pair.Value.ValueType == EAGASSSteamStatValueType::Integer)
			{
				int32 IntValue = 0;
				if (SteamUserStatsInterface->GetStat(TCHAR_TO_UTF8(*Pair.Key), &IntValue))
				{
					Value = static_cast<double>(IntValue);
				}
			}
			else
			{
				float FloatValue = 0.f;
				if (SteamUserStatsInterface->GetStat(TCHAR_TO_UTF8(*Pair.Key), &FloatValue))
				{
					Value = static_cast<double>(FloatValue);
				}
			}

			TrackedStatValues.Add(Pair.Key, Value);
		}

		for (const FAGASSSteamAchievementDefinition& AchievementDefinition : AchievementDefinitions)
		{
			bool bUnlocked = false;
			if (SteamUserStatsInterface->GetAchievement(TCHAR_TO_UTF8(*AchievementDefinition.SteamAchievementId), &bUnlocked) && bUnlocked)
			{
				UnlockedAchievementIds.Add(AchievementDefinition.SteamAchievementId);
			}
		}
	}
#endif

	ProcessQueuedGameplayEvents();
	FlushDirtyStats(true);
}

void UAGASSSteamPlatformSubsystem::HandleSteamStatsStored(const bool bWasSuccessful, const int32 ResultCode)
{
	bStoreInFlight = false;

	if (!bWasSuccessful)
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("Steam stats store failed. resultCode=%d"), ResultCode);
		return;
	}

	LastAcknowledgedGeneration = FMath::Max(LastAcknowledgedGeneration, LastSubmittedGeneration);
	if (LastAcknowledgedGeneration == DirtyGeneration)
	{
		DirtyStatNames.Reset();
	}
}

void UAGASSSteamPlatformSubsystem::HandleSteamAchievementStored(const FString& AchievementId)
{
	if (GetSteamSettings() != nullptr && GetSteamSettings()->bLogSteamWriteOperations)
	{
		UE_LOG(LogAGASSSteam, Display, TEXT("Steam achievement stored: %s"), *AchievementId);
	}
}

void UAGASSSteamPlatformSubsystem::ProcessQueuedGameplayEvents()
{
	for (const FAGASSGameplayEvent& GameplayEvent : QueuedGameplayEvents)
	{
		ApplyGameplayEvent(GameplayEvent);
	}

	QueuedGameplayEvents.Reset();
}

void UAGASSSteamPlatformSubsystem::ApplyGameplayEvent(const FAGASSGameplayEvent& GameplayEvent)
{
	if (GameplayEvent.EventName.IsNone())
	{
		return;
	}

	GameplayEventCounts.FindOrAdd(GameplayEvent.EventName) += 1;

	if (const TArray<FAGASSSteamStatDefinition>* StatDefinitions = StatDefinitionsByEvent.Find(GameplayEvent.EventName))
	{
		for (const FAGASSSteamStatDefinition& StatDefinition : *StatDefinitions)
		{
			UpdateTrackedStat(StatDefinition, ExtractEventValue(GameplayEvent, StatDefinition));
		}
	}

	EvaluateAchievementsForEvent(GameplayEvent);
}

double UAGASSSteamPlatformSubsystem::ExtractEventValue(const FAGASSGameplayEvent& GameplayEvent, const FAGASSSteamStatDefinition& StatDefinition) const
{
	double SourceValue = 1.0;

	switch (StatDefinition.ValueSource)
	{
	case EAGASSSteamEventValueSource::IntValue:
		SourceValue = static_cast<double>(GameplayEvent.IntValue);
		break;
	case EAGASSSteamEventValueSource::FloatValue:
		SourceValue = static_cast<double>(GameplayEvent.FloatValue);
		break;
	case EAGASSSteamEventValueSource::FlagValue:
		SourceValue = GameplayEvent.bFlagValue ? 1.0 : 0.0;
		break;
	case EAGASSSteamEventValueSource::UnitValue:
	default:
		break;
	}

	return SourceValue * static_cast<double>(StatDefinition.Multiplier);
}

void UAGASSSteamPlatformSubsystem::UpdateTrackedStat(const FAGASSSteamStatDefinition& StatDefinition, const double SourceValue)
{
	if (StatDefinition.SteamStatName.IsEmpty())
	{
		return;
	}

	const double CurrentValue = GetTrackedStatValue(StatDefinition.SteamStatName);
	double NewValue = SourceValue;

	switch (StatDefinition.Aggregation)
	{
	case EAGASSSteamStatAggregation::Add:
		NewValue = CurrentValue + SourceValue;
		break;
	case EAGASSSteamStatAggregation::Maximum:
		NewValue = FMath::Max(CurrentValue, SourceValue);
		break;
	case EAGASSSteamStatAggregation::Set:
	default:
		break;
	}

	NewValue = QuantizeStatValue(StatDefinition.ValueType, NewValue);
	if (FMath::IsNearlyEqual(CurrentValue, NewValue))
	{
		return;
	}

	SetTrackedStatValue(StatDefinition.SteamStatName, NewValue);
	MarkDirtyState(&StatDefinition.SteamStatName);
	EvaluateAchievementsForStat(StatDefinition.SteamStatName);

	if (const UAGASSSteamDeveloperSettings* Settings = GetSteamSettings(); Settings != nullptr && Settings->bLogSteamWriteOperations)
	{
		UE_LOG(LogAGASSSteam, Display, TEXT("Tracked Steam stat update stat=%s value=%f"), *StatDefinition.SteamStatName, NewValue);
	}
}

void UAGASSSteamPlatformSubsystem::EvaluateAchievementsForEvent(const FAGASSGameplayEvent& GameplayEvent)
{
	for (const FAGASSSteamAchievementDefinition& AchievementDefinition : AchievementDefinitions)
	{
		if (UnlockedAchievementIds.Contains(AchievementDefinition.SteamAchievementId))
		{
			continue;
		}

		switch (AchievementDefinition.TriggerType)
		{
		case EAGASSSteamAchievementTriggerType::GameplayEventCountAtLeast:
			if (AchievementDefinition.SourceEventName == GameplayEvent.EventName
				&& GameplayEventCounts.FindRef(AchievementDefinition.SourceEventName) >= AchievementDefinition.RequiredEventCount)
			{
				TryUnlockAchievement(AchievementDefinition.SteamAchievementId);
			}
			break;
		case EAGASSSteamAchievementTriggerType::GameplayEventFlagTrue:
			if (AchievementDefinition.SourceEventName == GameplayEvent.EventName && GameplayEvent.bFlagValue)
			{
				TryUnlockAchievement(AchievementDefinition.SteamAchievementId);
			}
			break;
		case EAGASSSteamAchievementTriggerType::StatValueAtLeast:
		default:
			break;
		}
	}
}

void UAGASSSteamPlatformSubsystem::EvaluateAchievementsForStat(const FString& SteamStatName)
{
	const double CurrentValue = GetTrackedStatValue(SteamStatName);

	for (const FAGASSSteamAchievementDefinition& AchievementDefinition : AchievementDefinitions)
	{
		if (AchievementDefinition.TriggerType == EAGASSSteamAchievementTriggerType::StatValueAtLeast
			&& AchievementDefinition.SourceStatName.Equals(SteamStatName, ESearchCase::IgnoreCase)
			&& CurrentValue >= AchievementDefinition.RequiredStatValue)
		{
			TryUnlockAchievement(AchievementDefinition.SteamAchievementId);
		}
	}
}

void UAGASSSteamPlatformSubsystem::TryUnlockAchievement(const FString& AchievementId)
{
	if (AchievementId.IsEmpty() || UnlockedAchievementIds.Contains(AchievementId))
	{
		return;
	}

	UnlockedAchievementIds.Add(AchievementId);
	MarkDirtyState();

	if (const UAGASSSteamDeveloperSettings* Settings = GetSteamSettings(); Settings != nullptr && Settings->bLogSteamWriteOperations)
	{
		UE_LOG(LogAGASSSteam, Display, TEXT("Queued Steam achievement unlock achievement=%s"), *AchievementId);
	}
}

double UAGASSSteamPlatformSubsystem::GetTrackedStatValue(const FString& SteamStatName) const
{
	if (const double* CurrentValue = TrackedStatValues.Find(SteamStatName))
	{
		return *CurrentValue;
	}

	return 0.0;
}

void UAGASSSteamPlatformSubsystem::SetTrackedStatValue(const FString& SteamStatName, const double NewValue)
{
	TrackedStatValues.Add(SteamStatName, NewValue);
}

void UAGASSSteamPlatformSubsystem::SyncInFlightPlaytime()
{
	if (!bTrackingSessionPlaytime)
	{
		return;
	}

	const double NowSeconds = GetNowSeconds();
	if (SessionPlaytimeTrackingStartSeconds > 0.0)
	{
		PendingPlaytimeSeconds += FMath::Max(0.0, NowSeconds - SessionPlaytimeTrackingStartSeconds);
	}

	SessionPlaytimeTrackingStartSeconds = NowSeconds;
}

void UAGASSSteamPlatformSubsystem::StartSessionPlaytimeTracking()
{
	if (bTrackingSessionPlaytime)
	{
		return;
	}

	bTrackingSessionPlaytime = true;
	SessionPlaytimeTrackingStartSeconds = GetNowSeconds();
}

void UAGASSSteamPlatformSubsystem::StopSessionPlaytimeTracking(const bool bFlushBeforeStop)
{
	if (!bTrackingSessionPlaytime)
	{
		return;
	}

	SyncInFlightPlaytime();
	bTrackingSessionPlaytime = false;
	SessionPlaytimeTrackingStartSeconds = 0.0;

	if (bFlushBeforeStop)
	{
		AddPendingPlaytimeStatIfNeeded(true);
	}
}

void UAGASSSteamPlatformSubsystem::AddPendingPlaytimeStatIfNeeded(const bool bForce)
{
	SyncInFlightPlaytime();

	const UAGASSSteamDeveloperSettings* Settings = GetSteamSettings();
	if (Settings == nullptr
		|| !Settings->PlaytimeTracking.bAccumulateSessionMinutesToSteamStat
		|| Settings->PlaytimeTracking.SteamStatName.IsEmpty()
		|| PendingPlaytimeSeconds <= 0.0)
	{
		return;
	}

	if (!bForce && PendingPlaytimeSeconds < Settings->PlaytimeTracking.MinimumSecondsPerWrite)
	{
		return;
	}

	const FAGASSSteamStatDefinition* PlaytimeStatDefinition = StatDefinitionsByName.Find(Settings->PlaytimeTracking.SteamStatName);
	if (PlaytimeStatDefinition == nullptr)
	{
		return;
	}

	const double PendingMinutes = PendingPlaytimeSeconds / 60.0;
	PendingPlaytimeSeconds = 0.0;
	UpdateTrackedStat(*PlaytimeStatDefinition, PendingMinutes);
}

bool UAGASSSteamPlatformSubsystem::SyncTrackedStateToSteam() const
{
#if WITH_STEAMWORKS
	ISteamUserStats* const SteamUserStatsInterface = SteamUserStats();
	if (SteamUserStatsInterface == nullptr)
	{
		return false;
	}

	bool bAnyWriteSucceeded = false;

	for (const TPair<FString, FAGASSSteamStatDefinition>& Pair : StatDefinitionsByName)
	{
		if (const double* TrackedValue = TrackedStatValues.Find(Pair.Key))
		{
			bAnyWriteSucceeded |= TryWriteSteamStat(Pair.Value, *TrackedValue);
		}
	}

	for (const FString& AchievementId : UnlockedAchievementIds)
	{
		bAnyWriteSucceeded |= SteamUserStatsInterface->SetAchievement(TCHAR_TO_UTF8(*AchievementId));
	}

	return bAnyWriteSucceeded;
#else
	return false;
#endif
}

bool UAGASSSteamPlatformSubsystem::TryWriteSteamStat(const FAGASSSteamStatDefinition& StatDefinition, const double Value) const
{
#if WITH_STEAMWORKS
	ISteamUserStats* const SteamUserStatsInterface = SteamUserStats();
	if (SteamUserStatsInterface == nullptr || StatDefinition.SteamStatName.IsEmpty())
	{
		return false;
	}

	if (StatDefinition.ValueType == EAGASSSteamStatValueType::Integer)
	{
		return SteamUserStatsInterface->SetStat(TCHAR_TO_UTF8(*StatDefinition.SteamStatName), FMath::RoundToInt(static_cast<float>(Value)));
	}

	return SteamUserStatsInterface->SetStat(TCHAR_TO_UTF8(*StatDefinition.SteamStatName), static_cast<float>(Value));
#else
	return false;
#endif
}

void UAGASSSteamPlatformSubsystem::FlushDirtyStats(const bool bForce)
{
	if (!IsSteamPlatformActive())
	{
		return;
	}

	AddPendingPlaytimeStatIfNeeded(bForce);

	if (!bSteamStatsReady || bStoreInFlight || !HasDirtyState())
	{
		return;
	}

	const UAGASSSteamDeveloperSettings* Settings = GetSteamSettings();
	if (Settings == nullptr)
	{
		return;
	}

	const double NowSeconds = GetNowSeconds();
	if (!bForce && NowSeconds - LastStatsStoreTimeSeconds < Settings->StoreStatsIntervalSeconds)
	{
		return;
	}

#if WITH_STEAMWORKS
	if (SteamUserStats() == nullptr)
	{
		return;
	}

	SyncTrackedStateToSteam();
	if (!SteamUserStats()->StoreStats())
	{
		UE_LOG(LogAGASSSteam, Warning, TEXT("SteamUserStats()->StoreStats() failed."));
		return;
	}

	bStoreInFlight = true;
	LastSubmittedGeneration = DirtyGeneration;
	LastStatsStoreTimeSeconds = NowSeconds;
#endif
}

double UAGASSSteamPlatformSubsystem::GetNowSeconds() const
{
	return FPlatformTime::Seconds();
}

bool UAGASSSteamPlatformSubsystem::HasDirtyState() const
{
	return DirtyGeneration > LastAcknowledgedGeneration;
}

void UAGASSSteamPlatformSubsystem::MarkDirtyState(const FString* DirtyStatName)
{
	++DirtyGeneration;
	if (DirtyStatName != nullptr)
	{
		DirtyStatNames.Add(*DirtyStatName);
	}
}
