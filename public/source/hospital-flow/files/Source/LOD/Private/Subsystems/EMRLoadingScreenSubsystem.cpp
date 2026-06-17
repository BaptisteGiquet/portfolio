#include "Subsystems/EMRLoadingScreenSubsystem.h"

#include "AudioDevice.h"
#include "HAL/ThreadHeartBeat.h"
#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/WorldSettings.h"
#include "Interfaces/EMRLoadingProcessInterface.h"
#include "Interfaces/EMRLoadingScreenInterface.h"
#include "Misc/ConfigCacheIni.h"
#include "PreLoadScreenManager.h"
#include "ShaderPipelineCache.h"
#include "UI/Frontend/EMRLoadingScreenSettings.h"
#include "Widgets/Images/SThrobber.h"

DEFINE_LOG_CATEGORY_STATIC(LogEMRLoadingScreen, Log, All);

namespace
{
class FEMRLoadingScreenInputPreProcessor : public IInputProcessor
{
public:
	bool CanEatInput() const
	{
		return !GIsEditor;
	}

	virtual void Tick(const float, FSlateApplication&, TSharedRef<ICursor>) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent&) override { return CanEatInput(); }
	virtual bool HandleKeyUpEvent(FSlateApplication&, const FKeyEvent&) override { return CanEatInput(); }
	virtual bool HandleAnalogInputEvent(FSlateApplication&, const FAnalogInputEvent&) override { return CanEatInput(); }
	virtual bool HandleMouseMoveEvent(FSlateApplication&, const FPointerEvent&) override { return CanEatInput(); }
	virtual bool HandleMouseButtonDownEvent(FSlateApplication&, const FPointerEvent&) override { return CanEatInput(); }
	virtual bool HandleMouseButtonUpEvent(FSlateApplication&, const FPointerEvent&) override { return CanEatInput(); }
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication&, const FPointerEvent&) override { return CanEatInput(); }
	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication&, const FPointerEvent&, const FPointerEvent*) override { return CanEatInput(); }
	virtual bool HandleMotionDetectedEvent(FSlateApplication&, const FMotionEvent&) override { return CanEatInput(); }
};
}

bool UEMRLoadingScreenSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UGameInstance* GameInstance = Cast<UGameInstance>(Outer);
	if (!GameInstance)
	{
		return false;
	}

	if (GameInstance->IsDedicatedServerInstance())
	{
		return false;
	}

	TArray<UClass*> FoundClasses;
	GetDerivedClasses(GetClass(), FoundClasses);

	return FoundClasses.IsEmpty();
}

void UEMRLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &ThisClass::OnMapPreloaded);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnMapPostLoaded);

	SetTickableTickType(ETickableTickType::Conditional);
}

void UEMRLoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	TryRemoveLoadingScreen();
	StopBlockingInput();

	ExternalLoadingProcessors.Reset();

	Super::Deinitialize();
}

void UEMRLoadingScreenSubsystem::RegisterLoadingProcessor(UObject* InLoadingProcessor)
{
	if (InLoadingProcessor == nullptr)
	{
		return;
	}

	ExternalLoadingProcessors.AddUnique(TWeakObjectPtr<UObject>(InLoadingProcessor));
}

void UEMRLoadingScreenSubsystem::UnregisterLoadingProcessor(UObject* InLoadingProcessor)
{
	if (InLoadingProcessor == nullptr)
	{
		return;
	}

	for (int32 ProcessorIndex = ExternalLoadingProcessors.Num() - 1; ProcessorIndex >= 0; --ProcessorIndex)
	{
		if (ExternalLoadingProcessors[ProcessorIndex].Get() == InLoadingProcessor)
		{
			ExternalLoadingProcessors.RemoveAtSwap(ProcessorIndex);
		}
	}
}

UWorld* UEMRLoadingScreenSubsystem::GetTickableGameObjectWorld() const
{
	if (UGameInstance* OwningGameInstance = GetGameInstance())
	{
		return OwningGameInstance->GetWorld();
	}

	return nullptr;
}

void UEMRLoadingScreenSubsystem::Tick(float DeltaTime)
{
	TryUpdateLoadingScreen();
}

ETickableTickType UEMRLoadingScreenSubsystem::GetTickableTickType() const
{
	if (IsTemplate())
	{
		return ETickableTickType::Never;
	}

	return ETickableTickType::Conditional;
}

bool UEMRLoadingScreenSubsystem::IsTickable() const
{
	return GetGameInstance() && GetGameInstance()->GetGameViewportClient();
}

TStatId UEMRLoadingScreenSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UEMRLoadingScreenSubsystem, STATGROUP_Tickables);
}

void UEMRLoadingScreenSubsystem::OnMapPreloaded(const FWorldContext& WorldContext, const FString& MapName)
{
	if (WorldContext.OwningGameInstance != GetGameInstance())
	{
		return;
	}

	bIsCurrentlyLoadingMap = true;
	UE_LOG(LogEMRLoadingScreen, Log, TEXT("[LoadingScreen][PreLoadMap] map=%s"), *MapName);

	TryUpdateLoadingScreen();
}

void UEMRLoadingScreenSubsystem::OnMapPostLoaded(UWorld* LoadedWorld)
{
	if (LoadedWorld && LoadedWorld->GetGameInstance() == GetGameInstance())
	{
		bIsCurrentlyLoadingMap = false;
		UE_LOG(LogEMRLoadingScreen, Log, TEXT("[LoadingScreen][PostLoadMap] world=%s"), *GetNameSafe(LoadedWorld));
	}
}

void UEMRLoadingScreenSubsystem::TryUpdateLoadingScreen()
{
	if (IsPreloadScreenActive())
	{
		return;
	}

	const UEMRLoadingScreenSettings* LoadingScreenSettings = GetDefault<UEMRLoadingScreenSettings>();
	const bool bWasVisible = bIsLoadingVisible;
	const bool bShouldShowLoadingScreen = ShouldShowLoadingScreen();

	if (bShouldShowLoadingScreen)
	{
		if (LoadingScreenSettings->LoadingScreenHeartbeatHangDuration > 0.0f)
		{
			FThreadHeartBeat::Get().MonitorCheckpointStart(GetFName(), LoadingScreenSettings->LoadingScreenHeartbeatHangDuration);
		}

		TryDisplayLoadingScreenIfNone();
	}
	else
	{
		FThreadHeartBeat::Get().MonitorCheckpointEnd(GetFName());
		TryRemoveLoadingScreen();
	}

	const bool bReasonChanged = (LastLoggedDebugReason != DebugReasonForShowingOrHidingLoadingScreen);
	const bool bVisibilityChanged = (bWasVisible != bIsLoadingVisible);
	const double CurrentTimeSeconds = FPlatformTime::Seconds();
	const bool bPeriodicWhileVisible = bIsLoadingVisible && (CurrentTimeSeconds - LastPeriodicStateLogTimeSeconds) >= 1.0;
	if (bReasonChanged || bVisibilityChanged || bPeriodicWhileVisible)
	{
		LogLoadingScreenState(TEXT("Tick"), bShouldShowLoadingScreen);
		LastLoggedDebugReason = DebugReasonForShowingOrHidingLoadingScreen;
		LastPeriodicStateLogTimeSeconds = CurrentTimeSeconds;
	}
}

bool UEMRLoadingScreenSubsystem::IsPreloadScreenActive() const
{
	if (FPreLoadScreenManager* PreLoadScreenManager = FPreLoadScreenManager::Get())
	{
		return PreLoadScreenManager->HasValidActivePreLoadScreen();
	}

	return false;
}

bool UEMRLoadingScreenSubsystem::ShouldShowLoadingScreen()
{
	const UEMRLoadingScreenSettings* LoadingScreenSettings = GetDefault<UEMRLoadingScreenSettings>();

	if (GIsEditor && !LoadingScreenSettings->bShowLoadingScreenInEditor)
	{
		return false;
	}

	if (GetGameInstance() == nullptr || GetGameInstance()->GetGameViewportClient() == nullptr)
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("No valid game viewport client");
		return false;
	}

	const bool bNeedToShowLoadingScreen = CheckTheNeedToShowLoadingScreen();
	if (bNeedToShowLoadingScreen)
	{
		bCanHoldLoadingScreenAfterNeed = true;
		LastNeedReasonForLoadingScreen = DebugReasonForShowingOrHidingLoadingScreen;
		HoldLoadingScreenStartUpTime = -1.f;
		return true;
	}

	// Don't enter the hold window unless a real loading need happened first.
	if (!bCanHoldLoadingScreenAfterNeed)
	{
		HoldLoadingScreenStartUpTime = -1.f;
		return false;
	}

	const bool bCanHoldLoadingScreen = !GIsEditor || LoadingScreenSettings->bHoldLoadingScreenAdditionalSecsEvenInEditor;
	if (!bCanHoldLoadingScreen || LoadingScreenSettings->HoldLoadingScreenAdditionalSecs <= 0.f)
	{
		bCanHoldLoadingScreenAfterNeed = false;
		HoldLoadingScreenStartUpTime = -1.f;
		return false;
	}

	const float CurrentTime = FPlatformTime::Seconds();
	if (HoldLoadingScreenStartUpTime < 0.f)
	{
		HoldLoadingScreenStartUpTime = CurrentTime;
	}

	const float ElapsedTime = CurrentTime - HoldLoadingScreenStartUpTime;
	if (ElapsedTime < LoadingScreenSettings->HoldLoadingScreenAdditionalSecs)
	{
		if (UGameViewportClient* GameViewportClient = GetGameInstance()->GetGameViewportClient())
		{
			// Keep world rendering enabled while holding to let streaming complete under the loading UI.
			GameViewportClient->bDisableWorldRendering = false;
		}

		DebugReasonForShowingOrHidingLoadingScreen = FString::Printf(
			TEXT("Holding loading screen for %.2f additional seconds (elapsed=%.2f, last need=%s)"),
			LoadingScreenSettings->HoldLoadingScreenAdditionalSecs,
			ElapsedTime,
			*LastNeedReasonForLoadingScreen);
		return true;
	}

	// Hold window consumed; don't restart until another real loading need occurs.
	bCanHoldLoadingScreenAfterNeed = false;
	HoldLoadingScreenStartUpTime = -1.f;
	return false;
}

bool UEMRLoadingScreenSubsystem::CheckTheNeedToShowLoadingScreen()
{
	DebugReasonForShowingOrHidingLoadingScreen = TEXT("Reason is unknown");

	const UGameInstance* LocalGameInstance = GetGameInstance();
	if (LocalGameInstance == nullptr)
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("GameInstance is null");
		return true;
	}

	const FWorldContext* Context = LocalGameInstance->GetWorldContext();
	if (Context == nullptr)
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("WorldContext is null");
		return true;
	}

	UWorld* World = Context->World();
	if (World == nullptr)
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("World is null");
		return true;
	}

	if (bIsCurrentlyLoadingMap)
	{
		const bool bMapLoadStillInProgress =
			(Context->PendingNetGame != nullptr)
			|| !World->NextURL.IsEmpty()
			|| !World->HasBegunPlay()
			|| World->IsInSeamlessTravel();

		if (!bMapLoadStillInProgress)
		{
			// Some flows can leave the map-load flag stale. Clear it once the world is stable.
			bIsCurrentlyLoadingMap = false;
		}
	}

	if (Context->PendingNetGame != nullptr)
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("PendingNetGame is valid");
		return true;
	}

	if (!World->HasBegunPlay())
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("World has not begun play");
		return true;
	}

	if (!World->NextURL.IsEmpty())
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("World has a pending travel URL");
		return true;
	}

	if (World->IsInSeamlessTravel())
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("World is in seamless travel");
		return true;
	}

	AGameStateBase* GameState = World->GetGameState<AGameStateBase>();
	if (GameState != nullptr)
	{
		if (IEMRLoadingProcessInterface::ShouldShowLoadingScreen(GameState, DebugReasonForShowingOrHidingLoadingScreen))
		{
			return true;
		}

		for (UActorComponent* Component : GameState->GetComponents())
		{
			if (IEMRLoadingProcessInterface::ShouldShowLoadingScreen(Component, DebugReasonForShowingOrHidingLoadingScreen))
			{
				return true;
			}
		}
	}

	for (int32 ProcessorIndex = ExternalLoadingProcessors.Num() - 1; ProcessorIndex >= 0; --ProcessorIndex)
	{
		const TWeakObjectPtr<UObject>& WeakProcessor = ExternalLoadingProcessors[ProcessorIndex];
		if (!WeakProcessor.IsValid())
		{
			ExternalLoadingProcessors.RemoveAtSwap(ProcessorIndex);
			continue;
		}

		if (IEMRLoadingProcessInterface::ShouldShowLoadingScreen(WeakProcessor.Get(), DebugReasonForShowingOrHidingLoadingScreen))
		{
			return true;
		}
	}

	bool bFoundAnyLocalPC = false;
	bool bMissingAnyLocalPC = false;
	for (ULocalPlayer* LocalPlayer : LocalGameInstance->GetLocalPlayers())
	{
		if (LocalPlayer == nullptr)
		{
			continue;
		}

		if (APlayerController* PlayerController = LocalPlayer->PlayerController)
		{
			bFoundAnyLocalPC = true;

			if (IEMRLoadingProcessInterface::ShouldShowLoadingScreen(PlayerController, DebugReasonForShowingOrHidingLoadingScreen))
			{
				return true;
			}

			for (UActorComponent* Component : PlayerController->GetComponents())
			{
				if (IEMRLoadingProcessInterface::ShouldShowLoadingScreen(Component, DebugReasonForShowingOrHidingLoadingScreen))
				{
					return true;
				}
			}
		}
		else
		{
			bMissingAnyLocalPC = true;
		}
	}

	const UGameViewportClient* GameViewportClient = LocalGameInstance->GetGameViewportClient();
	const bool bIsInSplitscreen = GameViewportClient
	&& GameViewportClient->GetCurrentSplitscreenConfiguration() != ESplitScreenType::None;

	if (bIsInSplitscreen && bMissingAnyLocalPC)
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("Missing local controller in split-screen");
		return true;
	}

	if (!bIsInSplitscreen && !bFoundAnyLocalPC)
	{
		DebugReasonForShowingOrHidingLoadingScreen = TEXT("No local controller available yet");
		return true;
	}

	DebugReasonForShowingOrHidingLoadingScreen = TEXT("(nothing wants to show loading screen)");
	return false;
}

void UEMRLoadingScreenSubsystem::LogLoadingScreenState(const TCHAR* ContextLabel, bool bShouldShowLoadingScreen)
{
	UE_LOG(LogEMRLoadingScreen, Log,
		TEXT("[LoadingScreen][%s] should_show=%d visible=%d reason=\"%s\" last_need=\"%s\" %s"),
		ContextLabel,
		bShouldShowLoadingScreen ? 1 : 0,
		bIsLoadingVisible ? 1 : 0,
		*DebugReasonForShowingOrHidingLoadingScreen,
		*LastNeedReasonForLoadingScreen,
		*BuildLoadingScreenStateSnapshot());
}

FString UEMRLoadingScreenSubsystem::BuildLoadingScreenStateSnapshot() const
{
	const UGameInstance* LocalGameInstance = GetGameInstance();
	if (LocalGameInstance == nullptr)
	{
		return TEXT("game_instance=null");
	}

	const FWorldContext* Context = LocalGameInstance->GetWorldContext();
	UWorld* World = Context ? Context->World() : nullptr;
	const UGameViewportClient* GameViewportClient = LocalGameInstance->GetGameViewportClient();

	int32 NumLocalPlayers = 0;
	int32 NumLocalControllers = 0;
	int32 NumMissingLocalControllers = 0;
	for (ULocalPlayer* LocalPlayer : LocalGameInstance->GetLocalPlayers())
	{
		if (LocalPlayer == nullptr)
		{
			continue;
		}

		++NumLocalPlayers;
		if (LocalPlayer->PlayerController != nullptr)
		{
			++NumLocalControllers;
		}
		else
		{
			++NumMissingLocalControllers;
		}
	}

	const bool bIsInSplitscreen = GameViewportClient
	&& GameViewportClient->GetCurrentSplitscreenConfiguration() != ESplitScreenType::None;

	const bool bHasPendingNetGame = Context && (Context->PendingNetGame != nullptr);
	const FString TravelURL = Context ? Context->TravelURL : TEXT("<no-world-context>");
	const FString NextURL = World ? World->NextURL : TEXT("<no-world>");
	const bool bHasBegunPlay = World ? World->HasBegunPlay() : false;
	const bool bIsInSeamlessTravel = World ? World->IsInSeamlessTravel() : false;
	const bool bHasGameState = World && (World->GetGameState<AGameStateBase>() != nullptr);
	const FString WorldMapName = World ? World->GetMapName() : TEXT("<null>");

	return FString::Printf(
		TEXT("map_loading=%d preload_active=%d hold_start=%.2f world=\"%s\" begun_play=%d seamless=%d game_state=%d local_players=%d local_pcs=%d missing_local_pcs=%d viewport=%d splitscreen=%d pending_net=%d travel_url=\"%s\" next_url=\"%s\" processors=%d"),
		bIsCurrentlyLoadingMap ? 1 : 0,
		IsPreloadScreenActive() ? 1 : 0,
		HoldLoadingScreenStartUpTime,
		*WorldMapName,
		bHasBegunPlay ? 1 : 0,
		bIsInSeamlessTravel ? 1 : 0,
		bHasGameState ? 1 : 0,
		NumLocalPlayers,
		NumLocalControllers,
		NumMissingLocalControllers,
		GameViewportClient ? 1 : 0,
		bIsInSplitscreen ? 1 : 0,
		bHasPendingNetGame ? 1 : 0,
		*TravelURL,
		*NextURL,
		ExternalLoadingProcessors.Num());
}

void UEMRLoadingScreenSubsystem::TryDisplayLoadingScreenIfNone()
{
	if (CachedCreatedLoadingScreenWidget.IsValid())
	{
		return;
	}

	// One-shot cleanup: stop currently playing map sounds, but don't globally mute audio.
	// This lets custom loading-screen audio start afterwards and play normally.
	if (GEngine)
	{
		if (UGameViewportClient* GameViewportClient = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
		{
			if (FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
			{
				if (UWorld* ViewportWorld = GameViewportClient->GetWorld())
				{
					AudioDevice->Flush(ViewportWorld);
				}
				else
				{
					AudioDevice->StopAllSounds(false);
				}
			}
		}
	}

	const UEMRLoadingScreenSettings* LoadingScreenSettings = GetDefault<UEMRLoadingScreenSettings>();
	TSubclassOf<UUserWidget> LoadingScreenWidgetClass = LoadingScreenSettings->GetLoadingScreenWidgetClassChecked();

	UUserWidget* CreatedWidget = UUserWidget::CreateWidgetInstance(*GetGameInstance(), LoadingScreenWidgetClass, NAME_None);
	if (CreatedWidget)
	{
		CachedCreatedLoadingScreenWidget = CreatedWidget->TakeWidget();
	}
	else
	{
		CachedCreatedLoadingScreenWidget = SNew(SThrobber);
	}

	GetGameInstance()->GetGameViewportClient()->AddViewportWidgetContent(
		CachedCreatedLoadingScreenWidget.ToSharedRef(),
		LoadingScreenSettings->LoadingScreenZOrder);

	StartBlockingInput();
	ChangePerformanceSettings(true);

	if (!GIsEditor || LoadingScreenSettings->bForceTickLoadingScreenEvenInEditor)
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().Tick();
		}
	}

	bIsLoadingVisible = true;
	NotifyLoadingScreenVisibilityChanged(true);
}

void UEMRLoadingScreenSubsystem::TryRemoveLoadingScreen()
{
	if (!bIsLoadingVisible && !CachedCreatedLoadingScreenWidget.IsValid())
	{
		return;
	}

	StopBlockingInput();

	if (CachedCreatedLoadingScreenWidget.IsValid())
	{
		if (UGameViewportClient* GameViewportClient = GetGameInstance()->GetGameViewportClient())
		{
			GameViewportClient->RemoveViewportWidgetContent(CachedCreatedLoadingScreenWidget.ToSharedRef());
		}
		CachedCreatedLoadingScreenWidget.Reset();
	}

	ChangePerformanceSettings(false);

	if (bIsLoadingVisible)
	{
		bIsLoadingVisible = false;
		NotifyLoadingScreenVisibilityChanged(false);
	}
}

void UEMRLoadingScreenSubsystem::StartBlockingInput()
{
	if (InputPreProcessor.IsValid() || !FSlateApplication::IsInitialized())
	{
		return;
	}

	InputPreProcessor = MakeShareable(new FEMRLoadingScreenInputPreProcessor());
	FSlateApplication::Get().RegisterInputPreProcessor(InputPreProcessor, 0);
}

void UEMRLoadingScreenSubsystem::StopBlockingInput()
{
	if (!InputPreProcessor.IsValid())
	{
		return;
	}

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputPreProcessor);
	}
	InputPreProcessor.Reset();
}

void UEMRLoadingScreenSubsystem::ChangePerformanceSettings(bool bEnableLoadingScreen)
{
	UGameInstance* LocalGameInstance = GetGameInstance();
	if (LocalGameInstance == nullptr)
	{
		return;
	}

	UGameViewportClient* GameViewportClient = LocalGameInstance->GetGameViewportClient();
	if (GameViewportClient == nullptr)
	{
		return;
	}

	FShaderPipelineCache::SetBatchMode(bEnableLoadingScreen
		? FShaderPipelineCache::BatchMode::Fast
		: FShaderPipelineCache::BatchMode::Background);

	GameViewportClient->bDisableWorldRendering = bEnableLoadingScreen;

	if (UWorld* ViewportWorld = GameViewportClient->GetWorld())
	{
		if (AWorldSettings* WorldSettings = ViewportWorld->GetWorldSettings(false, false))
		{
			WorldSettings->bHighPriorityLoadingLocal = bEnableLoadingScreen;
		}
	}

	if (bEnableLoadingScreen)
	{
		double HangDurationMultiplier = 1.0;
		if (GConfig)
		{
			GConfig->GetDouble(TEXT("Core.System"), TEXT("LoadingScreenHangDurationMultiplier"), HangDurationMultiplier, GEngineIni);
		}
		FThreadHeartBeat::Get().SetDurationMultiplier(HangDurationMultiplier);
		FGameThreadHitchHeartBeat::Get().SuspendHeartBeat();
	}
	else
	{
		FThreadHeartBeat::Get().SetDurationMultiplier(1.0);
		FGameThreadHitchHeartBeat::Get().ResumeHeartBeat();
	}
}

void UEMRLoadingScreenSubsystem::NotifyLoadingScreenVisibilityChanged(bool bIsVisible)
{
	for (ULocalPlayer* ExistingLocalPlayer : GetGameInstance()->GetLocalPlayers())
	{
		if (!ExistingLocalPlayer)
		{
			continue;
		}

		if (APlayerController* PC = ExistingLocalPlayer->GetPlayerController(GetGameInstance()->GetWorld()))
		{
			if (PC->Implements<UEMRLoadingScreenInterface>())
			{
				if (bIsVisible)
				{
					IEMRLoadingScreenInterface::Execute_OnLoadingScreenActivated(PC);
				}
				else
				{
					IEMRLoadingScreenInterface::Execute_OnLoadingScreenDeactivated(PC);
				}
			}

			if (APawn* OwningPawn = PC->GetPawn())
			{
				if (OwningPawn->Implements<UEMRLoadingScreenInterface>())
				{
					if (bIsVisible)
					{
						IEMRLoadingScreenInterface::Execute_OnLoadingScreenActivated(OwningPawn);
					}
					else
					{
						IEMRLoadingScreenInterface::Execute_OnLoadingScreenDeactivated(OwningPawn);
					}
				}
			}
		}
	}
}
