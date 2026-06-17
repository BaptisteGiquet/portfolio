#include "Subsystems/AGASSUIManagerSubsystem.h"

#include "CommonActivatableWidget.h"
#include "CommonInputModeTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Settings/AGASSSettingsLocal.h"
#include "Settings/AGASSFrontendDeveloperSettings.h"
#include "Settings/AGASSOnlineDeveloperSettings.h"
#include "Subsystems/AGASSSessionSubsystem.h"
#include "Subsystems/AGASSUIPolicy.h"
#include "UI/AGASSCreditsScreenWidget.h"
#include "UI/AGASSFrontendPrimaryLayoutWidget.h"
#include "UI/AGASSFrontendScreenWidget.h"
#include "UI/AGASSFrontendUITags.h"
#include "UI/AGASSGameMenuScreenWidget.h"
#include "UI/AGASSLoadingScreenWidget.h"
#include "UI/AGASSMainMenuScreenWidget.h"
#include "UI/AGASSModsAndMapsScreenWidget.h"
#include "UI/AGASSOptionsScreenWidget.h"
#include "UI/AGASSSessionBrowserScreenWidget.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSFrontendUI, Log, All);

const FName UAGASSUIManagerSubsystem::MainMenuScreenId(TEXT("MainMenu"));
const FName UAGASSUIManagerSubsystem::SessionBrowserScreenId(TEXT("SessionBrowser"));
const FName UAGASSUIManagerSubsystem::ModsAndMapsScreenId(TEXT("ModsAndMaps"));
const FName UAGASSUIManagerSubsystem::OptionsScreenId(TEXT("Options"));
const FName UAGASSUIManagerSubsystem::GameMenuScreenId(TEXT("GameMenu"));
const FName UAGASSUIManagerSubsystem::CreditsScreenId(TEXT("Credits"));
const FName UAGASSUIManagerSubsystem::LoadingScreenId(TEXT("Loading"));

namespace
{
	TSubclassOf<UAGASSFrontendScreenWidget> ResolveScreenClassOrFallback(
		const TCHAR* ScreenLabel,
		const TSoftClassPtr<UAGASSFrontendScreenWidget>& DesiredClass,
		TSubclassOf<UAGASSFrontendScreenWidget> FallbackClass)
	{
		if (TSubclassOf<UAGASSFrontendScreenWidget> LoadedClass = DesiredClass.LoadSynchronous())
		{
			return LoadedClass;
		}

		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("Screen %s failed to load configured class %s. Falling back to native class %s."),
			ScreenLabel,
			*DesiredClass.ToSoftObjectPath().ToString(),
			*GetNameSafe(FallbackClass));
		return FallbackClass;
	}
}

void UAGASSUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency(UAGASSSessionSubsystem::StaticClass());

	UE_LOG(LogAGASSFrontendUI, Log, TEXT("UIManager Initialize in world %s."), *GetNameSafe(GetWorld()));

	RegisterDefaultScreens();

	const UAGASSFrontendDeveloperSettings* FrontendSettings = UAGASSFrontendDeveloperSettings::Get();
	TSubclassOf<UAGASSUIPolicy> PolicyClass = FrontendSettings != nullptr ? FrontendSettings->DefaultUIPolicyClass.LoadSynchronous() : nullptr;
	if (PolicyClass == nullptr)
	{
		PolicyClass = UAGASSUIPolicy::StaticClass();
	}

	SwitchToPolicy(NewObject<UAGASSUIPolicy>(this, PolicyClass));
	UE_LOG(LogAGASSFrontendUI, Log, TEXT("Using UI policy class %s. CurrentPolicy=%s"), *GetNameSafe(PolicyClass), *GetNameSafe(CurrentPolicy));
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UAGASSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			SessionStateChangedHandle = SessionSubsystem->OnSessionStateChanged().AddUObject(this, &ThisClass::HandleSessionStateChanged);
		}

		LocalPlayerAddedHandle = GameInstance->OnLocalPlayerAddedEvent.AddUObject(this, &ThisClass::HandleLocalPlayerAdded);
		LocalPlayerRemovedHandle = GameInstance->OnLocalPlayerRemovedEvent.AddUObject(this, &ThisClass::HandleLocalPlayerRemoved);

		if (CurrentPolicy != nullptr)
		{
			for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
			{
				CurrentPolicy->NotifyPlayerAdded(LocalPlayer);
			}
		}
	}

	HandlePostLoadMap(GetWorld());
}

void UAGASSUIManagerSubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (CurrentPolicy != nullptr)
		{
			for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
			{
				CurrentPolicy->NotifyPlayerDestroyed(LocalPlayer);
			}
		}

		if (LocalPlayerAddedHandle.IsValid())
		{
			GameInstance->OnLocalPlayerAddedEvent.Remove(LocalPlayerAddedHandle);
			LocalPlayerAddedHandle.Reset();
		}

		if (LocalPlayerRemovedHandle.IsValid())
		{
			GameInstance->OnLocalPlayerRemovedEvent.Remove(LocalPlayerRemovedHandle);
			LocalPlayerRemovedHandle.Reset();
		}

		if (UAGASSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			if (SessionStateChangedHandle.IsValid())
			{
				SessionSubsystem->OnSessionStateChanged().Remove(SessionStateChangedHandle);
				SessionStateChangedHandle.Reset();
			}
		}
	}

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	SwitchToPolicy(nullptr);
	Super::Deinitialize();
}

bool UAGASSUIManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UGameInstance* GameInstance = Cast<UGameInstance>(Outer))
	{
		return !GameInstance->IsDedicatedServerInstance();
	}

	return false;
}

UAGASSFrontendPrimaryLayoutWidget* UAGASSUIManagerSubsystem::GetRootLayout(const ULocalPlayer* LocalPlayer) const
{
	return CurrentPolicy != nullptr ? CurrentPolicy->GetRootLayout(LocalPlayer) : nullptr;
}

void UAGASSUIManagerSubsystem::HandleRootLayoutReady(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("HandleRootLayoutReady called with null LocalPlayer."));
		return;
	}

	if (IsGameplayContext())
	{
		UE_LOG(LogAGASSFrontendUI, Log, TEXT("HandleRootLayoutReady applying gameplay UI cleanup for %s in world %s."),
			*GetNameSafe(LocalPlayer),
			*GetNameSafe(GetWorld()));
		HandleGameplayLayoutReady(LocalPlayer);
		return;
	}

	EAGASSSessionFlowState FlowState = EAGASSSessionFlowState::Idle;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UAGASSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			FlowState = SessionSubsystem->GetSessionFlowState();
		}
	}

	UE_LOG(LogAGASSFrontendUI, Log, TEXT("HandleRootLayoutReady resuming frontend flow for %s. FlowState=%d Layout=%s"),
		*GetNameSafe(LocalPlayer),
		static_cast<int32>(FlowState),
		*GetNameSafe(GetRootLayout(LocalPlayer)));

	ResumeFrontendForLocalPlayer(LocalPlayer);
}

UCommonActivatableWidget* UAGASSUIManagerSubsystem::PushWidgetToLayer(
	const FGameplayTag LayerTag,
	TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	return PushWidgetToLayer(GetPrimaryLocalPlayer(), LayerTag, WidgetClass);
}

UCommonActivatableWidget* UAGASSUIManagerSubsystem::PushWidgetToLayer(
	ULocalPlayer* LocalPlayer,
	const FGameplayTag LayerTag,
	TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (LocalPlayer == nullptr || WidgetClass == nullptr)
	{
		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("PushWidgetToLayer failed. LocalPlayer=%s WidgetClass=%s Layer=%s"),
			*GetNameSafe(LocalPlayer),
			*GetNameSafe(WidgetClass),
			*LayerTag.ToString());
		return nullptr;
	}

	UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer);
	if (Layout == nullptr)
	{
		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("PushWidgetToLayer failed for %s because root layout is null. Layer=%s WidgetClass=%s"),
			*GetNameSafe(LocalPlayer),
			*LayerTag.ToString(),
			*GetNameSafe(WidgetClass));
		return nullptr;
	}

	const TSubclassOf<UAGASSFrontendScreenWidget> MainMenuClass = GetRegisteredScreenClass(MainMenuScreenId);
	if (IsFrontendWorld(GetWorld())
		&& WidgetClass != MainMenuClass
		&& (LayerTag == AGASSFrontendTags::TAG_UI_LAYER_MENU || LayerTag == AGASSFrontendTags::TAG_UI_LAYER_MODAL))
	{
		EnsureMainMenuRoot(LocalPlayer);
	}

	if (UCommonActivatableWidget* ActiveWidget = Layout->GetActiveWidgetInLayer(LayerTag))
	{
		if (ActiveWidget->IsA(WidgetClass))
		{
			UE_LOG(LogAGASSFrontendUI, Log, TEXT("PushWidgetToLayer returning existing active widget %s for layer %s on %s."),
				*GetNameSafe(ActiveWidget),
				*LayerTag.ToString(),
				*GetNameSafe(Layout));
			return ActiveWidget;
		}
	}

	UCommonActivatableWidget* NewWidget = Layout->PushWidgetToLayerStack<UCommonActivatableWidget>(LayerTag, WidgetClass);
	UE_LOG(LogAGASSFrontendUI, Log, TEXT("PushWidgetToLayer attempted add to layer %s on %s. WidgetClass=%s Result=%s LayerWidget=%s"),
		*LayerTag.ToString(),
		*GetNameSafe(Layout),
		*GetNameSafe(WidgetClass),
		*GetNameSafe(NewWidget),
		*GetNameSafe(Layout->GetLayerWidget(LayerTag)));
	return NewWidget;
}

void UAGASSUIManagerSubsystem::ResetLayerToRoot(const FGameplayTag LayerTag)
{
	ResetLayerToRoot(GetPrimaryLocalPlayer(), LayerTag);
}

void UAGASSUIManagerSubsystem::ResetLayerToRoot(ULocalPlayer* LocalPlayer, const FGameplayTag LayerTag)
{
	if (LocalPlayer == nullptr)
	{
		return;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* Layout = GetRootLayout(LocalPlayer))
	{
		Layout->ResetLayerToRoot(LayerTag);
	}
}

void UAGASSUIManagerSubsystem::ClearLayer(const FGameplayTag LayerTag)
{
	ClearLayer(GetPrimaryLocalPlayer(), LayerTag);
}

void UAGASSUIManagerSubsystem::ClearLayer(ULocalPlayer* LocalPlayer, const FGameplayTag LayerTag)
{
	if (LocalPlayer == nullptr)
	{
		return;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* Layout = GetRootLayout(LocalPlayer))
	{
		Layout->ClearLayer(LayerTag);
	}
}

void UAGASSUIManagerSubsystem::ShowMainMenu()
{
	ShowMainMenu(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::ShowMainMenu(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsFrontendContext())
	{
		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("ShowMainMenu skipped. LocalPlayer=%s World=%s IsFrontendContext=%s"),
			*GetNameSafe(LocalPlayer),
			*GetNameSafe(GetWorld()),
			IsFrontendContext() ? TEXT("true") : TEXT("false"));
		return;
	}

	UE_LOG(LogAGASSFrontendUI, Log, TEXT("ShowMainMenu for %s in world %s."), *GetNameSafe(LocalPlayer), *GetNameSafe(GetWorld()));
	if (UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer))
	{
		Layout->SetFrontendShellVisible(true);
	}
	EnsureMainMenuRoot(LocalPlayer);
	ResetLayerToRoot(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MENU);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
}

void UAGASSUIManagerSubsystem::ShowSessionBrowser()
{
	ShowSessionBrowser(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::ShowSessionBrowser(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsFrontendContext())
	{
		return;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer))
	{
		Layout->SetFrontendShellVisible(true);
	}
	EnsureMainMenuRoot(LocalPlayer);
	ResetLayerToRoot(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MENU);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
	PushRegisteredScreenToLayer(LocalPlayer, SessionBrowserScreenId, AGASSFrontendTags::TAG_UI_LAYER_MENU);
}

void UAGASSUIManagerSubsystem::ShowModsAndMapsScreen()
{
	ShowModsAndMapsScreen(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::ShowModsAndMapsScreen(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsFrontendContext())
	{
		return;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer))
	{
		Layout->SetFrontendShellVisible(true);
	}
	EnsureMainMenuRoot(LocalPlayer);
	ResetLayerToRoot(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MENU);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
	PushRegisteredScreenToLayer(LocalPlayer, ModsAndMapsScreenId, AGASSFrontendTags::TAG_UI_LAYER_MENU);
}

void UAGASSUIManagerSubsystem::ShowOptionsScreen()
{
	ShowOptionsScreen(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::ShowOptionsScreen(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsFrontendContext())
	{
		return;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer))
	{
		Layout->SetFrontendShellVisible(true);
	}
	EnsureMainMenuRoot(LocalPlayer);
	ResetLayerToRoot(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MENU);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
	PushRegisteredScreenToLayer(LocalPlayer, OptionsScreenId, AGASSFrontendTags::TAG_UI_LAYER_MENU);
}

void UAGASSUIManagerSubsystem::ShowCreditsScreen()
{
	ShowCreditsScreen(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::ShowCreditsScreen(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsFrontendContext())
	{
		return;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer))
	{
		Layout->SetFrontendShellVisible(true);
	}
	EnsureMainMenuRoot(LocalPlayer);
	ResetLayerToRoot(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MENU);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
	PushRegisteredScreenToLayer(LocalPlayer, CreditsScreenId, AGASSFrontendTags::TAG_UI_LAYER_MENU);
}

void UAGASSUIManagerSubsystem::ShowLoadingScreen()
{
	ShowLoadingScreen(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::ShowLoadingScreen(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsFrontendContext())
	{
		return;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer))
	{
		Layout->SetFrontendShellVisible(true);
	}
	EnsureMainMenuRoot(LocalPlayer);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
	PushRegisteredScreenToLayer(LocalPlayer, LoadingScreenId, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
}

void UAGASSUIManagerSubsystem::DismissLoadingScreen()
{
	DismissLoadingScreen(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::DismissLoadingScreen(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr)
	{
		return;
	}

	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
}

void UAGASSUIManagerSubsystem::ShowGameMenu()
{
	ShowGameMenu(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::ShowGameMenu(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsGameplayContext())
	{
		return;
	}

	if (const UAGASSSessionSubsystem* SessionSubsystem = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UAGASSSessionSubsystem>() : nullptr)
	{
		if (SessionSubsystem->GetSessionFlowState() != EAGASSSessionFlowState::InSession)
		{
			return;
		}
	}

	ResolveOrCreateRootLayout(LocalPlayer);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MENU);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
	PushRegisteredScreenToLayer(LocalPlayer, GameMenuScreenId, AGASSFrontendTags::TAG_UI_LAYER_GAMEMENU);
}

void UAGASSUIManagerSubsystem::ToggleGameMenu()
{
	ToggleGameMenu(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::ToggleGameMenu(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsGameplayContext())
	{
		return;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer))
	{
		if (Layout->GetActiveWidgetInLayer(AGASSFrontendTags::TAG_UI_LAYER_GAMEMENU) != nullptr)
		{
			DismissGameMenu(LocalPlayer);
			return;
		}
	}

	ShowGameMenu(LocalPlayer);
}

void UAGASSUIManagerSubsystem::DismissGameMenu()
{
	DismissGameMenu(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::DismissGameMenu(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr)
	{
		return;
	}

	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_GAMEMENU);
	RestoreGameplayInputForLocalPlayer(LocalPlayer);
}

void UAGASSUIManagerSubsystem::ShowOptionsScreenInGameMenu()
{
	ShowOptionsScreenInGameMenu(GetPrimaryLocalPlayer());
}

void UAGASSUIManagerSubsystem::ShowOptionsScreenInGameMenu(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr)
	{
		return;
	}

	if (IsFrontendContext())
	{
		ShowOptionsScreen(LocalPlayer);
		return;
	}

	if (!IsGameplayContext())
	{
		return;
	}

	ShowGameMenu(LocalPlayer);
	PushRegisteredScreenToLayer(LocalPlayer, OptionsScreenId, AGASSFrontendTags::TAG_UI_LAYER_GAMEMENU);
}

bool UAGASSUIManagerSubsystem::IsFrontendWorld(const UWorld* World) const
{
	const UAGASSOnlineDeveloperSettings* OnlineSettings = UAGASSOnlineDeveloperSettings::Get();
	const FString FrontendPackagePath = OnlineSettings ? OnlineSettings->FrontendMap.ToSoftObjectPath().GetLongPackageName() : FString();
	return World != nullptr && World->IsGameWorld() && !FrontendPackagePath.IsEmpty()
		&& UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) == FrontendPackagePath;
}

bool UAGASSUIManagerSubsystem::IsFrontendContext() const
{
	if (CurrentPolicy != nullptr)
	{
		return CurrentPolicy->IsFrontendActive();
	}

	return IsFrontendWorld(GetWorld());
}

bool UAGASSUIManagerSubsystem::IsGameplayWorld(const UWorld* World) const
{
	return World != nullptr && World->IsGameWorld() && !IsFrontendWorld(World);
}

bool UAGASSUIManagerSubsystem::IsGameplayContext() const
{
	const UWorld* World = GetWorld();
	return World != nullptr && World->IsGameWorld() && !IsFrontendContext();
}

bool UAGASSUIManagerSubsystem::IsBlockingLoadFlowState(const EAGASSSessionFlowState FlowState) const
{
	return FlowState == EAGASSSessionFlowState::Hosting
		|| FlowState == EAGASSSessionFlowState::Joining
		|| FlowState == EAGASSSessionFlowState::ReturningToFrontend;
}

void UAGASSUIManagerSubsystem::RegisterDefaultScreens()
{
	const UAGASSFrontendDeveloperSettings* Settings = UAGASSFrontendDeveloperSettings::Get();
	if (Settings == nullptr)
	{
		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("RegisterDefaultScreens failed because frontend settings are null."));
		return;
	}

	RegisteredScreens.Reset();
	RegisteredScreens.Add(MainMenuScreenId, ResolveScreenClassOrFallback(TEXT("MainMenu"), Settings->MainMenuScreenClass, UAGASSMainMenuScreenWidget::StaticClass()));
	RegisteredScreens.Add(SessionBrowserScreenId, ResolveScreenClassOrFallback(TEXT("SessionBrowser"), Settings->SessionBrowserScreenClass, UAGASSSessionBrowserScreenWidget::StaticClass()));
	RegisteredScreens.Add(ModsAndMapsScreenId, ResolveScreenClassOrFallback(TEXT("ModsAndMaps"), Settings->ModsAndMapsScreenClass, UAGASSModsAndMapsScreenWidget::StaticClass()));
	RegisteredScreens.Add(OptionsScreenId, ResolveScreenClassOrFallback(TEXT("Options"), Settings->OptionsScreenClass, UAGASSOptionsScreenWidget::StaticClass()));
	RegisteredScreens.Add(GameMenuScreenId, ResolveScreenClassOrFallback(TEXT("GameMenu"), Settings->GameMenuScreenClass, UAGASSGameMenuScreenWidget::StaticClass()));
	RegisteredScreens.Add(CreditsScreenId, ResolveScreenClassOrFallback(TEXT("Credits"), Settings->CreditsScreenClass, UAGASSCreditsScreenWidget::StaticClass()));
	RegisteredScreens.Add(LoadingScreenId, ResolveScreenClassOrFallback(TEXT("Loading"), Settings->LoadingScreenClass, UAGASSLoadingScreenWidget::StaticClass()));

	for (const TPair<FName, TSubclassOf<UAGASSFrontendScreenWidget>>& Pair : RegisteredScreens)
	{
		UE_LOG(LogAGASSFrontendUI, Log, TEXT("Registered screen %s => %s"), *Pair.Key.ToString(), *GetNameSafe(Pair.Value));
	}
}

void UAGASSUIManagerSubsystem::SwitchToPolicy(UAGASSUIPolicy* InPolicy)
{
	if (CurrentPolicy == InPolicy)
	{
		return;
	}

	if (CurrentPolicy != nullptr)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
			{
				CurrentPolicy->NotifyPlayerDestroyed(LocalPlayer);
			}
		}
	}

	CurrentPolicy = InPolicy;
}

ULocalPlayer* UAGASSUIManagerSubsystem::GetPrimaryLocalPlayer() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		const TArray<ULocalPlayer*>& LocalPlayers = GameInstance->GetLocalPlayers();
		return LocalPlayers.IsEmpty() ? nullptr : LocalPlayers[0];
	}

	return nullptr;
}

TSubclassOf<UAGASSFrontendScreenWidget> UAGASSUIManagerSubsystem::GetRegisteredScreenClass(const FName& ScreenId) const
{
	if (const TSubclassOf<UAGASSFrontendScreenWidget>* ScreenClass = RegisteredScreens.Find(ScreenId))
	{
		if (*ScreenClass == nullptr)
		{
			UE_LOG(LogAGASSFrontendUI, Warning, TEXT("Registered screen %s resolved to null at lookup time."), *ScreenId.ToString());
		}
		return *ScreenClass;
	}

	UE_LOG(LogAGASSFrontendUI, Warning, TEXT("Registered screen %s was not found at lookup time."), *ScreenId.ToString());
	return nullptr;
}

UAGASSFrontendPrimaryLayoutWidget* UAGASSUIManagerSubsystem::ResolveOrCreateRootLayout(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || CurrentPolicy == nullptr)
	{
		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("ResolveOrCreateRootLayout failed. LocalPlayer=%s CurrentPolicy=%s"),
			*GetNameSafe(LocalPlayer),
			*GetNameSafe(CurrentPolicy));
		return nullptr;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* ExistingLayout = CurrentPolicy->GetRootLayout(LocalPlayer))
	{
		UE_LOG(LogAGASSFrontendUI, Log, TEXT("ResolveOrCreateRootLayout reusing %s for %s."),
			*GetNameSafe(ExistingLayout),
			*GetNameSafe(LocalPlayer));
		return ExistingLayout;
	}

	UE_LOG(LogAGASSFrontendUI, Log, TEXT("ResolveOrCreateRootLayout creating new layout for %s."), *GetNameSafe(LocalPlayer));
	CurrentPolicy->NotifyPlayerAdded(LocalPlayer);
	UAGASSFrontendPrimaryLayoutWidget* Layout = CurrentPolicy->GetRootLayout(LocalPlayer);
	UE_LOG(LogAGASSFrontendUI, Log, TEXT("ResolveOrCreateRootLayout result for %s => %s"),
		*GetNameSafe(LocalPlayer),
		*GetNameSafe(Layout));
	return Layout;
}

UAGASSFrontendScreenWidget* UAGASSUIManagerSubsystem::EnsureMainMenuRoot(ULocalPlayer* LocalPlayer)
{
	UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer);
	if (Layout == nullptr)
	{
		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("EnsureMainMenuRoot failed because layout is null for %s."), *GetNameSafe(LocalPlayer));
		return nullptr;
	}

	const TSubclassOf<UAGASSFrontendScreenWidget> MainMenuClass = GetRegisteredScreenClass(MainMenuScreenId);
	if (MainMenuClass == nullptr)
	{
		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("EnsureMainMenuRoot failed because MainMenu class is null."));
		return nullptr;
	}

	UAGASSFrontendScreenWidget* RootWidget = Cast<UAGASSFrontendScreenWidget>(Layout->GetLayerRootWidget(AGASSFrontendTags::TAG_UI_LAYER_MENU));
	UE_LOG(LogAGASSFrontendUI, Log, TEXT("EnsureMainMenuRoot on layout %s. ExistingRoot=%s MenuLayerWidget=%s MainMenuClass=%s"),
		*GetNameSafe(Layout),
		*GetNameSafe(RootWidget),
		*GetNameSafe(Layout->GetLayerWidget(AGASSFrontendTags::TAG_UI_LAYER_MENU)),
		*GetNameSafe(MainMenuClass));
	if (RootWidget == nullptr || !RootWidget->IsA(MainMenuClass))
	{
		Layout->ClearLayer(AGASSFrontendTags::TAG_UI_LAYER_MENU);
		RootWidget = Layout->PushWidgetToLayerStack<UAGASSFrontendScreenWidget>(AGASSFrontendTags::TAG_UI_LAYER_MENU, MainMenuClass);
		UE_LOG(LogAGASSFrontendUI, Log, TEXT("EnsureMainMenuRoot pushed MainMenu onto menu layer. Result=%s"), *GetNameSafe(RootWidget));
	}

	return RootWidget;
}

UAGASSFrontendScreenWidget* UAGASSUIManagerSubsystem::PushRegisteredScreenToLayer(
	ULocalPlayer* LocalPlayer,
	const FName& ScreenId,
	const FGameplayTag& LayerTag)
{
	const TSubclassOf<UAGASSFrontendScreenWidget> ScreenClass = GetRegisteredScreenClass(ScreenId);
	if (ScreenClass == nullptr)
	{
		UE_LOG(LogAGASSFrontendUI, Warning, TEXT("PushRegisteredScreenToLayer failed because screen %s is not registered."), *ScreenId.ToString());
		return nullptr;
	}

	if (ScreenId == MainMenuScreenId)
	{
		if (UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer))
		{
			return Cast<UAGASSFrontendScreenWidget>(Layout->GetLayerRootWidget(AGASSFrontendTags::TAG_UI_LAYER_MENU));
		}

		return nullptr;
	}

	UAGASSFrontendScreenWidget* ScreenWidget = Cast<UAGASSFrontendScreenWidget>(PushWidgetToLayer(LocalPlayer, LayerTag, ScreenClass));
	UE_LOG(LogAGASSFrontendUI, Log, TEXT("PushRegisteredScreenToLayer pushed %s to %s. Result=%s"),
		*ScreenId.ToString(),
		*LayerTag.ToString(),
		*GetNameSafe(ScreenWidget));
	return ScreenWidget;
}

void UAGASSUIManagerSubsystem::ResumeFrontendForLocalPlayer(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsFrontendContext())
	{
		return;
	}

	EAGASSSessionFlowState FlowState = EAGASSSessionFlowState::Idle;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UAGASSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			FlowState = SessionSubsystem->GetSessionFlowState();
		}
	}

	if (IsBlockingLoadFlowState(FlowState))
	{
		ShowLoadingScreen(LocalPlayer);
		return;
	}

	if (ShouldForceHardwareBenchmark(LocalPlayer))
	{
		if (UAGASSSettingsLocal* LocalSettings = UAGASSSettingsLocal::Get())
		{
			UE_LOG(LogAGASSFrontendUI, Log, TEXT("Running startup benchmark for %s before showing the main menu."), *GetNameSafe(LocalPlayer));
			LocalSettings->RunAutoBenchmark(true);
		}
	}

	ShowMainMenu(LocalPlayer);
}

bool UAGASSUIManagerSubsystem::ShouldForceHardwareBenchmark(ULocalPlayer* LocalPlayer) const
{
	if (LocalPlayer == nullptr || LocalPlayer != GetPrimaryLocalPlayer())
	{
		return false;
	}

	const UAGASSSettingsLocal* LocalSettings = UAGASSSettingsLocal::Get();
	return LocalSettings != nullptr && LocalSettings->ShouldRunAutoBenchmarkAtStartup();
}

void UAGASSUIManagerSubsystem::HandleGameplayLayoutReady(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr)
	{
		return;
	}

	if (UAGASSFrontendPrimaryLayoutWidget* Layout = ResolveOrCreateRootLayout(LocalPlayer))
	{
		Layout->SetFrontendShellVisible(false);
	}
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MENU);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL);
	ClearLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_GAMEMENU);
}

void UAGASSUIManagerSubsystem::RestoreGameplayInputForLocalPlayer(ULocalPlayer* LocalPlayer)
{
	if (LocalPlayer == nullptr || !IsGameplayContext())
	{
		return;
	}

	if (APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld()))
	{
		if (UCommonUIActionRouterBase* ActionRouter = LocalPlayer->GetSubsystem<UCommonUIActionRouterBase>())
		{
			FUIInputConfig GameplayInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
			GameplayInputConfig.bIgnoreMoveInput = false;
			GameplayInputConfig.bIgnoreLookInput = false;
			ActionRouter->SetActiveUIInputConfig(GameplayInputConfig, this);
		}

		FInputModeGameOnly GameplayInputMode;
		PlayerController->SetInputMode(GameplayInputMode);
		PlayerController->SetShowMouseCursor(false);
	}
}

void UAGASSUIManagerSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (LoadedWorld == nullptr || !LoadedWorld->IsGameWorld())
	{
		UE_LOG(LogAGASSFrontendUI, Log, TEXT("HandlePostLoadMap skipped. LoadedWorld=%s IsGameWorld=%s"),
			*GetNameSafe(LoadedWorld),
			(LoadedWorld != nullptr && LoadedWorld->IsGameWorld()) ? TEXT("true") : TEXT("false"));
		return;
	}

	UE_LOG(LogAGASSFrontendUI, Log, TEXT("HandlePostLoadMap world=%s package=%s frontend=%s"),
		*GetNameSafe(LoadedWorld),
		*GetNameSafe(LoadedWorld->GetPackage()),
		IsFrontendWorld(LoadedWorld) ? TEXT("true") : TEXT("false"));

	if (CurrentPolicy != nullptr)
	{
		CurrentPolicy->SetFrontendActive(IsFrontendWorld(LoadedWorld));
	}

	if (!IsFrontendWorld(LoadedWorld))
	{
		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
			{
				HandleGameplayLayoutReady(LocalPlayer);
			}
		}

		UE_LOG(LogAGASSFrontendUI, Log, TEXT("Loaded world %s is gameplay. Frontend layers cleared."), *GetNameSafe(LoadedWorld));
		return;
	}

	EAGASSSessionFlowState FlowState = EAGASSSessionFlowState::Idle;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UAGASSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			FlowState = SessionSubsystem->GetSessionFlowState();
		}
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
		{
			UE_LOG(LogAGASSFrontendUI, Log, TEXT("PostLoadMap flow for %s. FlowState=%d"), *GetNameSafe(LocalPlayer), static_cast<int32>(FlowState));
			ResumeFrontendForLocalPlayer(LocalPlayer);
		}
	}
}

void UAGASSUIManagerSubsystem::HandleSessionStateChanged()
{
	UGameInstance* GameInstance = GetGameInstance();
	UAGASSSessionSubsystem* SessionSubsystem = GameInstance != nullptr ? GameInstance->GetSubsystem<UAGASSSessionSubsystem>() : nullptr;
	if (SessionSubsystem == nullptr || GameInstance == nullptr)
	{
		return;
	}

	const EAGASSSessionFlowState FlowState = SessionSubsystem->GetSessionFlowState();
	UE_LOG(
		LogAGASSFrontendUI,
		Log,
		TEXT("HandleSessionStateChanged flow=%d gameplayContext=%s status=\"%s\" world=%s localPlayers=%d"),
		static_cast<int32>(FlowState),
		IsGameplayContext() ? TEXT("true") : TEXT("false"),
		*SessionSubsystem->GetStatusMessage(),
		*GetNameSafe(GetWorld()),
		GameInstance->GetLocalPlayers().Num());

	if (IsGameplayContext())
	{
		if (FlowState != EAGASSSessionFlowState::InSession)
		{
			for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
			{
				UE_LOG(LogAGASSFrontendUI, Log, TEXT("HandleSessionStateChanged dismissing game menu for %s because gameplay flow=%d."),
					*GetNameSafe(LocalPlayer),
					static_cast<int32>(FlowState));
				DismissGameMenu(LocalPlayer);
			}
		}

		return;
	}

	for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
	{
		if (IsBlockingLoadFlowState(FlowState))
		{
			UE_LOG(LogAGASSFrontendUI, Log, TEXT("HandleSessionStateChanged showing loading screen for %s. flow=%d status=\"%s\""),
				*GetNameSafe(LocalPlayer),
				static_cast<int32>(FlowState),
				*SessionSubsystem->GetStatusMessage());
			ShowLoadingScreen(LocalPlayer);
		}
		else
		{
			UE_LOG(LogAGASSFrontendUI, Log, TEXT("HandleSessionStateChanged dismissing loading screen for %s. flow=%d status=\"%s\""),
				*GetNameSafe(LocalPlayer),
				static_cast<int32>(FlowState),
				*SessionSubsystem->GetStatusMessage());
			DismissLoadingScreen(LocalPlayer);
		}
	}
}

void UAGASSUIManagerSubsystem::HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer)
{
	if (CurrentPolicy != nullptr)
	{
		CurrentPolicy->NotifyPlayerAdded(LocalPlayer);
	}

	if (IsGameplayContext())
	{
		HandleGameplayLayoutReady(LocalPlayer);
		return;
	}

	EAGASSSessionFlowState FlowState = EAGASSSessionFlowState::Idle;
	if (UAGASSSessionSubsystem* SessionSubsystem = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UAGASSSessionSubsystem>() : nullptr)
	{
		FlowState = SessionSubsystem->GetSessionFlowState();
	}

	if (IsBlockingLoadFlowState(FlowState))
	{
		ShowLoadingScreen(LocalPlayer);
	}
	else
	{
		ResumeFrontendForLocalPlayer(LocalPlayer);
	}
}

void UAGASSUIManagerSubsystem::HandleLocalPlayerRemoved(ULocalPlayer* LocalPlayer)
{
	if (CurrentPolicy != nullptr)
	{
		CurrentPolicy->NotifyPlayerDestroyed(LocalPlayer);
	}
}
