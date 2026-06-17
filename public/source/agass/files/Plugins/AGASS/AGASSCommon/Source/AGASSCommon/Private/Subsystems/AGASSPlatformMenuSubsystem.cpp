#include "Subsystems/AGASSPlatformMenuSubsystem.h"

#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSPlatformMenu, Log, All);

bool UAGASSPlatformMenuSubsystem::CanShowSessionInviteUI(APlayerController* RequestingPlayerController)
{
	UE_LOG(
		LogAGASSPlatformMenu,
		Display,
		TEXT("CanShowSessionInviteUI controller=%s delegatesBound=%d"),
		*GetNameSafe(RequestingPlayerController),
		SessionInviteUIAvailabilityQueryEvent.IsBound());

	if (RequestingPlayerController == nullptr || !SessionInviteUIAvailabilityQueryEvent.IsBound())
	{
		return false;
	}

	bool bCanShowInviteUI = false;
	SessionInviteUIAvailabilityQueryEvent.Broadcast(RequestingPlayerController, bCanShowInviteUI);
	return bCanShowInviteUI;
}

bool UAGASSPlatformMenuSubsystem::RequestShowFriendsUI(APlayerController* RequestingPlayerController)
{
	UE_LOG(
		LogAGASSPlatformMenu,
		Display,
		TEXT("RequestShowFriendsUI controller=%s delegatesBound=%d"),
		*GetNameSafe(RequestingPlayerController),
		FriendsUIRequestedEvent.IsBound());

	if (RequestingPlayerController == nullptr || !FriendsUIRequestedEvent.IsBound())
	{
		UE_LOG(LogAGASSPlatformMenu, Warning, TEXT("RequestShowFriendsUI rejected."));
		return false;
	}

	FriendsUIRequestedEvent.Broadcast(RequestingPlayerController);
	UE_LOG(LogAGASSPlatformMenu, Display, TEXT("RequestShowFriendsUI broadcast succeeded."));
	return true;
}

bool UAGASSPlatformMenuSubsystem::RequestShowSessionInviteUI(APlayerController* RequestingPlayerController)
{
	UE_LOG(
		LogAGASSPlatformMenu,
		Display,
		TEXT("RequestShowSessionInviteUI controller=%s delegatesBound=%d"),
		*GetNameSafe(RequestingPlayerController),
		SessionInviteUIRequestedEvent.IsBound());

	if (RequestingPlayerController == nullptr || !SessionInviteUIRequestedEvent.IsBound())
	{
		UE_LOG(LogAGASSPlatformMenu, Warning, TEXT("RequestShowSessionInviteUI rejected."));
		return false;
	}

	SessionInviteUIRequestedEvent.Broadcast(RequestingPlayerController);
	UE_LOG(LogAGASSPlatformMenu, Display, TEXT("RequestShowSessionInviteUI broadcast succeeded."));
	return true;
}
