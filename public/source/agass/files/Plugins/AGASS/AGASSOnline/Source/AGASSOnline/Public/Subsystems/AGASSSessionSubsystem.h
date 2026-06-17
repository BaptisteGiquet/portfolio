#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AGASSSessionSubsystem.generated.h"

class UNetDriver;
class UAGASSSessionInfoComponent;

UENUM(BlueprintType)
enum class EAGASSSessionFlowState : uint8
{
	Idle,
	Searching,
	Hosting,
	Joining,
	InSession,
	ReturningToFrontend
};

USTRUCT(BlueprintType)
struct AGASSONLINE_API FAGASSSessionSearchResultData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category="Session")
	FString SessionId;

	UPROPERTY(VisibleAnywhere, Category="Session")
	FString HostName;

	UPROPERTY(VisibleAnywhere, Category="Session")
	FString MapPath;

	UPROPERTY(VisibleAnywhere, Category="Session")
	FString MapId;

	UPROPERTY(VisibleAnywhere, Category="Session")
	FString MapDisplayName;

	UPROPERTY(VisibleAnywhere, Category="Session")
	FString InviteCode;

	UPROPERTY(VisibleAnywhere, Category="Session")
	FString ActiveModIdsCsv;

	UPROPERTY(VisibleAnywhere, Category="Session")
	int32 ActiveModCount = 0;

	UPROPERTY(VisibleAnywhere, Category="Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(VisibleAnywhere, Category="Session")
	int32 MaxPlayers = 0;

	UPROPERTY(VisibleAnywhere, Category="Session")
	bool bIsLANMatch = false;

	UPROPERTY(VisibleAnywhere, Category="Session")
	bool bIsJoinable = false;

	UPROPERTY(VisibleAnywhere, Category="Session")
	bool bIsCompatibleWithLocalSetup = true;

	UPROPERTY(VisibleAnywhere, Category="Session")
	FString JoinDisabledReason;
};

DECLARE_MULTICAST_DELEGATE(FAGASSSessionStateChangedEvent);
DECLARE_MULTICAST_DELEGATE(FAGASSSessionSearchResultsChangedEvent);

UCLASS()
class AGASSONLINE_API UAGASSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UAGASSSessionSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void HostSession();
	void FindSessions();
	void JoinSearchResult(int32 SearchResultIndex);
	void JoinResolvedSearchResult(const FOnlineSessionSearchResult& SearchResult, const FString& JoinStatusMessage = FString());
	void JoinInviteCode(const FString& InviteCode);
	void ReturnToFrontend(const FString& Reason, bool bDestroySession = true);
	void SetHostedSessionJoinInProgressEnabled(bool bAllowJoinInProgress);
	bool CanRegenerateInviteCode() const;
	bool IsInviteCodeRegenerationInProgress() const;
	bool RequestHostedSessionInviteCodeRegeneration();

	const TArray<FAGASSSessionSearchResultData>& GetSearchResults() const;
	EAGASSSessionFlowState GetSessionFlowState() const;
	const FString& GetStatusMessage() const;
	const FString& GetCurrentInviteCode() const;
	bool IsBusy() const;
	bool HasNamedSession() const;
	FName GetOnlineSubsystemName() const;

	FAGASSSessionStateChangedEvent& OnSessionStateChanged();
	FAGASSSessionSearchResultsChangedEvent& OnSearchResultsChanged();

private:
	enum class EPendingSessionAction : uint8
	{
		None,
		Host,
		JoinSearchResult,
		JoinResolvedSearchResult,
		ResolveInviteCode,
		ReturnToFrontend
	};

	enum class EAGASSSessionUpdateAction : uint8
	{
		None,
		ToggleJoinInProgress,
		RegenerateInviteCode
	};

	IOnlineSessionPtr GetSessionInterface() const;
	bool ShouldUseLanSessions() const;
	bool ShouldUseLobbies() const;
	FString GetFrontendMapPath() const;
	bool IsGameplaySessionWorld(const UWorld* World) const;
	bool IsFrontendWorld(const UWorld* World) const;
	bool IsNamedSessionActive() const;
	int32 GetLocalUserNum() const;

	void BeginHostSession();
	void BeginFindSessions();
	void BeginJoinSearchResult(int32 SearchResultIndex);
	void BeginJoinResolvedSearchResult(const FOnlineSessionSearchResult& SearchResult, const FString& JoinStatusMessage);
	void DestroyNamedSessionForPendingAction();
	void ExecutePendingActionAfterDestroy();
	void ResolveAndJoinInviteCode(const FString& NormalizedInviteCode);
	void TravelToHostedMap();
	void CompleteFrontendReturn(const FString& Reason);
	void CacheSearchResults();
	void RebuildCachedSearchResultCompatibility();
	void SetFlowState(EAGASSSessionFlowState NewState);
	void SetStatusMessage(const FString& NewStatusMessage);
	void ResetPendingAction();
	void StartHostCreateSessionDiagnostics();
	void ClearHostCreateSessionDiagnostics();
	void LogHostCreateSessionProgress();
	void StartHostCreateSessionWatchdog();
	void ClearHostCreateSessionWatchdog();
	void HandleHostCreateSessionWatchdogExpired();
	void BindSessionInfoComponentForCurrentWorld();
	void QueueBindSessionInfoComponentForCurrentWorld();
	void ClearSessionInfoComponentBinding();
	UAGASSSessionInfoComponent* GetSessionInfoComponent() const;
	void RefreshInviteCodeFromSessionInfo();
	void HandleSessionInfoChanged();
	void HandleFailure(const FString& FailureMessage, bool bForceFrontendTravel, bool bDestroySession);
	FString BuildConnectionTravelOptions(const FString& ContentCompatibilityHash) const;
	bool EvaluateJoinCompatibility(const FOnlineSessionSearchResult& SearchResult, FString& OutFailureMessage, FString& OutContentCompatibilityHash) const;
	void HandleModsSelectionChanged();

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleUpdateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	EAGASSSessionFlowState FlowState;
	FString StatusMessage;
	FString CurrentInviteCode;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	TArray<FOnlineSessionSearchResult> NativeSearchResults;
	TArray<FAGASSSessionSearchResultData> SearchResults;

	EPendingSessionAction PendingAction;
	int32 PendingSearchResultIndex;
	FOnlineSessionSearchResult PendingResolvedSearchResult;
	FString PendingJoinStatusMessage;
	FString PendingInviteCode;
	FString PendingFrontendReason;
	EAGASSSessionUpdateAction PendingSessionUpdateAction;
	FString PendingRegeneratedInviteCode;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle StartSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle UpdateSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	FDelegateHandle ModsSelectionChangedHandle;
	FDelegateHandle SessionInfoChangedHandle;

	FAGASSSessionStateChangedEvent SessionStateChangedEvent;
	FAGASSSessionSearchResultsChangedEvent SearchResultsChangedEvent;
	FString PendingHostedTravelMapPath;
	FString PendingHostedTravelOptions;
	FString PendingJoinTravelOptions;
	TWeakObjectPtr<UAGASSSessionInfoComponent> BoundSessionInfoComponent;
	FTimerHandle SessionInfoBindRetryHandle;
	FTimerHandle HostCreateSessionDiagnosticsHandle;
	FTimerHandle HostCreateSessionWatchdogHandle;
};
