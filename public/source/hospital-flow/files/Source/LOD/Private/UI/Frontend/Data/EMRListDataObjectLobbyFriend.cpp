

#include "UI/Frontend/Data/EMRListDataObjectLobbyFriend.h"

#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"


void UEMRListDataObjectLobbyFriend::SetFriendName(const FText& InFriendName)
{
	FriendName = InFriendName;
	SetDataDisplayName(InFriendName);
	NotifyListDataModified(this, EOptionsListDataModifyReason::DirectlyModified);
}


void UEMRListDataObjectLobbyFriend::SetFriendPortrait(const TSoftObjectPtr<UTexture2D>& InFriendPortrait)
{
	FriendPortrait = InFriendPortrait;
	NotifyListDataModified(this, EOptionsListDataModifyReason::DirectlyModified);
}

void UEMRListDataObjectLobbyFriend::SetFriendPortraitTexture(UTexture2D* InFriendPortraitTexture)
{
	FriendPortraitTexture = InFriendPortraitTexture;
	NotifyListDataModified(this, EOptionsListDataModifyReason::DirectlyModified);
}


void UEMRListDataObjectLobbyFriend::SetFriendId(const FName& InFriendId)
{
	FriendId = InFriendId;
}

void UEMRListDataObjectLobbyFriend::SetFriendUniqueNetId(const FBPUniqueNetId& InFriendUniqueNetId)
{
	FriendUniqueNetId = InFriendUniqueNetId;
}


void UEMRListDataObjectLobbyFriend::SetFriendData(const FName& InFriendId, const FText& InFriendName, const TSoftObjectPtr<UTexture2D>& InFriendPortrait)
{
	FriendId = InFriendId;
	FriendName = InFriendName;
	FriendPortrait = InFriendPortrait;
	SetDataDisplayName(InFriendName);
	NotifyListDataModified(this, EOptionsListDataModifyReason::DirectlyModified);
}


void UEMRListDataObjectLobbyFriend::OnDataObjectInitialized()
{
	Super::OnDataObjectInitialized();

	if (!FriendName.IsEmpty())
	{
		SetDataDisplayName(FriendName);
	}
}
