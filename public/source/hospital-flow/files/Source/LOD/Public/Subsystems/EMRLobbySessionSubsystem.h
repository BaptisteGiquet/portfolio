#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/EMRLoadingProcessInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EMRLobbySessionSubsystem.generated.h"

class APlayerController;
class UWorld;
class UNetDriver;

UENUM(BlueprintType)
enum class ELobbySessionState : uint8
{
	Idle,
	Creating,
	Hosting,
	Joining,
	Joined,
	Failed
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbySessionStateChanged, ELobbySessionState);

UCLASS(Config = Game)
class LOD_API UEMRLobbySessionSubsystem : public UGameInstanceSubsystem, public IEMRLoadingProcessInterface
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;

	UFUNCTION(BlueprintCallable, Category = "Lobby|Session")
	void HostLobbySession(APlayerController* OwningPC);

	UFUNCTION(BlueprintCallable, Category = "Lobby|Session")
	void DestroyLobbySession();

	UFUNCTION(BlueprintCallable, Category = "Lobby|Session")
	ELobbySessionState GetLobbySessionState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "Lobby|Session")
	void ReturnToFrontend(APlayerController* OwningPC);

	void EnsureLobbyPresentation();

	void QueueKickedFromLobbyMessage(const FText& Title, const FText& Message);

	FOnLobbySessionStateChanged OnLobbySessionStateChanged;

private:
	void EnsureLobbyPresentationForWorld(UWorld* PresentationWorld);
	void SetState(ELobbySessionState NewState);
	FName GetLobbySessionName() const;
	FName GetHubMapName() const;
	FName GetLobbyMapName() const;
	FName GetFrontendMapName() const;
	bool IsLobbyMapLoaded(const UWorld* LoadedWorld) const;
	int32 GetPublicConnections() const;
	void TryPushLobbyScreen();
	void TryPushLobbyScreenForWorld(UWorld* PresentationWorld);
	void TravelToFrontend(APlayerController* OwningPC);

	void CreateSessionInternal();
	void DestroySessionInternal();
	void JoinSessionInternal(const FOnlineSessionSearchResult& Result);

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleSessionUserInviteAccepted(bool bWasSuccessful, int32 LocalUserNum, FUniqueNetIdPtr PersonInvited, const FOnlineSessionSearchResult& SearchResult);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void TryShowPendingKickedMessage(UWorld* LoadedWorld);
	void ResetLocalSessionFlowState(const TCHAR* Context);

	UPROPERTY(Config)
	FName LobbySessionName;

	UPROPERTY(Config)
	int32 DefaultPublicConnections = 8;

	UPROPERTY(Config)
	FName LobbyMapName;

	UPROPERTY(Config)
	FName HubMapName;

	UPROPERTY(Config)
	FName FrontendMapName;

	ELobbySessionState CurrentState = ELobbySessionState::Idle;

	TWeakObjectPtr<APlayerController> PendingHostPlayerController;
	TWeakObjectPtr<APlayerController> PendingJoinPlayerController;
	TOptional<FOnlineSessionSearchResult> PendingInviteJoinResult;
	TWeakObjectPtr<APlayerController> PendingReturnPlayerController;
	bool bPendingShowLobbyScreen = false;
	bool bPendingHostListenTravel = false;
	bool bPendingReturnToFrontend = false;
	bool bPendingKickedMessage = false;

	FText PendingKickedTitle;
	FText PendingKickedMessage;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle StartSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle SessionInviteAcceptedHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;

	FTimerHandle LobbyScreenRetryHandle;
	int32 LobbyScreenRetryCount = 0;
	bool bLobbyScreenPushInFlight = false;
	TWeakObjectPtr<UWorld> LobbyScreenPushWorld;
};
