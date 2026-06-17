#include "Subsystems/EMRLobbyInviteCodeSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Subsystems/EMRLoadingScreenSubsystem.h"

const FName UEMRLobbyInviteCodeSubsystem::LobbyInviteCodeSessionKey(TEXT("LobbyInviteCode"));
const FName UEMRLobbyInviteCodeSubsystem::LobbyHostPortSessionKey(TEXT("LobbyHostPort"));

void UEMRLobbyInviteCodeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UEMRLoadingScreenSubsystem* LoadingScreenSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLoadingScreenSubsystem>() : nullptr)
	{
		LoadingScreenSubsystem->RegisterLoadingProcessor(this);
	}

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
                this,
                &ThisClass::HandlePostLoadMap);

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(
                        this,
                        &ThisClass::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(
                        this,
                        &ThisClass::HandleTravelFailure);
	}
}

void UEMRLobbyInviteCodeSubsystem::Deinitialize()
{
	if (UEMRLoadingScreenSubsystem* LoadingScreenSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLoadingScreenSubsystem>() : nullptr)
	{
		LoadingScreenSubsystem->UnregisterLoadingProcessor(this);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	if (GEngine)
	{
		if (NetworkFailureHandle.IsValid())
		{
			GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		}
		if (TravelFailureHandle.IsValid())
		{
			GEngine->OnTravelFailure().Remove(TravelFailureHandle);
		}
	}

	Super::Deinitialize();
}

bool UEMRLobbyInviteCodeSubsystem::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (JoinState == ELobbyInviteJoinState::DestroyingExistingSession)
	{
		OutReason = TEXT("Destroying existing session before invite join");
		return true;
	}

	if (JoinState == ELobbyInviteJoinState::Searching)
	{
		OutReason = TEXT("Searching invite session");
		return true;
	}

	if (JoinState == ELobbyInviteJoinState::Joining)
	{
		OutReason = TEXT("Joining invite session");
		return true;
	}

	if (bAwaitingJoinTravel)
	{
		OutReason = TEXT("Awaiting invite join travel completion");
		return true;
	}

	return false;
}

FString UEMRLobbyInviteCodeSubsystem::GenerateInviteCode(int32 Length) const
{
	(void)Length;

	FString Result;
	Result.Reserve(InviteCodeLength);

	for (int32 Index = 0; Index < InviteCodeLength; ++Index)
	{
		const int32 Digit = FMath::RandRange(0, 9);
		Result.AppendChar(static_cast<TCHAR>(TEXT('0') + Digit));
	}

	return Result;
}

bool UEMRLobbyInviteCodeSubsystem::IsInviteCodeValid(const FString& Code) const
{
	if (Code.Len() != InviteCodeLength)
	{
		return false;
	}

	for (int32 Index = 0; Index < InviteCodeLength; ++Index)
	{
		if (!FChar::IsDigit(Code[Index]))
		{
			return false;
		}
	}

	return true;
}

bool UEMRLobbyInviteCodeSubsystem::GetCurrentSessionInviteCode(FString& OutCode) const
{
	OutCode.Reset();

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		return false;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	FOnlineSessionSettings* SessionSettings = SessionInterface->GetSessionSettings(GetLobbySessionName());
	if (!SessionSettings)
	{
		return false;
	}

	FString FoundCode;
	if (!SessionSettings->Get(LobbyInviteCodeSessionKey, FoundCode))
	{
		return false;
	}

	FoundCode = FoundCode.TrimStartAndEnd();
	if (!IsInviteCodeValid(FoundCode))
	{
		return false;
	}

	OutCode = MoveTemp(FoundCode);
	return true;
}

bool UEMRLobbyInviteCodeSubsystem::UpdateSessionInviteCode(const FString& Code)
{
	const FString TrimmedCode = Code.TrimStartAndEnd();
	if (!IsInviteCodeValid(TrimmedCode))
	{
		return false;
	}

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		return false;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	FOnlineSessionSettings* SessionSettings = SessionInterface->GetSessionSettings(GetLobbySessionName());
	if (!SessionSettings)
	{
		return false;
	}

	SessionSettings->Set(LobbyInviteCodeSessionKey, TrimmedCode, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	return SessionInterface->UpdateSession(GetLobbySessionName(), *SessionSettings, true);
}

void UEMRLobbyInviteCodeSubsystem::FindAndJoinSessionByInviteCode(const FString& Code, APlayerController* PlayerController)
{
	const FString TrimmedCode = Code.TrimStartAndEnd();
	UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin] Request Code=%s PC=%s"), *TrimmedCode, *GetNameSafe(PlayerController));

	if (!IsInviteCodeValid(TrimmedCode))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin] Invalid code: %s"), *TrimmedCode);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::InvalidCode);
		return;
	}

	StartJoinRequest(TrimmedCode, PlayerController);
}

void UEMRLobbyInviteCodeSubsystem::StartJoinRequest(const FString& Code, APlayerController* PlayerController)
{
	FLobbyInviteJoinRequest NewRequest;
	NewRequest.RequestId = NextRequestId++;
	NewRequest.Code = Code;
	NewRequest.PlayerController = PlayerController;

	if (JoinState == ELobbyInviteJoinState::Joining)
	{
		DeferredRequest = NewRequest;
		bHasDeferredRequest = true;
		UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] Join in progress. Deferring request %u"),
			ActiveRequest.RequestId,
			NewRequest.RequestId);
		return;
	}

	if (JoinState == ELobbyInviteJoinState::Searching)
	{
		DeferredRequest = NewRequest;
		bHasDeferredRequest = true;
		bSearchRestartPending = true;
		UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] Search in progress. Cancelling and deferring request %u"),
			ActiveRequest.RequestId,
			NewRequest.RequestId);

		IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
		IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
		if (SessionInterface.IsValid())
		{
			CancelActiveSearch(SessionInterface);
		}
		return;
	}

	if (JoinState == ELobbyInviteJoinState::DestroyingExistingSession)
	{
		ActiveRequest = NewRequest;
		PendingInviteJoinResult = FOnlineSessionSearchResult();
		UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] Updated request while destroying"), ActiveRequest.RequestId);
		return;
	}

	ActiveRequest = NewRequest;
	PendingInviteJoinResult = FOnlineSessionSearchResult();

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Missing OnlineSubsystem"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::SearchFailed);
		ResetInviteJoinState(true);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Invalid SessionInterface"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::SearchFailed);
		ResetInviteJoinState(true);
		return;
	}

	if (SessionInterface->GetNamedSession(GetLobbySessionName()) != nullptr)
	{
		BeginDestroyExistingSession(SessionInterface);
		return;
	}

	StartFindSessions(ActiveRequest.Code, ActiveRequest.PlayerController.Get());
}

void UEMRLobbyInviteCodeSubsystem::CancelActiveSearch(const IOnlineSessionPtr& SessionInterface)
{
	if (!InviteCodeSearch.IsValid())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] Cancelling active search"), ActiveRequest.RequestId);
	SessionInterface->CancelFindSessions();
}

void UEMRLobbyInviteCodeSubsystem::BeginDestroyExistingSession(const IOnlineSessionPtr& SessionInterface)
{
	JoinState = ELobbyInviteJoinState::DestroyingExistingSession;
	UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] Destroying existing session before join"), ActiveRequest.RequestId);

	if (DestroySessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}

	DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));

	if (!SessionInterface->DestroySession(GetLobbySessionName()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] DestroySession failed to start"), ActiveRequest.RequestId);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		ResetInviteJoinState(true);
	}
}

void UEMRLobbyInviteCodeSubsystem::PromoteDeferredRequest()
{
	if (!bHasDeferredRequest)
	{
		return;
	}

	ActiveRequest = DeferredRequest;
	DeferredRequest = FLobbyInviteJoinRequest();
	bHasDeferredRequest = false;
}

void UEMRLobbyInviteCodeSubsystem::StartFindSessions(const FString& Code, APlayerController* PlayerController)
{
	UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] StartFindSessions Code=%s PC=%s"),
		ActiveRequest.RequestId,
		*Code,
		*GetNameSafe(PlayerController));

	JoinState = ELobbyInviteJoinState::Searching;

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Missing OnlineSubsystem in StartFindSessions"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::SearchFailed);
		ResetInviteJoinState(true);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Invalid SessionInterface in StartFindSessions"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::SearchFailed);
		ResetInviteJoinState(true);
		return;
	}

	if (InviteCodeSearch.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] Clearing previous search"), ActiveRequest.RequestId);
		InviteCodeSearch.Reset();
	}

	InviteCodeSearch = MakeShared<FOnlineSessionSearch>();
	InviteCodeSearch->MaxSearchResults = 50;

	const FName SubsystemName = Subsystem->GetSubsystemName();
	const FName NullSubsystemName(TEXT("NULL"));
	const FName LanSubsystemName(TEXT("LAN"));
	const bool bIsLanQuery = (SubsystemName == NAME_None || SubsystemName == NullSubsystemName || SubsystemName == LanSubsystemName);
	InviteCodeSearch->bIsLanQuery = bIsLanQuery;
	InviteCodeSearch->QuerySettings.Set(LobbyInviteCodeSessionKey, Code, EOnlineComparisonOp::Equals);
	if (!bIsLanQuery)
	{
		const FName SearchPresenceKey(TEXT("SEARCH_PRESENCE"));
		InviteCodeSearch->QuerySettings.Set(SearchPresenceKey, true, EOnlineComparisonOp::Equals);
	}

	if (FindSessionsCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));

	const int32 LocalUserNum = PlayerController && PlayerController->GetLocalPlayer()
	? PlayerController->GetLocalPlayer()->GetControllerId()
	: 0;

	if (!SessionInterface->FindSessions(LocalUserNum, InviteCodeSearch.ToSharedRef()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] FindSessions failed to start"), ActiveRequest.RequestId);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::SearchFailed);
		ResetInviteJoinState(true);
	}
}

void UEMRLobbyInviteCodeSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] DestroySessionComplete %s Success=%d"),
		ActiveRequest.RequestId,
		*SessionName.ToString(),
		bWasSuccessful);

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] DestroySession failed"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		ResetInviteJoinState(true);
		return;
	}

	if (PendingInviteJoinResult.IsValid())
	{
		const FOnlineSessionSearchResult Result = PendingInviteJoinResult;
		PendingInviteJoinResult = FOnlineSessionSearchResult();
		JoinFoundSession(Result);
		return;
	}

	StartFindSessions(ActiveRequest.Code, ActiveRequest.PlayerController.Get());
}

void UEMRLobbyInviteCodeSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] FindSessionsComplete Success=%d Results=%d"),
		ActiveRequest.RequestId,
		bWasSuccessful,
		InviteCodeSearch.IsValid() ? InviteCodeSearch->SearchResults.Num() : -1);

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
	}

	if (bSearchRestartPending)
	{
		UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] Search restart pending; ignoring results"), ActiveRequest.RequestId);
		bSearchRestartPending = false;
		InviteCodeSearch.Reset();
		ResetInviteJoinState(false);
		PromoteDeferredRequest();
		if (ActiveRequest.RequestId != 0)
		{
			StartJoinRequest(ActiveRequest.Code, ActiveRequest.PlayerController.Get());
		}
		return;
	}

	if (!bWasSuccessful || !InviteCodeSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] FindSessions failed or search invalid"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::SearchFailed);
		InviteCodeSearch.Reset();
		ResetInviteJoinState(true);
		return;
	}

	FOnlineSessionSearchResult FoundResult;
	int32 MatchCount = 0;
	for (const FOnlineSessionSearchResult& Result : InviteCodeSearch->SearchResults)
	{
		FString FoundCode;
		if (Result.Session.SessionSettings.Get(LobbyInviteCodeSessionKey, FoundCode) && FoundCode == ActiveRequest.Code)
		{
			++MatchCount;
			if (!FoundResult.IsValid())
			{
				FoundResult = Result;
			}
		}
	}

	if (!FoundResult.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] No session found for code %s"), ActiveRequest.RequestId, *ActiveRequest.Code);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::NotFound);
		InviteCodeSearch.Reset();
		ResetInviteJoinState(true);
		return;
	}

	if (MatchCount > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Multiple sessions matched code %s (%d results). Joining first."),
			ActiveRequest.RequestId,
			*ActiveRequest.Code,
			MatchCount);
	}

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] SessionInterface invalid in HandleFindSessionsComplete"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		InviteCodeSearch.Reset();
		ResetInviteJoinState(true);
		return;
	}

	if (SessionInterface->GetNamedSession(GetLobbySessionName()) != nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] Existing session detected after search. Destroying before join."), ActiveRequest.RequestId);
		PendingInviteJoinResult = FoundResult;
		BeginDestroyExistingSession(SessionInterface);
		return;
	}

	JoinFoundSession(FoundResult);
}

void UEMRLobbyInviteCodeSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] JoinSessionComplete %s Result=%d"),
		ActiveRequest.RequestId,
		*SessionName.ToString(),
		static_cast<int32>(Result));

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}

	InviteCodeSearch.Reset();

	if (Result == EOnJoinSessionCompleteResult::SessionIsFull)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] JoinSession failed: session is full"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::LobbyFull);
		ResetInviteJoinState(true);
		return;
	}

	if (Result != EOnJoinSessionCompleteResult::Success || !SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] JoinSession failed Result=%d"), ActiveRequest.RequestId, static_cast<int32>(Result));
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		const bool bRestartDeferred = bHasDeferredRequest;
		const FLobbyInviteJoinRequest RestartRequest = bRestartDeferred ? DeferredRequest : FLobbyInviteJoinRequest();

		ResetInviteJoinState(true);

		if (bRestartDeferred)
		{
			StartJoinRequest(RestartRequest.Code, RestartRequest.PlayerController.Get());
		}
		return;
	}

	APlayerController* JoinPlayerController = ActiveRequest.PlayerController.Get();
	if (!JoinPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Pending player controller invalid"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		ResetInviteJoinState(true);
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] ResolveConnectString failed"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		ResetInviteJoinState(true);
		return;
	}

	if (ConnectString.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] ResolveConnectString returned empty string"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		ResetInviteJoinState(true);
		return;
	}

	const FName SubsystemName = Subsystem ? Subsystem->GetSubsystemName() : NAME_None;
	const FName NullSubsystemName(TEXT("NULL"));
	const FName LanSubsystemName(TEXT("LAN"));
	const bool bIsLan = (SubsystemName == NAME_None || SubsystemName == NullSubsystemName || SubsystemName == LanSubsystemName);

	// If we got a :0 port, ClientTravel will fail. For LAN/NULL testing, allow a configured fallback.
	FString HostPart;
	FString PortPart;
	if (ConnectString.Split(TEXT(":"), &HostPart, &PortPart) && PortPart == TEXT("0") && bIsLan)
	{
		int32 SessionPortOverride = 0;
		if (ActiveJoinResult.IsValid())
		{
			ActiveJoinResult.Session.SessionSettings.Get(LobbyHostPortSessionKey, SessionPortOverride);
		}

		const int32 DesiredPort = SessionPortOverride > 0 ? SessionPortOverride : FallbackJoinPort;
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] ConnectString has port 0 (%s). Applying LAN fallback port %d."),
                        ActiveRequest.RequestId,
                        *ConnectString,
                        DesiredPort);
		ConnectString = FString::Printf(TEXT("%s:%d"), *HostPart, DesiredPort);
	}

	// Also try to resolve using the beacon port if configured (some subsystems store the real port there).
	if (ConnectString.EndsWith(TEXT(":0")))
	{
		FString BeaconConnectString;
		if (SessionInterface->GetResolvedConnectString(SessionName, BeaconConnectString, NAME_BeaconPort) && !BeaconConnectString.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] Beacon connect string resolved as %s"), ActiveRequest.RequestId, *BeaconConnectString);
			ConnectString = BeaconConnectString;
		}
	}

	if (ConnectString.EndsWith(TEXT(":0")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Unusable connect string after fallbacks: %s (OSS=%s)"),
			ActiveRequest.RequestId,
			*ConnectString,
			*SubsystemName.ToString());
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		ResetInviteJoinState(true);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[InviteCodeJoin][ReqId=%u] ClientTravel to %s (Absolute, non-seamless)"), ActiveRequest.RequestId, *ConnectString);
	JoinPlayerController->ClientTravel(ConnectString, TRAVEL_Absolute, false, FGuid());
	bAwaitingJoinTravel = true;
}

void UEMRLobbyInviteCodeSubsystem::JoinFoundSession(const FOnlineSessionSearchResult& Result)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Invalid SessionInterface in JoinFoundSession"), ActiveRequest.RequestId);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		ResetInviteJoinState(true);
		return;
	}

	JoinState = ELobbyInviteJoinState::Joining;
	ActiveJoinResult = Result;

	if (JoinSessionCompleteHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}

	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));

	const int32 LocalUserNum = ActiveRequest.PlayerController.IsValid() && ActiveRequest.PlayerController->GetLocalPlayer()
	? ActiveRequest.PlayerController->GetLocalPlayer()->GetControllerId()
	: 0;

	if (!SessionInterface->JoinSession(LocalUserNum, GetLobbySessionName(), Result))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] JoinSession failed to start"), ActiveRequest.RequestId);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
		InviteCodeSearch.Reset();
		ResetInviteJoinState(true);
	}
}

void UEMRLobbyInviteCodeSubsystem::ResetInviteJoinState(bool bClearDeferred)
{
	JoinState = ELobbyInviteJoinState::Idle;
	InviteCodeSearch.Reset();
	ActiveRequest = FLobbyInviteJoinRequest();
	PendingInviteJoinResult = FOnlineSessionSearchResult();
	ActiveJoinResult = FOnlineSessionSearchResult();
	bSearchRestartPending = false;
	bAwaitingJoinTravel = false;

	if (bClearDeferred)
	{
		DeferredRequest = FLobbyInviteJoinRequest();
		bHasDeferredRequest = false;
	}
}

void UEMRLobbyInviteCodeSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	(void)LoadedWorld;

	if (!bAwaitingJoinTravel || JoinState != ELobbyInviteJoinState::Joining)
	{
		return;
	}

	OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::Success);
	ResetInviteJoinState(true);
}

void UEMRLobbyInviteCodeSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver,
        ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	(void)World;
	(void)NetDriver;
	(void)FailureType;

	if (!bAwaitingJoinTravel || JoinState != ELobbyInviteJoinState::Joining)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Network failure: %s"),
                ActiveRequest.RequestId,
                *ErrorString);
	OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
	ResetInviteJoinState(true);
}

void UEMRLobbyInviteCodeSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType,
        const FString& ErrorString)
{
	(void)World;
	(void)FailureType;

	if (!bAwaitingJoinTravel || JoinState != ELobbyInviteJoinState::Joining)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[InviteCodeJoin][ReqId=%u] Travel failure: %s"),
                ActiveRequest.RequestId,
                *ErrorString);
	OnInviteJoinResult.Broadcast(ELobbyInviteJoinResult::JoinFailed);
	ResetInviteJoinState(true);
}

FName UEMRLobbyInviteCodeSubsystem::GetLobbySessionName() const
{
	const FName BaseName = LobbySessionName.IsNone() ? NAME_GameSession : LobbySessionName;
#if WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		if (World->WorldType == EWorldType::PIE && GEngine)
		{
			if (const FWorldContext* Context = GEngine->GetWorldContextFromWorld(World))
			{
				return FName(*FString::Printf(TEXT("%s_PIE_%d"), *BaseName.ToString(), Context->PIEInstance));
			}
		}
	}
#endif
	return BaseName;
}
