#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AGASSPlatformMenuSubsystem.generated.h"

class APlayerController;

DECLARE_MULTICAST_DELEGATE_OneParam(FAGASSPlatformMenuFriendsUIRequestedEvent, APlayerController*);
DECLARE_MULTICAST_DELEGATE_OneParam(FAGASSPlatformMenuSessionInviteUIRequestedEvent, APlayerController*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FAGASSPlatformMenuSessionInviteUIAvailabilityQueryEvent, APlayerController*, bool&);

UCLASS()
class AGASSCOMMON_API UAGASSPlatformMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool CanShowSessionInviteUI(APlayerController* RequestingPlayerController);
	bool RequestShowFriendsUI(APlayerController* RequestingPlayerController);
	bool RequestShowSessionInviteUI(APlayerController* RequestingPlayerController);

	FAGASSPlatformMenuFriendsUIRequestedEvent& OnShowFriendsUIRequested()
	{
		return FriendsUIRequestedEvent;
	}

	FAGASSPlatformMenuSessionInviteUIRequestedEvent& OnShowSessionInviteUIRequested()
	{
		return SessionInviteUIRequestedEvent;
	}

	FAGASSPlatformMenuSessionInviteUIAvailabilityQueryEvent& OnQuerySessionInviteUIAvailability()
	{
		return SessionInviteUIAvailabilityQueryEvent;
	}

private:
	FAGASSPlatformMenuFriendsUIRequestedEvent FriendsUIRequestedEvent;
	FAGASSPlatformMenuSessionInviteUIRequestedEvent SessionInviteUIRequestedEvent;
	FAGASSPlatformMenuSessionInviteUIAvailabilityQueryEvent SessionInviteUIAvailabilityQueryEvent;
};
