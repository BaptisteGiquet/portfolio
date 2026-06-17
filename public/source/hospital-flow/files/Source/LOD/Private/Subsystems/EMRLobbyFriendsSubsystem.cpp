#include "Subsystems/EMRLobbyFriendsSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemImpl.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"
#include "BlueprintDataDefinitions.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Frontend/Data/EMRListDataObjectLobbyFriend.h"
#include "AdvancedSteamFriendsLibrary.h"

namespace
{
	const FName SteamSubsystemName(TEXT("STEAM"));
	constexpr float FriendPortraitPollIntervalSeconds = 0.5f;
}

void UEMRLobbyFriendsSubsystem::Deinitialize()
{
	StopPortraitPolling();
	PendingFriendPortraits.Reset();
	Super::Deinitialize();
}

void UEMRLobbyFriendsSubsystem::RequestFriendsList(APlayerController* OwningPC)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (!Subsystem)
	{
		return;
	}

	IOnlineFriendsPtr FriendsInterface = Subsystem->GetFriendsInterface();
	if (!FriendsInterface.IsValid())
	{
		return;
	}

	const int32 LocalUserNum = OwningPC && OwningPC->GetLocalPlayer()
	? OwningPC->GetLocalPlayer()->GetControllerId()
	: 0;

	const FString ListName = EFriendsLists::ToString(EFriendsLists::Default);
	const FOnReadFriendsListComplete ReadDelegate = FOnReadFriendsListComplete::CreateUObject(
		this, &ThisClass::HandleReadFriendsListComplete);

	if (!FriendsInterface->ReadFriendsList(LocalUserNum, ListName, ReadDelegate))
	{
	}
}

void UEMRLobbyFriendsSubsystem::HandleReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	IOnlineFriendsPtr FriendsInterface = Subsystem ? Subsystem->GetFriendsInterface() : nullptr;
	CachedFriendItems.Reset();
	CachedFriendItemsView.Reset();
	PendingFriendPortraits.Reset();
	StopPortraitPolling();

	if (!bWasSuccessful || !FriendsInterface.IsValid())
	{
		OnFriendsListUpdated.Broadcast();
		return;
	}

	TArray<TSharedRef<FOnlineFriend>> Friends;
	if (!FriendsInterface->GetFriendsList(LocalUserNum, ListName, Friends))
	{
		OnFriendsListUpdated.Broadcast();
		return;
	}

	BuildFriendItems(Friends);
	OnFriendsListUpdated.Broadcast();
}

void UEMRLobbyFriendsSubsystem::BuildFriendItems(const TArray<TSharedRef<FOnlineFriend>>& Friends)
{
	for (const TSharedRef<FOnlineFriend>& Friend : Friends)
	{
		UEMRListDataObjectLobbyFriend* FriendItem = NewObject<UEMRListDataObjectLobbyFriend>(this);

		const TSharedPtr<const FUniqueNetId> UniqueNetId = Friend->GetUserId();
		if (UniqueNetId.IsValid())
		{
			FBPUniqueNetId BPUniqueNetId;
			BPUniqueNetId.SetUniqueNetId(UniqueNetId);
			FriendItem->SetFriendUniqueNetId(BPUniqueNetId);
		}

		FriendItem->SetFriendName(FText::FromString(Friend->GetDisplayName()));
		FriendItem->SetFriendId(FName(*Friend->GetDisplayName()));
		TryResolveFriendPortrait(FriendItem);

		CachedFriendItems.Add(FriendItem);
	}

	for (const TObjectPtr<UEMRListDataObjectLobbyFriend>& Item : CachedFriendItems)
	{
		CachedFriendItemsView.Add(Item.Get());
	}
}

FString UEMRLobbyFriendsSubsystem::GetFriendPortraitCacheKey(const FBPUniqueNetId& FriendNetId) const
{
	const FUniqueNetId* UniqueNetId = FriendNetId.GetUniqueNetId();
	if (!UniqueNetId)
	{
		return FString();
	}

	return UniqueNetId->ToString();
}

void UEMRLobbyFriendsSubsystem::TryResolveFriendPortrait(UEMRListDataObjectLobbyFriend* FriendItem)
{
	if (!FriendItem)
	{
		return;
	}

	const FBPUniqueNetId& FriendNetId = FriendItem->GetFriendUniqueNetId();
	const FUniqueNetId* UniqueNetId = FriendNetId.GetUniqueNetId();
	if (!UniqueNetId || UniqueNetId->GetType() != SteamSubsystemName)
	{
		return;
	}

	const FString CacheKey = GetFriendPortraitCacheKey(FriendNetId);
	if (CacheKey.IsEmpty())
	{
		return;
	}

	if (const TObjectPtr<UTexture2D>* CachedPortrait = FriendPortraitCache.Find(CacheKey))
	{
		if (*CachedPortrait)
		{
			FriendItem->SetFriendPortraitTexture(*CachedPortrait);
		}
		PendingFriendPortraits.Remove(CacheKey);
		return;
	}

	EBlueprintAsyncResultSwitch Result = EBlueprintAsyncResultSwitch::OnFailure;
	UTexture2D* AvatarTexture = UAdvancedSteamFriendsLibrary::GetSteamFriendAvatar(
		FriendNetId,
		Result,
		SteamAvatarSize::SteamAvatar_Medium);

	if (Result == EBlueprintAsyncResultSwitch::OnSuccess && AvatarTexture)
	{
		FriendPortraitCache.Add(CacheKey, AvatarTexture);
		FriendItem->SetFriendPortraitTexture(AvatarTexture);
		PendingFriendPortraits.Remove(CacheKey);
		return;
	}

	if (Result == EBlueprintAsyncResultSwitch::AsyncLoading)
	{
		const bool bWasPending = PendingFriendPortraits.Contains(CacheKey);
		PendingFriendPortraits.Add(CacheKey, FriendItem);
		if (!bWasPending)
		{
			UAdvancedSteamFriendsLibrary::RequestSteamFriendInfo(FriendNetId, false);
		}
		StartPortraitPolling();
		return;
	}

	PendingFriendPortraits.Remove(CacheKey);
}

void UEMRLobbyFriendsSubsystem::StartPortraitPolling()
{
	UWorld* World = GetWorld();
	if (!World || World->GetTimerManager().IsTimerActive(FriendPortraitPollHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		FriendPortraitPollHandle,
		this,
		&ThisClass::PollPendingFriendPortraits,
		FriendPortraitPollIntervalSeconds,
		true);
}

void UEMRLobbyFriendsSubsystem::StopPortraitPolling()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FriendPortraitPollHandle);
	}
}

void UEMRLobbyFriendsSubsystem::PollPendingFriendPortraits()
{
	if (PendingFriendPortraits.Num() == 0)
	{
		StopPortraitPolling();
		return;
	}

	TArray<FString> PendingKeys;
	PendingFriendPortraits.GetKeys(PendingKeys);

	for (const FString& CacheKey : PendingKeys)
	{
		const TWeakObjectPtr<UEMRListDataObjectLobbyFriend> FriendItem = PendingFriendPortraits.FindRef(CacheKey);
		if (!FriendItem.IsValid())
		{
			PendingFriendPortraits.Remove(CacheKey);
			continue;
		}

		TryResolveFriendPortrait(FriendItem.Get());
	}

	if (PendingFriendPortraits.Num() == 0)
	{
		StopPortraitPolling();
	}
}

bool UEMRLobbyFriendsSubsystem::SendLobbyInvite(const FBPUniqueNetId& FriendId, APlayerController* OwningPC)
{
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

	const FUniqueNetId* UniqueNetId = FriendId.GetUniqueNetId();
	if (!UniqueNetId)
	{
		return false;
	}

	const int32 LocalUserNum = OwningPC && OwningPC->GetLocalPlayer()
	? OwningPC->GetLocalPlayer()->GetControllerId()
	: 0;

	return SessionInterface->SendSessionInviteToFriend(LocalUserNum, GetLobbySessionName(), *UniqueNetId);
}

const TArray<UEMRListDataObjectLobbyFriend*>& UEMRLobbyFriendsSubsystem::GetFriendListItems() const
{
	return CachedFriendItemsView;
}

FName UEMRLobbyFriendsSubsystem::GetLobbySessionName() const
{
	return LobbySessionName.IsNone() ? NAME_GameSession : LobbySessionName;
}
