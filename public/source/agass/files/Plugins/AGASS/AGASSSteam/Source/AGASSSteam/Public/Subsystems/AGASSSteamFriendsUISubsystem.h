#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "AGASSSteamFriendsUISubsystem.generated.h"

class APlayerController;
class UAGASSSteamPlatformSubsystem;

UCLASS()
class AGASSSTEAM_API UAGASSSteamFriendsUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandleQuerySessionInviteUIAvailability(APlayerController* RequestingPlayerController, bool& bOutCanShowInviteUI);
	void HandleShowFriendsUIRequested(APlayerController* RequestingPlayerController);
	void HandleShowSessionInviteUIRequested(APlayerController* RequestingPlayerController);
	UAGASSSteamPlatformSubsystem* GetSteamPlatformSubsystem() const;

	FDelegateHandle QuerySessionInviteUIAvailabilityHandle;
	FDelegateHandle ShowFriendsUIRequestedHandle;
	FDelegateHandle ShowSessionInviteUIRequestedHandle;
};
