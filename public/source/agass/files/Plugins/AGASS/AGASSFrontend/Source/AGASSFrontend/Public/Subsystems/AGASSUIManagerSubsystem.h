#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Templates/SubclassOf.h"
#include "AGASSUIManagerSubsystem.generated.h"

class UAGASSFrontendPrimaryLayoutWidget;
class UAGASSFrontendScreenWidget;
class UAGASSUIPolicy;
class UCommonActivatableWidget;
class ULocalPlayer;
enum class EAGASSSessionFlowState : uint8;

UCLASS()
class AGASSFRONTEND_API UAGASSUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	const UAGASSUIPolicy* GetCurrentUIPolicy() const { return CurrentPolicy; }
	UAGASSUIPolicy* GetCurrentUIPolicy() { return CurrentPolicy; }
	UAGASSFrontendPrimaryLayoutWidget* GetRootLayout(const ULocalPlayer* LocalPlayer) const;
	void HandleRootLayoutReady(ULocalPlayer* LocalPlayer);
	UCommonActivatableWidget* PushWidgetToLayer(FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);
	UCommonActivatableWidget* PushWidgetToLayer(ULocalPlayer* LocalPlayer, FGameplayTag LayerTag, TSubclassOf<UCommonActivatableWidget> WidgetClass);
	void ResetLayerToRoot(FGameplayTag LayerTag);
	void ResetLayerToRoot(ULocalPlayer* LocalPlayer, FGameplayTag LayerTag);
	void ClearLayer(FGameplayTag LayerTag);
	void ClearLayer(ULocalPlayer* LocalPlayer, FGameplayTag LayerTag);

	void ShowMainMenu();
	void ShowMainMenu(ULocalPlayer* LocalPlayer);
	void ShowSessionBrowser();
	void ShowSessionBrowser(ULocalPlayer* LocalPlayer);
	void ShowModsAndMapsScreen();
	void ShowModsAndMapsScreen(ULocalPlayer* LocalPlayer);
	void ShowOptionsScreen();
	void ShowOptionsScreen(ULocalPlayer* LocalPlayer);
	void ShowCreditsScreen();
	void ShowCreditsScreen(ULocalPlayer* LocalPlayer);
	void ShowLoadingScreen();
	void ShowLoadingScreen(ULocalPlayer* LocalPlayer);
	void DismissLoadingScreen();
	void DismissLoadingScreen(ULocalPlayer* LocalPlayer);
	void ShowGameMenu();
	void ShowGameMenu(ULocalPlayer* LocalPlayer);
	void ToggleGameMenu();
	void ToggleGameMenu(ULocalPlayer* LocalPlayer);
	void DismissGameMenu();
	void DismissGameMenu(ULocalPlayer* LocalPlayer);
	void ShowOptionsScreenInGameMenu();
	void ShowOptionsScreenInGameMenu(ULocalPlayer* LocalPlayer);

private:
	static const FName MainMenuScreenId;
	static const FName SessionBrowserScreenId;
	static const FName ModsAndMapsScreenId;
	static const FName OptionsScreenId;
	static const FName GameMenuScreenId;
	static const FName CreditsScreenId;
	static const FName LoadingScreenId;

	bool IsFrontendContext() const;
	bool IsGameplayContext() const;
	bool IsFrontendWorld(const UWorld* World) const;
	bool IsGameplayWorld(const UWorld* World) const;
	bool IsBlockingLoadFlowState(EAGASSSessionFlowState FlowState) const;
	void RegisterDefaultScreens();
	void SwitchToPolicy(UAGASSUIPolicy* InPolicy);
	ULocalPlayer* GetPrimaryLocalPlayer() const;
	TSubclassOf<UAGASSFrontendScreenWidget> GetRegisteredScreenClass(const FName& ScreenId) const;
	UAGASSFrontendPrimaryLayoutWidget* ResolveOrCreateRootLayout(ULocalPlayer* LocalPlayer);
	UAGASSFrontendScreenWidget* EnsureMainMenuRoot(ULocalPlayer* LocalPlayer);
	UAGASSFrontendScreenWidget* PushRegisteredScreenToLayer(ULocalPlayer* LocalPlayer, const FName& ScreenId, const FGameplayTag& LayerTag);
	void ResumeFrontendForLocalPlayer(ULocalPlayer* LocalPlayer);
	bool ShouldForceHardwareBenchmark(ULocalPlayer* LocalPlayer) const;
	void HandleGameplayLayoutReady(ULocalPlayer* LocalPlayer);
	void RestoreGameplayInputForLocalPlayer(ULocalPlayer* LocalPlayer);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleSessionStateChanged();
	void HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer);
	void HandleLocalPlayerRemoved(ULocalPlayer* LocalPlayer);

	UPROPERTY(Transient)
	TMap<FName, TSubclassOf<UAGASSFrontendScreenWidget>> RegisteredScreens;

	UPROPERTY(Transient)
	TObjectPtr<UAGASSUIPolicy> CurrentPolicy;

	FDelegateHandle PostLoadMapHandle;
	FDelegateHandle SessionStateChangedHandle;
	FDelegateHandle LocalPlayerAddedHandle;
	FDelegateHandle LocalPlayerRemovedHandle;
};
