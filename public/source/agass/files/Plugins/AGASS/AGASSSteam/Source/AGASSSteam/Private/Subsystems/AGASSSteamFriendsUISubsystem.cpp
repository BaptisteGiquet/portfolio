#include "Subsystems/AGASSSteamFriendsUISubsystem.h"

#include "Subsystems/AGASSPlatformMenuSubsystem.h"
#include "Subsystems/AGASSSteamPlatformSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSSteamFriendsUI, Log, All);

namespace
{
	bool DoesRequestMatchLocalPlayer(
		const ULocalPlayer* LocalPlayer,
		UWorld* World,
		const APlayerController* RequestingPlayerController)
	{
		if (LocalPlayer == nullptr || RequestingPlayerController == nullptr)
		{
			return false;
		}

		if (RequestingPlayerController->GetLocalPlayer() == LocalPlayer)
		{
			return true;
		}

		return RequestingPlayerController == LocalPlayer->GetPlayerController(World);
	}
}

void UAGASSSteamFriendsUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(
		LogAGASSSteamFriendsUI,
		Display,
		TEXT("Initialize localPlayer=%s gameInstance=%s"),
		*GetNameSafe(GetLocalPlayer()),
		*GetNameSafe(GetLocalPlayer() != nullptr ? GetLocalPlayer()->GetGameInstance() : nullptr));

	if (UGameInstance* GameInstance = GetLocalPlayer() != nullptr ? GetLocalPlayer()->GetGameInstance() : nullptr)
	{
		if (UAGASSPlatformMenuSubsystem* PlatformMenuSubsystem = GameInstance->GetSubsystem<UAGASSPlatformMenuSubsystem>())
		{
			QuerySessionInviteUIAvailabilityHandle = PlatformMenuSubsystem->OnQuerySessionInviteUIAvailability().AddUObject(this, &ThisClass::HandleQuerySessionInviteUIAvailability);
			ShowFriendsUIRequestedHandle = PlatformMenuSubsystem->OnShowFriendsUIRequested().AddUObject(this, &ThisClass::HandleShowFriendsUIRequested);
			ShowSessionInviteUIRequestedHandle = PlatformMenuSubsystem->OnShowSessionInviteUIRequested().AddUObject(this, &ThisClass::HandleShowSessionInviteUIRequested);
			UE_LOG(
				LogAGASSSteamFriendsUI,
				Display,
				TEXT("Bound platform menu delegates localPlayer=%s queryHandleValid=%d friendsHandleValid=%d inviteHandleValid=%d"),
				*GetNameSafe(GetLocalPlayer()),
				QuerySessionInviteUIAvailabilityHandle.IsValid(),
				ShowFriendsUIRequestedHandle.IsValid(),
				ShowSessionInviteUIRequestedHandle.IsValid());
		}
		else
		{
			UE_LOG(LogAGASSSteamFriendsUI, Warning, TEXT("Initialize failed to find AGASSPlatformMenuSubsystem."));
		}
	}
	else
	{
		UE_LOG(LogAGASSSteamFriendsUI, Warning, TEXT("Initialize failed because LocalPlayer or GameInstance was null."));
	}
}

void UAGASSSteamFriendsUISubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetLocalPlayer() != nullptr ? GetLocalPlayer()->GetGameInstance() : nullptr)
	{
		if (UAGASSPlatformMenuSubsystem* PlatformMenuSubsystem = GameInstance->GetSubsystem<UAGASSPlatformMenuSubsystem>())
		{
			if (QuerySessionInviteUIAvailabilityHandle.IsValid())
			{
				PlatformMenuSubsystem->OnQuerySessionInviteUIAvailability().Remove(QuerySessionInviteUIAvailabilityHandle);
				QuerySessionInviteUIAvailabilityHandle.Reset();
			}

			if (ShowFriendsUIRequestedHandle.IsValid())
			{
				PlatformMenuSubsystem->OnShowFriendsUIRequested().Remove(ShowFriendsUIRequestedHandle);
				ShowFriendsUIRequestedHandle.Reset();
			}

			if (ShowSessionInviteUIRequestedHandle.IsValid())
			{
				PlatformMenuSubsystem->OnShowSessionInviteUIRequested().Remove(ShowSessionInviteUIRequestedHandle);
				ShowSessionInviteUIRequestedHandle.Reset();
			}
		}
	}

	Super::Deinitialize();
}

void UAGASSSteamFriendsUISubsystem::HandleQuerySessionInviteUIAvailability(
	APlayerController* RequestingPlayerController,
	bool& bOutCanShowInviteUI)
{
	if (bOutCanShowInviteUI || !DoesRequestMatchLocalPlayer(GetLocalPlayer(), GetWorld(), RequestingPlayerController))
	{
		return;
	}

	if (UAGASSSteamPlatformSubsystem* SteamPlatformSubsystem = GetSteamPlatformSubsystem())
	{
		bOutCanShowInviteUI = SteamPlatformSubsystem->CanShowInviteUI();
	}
}

void UAGASSSteamFriendsUISubsystem::HandleShowFriendsUIRequested(APlayerController* RequestingPlayerController)
{
	if (!DoesRequestMatchLocalPlayer(GetLocalPlayer(), GetWorld(), RequestingPlayerController))
	{
		UE_LOG(
			LogAGASSSteamFriendsUI,
			Verbose,
			TEXT("Ignoring Steam friends UI request because it does not target the active local player. RequestController=%s LocalPlayer=%s"),
			*GetNameSafe(RequestingPlayerController),
			*GetNameSafe(GetLocalPlayer()));
		return;
	}

	if (UAGASSSteamPlatformSubsystem* SteamPlatformSubsystem = GetSteamPlatformSubsystem())
	{
		if (!SteamPlatformSubsystem->ShowFriendsUI())
		{
			UE_LOG(LogAGASSSteamFriendsUI, Warning, TEXT("Steam friends UI request was accepted but ShowFriendsUI returned false."));
		}
	}
}

void UAGASSSteamFriendsUISubsystem::HandleShowSessionInviteUIRequested(APlayerController* RequestingPlayerController)
{
	UE_LOG(
		LogAGASSSteamFriendsUI,
		Display,
		TEXT("HandleShowSessionInviteUIRequested controller=%s localPlayer=%s world=%s"),
		*GetNameSafe(RequestingPlayerController),
		*GetNameSafe(GetLocalPlayer()),
		*GetNameSafe(GetWorld()));

	if (!DoesRequestMatchLocalPlayer(GetLocalPlayer(), GetWorld(), RequestingPlayerController))
	{
		UE_LOG(
			LogAGASSSteamFriendsUI,
			Verbose,
			TEXT("Ignoring Steam invite UI request because it does not target the active local player. RequestController=%s LocalPlayer=%s"),
			*GetNameSafe(RequestingPlayerController),
			*GetNameSafe(GetLocalPlayer()));
		return;
	}

	if (UAGASSSteamPlatformSubsystem* SteamPlatformSubsystem = GetSteamPlatformSubsystem())
	{
		if (!SteamPlatformSubsystem->ShowInviteUI())
		{
			UE_LOG(LogAGASSSteamFriendsUI, Warning, TEXT("Steam session invite UI request was accepted but ShowInviteUI returned false."));
		}
		else
		{
			UE_LOG(LogAGASSSteamFriendsUI, Display, TEXT("Steam session invite UI request reached ShowInviteUI successfully."));
		}
	}
	else
	{
		UE_LOG(LogAGASSSteamFriendsUI, Warning, TEXT("Steam session invite UI request could not find UAGASSSteamPlatformSubsystem."));
	}
}

UAGASSSteamPlatformSubsystem* UAGASSSteamFriendsUISubsystem::GetSteamPlatformSubsystem() const
{
	UGameInstance* const GameInstance = GetLocalPlayer() != nullptr ? GetLocalPlayer()->GetGameInstance() : nullptr;
	return GameInstance != nullptr ? GameInstance->GetSubsystem<UAGASSSteamPlatformSubsystem>() : nullptr;
}
