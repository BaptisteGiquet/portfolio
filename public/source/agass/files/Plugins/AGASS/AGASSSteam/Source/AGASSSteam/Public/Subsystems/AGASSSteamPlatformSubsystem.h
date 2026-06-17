#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Settings/AGASSSteamDeveloperSettings.h"
#include "Subsystems/AGASSGameplayEventSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AGASSSteamPlatformSubsystem.generated.h"

class FAGASSSteamStatsCallbackBridge;
class UAGASSGameplayEventSubsystem;
class UAGASSSessionSubsystem;

USTRUCT(BlueprintType)
struct AGASSSTEAM_API FAGASSSteamTimedRunFriendEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Steam")
	FString FriendId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Steam")
	FString FriendDisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Steam")
	int32 TimeMilliseconds = 0;
};

enum class EAGASSSteamTimedRunFriendsQueryState : uint8
{
	Idle,
	Querying,
	Ready,
	Unavailable,
	Failed
};

DECLARE_MULTICAST_DELEGATE_OneParam(FAGASSSteamTimedRunFriendsUpdatedEvent, const FString&);

UCLASS()
class AGASSSTEAM_API UAGASSSteamPlatformSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual ~UAGASSSteamPlatformSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool IsSteamPlatformActive() const;
	bool CanShowFriendsUI() const;
	bool ShowFriendsUI();
	bool CanShowInviteUI() const;
	bool ShowInviteUI();
	bool SubmitTimedRunResult(const FString& LeaderboardKey, int32 TimeMilliseconds);
	bool QueryTimedRunFriends(const FString& LeaderboardKey);
	const TArray<FAGASSSteamTimedRunFriendEntry>* FindCachedTimedRunFriends(const FString& LeaderboardKey) const;
	EAGASSSteamTimedRunFriendsQueryState GetTimedRunFriendsQueryState(const FString& LeaderboardKey) const;

	FAGASSSteamTimedRunFriendsUpdatedEvent& OnTimedRunFriendsUpdated()
	{
		return TimedRunFriendsUpdatedEvent;
	}

private:
	friend class FAGASSSteamStatsCallbackBridge;

	IOnlineSessionPtr GetSessionInterface() const;
	IOnlineLeaderboardsPtr GetLeaderboardsInterface() const;
	IOnlineIdentityPtr GetIdentityInterface() const;
	bool TryResolveNamedSessionLobbyId(uint64& OutLobbyIdValue) const;
	UAGASSSessionSubsystem* GetSessionSubsystem() const;
	UAGASSGameplayEventSubsystem* GetGameplayEventSubsystem() const;
	const UAGASSSteamDeveloperSettings* GetSteamSettings() const;

	void RefreshSteamDefinitionsFromSettings();
	void BindSessionInterfaceDelegates();
	void UnbindSessionInterfaceDelegates();
	void BindExternalUIDelegates();
	void UnbindExternalUIDelegates();

	void HandleSessionStateChanged();
	void HandleExternalUIChange(bool bIsOpening);
	void HandleGameplayEventReceived(const FAGASSGameplayEvent& GameplayEvent);
	void HandleSessionUserInviteAccepted(bool bWasSuccessful, int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);
	bool StartTimedRunFriendsRead(const FString& LeaderboardKey, uint64 LeaderboardHandle);
	void TickTimedRunFriendsQuery();
	void TickTimedRunFriendsRead();
	void TickTimedRunSubmission();
	void CompleteTimedRunFriendsQuery(const FString& LeaderboardKey, EAGASSSteamTimedRunFriendsQueryState QueryState, bool bResetCachedEntries);

	bool TickStats(float DeltaTime);
	bool EnsureSteamStatsRequested();
	void HandleSteamStatsReady(bool bWasSuccessful);
	void HandleSteamStatsStored(bool bWasSuccessful, int32 ResultCode);
	void HandleSteamAchievementStored(const FString& AchievementId);

	void ProcessQueuedGameplayEvents();
	void ApplyGameplayEvent(const FAGASSGameplayEvent& GameplayEvent);
	double ExtractEventValue(const FAGASSGameplayEvent& GameplayEvent, const FAGASSSteamStatDefinition& StatDefinition) const;
	void UpdateTrackedStat(const FAGASSSteamStatDefinition& StatDefinition, double SourceValue);
	void EvaluateAchievementsForEvent(const FAGASSGameplayEvent& GameplayEvent);
	void EvaluateAchievementsForStat(const FString& SteamStatName);
	void TryUnlockAchievement(const FString& AchievementId);
	double GetTrackedStatValue(const FString& SteamStatName) const;
	void SetTrackedStatValue(const FString& SteamStatName, double NewValue);

	void SyncInFlightPlaytime();
	void StartSessionPlaytimeTracking();
	void StopSessionPlaytimeTracking(bool bFlushBeforeStop);
	void AddPendingPlaytimeStatIfNeeded(bool bForce);

	bool SyncTrackedStateToSteam() const;
	bool TryWriteSteamStat(const FAGASSSteamStatDefinition& StatDefinition, double Value) const;
	void FlushDirtyStats(bool bForce);

	double GetNowSeconds() const;
	bool HasDirtyState() const;
	void MarkDirtyState(const FString* DirtyStatName = nullptr);

	FDelegateHandle SessionStateChangedHandle;
	FDelegateHandle GameplayEventReceivedHandle;
	FDelegateHandle SessionInviteAcceptedHandle;
	FDelegateHandle ExternalUIChangeHandle;
	FTSTicker::FDelegateHandle StatsTickerHandle;

	TArray<FAGASSGameplayEvent> QueuedGameplayEvents;
	TMap<FName, TArray<FAGASSSteamStatDefinition>> StatDefinitionsByEvent;
	TMap<FString, FAGASSSteamStatDefinition> StatDefinitionsByName;
	TArray<FAGASSSteamAchievementDefinition> AchievementDefinitions;
	TMap<FName, int32> GameplayEventCounts;
	TMap<FString, double> TrackedStatValues;
	TSet<FString> DirtyStatNames;
	TSet<FString> UnlockedAchievementIds;
	TMap<FString, TArray<FAGASSSteamTimedRunFriendEntry>> TimedRunFriendsCache;
	TMap<FString, EAGASSSteamTimedRunFriendsQueryState> TimedRunFriendsQueryStates;
	FAGASSSteamStatsCallbackBridge* SteamStatsCallbackBridge = nullptr;
	FAGASSSteamTimedRunFriendsUpdatedEvent TimedRunFriendsUpdatedEvent;

	double LastStatsStoreTimeSeconds = 0.0;
	double SessionPlaytimeTrackingStartSeconds = 0.0;
	double PendingPlaytimeSeconds = 0.0;
	uint64 DirtyGeneration = 0;
	uint64 LastSubmittedGeneration = 0;
	uint64 LastAcknowledgedGeneration = 0;
	bool bSteamStatsRequestSubmitted = false;
	bool bSteamStatsReady = false;
	bool bStoreInFlight = false;
	bool bTrackingSessionPlaytime = false;
	uint64 InFlightTimedRunLeaderboardLookupHandle = 0;
	FString InFlightTimedRunLeaderboardLookupKey;
	uint64 InFlightTimedRunLeaderboardReadHandle = 0;
	uint64 InFlightTimedRunLeaderboardReadLeaderboardHandle = 0;
	FString InFlightTimedRunLeaderboardReadKey;
	uint64 InFlightTimedRunSubmitLookupHandle = 0;
	FString InFlightTimedRunSubmitKey;
	int32 InFlightTimedRunSubmitMilliseconds = 0;
	uint64 InFlightTimedRunSubmitUploadHandle = 0;
	uint64 InFlightTimedRunSubmitLeaderboardHandle = 0;
};
