#include "Subsystems/AGASSTowerHUDSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Settings/AGASSTowerDeveloperSettings.h"
#include "Subsystems/AGASSTowerBootstrapSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSTowerHUD, Log, All);

void UAGASSTowerHUDSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GetLocalPlayer() == nullptr)
	{
		UE_LOG(LogAGASSTowerHUD, Warning, TEXT("Initialize skipped because LocalPlayer is null."));
		return;
	}

	UE_LOG(
		LogAGASSTowerHUD,
		Display,
		TEXT("Initialize localPlayer=%s controllerId=%d world=%s"),
		*GetNameSafe(GetLocalPlayer()),
		GetLocalPlayer()->GetControllerId(),
		*GetNameSafe(GetWorld()));

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
	HandlePostLoadMap(GetWorld());
}

void UAGASSTowerHUDSubsystem::Deinitialize()
{
	StopRetryTimer();
	RemoveTimedRunWidget();
	RemoveEndOfRunWidget();

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	Super::Deinitialize();
}

void UAGASSTowerHUDSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);

	UE_LOG(
		LogAGASSTowerHUD,
		Display,
		TEXT("PlayerControllerChanged localPlayer=%s newController=%s world=%s shouldManage=%d shouldDisplay=%d"),
		*GetNameSafe(GetLocalPlayer()),
		*GetNameSafe(NewPlayerController),
		*GetNameSafe(GetWorld()),
		ShouldManageThisLocalPlayer() ? 1 : 0,
		ShouldDisplayForWorld(GetWorld()) ? 1 : 0);

	if (!ShouldManageThisLocalPlayer())
	{
		return;
	}

	if (!ShouldDisplayForWorld(GetWorld()))
	{
		StopRetryTimer();
		RemoveTimedRunWidget();
		RemoveEndOfRunWidget();
		return;
	}

	APlayerController* const RuntimePlayerController = ResolveRuntimePlayerController(GetWorld());
	if (TimedRunWidget != nullptr && TimedRunWidget->GetOwningPlayer() != RuntimePlayerController)
	{
		RemoveTimedRunWidget();
	}

	if (EndOfRunWidget != nullptr && EndOfRunWidget->GetOwningPlayer() != RuntimePlayerController)
	{
		UE_LOG(
			LogAGASSTowerHUD,
			Display,
			TEXT("PlayerControllerChanged replacing stale widget owner=%s with newController=%s"),
			*GetNameSafe(EndOfRunWidget->GetOwningPlayer()),
			*GetNameSafe(RuntimePlayerController));
		RemoveEndOfRunWidget();
	}

	TryCreateTimedRunWidget();
	TryCreateEndOfRunWidget();
	if (TimedRunWidget == nullptr || EndOfRunWidget == nullptr)
	{
		StartRetryTimer();
	}
}

bool UAGASSTowerHUDSubsystem::ShouldManageThisLocalPlayer() const
{
	return GetLocalPlayer() != nullptr;
}

bool UAGASSTowerHUDSubsystem::ShouldDisplayForWorld(const UWorld* World) const
{
	return World != nullptr
		&& World->IsGameWorld()
		&& World->GetNetMode() != NM_DedicatedServer
		&& UAGASSTowerBootstrapSubsystem::IsGameplaySessionWorld(*World);
}

APlayerController* UAGASSTowerHUDSubsystem::ResolveRuntimePlayerController(UWorld* World) const
{
	return GetLocalPlayer() != nullptr ? GetLocalPlayer()->GetPlayerController(World) : nullptr;
}

bool UAGASSTowerHUDSubsystem::IsRuntimePlayerControllerReady(const APlayerController* PlayerController) const
{
	return PlayerController != nullptr
		&& PlayerController->IsLocalController()
		&& PlayerController->GetClass() != APlayerController::StaticClass();
}

void UAGASSTowerHUDSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	UE_LOG(
		LogAGASSTowerHUD,
		Display,
		TEXT("HandlePostLoadMap localPlayer=%s loadedWorld=%s shouldManage=%d shouldDisplay=%d"),
		*GetNameSafe(GetLocalPlayer()),
		*GetNameSafe(LoadedWorld),
		ShouldManageThisLocalPlayer() ? 1 : 0,
		ShouldDisplayForWorld(LoadedWorld) ? 1 : 0);

	if (!ShouldManageThisLocalPlayer())
	{
		return;
	}

	StopRetryTimer();
	RemoveTimedRunWidget();
	RemoveEndOfRunWidget();

	if (!ShouldDisplayForWorld(LoadedWorld))
	{
		return;
	}

	TryCreateTimedRunWidget();
	TryCreateEndOfRunWidget();
	if (TimedRunWidget == nullptr || EndOfRunWidget == nullptr)
	{
		StartRetryTimer();
	}
}

void UAGASSTowerHUDSubsystem::HandleRetryCreateWidget()
{
	TryCreateTimedRunWidget();
	TryCreateEndOfRunWidget();
	if (TimedRunWidget != nullptr && EndOfRunWidget != nullptr)
	{
		StopRetryTimer();
	}
}

void UAGASSTowerHUDSubsystem::TryCreateTimedRunWidget()
{
	if (TimedRunWidget != nullptr)
	{
		APlayerController* const RuntimePlayerController = ResolveRuntimePlayerController(GetWorld());
		if (TimedRunWidget->GetOwningPlayer() == RuntimePlayerController)
		{
			return;
		}

		RemoveTimedRunWidget();
	}

	UWorld* const World = GetWorld();
	if (!ShouldDisplayForWorld(World))
	{
		return;
	}

	const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get();
	if (TowerSettings == nullptr)
	{
		return;
	}

	const TSubclassOf<UUserWidget> WidgetClass = TowerSettings->TimedRunHUDWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		return;
	}

	APlayerController* const PlayerController = ResolveRuntimePlayerController(World);
	if (!IsRuntimePlayerControllerReady(PlayerController))
	{
		return;
	}

	TimedRunWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
	if (TimedRunWidget == nullptr)
	{
		return;
	}

	TimedRunWidget->AddToPlayerScreen(TowerSettings->TimedRunHUDWidgetZOrder);
}

void UAGASSTowerHUDSubsystem::TryCreateEndOfRunWidget()
{
	if (EndOfRunWidget != nullptr)
	{
		APlayerController* const RuntimePlayerController = ResolveRuntimePlayerController(GetWorld());
		if (EndOfRunWidget->GetOwningPlayer() == RuntimePlayerController)
		{
			UE_LOG(LogAGASSTowerHUD, Verbose, TEXT("TryCreateEndOfRunWidget skipped because widget already exists for the current controller."));
			return;
		}

		UE_LOG(
			LogAGASSTowerHUD,
			Display,
			TEXT("TryCreateEndOfRunWidget removing stale widget owner=%s currentController=%s"),
			*GetNameSafe(EndOfRunWidget->GetOwningPlayer()),
			*GetNameSafe(RuntimePlayerController));
		RemoveEndOfRunWidget();
	}

	UWorld* const World = GetWorld();
	if (!ShouldDisplayForWorld(World))
	{
		UE_LOG(LogAGASSTowerHUD, Verbose, TEXT("TryCreateEndOfRunWidget skipped because world is not eligible. world=%s"), *GetNameSafe(World));
		return;
	}

	const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get();
	if (TowerSettings == nullptr)
	{
		UE_LOG(LogAGASSTowerHUD, Warning, TEXT("TryCreateEndOfRunWidget failed because TowerSettings is null."));
		return;
	}

	const TSubclassOf<UUserWidget> WidgetClass = TowerSettings->EndOfRunWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogAGASSTowerHUD, Warning, TEXT("TryCreateEndOfRunWidget failed because EndOfRunWidgetClass is not set."));
		return;
	}

	APlayerController* const PlayerController = ResolveRuntimePlayerController(World);
	if (!IsRuntimePlayerControllerReady(PlayerController))
	{
		UE_LOG(
			LogAGASSTowerHUD,
			Display,
			TEXT("TryCreateEndOfRunWidget waiting for gameplay controller. localPlayer=%s world=%s controller=%s controllerClass=%s"),
			*GetNameSafe(GetLocalPlayer()),
			*GetNameSafe(World),
			*GetNameSafe(PlayerController),
			PlayerController != nullptr ? *GetNameSafe(PlayerController->GetClass()) : TEXT("None"));
		return;
	}

	EndOfRunWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
	if (EndOfRunWidget == nullptr)
	{
		UE_LOG(LogAGASSTowerHUD, Warning, TEXT("TryCreateEndOfRunWidget failed to instantiate widget. controller=%s widgetClass=%s"), *GetNameSafe(PlayerController), *GetNameSafe(WidgetClass));
		return;
	}

	EndOfRunWidget->AddToPlayerScreen(TowerSettings->EndOfRunWidgetZOrder);
	UE_LOG(
		LogAGASSTowerHUD,
		Display,
		TEXT("End-of-run widget created localPlayer=%s controller=%s widget=%s zOrder=%d"),
		*GetNameSafe(GetLocalPlayer()),
		*GetNameSafe(PlayerController),
		*GetNameSafe(EndOfRunWidget),
		TowerSettings->EndOfRunWidgetZOrder);
}

void UAGASSTowerHUDSubsystem::RemoveEndOfRunWidget()
{
	if (EndOfRunWidget != nullptr)
	{
		UE_LOG(LogAGASSTowerHUD, Display, TEXT("Removing end-of-run widget %s"), *GetNameSafe(EndOfRunWidget));
		EndOfRunWidget->RemoveFromParent();
		EndOfRunWidget = nullptr;
	}
}

void UAGASSTowerHUDSubsystem::RemoveTimedRunWidget()
{
	if (TimedRunWidget != nullptr)
	{
		TimedRunWidget->RemoveFromParent();
		TimedRunWidget = nullptr;
	}
}

void UAGASSTowerHUDSubsystem::StartRetryTimer()
{
	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetTimerManager().IsTimerActive(RetryTimerHandle))
	{
		return;
	}

	UE_LOG(LogAGASSTowerHUD, Display, TEXT("Starting retry timer for end-of-run widget creation. world=%s"), *GetNameSafe(World));
	World->GetTimerManager().SetTimer(RetryTimerHandle, this, &ThisClass::HandleRetryCreateWidget, 0.25f, true);
}

void UAGASSTowerHUDSubsystem::StopRetryTimer()
{
	if (UWorld* const World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerActive(RetryTimerHandle))
		{
			UE_LOG(LogAGASSTowerHUD, Display, TEXT("Stopping retry timer for end-of-run widget creation. world=%s"), *GetNameSafe(World));
		}
		World->GetTimerManager().ClearTimer(RetryTimerHandle);
	}
}
