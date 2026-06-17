

#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyFriendListEntryWidget.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "Styling/SlateBrush.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/Data/EMRListDataObjectLobbyFriend.h"
#include "Subsystems/EMRLobbyFriendsSubsystem.h"
#include "Engine/LocalPlayer.h"


void UEMRFrontendLobbyFriendListEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (CommonButton_Invite)
	{
		CommonButton_Invite->OnClicked().AddUObject(this, &ThisClass::HandleInviteButtonClicked);
	}
}


void UEMRFrontendLobbyFriendListEntryWidget::OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject)
{
	Super::OnOwningListDataObjectSet(InOwningListDataObject);

	CachedFriendDataObject = CastChecked<UEMRListDataObjectLobbyFriend>(InOwningListDataObject);
	RefreshFromData();
}


void UEMRFrontendLobbyFriendListEntryWidget::OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason)
{
	Super::OnOwningListDataObjectModified(OwningModifiedListData, ModifyReason);

	if (OwningModifiedListData == CachedFriendDataObject)
	{
		RefreshFromData();
	}
}


void UEMRFrontendLobbyFriendListEntryWidget::HandleInviteButtonClicked()
{
	SelectThisEntryWidget();

	if (CachedFriendDataObject)
	{
		if (APlayerController* OwningPC = GetOwningPlayer())
		{
			if (ULocalPlayer* LocalPlayer = OwningPC->GetLocalPlayer())
			{
				if (UEMRLobbyFriendsSubsystem* FriendsSubsystem = LocalPlayer->GetSubsystem<UEMRLobbyFriendsSubsystem>())
				{
					FriendsSubsystem->SendLobbyInvite(CachedFriendDataObject->GetFriendUniqueNetId(), OwningPC);
				}
			}
		}

		BP_OnInviteButtonClicked(CachedFriendDataObject);
	}
}


void UEMRFrontendLobbyFriendListEntryWidget::RefreshFromData()
{
	if (!CachedFriendDataObject)
	{
		return;
	}

	if (CommonTextBlock_FriendName)
	{
		CommonTextBlock_FriendName->SetText(CachedFriendDataObject->GetFriendName());
	}

	if (CommonLazyImage_FriendPortrait)
	{
		if (UTexture2D* FriendPortraitTexture = CachedFriendDataObject->GetFriendPortraitTexture())
		{
			CommonLazyImage_FriendPortrait->SetBrushFromTexture(FriendPortraitTexture);
		}
		else if (CachedFriendDataObject->GetFriendPortrait().IsNull())
		{
			CommonLazyImage_FriendPortrait->SetBrush(FSlateBrush());
		}
		else
		{
			CommonLazyImage_FriendPortrait->SetBrushFromLazyTexture(CachedFriendDataObject->GetFriendPortrait());
		}
	}
}
