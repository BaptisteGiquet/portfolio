#include "Subsystems/AGASSEconomyHUDSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Settings/AGASSEconomyDeveloperSettings.h"
#include "TimerManager.h"

void UAGASSEconomyHUDSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!ShouldManageThisLocalPlayer())
	{
		return;
	}

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
	HandlePostLoadMap(GetWorld());
}

void UAGASSEconomyHUDSubsystem::Deinitialize()
{
	StopCreateRetryTimer();
	RemoveTeamMoneyWidget();

	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	Super::Deinitialize();
}

bool UAGASSEconomyHUDSubsystem::ShouldManageThisLocalPlayer() const
{
	return GetLocalPlayer() != nullptr && GetLocalPlayer()->GetControllerId() == 0;
}

bool UAGASSEconomyHUDSubsystem::ShouldDisplayInWorld(const UWorld* World) const
{
	const UAGASSEconomyDeveloperSettings* const EconomySettings = UAGASSEconomyDeveloperSettings::Get();
	return EconomySettings != nullptr
		&& EconomySettings->bShowTeamMoneyTestWidget
		&& World != nullptr
		&& World->IsGameWorld()
		&& World->GetNetMode() != NM_DedicatedServer;
}

void UAGASSEconomyHUDSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!ShouldManageThisLocalPlayer())
	{
		return;
	}

	StopCreateRetryTimer();
	RemoveTeamMoneyWidget();

	if (!ShouldDisplayInWorld(LoadedWorld))
	{
		return;
	}

	TryCreateTeamMoneyWidget();
	if (TeamMoneyWidget == nullptr)
	{
		StartCreateRetryTimer();
	}
}

void UAGASSEconomyHUDSubsystem::HandleRetryCreateWidget()
{
	TryCreateTeamMoneyWidget();
	if (TeamMoneyWidget != nullptr)
	{
		StopCreateRetryTimer();
	}
}

void UAGASSEconomyHUDSubsystem::TryCreateTeamMoneyWidget()
{
	if (TeamMoneyWidget != nullptr)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (!ShouldDisplayInWorld(World))
	{
		return;
	}

	const UAGASSEconomyDeveloperSettings* const EconomySettings = UAGASSEconomyDeveloperSettings::Get();
	if (EconomySettings == nullptr)
	{
		return;
	}

	TSubclassOf<UUserWidget> WidgetClass = EconomySettings->TeamMoneyTestWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		return;
	}

	APlayerController* const PlayerController = GetLocalPlayer() != nullptr ? GetLocalPlayer()->GetPlayerController(World) : nullptr;
	if (PlayerController == nullptr || PlayerController->GetPawn() == nullptr)
	{
		return;
	}

	TeamMoneyWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
	if (TeamMoneyWidget == nullptr)
	{
		return;
	}

	TeamMoneyWidget->AddToPlayerScreen(EconomySettings->TeamMoneyTestWidgetZOrder);
}

void UAGASSEconomyHUDSubsystem::RemoveTeamMoneyWidget()
{
	if (TeamMoneyWidget != nullptr)
	{
		TeamMoneyWidget->RemoveFromParent();
		TeamMoneyWidget = nullptr;
	}
}

void UAGASSEconomyHUDSubsystem::StartCreateRetryTimer()
{
	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetTimerManager().IsTimerActive(CreateRetryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(CreateRetryTimerHandle, this, &ThisClass::HandleRetryCreateWidget, 0.25f, true);
}

void UAGASSEconomyHUDSubsystem::StopCreateRetryTimer()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CreateRetryTimerHandle);
	}
}
