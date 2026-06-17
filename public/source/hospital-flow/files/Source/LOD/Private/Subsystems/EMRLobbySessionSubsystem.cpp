#include "Subsystems/EMRLobbySessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UI/Frontend/Core/EMRCommonPrimaryLayoutWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/EMRFrontendGameState.h"
#include "Subsystems/EMRLoadingScreenSubsystem.h"
#include "Subsystems/EMRLobbyInviteCodeSubsystem.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "UI/Frontend/Utils/EMRFrontendFunctionLibrary.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"
#include "GAS/EMRTags.h"
#include "Framework/EMRLobbyGameState.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Subsystems/EMRGameplayTelemetrySubsystem.h"
#include "Misc/PackageName.h"
#include "Camera/CameraActor.h"
#include "Subsystems/EMRRunProgressionSubsystem.h"
#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyPlayerController.h"
#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Utils/EMREndSessionTrace.h"

namespace
{
	APlayerController* ResolvePreferredLocalPresentationController(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		APlayerController* FirstLocalController = nullptr;
		APlayerController* LobbyLocalControllerWithoutState = nullptr;

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Candidate = It->Get();
			if (!Candidate || !Candidate->IsLocalController())
			{
				continue;
			}

			if (!FirstLocalController)
			{
				FirstLocalController = Candidate;
			}

			AEMRFrontendLobbyPlayerController* LobbyController = Cast<AEMRFrontendLobbyPlayerController>(Candidate);
			if (!LobbyController)
			{
				continue;
			}

			if (Candidate->PlayerState)
			{
				return Candidate;
			}

			if (!LobbyLocalControllerWithoutState)
			{
				LobbyLocalControllerWithoutState = Candidate;
			}
		}

		return LobbyLocalControllerWithoutState ? LobbyLocalControllerWithoutState : FirstLocalController;
	}

	void LogFrontendStackState(UEMRLobbySessionSubsystem* SessionSubsystem, UEMRCommonPrimaryLayoutWidget* PrimaryLayout, APlayerController* PrimaryPC, const TCHAR* Context)
	{
		EMR_END_SESSION_TRACE(SessionSubsystem, TEXT("[EndSessionLobbyTrace] LobbySession.StackState context=%s layout=%s layoutInViewport=%d pc=%s"),
			Context ? Context : TEXT("<null>"),
			*GetNameSafe(PrimaryLayout),
			(PrimaryLayout && PrimaryLayout->IsInViewport()) ? 1 : 0,
			*GetNameSafe(PrimaryPC));

		if (!PrimaryLayout)
		{
			return;
		}

		UCommonActivatableWidgetContainerBase* Stack = PrimaryLayout->FindWidgetStackByTag(EMRTags::UI::WidgetStack::Frontend);
		if (!Stack)
		{
			EMR_END_SESSION_TRACE(SessionSubsystem, TEXT("[EndSessionLobbyTrace] LobbySession.StackState context=%s missing Frontend stack"),
				Context ? Context : TEXT("<null>"));
			return;
		}

		UCommonActivatableWidget* ActiveWidget = Stack->GetActiveWidget();
		EMR_END_SESSION_TRACE(SessionSubsystem, TEXT("[EndSessionLobbyTrace] LobbySession.StackState context=%s activeWidget=%s activeClass=%s activeOwner=%s activeActivated=%d activeVisibility=%d activeInViewport=%d totalWidgets=%d"),
			Context ? Context : TEXT("<null>"),
			*GetNameSafe(ActiveWidget),
			ActiveWidget ? *GetNameSafe(ActiveWidget->GetClass()) : TEXT("<null>"),
			ActiveWidget ? *GetNameSafe(ActiveWidget->GetOwningPlayer()) : TEXT("<null>"),
			(ActiveWidget && ActiveWidget->IsActivated()) ? 1 : 0,
			ActiveWidget ? static_cast<int32>(ActiveWidget->GetVisibility()) : -1,
			(ActiveWidget && ActiveWidget->IsInViewport()) ? 1 : 0,
			Stack->GetWidgetList().Num());

		const TArray<UCommonActivatableWidget*> Widgets = Stack->GetWidgetList();
		for (int32 Index = 0; Index < Widgets.Num(); ++Index)
		{
			const UCommonActivatableWidget* Widget = Widgets[Index];
			EMR_END_SESSION_TRACE(SessionSubsystem, TEXT("[EndSessionLobbyTrace] LobbySession.StackState context=%s entry[%d]=%s class=%s owner=%s activated=%d visibility=%d inViewport=%d"),
				Context ? Context : TEXT("<null>"),
				Index,
				*GetNameSafe(Widget),
				Widget ? *GetNameSafe(Widget->GetClass()) : TEXT("<null>"),
				Widget ? *GetNameSafe(Widget->GetOwningPlayer()) : TEXT("<null>"),
				(Widget && Widget->IsActivated()) ? 1 : 0,
				Widget ? static_cast<int32>(Widget->GetVisibility()) : -1,
				(Widget && Widget->IsInViewport()) ? 1 : 0);
		}
	}
}

void UEMRLobbySessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UEMRLoadingScreenSubsystem* LoadingScreenSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLoadingScreenSubsystem>() : nullptr)
	{
		LoadingScreenSubsystem->RegisterLoadingProcessor(this);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInviteAcceptedHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::HandleSessionUserInviteAccepted));
	}
}

void UEMRLobbySessionSubsystem::Deinitialize()
{
	if (UEMRLoadingScreenSubsystem* LoadingScreenSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLoadingScreenSubsystem>() : nullptr)
	{
		LoadingScreenSubsystem->UnregisterLoadingProcessor(this);
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid() && SessionInviteAcceptedHandle.IsValid())
		{
			SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(SessionInviteAcceptedHandle);
		}
	}

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

bool UEMRLobbySessionSubsystem::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (CurrentState == ELobbySessionState::Creating)
	{
		OutReason = TEXT("Creating lobby session");
		return true;
	}

	if (CurrentState == ELobbySessionState::Joining)
	{
		OutReason = TEXT("Joining lobby session");
		return true;
	}

	if (bPendingHostListenTravel)
	{
		OutReason = TEXT("Pending host listen travel");
		return true;
	}

	if (bPendingShowLobbyScreen)
	{
		OutReason = TEXT("Pending lobby presentation");
		return true;
	}

	if (bPendingReturnToFrontend)
	{
		OutReason = TEXT("Pending return to frontend");
		return true;
	}

	return false;
}

void UEMRLobbySessionSubsystem::HostLobbySession(APlayerController* OwningPC)
{
	PendingHostPlayerController = OwningPC;

	if (UWorld* World = GetWorld())
	{
		const bool bIsStandaloneNetMode = (World->GetNetMode() == NM_Standalone);
		const bool bCanListenTravel = bIsStandaloneNetMode
                        && (World->WorldType == EWorldType::Game
                                || World->WorldType == EWorldType::PIE
                                || World->WorldType == EWorldType::GamePreview);
		if (bCanListenTravel)
		{
			const FName HubMap = GetHubMapName();
			const FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(World, true);
			const FString CurrentHubMapName = FPackageName::GetShortName(HubMap.ToString());
			const bool bAlreadyInHubMap = !CurrentMapName.IsEmpty() && (CurrentMapName == CurrentHubMapName);
			if (!HubMap.IsNone() && !bAlreadyInHubMap)
			{
				bPendingHostListenTravel = true;
				UE_LOG(LogTemp, Log, TEXT("[LobbySessionHost] Opening listen session on %s"), *HubMap.ToString());
				UGameplayStatics::OpenLevel(World, HubMap, true, TEXT("listen"));
				return;
			}
		}
	}

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		SetState(ELobbySessionState::Failed);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		SetState(ELobbySessionState::Failed);
		return;
	}


	if (SessionInterface->GetNamedSession(GetLobbySessionName()) != nullptr)
	{
		DestroySessionInternal();
		return;
	}

	CreateSessionInternal();
}

void UEMRLobbySessionSubsystem::DestroyLobbySession()
{
	DestroySessionInternal();
}

void UEMRLobbySessionSubsystem::DestroySessionInternal()
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (SessionInterface->GetNamedSession(GetLobbySessionName()) == nullptr)
	{
		SetState(ELobbySessionState::Idle);
		return;
	}

	DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));

	SessionInterface->DestroySession(GetLobbySessionName());
}

void UEMRLobbySessionSubsystem::CreateSessionInternal()
{
	SetState(ELobbySessionState::Creating);

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		SetState(ELobbySessionState::Failed);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		SetState(ELobbySessionState::Failed);
		return;
	}

	const FName SubsystemName = Subsystem->GetSubsystemName();
	const FName NullSubsystemName(TEXT("NULL"));
	const FName LanSubsystemName(TEXT("LAN"));
	const bool bIsLan = (SubsystemName == NAME_None || SubsystemName == NullSubsystemName || SubsystemName == LanSubsystemName);

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = bIsLan;
	SessionSettings.bUsesPresence = !bIsLan;
	SessionSettings.bUseLobbiesIfAvailable = !bIsLan;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = false;
	SessionSettings.NumPublicConnections = GetPublicConnections();
	if (UWorld* World = GetWorld())
	{
		const int32 HostPort = World->URL.Port;
		if (HostPort > 0)
		{
			SessionSettings.Set(UEMRLobbyInviteCodeSubsystem::LobbyHostPortSessionKey, HostPort,
                                EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		}
	}

	CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));

	const int32 LocalUserNum = PendingHostPlayerController.IsValid() && PendingHostPlayerController->GetLocalPlayer()
	? PendingHostPlayerController->GetLocalPlayer()->GetControllerId()
	: 0;

	if (!SessionInterface->CreateSession(LocalUserNum, GetLobbySessionName(), SessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
		SetState(ELobbySessionState::Failed);
	}
}

void UEMRLobbySessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
	}

	if (!bWasSuccessful)
	{
		PendingHostPlayerController.Reset();
		SetState(ELobbySessionState::Failed);
		return;
	}

	StartSessionCompleteHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(
		FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleStartSessionComplete));

	SessionInterface->StartSession(SessionName);
}

void UEMRLobbySessionSubsystem::HandleStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);
	}

	if (!bWasSuccessful)
	{
		PendingHostPlayerController.Reset();
		SetState(ELobbySessionState::Failed);
		return;
	}

	SetState(ELobbySessionState::Hosting);
	PendingHostPlayerController.Reset();

	if (AEMRFrontendGameState* FrontendGameState = GetWorld() ? GetWorld()->GetGameState<AEMRFrontendGameState>() : nullptr)
	{
		FrontendGameState->SetLobbyHosting(true);
	}

	if (UEMRLobbyInviteCodeSubsystem* InviteCodeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbyInviteCodeSubsystem>() : nullptr)
	{
		const FString NewCode = InviteCodeSubsystem->GenerateInviteCode();
		if (!NewCode.IsEmpty() && InviteCodeSubsystem->UpdateSessionInviteCode(NewCode))
		{
			if (AEMRNightShiftGameState* RunGameState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr)
			{
				RunGameState->SetSessionInviteCode(NewCode);

				if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRRunProgressionSubsystem>() : nullptr)
				{
					ProgressionSubsystem->CacheFromGameState(RunGameState);
				}
			}
		}
	}
}

void UEMRLobbySessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
	}

	if (AEMRFrontendGameState* FrontendGameState = GetWorld() ? GetWorld()->GetGameState<AEMRFrontendGameState>() : nullptr)
	{
		FrontendGameState->SetLobbyHosting(false);
	}

	if (bPendingReturnToFrontend)
	{
		APlayerController* ReturnController = PendingReturnPlayerController.IsValid()
			? PendingReturnPlayerController.Get()
			: (GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr);
		bPendingReturnToFrontend = false;
		PendingReturnPlayerController.Reset();
		TravelToFrontend(ReturnController);
		SetState(ELobbySessionState::Idle);
		return;
	}

	if (PendingInviteJoinResult.IsSet())
	{
		FOnlineSessionSearchResult PendingResult = PendingInviteJoinResult.GetValue();
		PendingInviteJoinResult.Reset();
		JoinSessionInternal(PendingResult);
		return;
	}

	if (PendingHostPlayerController.IsValid())
	{
		CreateSessionInternal();
		return;
	}

	SetState(ELobbySessionState::Idle);
}

void UEMRLobbySessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Log, TEXT("[LobbySessionJoin] JoinSessionComplete %s Result=%d"), *SessionName.ToString(), static_cast<int32>(Result));
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
	}

	if (Result != EOnJoinSessionCompleteResult::Success || !SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySessionJoin] JoinSession failed Result=%d"), static_cast<int32>(Result));
		PendingJoinPlayerController.Reset();
		SetState(ELobbySessionState::Failed);
		return;
	}

	APlayerController* JoinController = PendingJoinPlayerController.IsValid()
	? PendingJoinPlayerController.Get()
	: (GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr);

	FString ConnectString;
	if (!JoinController || !SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySessionJoin] ResolveConnectString failed. PC=%s"), *GetNameSafe(JoinController));
		PendingJoinPlayerController.Reset();
		SetState(ELobbySessionState::Failed);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbySessionJoin] ClientTravel to %s (Absolute, non-seamless)"), *ConnectString);
	JoinController->ClientTravel(ConnectString, TRAVEL_Absolute, false, FGuid());
	PendingJoinPlayerController.Reset();
	SetState(ELobbySessionState::Joined);
}

void UEMRLobbySessionSubsystem::HandleSessionUserInviteAccepted(bool bWasSuccessful, int32 LocalUserNum, FUniqueNetIdPtr PersonInvited, const FOnlineSessionSearchResult& SearchResult)
{
	if (!bWasSuccessful || !SearchResult.IsValid())
	{
		SetState(ELobbySessionState::Failed);
		return;
	}

	PendingJoinPlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!SessionInterface.IsValid())
	{
		SetState(ELobbySessionState::Failed);
		return;
	}

	if (SessionInterface->GetNamedSession(GetLobbySessionName()) != nullptr)
	{
		PendingInviteJoinResult = SearchResult;
		DestroySessionInternal();
		return;
	}

	JoinSessionInternal(SearchResult);
}

void UEMRLobbySessionSubsystem::JoinSessionInternal(const FOnlineSessionSearchResult& Result)
{
	UE_LOG(LogTemp, Log, TEXT("[LobbySessionJoin] JoinSessionInternal ResultValid=%d"), Result.IsValid() ? 1 : 0);
	SetState(ELobbySessionState::Joining);

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySessionJoin] Invalid SessionInterface"));
		SetState(ELobbySessionState::Failed);
		return;
	}

	JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));

	const int32 LocalUserNum = PendingJoinPlayerController.IsValid() && PendingJoinPlayerController->GetLocalPlayer()
	? PendingJoinPlayerController->GetLocalPlayer()->GetControllerId()
	: 0;

	if (!SessionInterface->JoinSession(LocalUserNum, GetLobbySessionName(), Result))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySessionJoin] JoinSession failed to start"));
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
		SetState(ELobbySessionState::Failed);
	}
}

void UEMRLobbySessionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (LobbyScreenPushWorld.IsValid() && LobbyScreenPushWorld.Get() != LoadedWorld)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.HandlePostLoadMap reset stale push-in-flight oldWorld=%s newWorld=%s"),
			*GetNameSafe(LobbyScreenPushWorld.Get()),
			*GetNameSafe(LoadedWorld));
		bLobbyScreenPushInFlight = false;
		LobbyScreenPushWorld.Reset();
	}

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.HandlePostLoadMap world=%s netmode=%d state=%d pendingShow=%d pendingHostTravel=%d pendingReturn=%d"),
		*GetNameSafe(LoadedWorld),
		LoadedWorld ? static_cast<int32>(LoadedWorld->GetNetMode()) : -1,
		static_cast<int32>(CurrentState),
		bPendingShowLobbyScreen ? 1 : 0,
		bPendingHostListenTravel ? 1 : 0,
		bPendingReturnToFrontend ? 1 : 0);

	TryShowPendingKickedMessage(LoadedWorld);

	const FName FrontendMap = GetFrontendMapName();
	const FString CurrentMapName = LoadedWorld ? UGameplayStatics::GetCurrentLevelName(LoadedWorld, true) : FString();
	const FString FrontendMapShortName = FPackageName::GetShortName(FrontendMap.ToString());
	const bool bIsFrontendMapLoaded = LoadedWorld && !FrontendMap.IsNone() && CurrentMapName == FrontendMapShortName;
	const bool bLikelyActiveSession = CurrentState == ELobbySessionState::Joined
	|| CurrentState == ELobbySessionState::Hosting
	|| bPendingReturnToFrontend;
	if (bIsFrontendMapLoaded && bLikelyActiveSession)
	{
		EMR_END_SESSION_TRACE(this,
			TEXT("[EndSessionLobbyTrace] LobbySession.HandlePostLoadMap frontend fallback map=%s state=%d pendingReturn=%d"),
			*CurrentMapName,
			static_cast<int32>(CurrentState),
			bPendingReturnToFrontend ? 1 : 0);

		ResetLocalSessionFlowState(TEXT("PostLoadMapFrontendFallback"));
		bPendingReturnToFrontend = false;
		PendingReturnPlayerController.Reset();
		SetState(ELobbySessionState::Idle);
	}

	if (bPendingHostListenTravel)
	{
		bPendingHostListenTravel = false;
		PendingHostPlayerController = LoadedWorld ? LoadedWorld->GetFirstPlayerController() : nullptr;

		IOnlineSubsystem* Subsystem = Online::GetSubsystem(LoadedWorld);
		IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
		if (!SessionInterface.IsValid())
		{
			SetState(ELobbySessionState::Failed);
			return;
		}

		if (SessionInterface->GetNamedSession(GetLobbySessionName()) != nullptr)
		{
			DestroySessionInternal();
			return;
		}

		CreateSessionInternal();
		return;
	}

	if (!LoadedWorld)
	{
		return;
	}

	const bool bShouldEnsureLobbyPresentation = bPendingShowLobbyScreen || IsLobbyMapLoaded(LoadedWorld);
	if (!bShouldEnsureLobbyPresentation)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.HandlePostLoadMap no presentation needed map=%s"), *CurrentMapName);
		return;
	}

	bPendingShowLobbyScreen = false;

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.HandlePostLoadMap ensuring lobby presentation map=%s"),
		*CurrentMapName);
	EnsureLobbyPresentationForWorld(LoadedWorld);
}

void UEMRLobbySessionSubsystem::QueueKickedFromLobbyMessage(const FText& Title, const FText& Message)
{
	PendingKickedTitle = Title;
	PendingKickedMessage = Message;
	bPendingKickedMessage = true;
}

void UEMRLobbySessionSubsystem::TryShowPendingKickedMessage(UWorld* LoadedWorld)
{
	if (!bPendingKickedMessage || !LoadedWorld)
	{
		return;
	}

	const FName FrontendMap = GetFrontendMapName();
	if (FrontendMap.IsNone())
	{
		return;
	}

	const FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(LoadedWorld, true);
	const FString FrontendMapShortName = FPackageName::GetShortName(FrontendMap.ToString());
	if (CurrentMapName != FrontendMapShortName)
	{
		return;
	}

	bPendingKickedMessage = false;

	UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(LoadedWorld);
	if (!UIManagerSubsystem)
	{
		return;
	}

	UIManagerSubsystem->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::Ok,
		PendingKickedTitle,
		PendingKickedMessage,
		[](EConfirmScreenButtonType) {});
}

void UEMRLobbySessionSubsystem::EnsureLobbyPresentation()
{
	EnsureLobbyPresentationForWorld(GetWorld());
}

void UEMRLobbySessionSubsystem::EnsureLobbyPresentationForWorld(UWorld* PresentationWorld)
{
	if (!PresentationWorld)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.EnsureLobbyPresentationForWorld aborted: null world"));
		return;
	}

	APlayerController* PrimaryPC = ResolvePreferredLocalPresentationController(PresentationWorld);
	if (!PrimaryPC || !PrimaryPC->IsLocalController())
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.EnsureLobbyPresentationForWorld skipped local controller check PC=%s IsLocal=%d"),
			*GetNameSafe(PrimaryPC),
			PrimaryPC && PrimaryPC->IsLocalController() ? 1 : 0);
		return;
	}

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.EnsureLobbyPresentationForWorld begin world=%s pc=%s authority=%d"),
		*GetNameSafe(PresentationWorld),
		*GetNameSafe(PrimaryPC),
		PrimaryPC->HasAuthority() ? 1 : 0);
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.EnsureLobbyPresentationForWorld controller details class=%s playerState=%s pawn=%s local=%d"),
		*GetNameSafe(PrimaryPC->GetClass()),
		*GetNameSafe(PrimaryPC->PlayerState.Get()),
		*GetNameSafe(PrimaryPC->GetPawn()),
		PrimaryPC->IsLocalController() ? 1 : 0);

	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClassWithTag(PresentationWorld, ACameraActor::StaticClass(), FName("Default"), FoundCameras);
	if (!FoundCameras.IsEmpty())
	{
		PrimaryPC->SetViewTarget(FoundCameras[0]);
	}

	TryPushLobbyScreenForWorld(PresentationWorld);
}

void UEMRLobbySessionSubsystem::TryPushLobbyScreen()
{
	TryPushLobbyScreenForWorld(GetWorld());
}

void UEMRLobbySessionSubsystem::TryPushLobbyScreenForWorld(UWorld* PresentationWorld)
{
	UWorld* World = PresentationWorld;
	if (!World)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld aborted: null world"));
		return;
	}

	APlayerController* PrimaryPC = ResolvePreferredLocalPresentationController(World);
	if (!PrimaryPC || !PrimaryPC->IsLocalController())
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld aborted missing local controller pc=%s"),
			*GetNameSafe(PrimaryPC));
		return;
	}

	UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(PrimaryPC);
	if (!UIManagerSubsystem)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld missing UIManager world=%s"),
			*GetNameSafe(World));
		return;
	}

	if (IsLobbyMapLoaded(World) && !World->GetGameState<AEMRLobbyGameState>())
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld proceeding without LobbyGameState (widget will retry bind)"));
	}

	UEMRCommonPrimaryLayoutWidget* PrimaryLayout = UIManagerSubsystem->GetPrimaryLayoutWidget();
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld pre-ensure layout=%s pc=%s pcClass=%s pcPlayerState=%s"),
		*GetNameSafe(PrimaryLayout),
		*GetNameSafe(PrimaryPC),
		PrimaryPC ? *GetNameSafe(PrimaryPC->GetClass()) : TEXT("<null>"),
		PrimaryPC ? *GetNameSafe(PrimaryPC->PlayerState.Get()) : TEXT("<null>"));

	if (PrimaryLayout && PrimaryPC)
	{
		PrimaryLayout = UIManagerSubsystem->EnsurePrimaryLayout(PrimaryPC, PrimaryLayout->GetClass());
	}

	if (!PrimaryLayout)
	{
		const int32 MaxRetries = 20;
		if (LobbyScreenRetryCount < MaxRetries)
		{
			++LobbyScreenRetryCount;
			EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld waiting PrimaryLayout retry=%d"),
				LobbyScreenRetryCount);
			TWeakObjectPtr<UWorld> WeakWorld = World;
			FTimerDelegate RetryDelegate;
			RetryDelegate.BindWeakLambda(this, [this, WeakWorld]()
			{
				TryPushLobbyScreenForWorld(WeakWorld.Get());
			});
			World->GetTimerManager().SetTimer(LobbyScreenRetryHandle, RetryDelegate, 0.1f, false);
		}
		else
		{
			EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld timeout waiting PrimaryLayout instance"));
		}
		return;
	}

	LogFrontendStackState(this, PrimaryLayout, PrimaryPC, TEXT("BeforePushDecision"));

	const bool bLayoutReady = PrimaryLayout && PrimaryLayout->IsInViewport();
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld layout state world=%s layout=%s inViewport=%d retryCount=%d"),
		*GetNameSafe(World),
		*GetNameSafe(PrimaryLayout),
		bLayoutReady ? 1 : 0,
		LobbyScreenRetryCount);
	if (!bLayoutReady)
	{
		const int32 MaxRetries = 20;
		if (LobbyScreenRetryCount < MaxRetries)
		{
			++LobbyScreenRetryCount;
			TWeakObjectPtr<UWorld> WeakWorld = World;
			FTimerDelegate RetryDelegate;
			RetryDelegate.BindWeakLambda(this, [this, WeakWorld]()
			{
				TryPushLobbyScreenForWorld(WeakWorld.Get());
			});
			World->GetTimerManager().SetTimer(LobbyScreenRetryHandle, RetryDelegate, 0.1f, false);
		}
		else
		{
			EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld timeout waiting primary layout"));
		}
		return;
	}

	LobbyScreenRetryCount = 0;
	World->GetTimerManager().ClearTimer(LobbyScreenRetryHandle);

	const TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> LobbyScreenClass =
	UEMRFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(EMRTags::UI::Widgets::LobbyScreen);
	if (LobbyScreenClass.IsNull())
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld aborted missing LobbyScreen class"));
		return;
	}

	if (bLobbyScreenPushInFlight && LobbyScreenPushWorld.Get() == World)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld skip push already in-flight world=%s"),
			*GetNameSafe(World));
		LogFrontendStackState(this, PrimaryLayout, PrimaryPC, TEXT("SkipInFlight"));
		return;
	}

	bLobbyScreenPushInFlight = true;
	LobbyScreenPushWorld = World;
	TWeakObjectPtr<UWorld> WeakWorld = World;

	UIManagerSubsystem->PushOrReplaceSoftWidgetToStackAsync(
		EMRTags::UI::WidgetStack::Frontend,
		LobbyScreenClass,
		[this, WeakWorld](EAsyncPushWidgetState PushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
		{
			EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld push callback state=%d widget=%s owner=%s activated=%d visibility=%d inViewport=%d"),
				static_cast<int32>(PushState),
				*GetNameSafe(PushedWidget),
				PushedWidget ? *GetNameSafe(PushedWidget->GetOwningPlayer()) : TEXT("<null>"),
				(PushedWidget && PushedWidget->IsActivated()) ? 1 : 0,
				PushedWidget ? static_cast<int32>(PushedWidget->GetVisibility()) : -1,
				(PushedWidget && PushedWidget->IsInViewport()) ? 1 : 0);

			if (PushState == EAsyncPushWidgetState::AfterPush)
			{
				if (PushedWidget && !PushedWidget->IsActivated())
				{
					PushedWidget->ActivateWidget();
					EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld activated lobby widget after replace widget=%s"),
						*GetNameSafe(PushedWidget));
				}

				bLobbyScreenPushInFlight = false;
				LobbyScreenPushWorld.Reset();
				EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld push completed widget=%s world=%s"),
					*GetNameSafe(PushedWidget),
					*GetNameSafe(WeakWorld.Get()));

				if (UWorld* CallbackWorld = WeakWorld.Get())
				{
					if (UEMRUIManagerSubsystem* CallbackUIManager = UEMRUIManagerSubsystem::Get(CallbackWorld))
					{
						LogFrontendStackState(this, CallbackUIManager->GetPrimaryLayoutWidget(), CallbackWorld->GetFirstPlayerController(), TEXT("AfterPushCallback"));
					}

					TWeakObjectPtr<UWorld> PostPushWorld = CallbackWorld;
					FTimerHandle PostPushInspectHandle;
					FTimerDelegate PostPushInspectDelegate;
					PostPushInspectDelegate.BindWeakLambda(this, [this, PostPushWorld]()
					{
						if (UWorld* InspectWorld = PostPushWorld.Get())
						{
							if (UEMRUIManagerSubsystem* InspectUIManager = UEMRUIManagerSubsystem::Get(InspectWorld))
							{
								LogFrontendStackState(this, InspectUIManager->GetPrimaryLayoutWidget(), InspectWorld->GetFirstPlayerController(), TEXT("AfterPushDelay"));
							}
						}
					});
					CallbackWorld->GetTimerManager().SetTimer(PostPushInspectHandle, PostPushInspectDelegate, 0.15f, false);
				}
			}
		});
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.TryPushLobbyScreenForWorld requested push of LobbyScreen"));
}

void UEMRLobbySessionSubsystem::SetState(ELobbySessionState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.SetState old=%d new=%d"),
		static_cast<int32>(CurrentState),
		static_cast<int32>(NewState));
	CurrentState = NewState;
	OnLobbySessionStateChanged.Broadcast(CurrentState);
}


FName UEMRLobbySessionSubsystem::GetLobbySessionName() const
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

FName UEMRLobbySessionSubsystem::GetLobbyMapName() const
{
	if (!LobbyMapName.IsNone())
	{
		return LobbyMapName;
	}

	return FName(TEXT("/Game/EmergencyRoom/Maps/NewLobbyMap"));
}

FName UEMRLobbySessionSubsystem::GetHubMapName() const
{
	if (!HubMapName.IsNone())
	{
		return HubMapName;
	}

	return FName(TEXT("/Game/EmergencyRoom/Maps/HubMap"));
}

FName UEMRLobbySessionSubsystem::GetFrontendMapName() const
{
	if (!FrontendMapName.IsNone())
	{
		return FrontendMapName;
	}

	return FName(TEXT("/Game/EmergencyRoom/Maps/FrontEndTestMap"));
}

bool UEMRLobbySessionSubsystem::IsLobbyMapLoaded(const UWorld* LoadedWorld) const
{
	const FName LobbyMap = GetLobbyMapName();
	if (!LoadedWorld || LobbyMap.IsNone())
	{
		return false;
	}

	const FString CurrentMapName = UGameplayStatics::GetCurrentLevelName(LoadedWorld, true);
	const FString LobbyMapShortName = FPackageName::GetShortName(LobbyMap.ToString());
	return !CurrentMapName.IsEmpty() && CurrentMapName == LobbyMapShortName;
}

int32 UEMRLobbySessionSubsystem::GetPublicConnections() const
{
	return DefaultPublicConnections > 0 ? DefaultPublicConnections : 8;
}

void UEMRLobbySessionSubsystem::ReturnToFrontend(APlayerController* OwningPC)
{
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbySession.ReturnToFrontend owningPC=%s world=%s"),
		*GetNameSafe(OwningPC),
		*GetNameSafe(GetWorld()));

	ResetLocalSessionFlowState(TEXT("ReturnToFrontend"));

	PendingReturnPlayerController = OwningPC;
	bPendingReturnToFrontend = true;

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!SessionInterface.IsValid() || SessionInterface->GetNamedSession(GetLobbySessionName()) == nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbySession] ReturnToFrontend no active session; traveling to frontend."));
		TravelToFrontend(OwningPC);
		bPendingReturnToFrontend = false;
		PendingReturnPlayerController.Reset();
		SetState(ELobbySessionState::Idle);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbySession] ReturnToFrontend destroying session before travel."));
	DestroySessionInternal();
}

void UEMRLobbySessionSubsystem::TravelToFrontend(APlayerController* OwningPC)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] TravelToFrontend failed: World is null."));
		return;
	}

	const FName FrontendMap = GetFrontendMapName();
	if (FrontendMap.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] TravelToFrontend failed: FrontendMapName is None."));
		return;
	}

	APlayerController* TravelController = OwningPC ? OwningPC : World->GetFirstPlayerController();
	if (TravelController && !TravelController->HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbySession] TravelToFrontend: ClientTravel to %s."), *FrontendMap.ToString());
		TravelController->ClientTravel(FrontendMap.ToString(), TRAVEL_Absolute, false);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbySession] TravelToFrontend: OpenLevel %s."), *FrontendMap.ToString());
	UGameplayStatics::OpenLevel(World, FrontendMap);
}

void UEMRLobbySessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	(void)World;
	(void)NetDriver;

	const bool bLikelyActiveSession = CurrentState == ELobbySessionState::Joined
	|| CurrentState == ELobbySessionState::Hosting
	|| bPendingReturnToFrontend;
	if (!bLikelyActiveSession)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[TempFlowReset] Network failure detected. FailureType=%d Error=%s"),
		static_cast<int32>(FailureType),
		*ErrorString);
	ResetLocalSessionFlowState(TEXT("NetworkFailure"));
}

void UEMRLobbySessionSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	(void)World;

	const bool bLikelyActiveSession = CurrentState == ELobbySessionState::Joined
	|| CurrentState == ELobbySessionState::Hosting
	|| bPendingReturnToFrontend;
	if (!bLikelyActiveSession)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[TempFlowReset] Travel failure detected. FailureType=%d Error=%s"),
		static_cast<int32>(FailureType),
		*ErrorString);
	ResetLocalSessionFlowState(TEXT("TravelFailure"));
}

void UEMRLobbySessionSubsystem::ResetLocalSessionFlowState(const TCHAR* Context)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[TempFlowReset] Begin local flow reset. Context=%s State=%d PendingReturn=%d"),
		Context ? Context : TEXT("Unknown"),
		static_cast<int32>(CurrentState),
		bPendingReturnToFrontend ? 1 : 0);

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TempFlowReset] Skip local flow reset: GameInstance is null."));
		return;
	}

	if (UEMRRunProgressionSubsystem* RunProgressionSubsystem = GI->GetSubsystem<UEMRRunProgressionSubsystem>())
	{
		RunProgressionSubsystem->ResetCachedState();
		UE_LOG(LogTemp, Warning, TEXT("[TempFlowReset] Run progression cache reset."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TempFlowReset] Run progression subsystem missing; cache reset skipped."));
	}

	bLobbyScreenPushInFlight = false;
	LobbyScreenPushWorld.Reset();
	LobbyScreenRetryCount = 0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LobbyScreenRetryHandle);
	}

	if (UEMRGameplayTelemetrySubsystem* TelemetrySubsystem = GI->GetSubsystem<UEMRGameplayTelemetrySubsystem>())
	{
		TelemetrySubsystem->ResetRuntimeState();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TempFlowReset] Telemetry subsystem missing; runtime reset skipped."));
	}
}

