#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/EMRLoadingProcessInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EMRLobbyInviteCodeSubsystem.generated.h"

class APlayerController;
class UWorld;
class UNetDriver;

UENUM(BlueprintType)
enum class ELobbyInviteJoinResult : uint8
{
	Success,
	NotFound,
	InvalidCode,
	SearchFailed,
	LobbyFull,
	JoinFailed
};

UENUM(BlueprintType)
enum class ELobbyInviteJoinState : uint8
{
	Idle,
	DestroyingExistingSession,
	Searching,
	Joining
};

struct FLobbyInviteJoinRequest
{
	uint32 RequestId = 0;
	FString Code;
	TWeakObjectPtr<APlayerController> PlayerController;
};

UCLASS(Config = Game)
class LOD_API UEMRLobbyInviteCodeSubsystem : public UGameInstanceSubsystem, public IEMRLoadingProcessInterface
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;

	static const FName LobbyInviteCodeSessionKey;
	static const FName LobbyHostPortSessionKey;

	UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
	FString GenerateInviteCode(int32 Length = 5) const;

	UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
	bool IsInviteCodeValid(const FString& Code) const;

	UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
	bool GetCurrentSessionInviteCode(FString& OutCode) const;

	UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
	bool UpdateSessionInviteCode(const FString& Code);

	UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
	void FindAndJoinSessionByInviteCode(const FString& Code, APlayerController* PlayerController);

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnInviteJoinResult, ELobbyInviteJoinResult);
	FOnInviteJoinResult OnInviteJoinResult;

private:
    void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	void StartJoinRequest(const FString& Code, APlayerController* PlayerController);
	void CancelActiveSearch(const IOnlineSessionPtr& SessionInterface);
	void BeginDestroyExistingSession(const IOnlineSessionPtr& SessionInterface);
	void PromoteDeferredRequest();

	void StartFindSessions(const FString& Code, APlayerController* PlayerController);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void JoinFoundSession(const FOnlineSessionSearchResult& Result);
	void ResetInviteJoinState(bool bClearDeferred);
	FName GetLobbySessionName() const;

	static constexpr int32 InviteCodeLength = 5;

	UPROPERTY(Config)
	FName LobbySessionName;

	TSharedPtr<class FOnlineSessionSearch> InviteCodeSearch;
	FDelegateHandle DestroySessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	FOnlineSessionSearchResult PendingInviteJoinResult;
	FOnlineSessionSearchResult ActiveJoinResult;
	ELobbyInviteJoinState JoinState = ELobbyInviteJoinState::Idle;
	FLobbyInviteJoinRequest ActiveRequest;
	FLobbyInviteJoinRequest DeferredRequest;
	bool bHasDeferredRequest = false;
	bool bSearchRestartPending = false;
	bool bAwaitingJoinTravel = false;
	uint32 NextRequestId = 1;

	// Fallback ports used when GetResolvedConnectString returns a port of 0.
	// This can happen in some local/NULL/LAN setups when the host isn't advertising a resolved port.
	UPROPERTY(Config)
	int32 FallbackJoinPort = 7777;

	UPROPERTY(Config)
	int32 FallbackBeaconPort = 15000;
};
