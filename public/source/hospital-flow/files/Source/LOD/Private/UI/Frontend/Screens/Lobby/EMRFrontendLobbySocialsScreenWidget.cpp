

#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbySocialsScreenWidget.h"

#include "UI/Frontend/Data/EMRListDataObjectLobbyFriend.h"
#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyFriendList.h"
#include "UI/Frontend/Utils/DebugHelper.h"
#include "Subsystems/EMRLobbyFriendsSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GAS/EMRTags.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Core/EMRCommonPrimaryLayoutWidget.h"
#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyScreenWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"


void UEMRFrontendLobbySocialsScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BackActionHandle = RegisterUIActionBinding(FBindUIActionArgs(ICommonInputModule::GetSettings().GetDefaultBackAction(),
		true,
		FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)));

	RefreshFriendsList();
}

void UEMRFrontendLobbySocialsScreenWidget::RefreshFriendsList()
{
	if (APlayerController* OwningPC = GetOwningPlayer())
	{
		if (ULocalPlayer* LocalPlayer = OwningPC->GetLocalPlayer())
		{
			UEMRLobbyFriendsSubsystem* FriendsSubsystem = LocalPlayer->GetSubsystem<UEMRLobbyFriendsSubsystem>();
			if (!FriendsSubsystem)
			{
				return;
			}

			CachedFriendsSubsystem = FriendsSubsystem;
			FriendsSubsystem->OnFriendsListUpdated.RemoveAll(this);
			FriendsSubsystem->OnFriendsListUpdated.AddUObject(this, &ThisClass::HandleFriendsListUpdated);
			FriendsSubsystem->RequestFriendsList(OwningPC);
		}
	}
}

void UEMRFrontendLobbySocialsScreenWidget::HandleFriendsListUpdated()
{
	if (!CachedFriendsSubsystem.IsValid())
	{
		return;
	}

	SetFriendListItems(CachedFriendsSubsystem->GetFriendListItems());
}

void UEMRFrontendLobbySocialsScreenWidget::SetFriendListItems(const TArray<UEMRListDataObjectLobbyFriend*>& InFriendListItems)
{
	if (!CommonListView_FriendList)
	{
		Debug::Print(TEXT("[LobbyFriends] Friend list widget missing on socials screen."));
		return;
	}
	CommonListView_FriendList->SetListItems(InFriendListItems);
	CommonListView_FriendList->RequestRefresh();
	Debug::Print(FString::Printf(TEXT("[LobbyFriends] Friend list updated (%d)."), InFriendListItems.Num()));
}

void UEMRFrontendLobbySocialsScreenWidget::OnBackBoundActionTriggered()
{
	DeactivateWidget();

	UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this);
	if (!UIManagerSubsystem)
	{
		return;
	}

	UEMRCommonPrimaryLayoutWidget* PrimaryLayout = UIManagerSubsystem->GetPrimaryLayoutWidget();
	if (!PrimaryLayout)
	{
		return;
	}

	UCommonActivatableWidgetContainerBase* FrontendStack = PrimaryLayout->FindWidgetStackByTag(EMRTags::UI::WidgetStack::Frontend);
	if (!FrontendStack)
	{
		return;
	}

	for (UCommonActivatableWidget* Widget : FrontendStack->GetWidgetList())
	{
		if (UEMRFrontendLobbyScreenWidget* LobbyScreen = Cast<UEMRFrontendLobbyScreenWidget>(Widget))
		{
			LobbyScreen->FocusInviteFriendsButton();
			break;
		}
	}
}


UWidget* UEMRFrontendLobbySocialsScreenWidget::NativeGetDesiredFocusTarget() const
{
	if (CommonListView_FriendList)
	{
		return CommonListView_FriendList;
	}

	return Super::NativeGetDesiredFocusTarget();
}
