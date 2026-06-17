#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AGASSUIPolicy.generated.h"

class APlayerController;
class UAGASSFrontendPrimaryLayoutWidget;
class UAGASSUIManagerSubsystem;
class ULocalPlayer;

USTRUCT()
struct FAGASSRootViewportLayoutInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TObjectPtr<ULocalPlayer> LocalPlayer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAGASSFrontendPrimaryLayoutWidget> RootLayout = nullptr;

	UPROPERTY(Transient)
	bool bAddedToViewport = false;

	FAGASSRootViewportLayoutInfo() = default;

	FAGASSRootViewportLayoutInfo(ULocalPlayer* InLocalPlayer, UAGASSFrontendPrimaryLayoutWidget* InRootLayout, const bool bInAddedToViewport)
		: LocalPlayer(InLocalPlayer)
		, RootLayout(InRootLayout)
		, bAddedToViewport(bInAddedToViewport)
	{
	}

	bool operator==(const ULocalPlayer* OtherLocalPlayer) const
	{
		return LocalPlayer == OtherLocalPlayer;
	}
};

UCLASS(Blueprintable, Within = AGASSUIManagerSubsystem)
class AGASSFRONTEND_API UAGASSUIPolicy : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	UAGASSUIManagerSubsystem* GetOwningUIManager() const;
	UAGASSFrontendPrimaryLayoutWidget* GetRootLayout(const ULocalPlayer* LocalPlayer) const;

	void NotifyPlayerAdded(ULocalPlayer* LocalPlayer);
	void NotifyPlayerRemoved(ULocalPlayer* LocalPlayer);
	void NotifyPlayerDestroyed(ULocalPlayer* LocalPlayer);
	void SetFrontendActive(bool bInFrontendActive);
	bool IsFrontendActive() const { return bFrontendActive; }

protected:
	virtual void OnRootLayoutAddedToViewport(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout);
	virtual void OnRootLayoutRemovedFromViewport(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout);
	virtual void OnRootLayoutReleased(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout);

	void AddLayoutToViewport(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout);
	void RemoveLayoutFromViewport(ULocalPlayer* LocalPlayer, UAGASSFrontendPrimaryLayoutWidget* Layout);
	void CreateLayoutWidget(ULocalPlayer* LocalPlayer);
	TSubclassOf<UAGASSFrontendPrimaryLayoutWidget> GetLayoutWidgetClass(ULocalPlayer* LocalPlayer) const;

private:
	void HandlePlayerControllerChanged(ULocalPlayer* LocalPlayer, APlayerController* NewPlayerController);

	UPROPERTY(Transient)
	TArray<FAGASSRootViewportLayoutInfo> RootViewportLayouts;

	TMap<ULocalPlayer*, FDelegateHandle> PlayerControllerChangedHandles;
	bool bFrontendActive = false;
};
