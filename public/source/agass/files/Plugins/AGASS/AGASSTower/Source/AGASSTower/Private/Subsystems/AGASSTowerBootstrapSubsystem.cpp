#include "Subsystems/AGASSTowerBootstrapSubsystem.h"

#include "Actors/AGASSObjectiveActor.h"
#include "Components/AGASSSessionEventManagerComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Settings/AGASSTowerDeveloperSettings.h"
#include "Subsystems/AGASSModsSubsystem.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSTowerBootstrap, Log, All);

bool UAGASSTowerBootstrapSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAGASSTowerBootstrapSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld() || InWorld.GetNetMode() == NM_Client || !IsGameplaySessionWorld(InWorld))
	{
		return;
	}

	EnsureDefaultObjective(InWorld);
	InitializeSessionEvents(InWorld);
}

bool UAGASSTowerBootstrapSubsystem::IsGameplaySessionWorld(const UWorld& World)
{
	if (World.URL.HasOption(TEXT("AGASS_GAMEPLAY_SESSION")) || World.URL.HasOption(TEXT("AGASS_TOWER_SESSION")))
	{
		return true;
	}

	if (UGameInstance* GameInstance = World.GetGameInstance())
	{
		if (const UAGASSModsSubsystem* ModsSubsystem = GameInstance->GetSubsystem<UAGASSModsSubsystem>())
		{
			return ModsSubsystem->IsGameplaySessionWorld(World);
		}
	}

	return false;
}

void UAGASSTowerBootstrapSubsystem::EnsureDefaultObjective(UWorld& World)
{
	for (TActorIterator<AAGASSObjectiveActor> It(&World); It; ++It)
	{
		return;
	}

	const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get();
	if (TowerSettings == nullptr || !TowerSettings->bAutoSpawnDefaultObjectiveWhenMissing)
	{
		return;
	}

	UClass* ObjectiveActorClass = TowerSettings->DefaultObjectiveActorClass.LoadSynchronous();
	if (ObjectiveActorClass == nullptr)
	{
		ObjectiveActorClass = AAGASSObjectiveActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	World.SpawnActor<AAGASSObjectiveActor>(
		ObjectiveActorClass,
		TowerSettings->DefaultObjectiveSpawnTransform,
		SpawnParameters);
}

void UAGASSTowerBootstrapSubsystem::InitializeSessionEvents(UWorld& World)
{
	const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get();
	if (TowerSettings == nullptr || !TowerSettings->bEnableSessionEvents)
	{
		return;
	}

	AGameStateBase* const GameState = World.GetGameState();
	UAGASSSessionEventManagerComponent* const EventManager = GameState != nullptr
		? GameState->FindComponentByClass<UAGASSSessionEventManagerComponent>()
		: nullptr;
	if (EventManager == nullptr)
	{
		UE_LOG(LogAGASSTowerBootstrap, Warning, TEXT("Session events could not initialize because the GameState event manager component is missing."));
		return;
	}

	UGameInstance* const GameInstance = World.GetGameInstance();
	UAGASSModsSubsystem* const ModsSubsystem = GameInstance != nullptr ? GameInstance->GetSubsystem<UAGASSModsSubsystem>() : nullptr;
	if (ModsSubsystem == nullptr)
	{
		UE_LOG(LogAGASSTowerBootstrap, Warning, TEXT("Session events could not initialize because the mods subsystem is unavailable."));
		return;
	}

	FAGASSResolvedContentSelection ResolvedSelection;
	FString FailureMessage;
	if (!ModsSubsystem->TryBuildRuntimeSelectionForWorld(World, ResolvedSelection, FailureMessage) || !ResolvedSelection.bIsValid)
	{
		UE_LOG(
			LogAGASSTowerBootstrap,
			Warning,
			TEXT("Session events could not resolve runtime content selection. reason=\"%s\""),
			*FailureMessage);
		return;
	}

	if (!EventManager->InitializeAvailableEventsFromResolvedSelection(ResolvedSelection, FailureMessage))
	{
		UE_LOG(
			LogAGASSTowerBootstrap,
			Warning,
			TEXT("Session events failed to initialize from the resolved content selection. reason=\"%s\""),
			*FailureMessage);
		return;
	}

	UE_LOG(
		LogAGASSTowerBootstrap,
		Display,
		TEXT("Initialized %d session event definition(s) for world %s."),
		EventManager->GetAvailableEventDefinitions().Num(),
		*GetNameSafe(&World));
}
