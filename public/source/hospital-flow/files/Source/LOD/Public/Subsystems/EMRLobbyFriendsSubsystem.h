#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TimerManager.h"
#include "EMRLobbyFriendsSubsystem.generated.h"

class UEMRListDataObjectLobbyFriend;
struct FBPUniqueNetId;
class APlayerController;
class UTexture2D;

DECLARE_MULTICAST_DELEGATE(FOnLobbyFriendsListUpdated);

UCLASS(Config = Game)
class LOD_API UEMRLobbyFriendsSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Lobby|Friends")
	void RequestFriendsList(APlayerController* OwningPC);

	UFUNCTION(BlueprintCallable, Category = "Lobby|Friends")
	bool SendLobbyInvite(const FBPUniqueNetId& FriendId, APlayerController* OwningPC);

	const TArray<UEMRListDataObjectLobbyFriend*>& GetFriendListItems() const;

	FOnLobbyFriendsListUpdated OnFriendsListUpdated;

private:
	FName GetLobbySessionName() const;
	void HandleReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);
	void BuildFriendItems(const TArray<TSharedRef<class FOnlineFriend>>& Friends);
	FString GetFriendPortraitCacheKey(const FBPUniqueNetId& FriendNetId) const;
	void TryResolveFriendPortrait(UEMRListDataObjectLobbyFriend* FriendItem);
	void StartPortraitPolling();
	void StopPortraitPolling();
	void PollPendingFriendPortraits();

	UPROPERTY(Config)
	FName LobbySessionName;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UEMRListDataObjectLobbyFriend>> CachedFriendItems;

	UPROPERTY(Transient)
	TArray<UEMRListDataObjectLobbyFriend*> CachedFriendItemsView;

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UTexture2D>> FriendPortraitCache;

	TMap<FString, TWeakObjectPtr<UEMRListDataObjectLobbyFriend>> PendingFriendPortraits;
	FTimerHandle FriendPortraitPollHandle;
};
