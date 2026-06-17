#include "AGASSGameModeBase.h"

#include "AGASSCharacter.h"
#include "AGASSGameStateBase.h"
#include "AGASSPlayerController.h"
#include "Actors/AGASSObjectiveActor.h"
#include "Actors/AGASSPlaceableItemActor.h"
#include "Components/AGASSInteractionComponent.h"
#include "Components/AGASSRunStateComponent.h"
#include "Components/AGASSSessionEventManagerComponent.h"
#include "Components/AGASSTeamWalletComponent.h"
#include "Misc/PackageName.h"
#include "Subsystems/AGASSModsSubsystem.h"
#include "Settings/AGASSPlayerDeveloperSettings.h"
#include "Settings/AGASSOnlineDeveloperSettings.h"
#include "Settings/AGASSTowerDeveloperSettings.h"
#include "Data/AGASSRespawnTuningData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/AGASSSessionSubsystem.h"
#include "Subsystems/AGASSTowerBootstrapSubsystem.h"
#include "TimerManager.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSGameModeFlow, Log, All);

namespace AGASSGameModePrivate
{
	static bool HasGameplaySessionTravelOption(const UWorld* World, const FString& Options)
	{
		return (World != nullptr && (World->URL.HasOption(TEXT("AGASS_GAMEPLAY_SESSION")) || World->URL.HasOption(TEXT("AGASS_TOWER_SESSION"))))
			|| UGameplayStatics::HasOption(Options, TEXT("AGASS_GAMEPLAY_SESSION"))
			|| UGameplayStatics::HasOption(Options, TEXT("AGASS_TOWER_SESSION"));
	}

	static bool IsGameplaySessionWorld(const UWorld* World, const FString& Options)
	{
		if (HasGameplaySessionTravelOption(World, Options))
		{
			return true;
		}

		if (World != nullptr)
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (const UAGASSModsSubsystem* ModsSubsystem = GameInstance->GetSubsystem<UAGASSModsSubsystem>())
				{
					return ModsSubsystem->IsGameplaySessionWorld(*World);
				}
			}
		}

		return false;
	}

	static FString BuildSafeTimedRunKeySuffix(const FString& SourceValue)
	{
		FString Sanitized = SourceValue.TrimStartAndEnd().ToLower();
		for (TCHAR& Character : Sanitized)
		{
			if (!FChar::IsAlnum(Character))
			{
				Character = TEXT('_');
			}
		}

		while (Sanitized.ReplaceInline(TEXT("__"), TEXT("_")) > 0)
		{
		}

		while (Sanitized.StartsWith(TEXT("_")))
		{
			Sanitized.RightChopInline(1, EAllowShrinking::No);
		}

		while (Sanitized.EndsWith(TEXT("_")))
		{
			Sanitized.LeftChopInline(1, EAllowShrinking::No);
		}

		return Sanitized.Left(56);
	}
}

AAGASSGameModeBase::AAGASSGameModeBase()
{
	PlayerControllerClass = AAGASSPlayerController::StaticClass();
	GameStateClass = AAGASSGameStateBase::StaticClass();
	DefaultPawnClass = nullptr;
	bStartPlayersAsSpectators = true;
}

void AAGASSGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const UAGASSPlayerDeveloperSettings* const PlayerSettings = UAGASSPlayerDeveloperSettings::Get();
	const bool bIsGameplaySessionMap = AGASSGameModePrivate::IsGameplaySessionWorld(GetWorld(), Options);

	UClass* ResolvedPlayerControllerClass = PlayerSettings ? PlayerSettings->PlayerControllerClass.LoadSynchronous() : nullptr;
	if (ResolvedPlayerControllerClass == nullptr)
	{
		ResolvedPlayerControllerClass = AAGASSPlayerController::StaticClass();
	}

	UClass* ResolvedTowerPawnClass = PlayerSettings ? PlayerSettings->TowerPawnClass.LoadSynchronous() : nullptr;
	if (ResolvedTowerPawnClass == nullptr)
	{
		ResolvedTowerPawnClass = AAGASSCharacter::StaticClass();
	}

	PlayerControllerClass = ResolvedPlayerControllerClass;
	DefaultPawnClass = bIsGameplaySessionMap ? ResolvedTowerPawnClass : nullptr;
	bStartPlayersAsSpectators = !bIsGameplaySessionMap;
}

void AAGASSGameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		UE_LOG(LogAGASSGameModeFlow, Display, TEXT("PreLogin already rejected by superclass. address=%s error=%s"), *Address, *ErrorMessage);
		return;
	}

	const UWorld* const World = GetWorld();
	const UAGASSRunStateComponent* const RunStateComponent = GetRunStateComponent();
	const UAGASSOnlineDeveloperSettings* const OnlineSettings = GetDefault<UAGASSOnlineDeveloperSettings>();
	UE_LOG(
		LogAGASSGameModeFlow,
		Display,
		TEXT("PreLogin address=%s world=%s runState=%s phase=%d joinClosed=%d"),
		*Address,
		*GetNameSafe(World),
		*GetNameSafe(RunStateComponent),
		RunStateComponent != nullptr ? static_cast<int32>(RunStateComponent->GetRunPhase()) : -1,
		RunStateComponent != nullptr && RunStateComponent->IsJoinInProgressClosed() ? 1 : 0);

	const FString ClientBuildKey = UGameplayStatics::ParseOption(Options, TEXT("AGASS_BUILD_KEY"));
	const FString ExpectedBuildKey = OnlineSettings ? OnlineSettings->BuildCompatibilityKey : FString();
	if (!ExpectedBuildKey.IsEmpty() && !ClientBuildKey.Equals(ExpectedBuildKey, ESearchCase::IgnoreCase))
	{
		ErrorMessage = TEXT("AGASS_BuildMismatch");
		UE_LOG(
			LogAGASSGameModeFlow,
			Warning,
			TEXT("PreLogin rejecting build mismatch for address=%s clientBuildKey=%s expectedBuildKey=%s"),
			*Address,
			*ClientBuildKey,
			*ExpectedBuildKey);
		return;
	}

	const FString ClientContentCompatibility = UGameplayStatics::ParseOption(Options, TEXT("AGASS_CONTENT_COMPAT"));
	const FString ExpectedContentCompatibility = World != nullptr
		? FString(World->URL.GetOption(TEXT("AGASS_CONTENT_COMPAT="), TEXT("")))
		: FString();
	if (!ExpectedContentCompatibility.IsEmpty()
		&& !ClientContentCompatibility.Equals(ExpectedContentCompatibility, ESearchCase::IgnoreCase))
	{
		ErrorMessage = TEXT("AGASS_ContentMismatch");
		UE_LOG(
			LogAGASSGameModeFlow,
			Warning,
			TEXT("PreLogin rejecting content mismatch for address=%s clientCompat=%s expectedCompat=%s"),
			*Address,
			*ClientContentCompatibility,
			*ExpectedContentCompatibility);
		return;
	}

	if (World != nullptr
		&& UAGASSTowerBootstrapSubsystem::IsGameplaySessionWorld(*World)
		&& RunStateComponent != nullptr
		&& RunStateComponent->IsJoinInProgressClosed())
	{
		ErrorMessage = TEXT("AGASS_RunClosed");
		UE_LOG(LogAGASSGameModeFlow, Warning, TEXT("PreLogin rejecting closed run for address=%s"), *Address);
	}
}

void AAGASSGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const UWorld* const World = GetWorld();
	const UAGASSRunStateComponent* const RunStateComponent = GetRunStateComponent();
	UE_LOG(
		LogAGASSGameModeFlow,
		Display,
		TEXT("PostLogin player=%s world=%s runState=%s phase=%d joinClosed=%d"),
		*GetNameSafe(NewPlayer),
		*GetNameSafe(World),
		*GetNameSafe(RunStateComponent),
		RunStateComponent != nullptr ? static_cast<int32>(RunStateComponent->GetRunPhase()) : -1,
		RunStateComponent != nullptr && RunStateComponent->IsJoinInProgressClosed() ? 1 : 0);
	if (NewPlayer != nullptr
		&& World != nullptr
		&& UAGASSTowerBootstrapSubsystem::IsGameplaySessionWorld(*World)
		&& RunStateComponent != nullptr
		&& RunStateComponent->IsJoinInProgressClosed())
	{
		RedirectLateJoinerToFrontend(NewPlayer);
	}
}

bool AAGASSGameModeBase::HandleAGASSObjectiveCompleted(AController* CompletingController, AActor* ObjectiveActor)
{
	const AAGASSObjectiveActor* const Objective = Cast<AAGASSObjectiveActor>(ObjectiveActor);
	FAGASSTimedRunCompletionData CompletionData;
	CompletionData.bWasVictory = true;
	CompletionData.ObjectiveName = Objective != nullptr
		? Objective->GetObjectiveNameText()
		: NSLOCTEXT("AGASSGameMode", "ObjectiveNameFallback", "Tower Objective");
	CompletionData.CompletionTitle = Objective != nullptr
		? Objective->GetCompletionTitleText()
		: NSLOCTEXT("AGASSGameMode", "CompletionTitleFallback", "Objective Complete");
	CompletionData.CompletionBody = Objective != nullptr
		? Objective->GetCompletionBodyText()
		: NSLOCTEXT("AGASSGameMode", "CompletionBodyFallback", "The team reached the tower objective. Returning to FrontendMap shortly.");
	CompletionData.ReturnToFrontendReason = Objective != nullptr
		? Objective->GetReturnToFrontendReasonText()
		: NSLOCTEXT("AGASSGameMode", "ReturnToFrontendReasonFallback", "Run complete. Returning to FrontendMap.");
	CompletionData.ReturnDelaySeconds = Objective != nullptr
		? Objective->GetReturnToFrontendDelaySeconds()
		: 8.f;
	return HandleAGASSTimedRunCompleted(CompletingController, ObjectiveActor, CompletionData);
}

bool AAGASSGameModeBase::HandleAGASSTimedRunStarted(AController* StartingController, AActor* StartActor)
{
	if (!HasAuthority())
	{
		UE_LOG(LogAGASSGameModeFlow, Warning, TEXT("HandleAGASSTimedRunStarted ignored because authority is missing."));
		return false;
	}

	UAGASSRunStateComponent* const RunStateComponent = GetRunStateComponent();
	if (RunStateComponent == nullptr
		|| RunStateComponent->GetRunPhase() != EAGASSRunPhase::Active
		|| RunStateComponent->HasTimedRunStarted())
	{
		UE_LOG(
			LogAGASSGameModeFlow,
			Warning,
			TEXT("HandleAGASSTimedRunStarted ignored. controller=%s startActor=%s runState=%s phase=%d timedRunState=%d"),
			*GetNameSafe(StartingController),
			*GetNameSafe(StartActor),
			*GetNameSafe(RunStateComponent),
			RunStateComponent != nullptr ? static_cast<int32>(RunStateComponent->GetRunPhase()) : -1,
			RunStateComponent != nullptr ? static_cast<int32>(RunStateComponent->GetTimedRunState()) : -1);
		return false;
	}

	RunStateComponent->MarkTimedRunStarted(GetCurrentServerWorldTimeSeconds());
	UE_LOG(
		LogAGASSGameModeFlow,
		Display,
		TEXT("Timed run started by=%s startActor=%s"),
		*GetNameSafe(StartingController),
		*GetNameSafe(StartActor));
	return true;
}

bool AAGASSGameModeBase::HandleAGASSTimedRunCompleted(
	AController* CompletingController,
	AActor* CompletionSourceActor,
	const FAGASSTimedRunCompletionData& CompletionData)
{
	if (!HasAuthority())
	{
		UE_LOG(LogAGASSGameModeFlow, Warning, TEXT("HandleAGASSTimedRunCompleted ignored because authority is missing."));
		return false;
	}

	UAGASSRunStateComponent* const RunStateComponent = GetRunStateComponent();
	if (RunStateComponent == nullptr
		|| RunStateComponent->GetRunPhase() != EAGASSRunPhase::Active
		|| RunStateComponent->GetTimedRunState() != EAGASSTimedRunState::Active)
	{
		UE_LOG(
			LogAGASSGameModeFlow,
			Warning,
			TEXT("HandleAGASSTimedRunCompleted ignored. source=%s runState=%s phase=%d timedRunState=%d"),
			*GetNameSafe(CompletionSourceActor),
			*GetNameSafe(RunStateComponent),
			RunStateComponent != nullptr ? static_cast<int32>(RunStateComponent->GetRunPhase()) : -1,
			RunStateComponent != nullptr ? static_cast<int32>(RunStateComponent->GetTimedRunState()) : -1);
		return false;
	}

	const AAGASSGameStateBase* const AGASSGameState = Cast<AAGASSGameStateBase>(GameState);
	const UAGASSTeamWalletComponent* const TeamWalletComponent = AGASSGameState != nullptr ? AGASSGameState->GetTeamWalletComponent() : nullptr;

	FAGASSRunSummaryData RunSummary;
	RunSummary.bHasRunSummary = true;
	RunSummary.bWasVictory = CompletionData.bWasVictory;
	RunSummary.ObjectiveName = !CompletionData.ObjectiveName.IsEmpty()
		? CompletionData.ObjectiveName
		: NSLOCTEXT("AGASSGameMode", "ObjectiveNameFallback", "Tower Objective");
	RunSummary.CompletionTitle = !CompletionData.CompletionTitle.IsEmpty()
		? CompletionData.CompletionTitle
		: NSLOCTEXT("AGASSGameMode", "CompletionTitleFallback", "Objective Complete");
	RunSummary.CompletionBody = !CompletionData.CompletionBody.IsEmpty()
		? CompletionData.CompletionBody
		: NSLOCTEXT("AGASSGameMode", "CompletionBodyFallback", "The team reached the tower objective. Returning to FrontendMap shortly.");
	RunSummary.FinalTeamMoney = TeamWalletComponent != nullptr ? TeamWalletComponent->GetCurrentBalance() : 0;
	RunSummary.ConnectedPlayerCount = GameState != nullptr ? GameState->PlayerArray.Num() : 0;
	RunSummary.RemainingPlaceableCount = CountRemainingTowerPlaceables();
	RunSummary.OfficialTimedRunMilliseconds = RunStateComponent->GetCurrentElapsedTimedRunMilliseconds();
	RunSummary.bHasOfficialTimedRunResult = true;
	RunSummary.RunDurationSeconds = static_cast<float>(RunSummary.OfficialTimedRunMilliseconds) / 1000.f;
	RunSummary.ReturnDelaySeconds = FMath::Max(CompletionData.ReturnDelaySeconds, 0.f);
	ResolveTimedRunSelectionMetadata(
		RunSummary.TimedRunLeaderboardKey,
		RunSummary.TimedRunMapId,
		RunSummary.TimedRunOwningModId);

	PendingCompletedRunFrontendReason = !CompletionData.ReturnToFrontendReason.IsEmpty()
		? CompletionData.ReturnToFrontendReason.ToString()
		: NSLOCTEXT("AGASSGameMode", "ReturnToFrontendReasonFallback", "Run complete. Returning to FrontendMap.").ToString();

	RunStateComponent->MarkRunCompleted(
		RunSummary,
		GetCurrentServerWorldTimeSeconds() + RunSummary.ReturnDelaySeconds);
	if (CompletionData.bWasVictory)
	{
		for (FConstPlayerControllerIterator ControllerIt = GetWorld()->GetPlayerControllerIterator(); ControllerIt; ++ControllerIt)
		{
			if (AAGASSPlayerController* const AGASSPlayerController = Cast<AAGASSPlayerController>(ControllerIt->Get()))
			{
				AGASSPlayerController->ClientPlayObjectiveReachedSound();
			}
		}
	}
	UE_LOG(
		LogAGASSGameModeFlow,
		Display,
		TEXT("Timed run completed by=%s source=%s officialMs=%d delay=%.2f frontendReason=\"%s\""),
		*GetNameSafe(CompletingController),
		*GetNameSafe(CompletionSourceActor),
		RunSummary.OfficialTimedRunMilliseconds,
		RunSummary.ReturnDelaySeconds,
		*PendingCompletedRunFrontendReason);
	CloseJoinInProgressForCompletedRun();

	if (RunSummary.ReturnDelaySeconds <= 0.f)
	{
		StartReturnToFrontendFromCompletedRun();
		return true;
	}

	GetWorldTimerManager().SetTimer(
		PendingFrontendReturnTimer,
		this,
		&ThisClass::StartReturnToFrontendFromCompletedRun,
		RunSummary.ReturnDelaySeconds,
		false);
	return true;
}

void AAGASSGameModeBase::Logout(AController* Exiting)
{
	if (FTimerHandle* const TimerHandle = PendingRespawnTimers.Find(Exiting))
	{
		GetWorldTimerManager().ClearTimer(*TimerHandle);
		PendingRespawnTimers.Remove(Exiting);
	}

	DestroyPendingRespawnCorpse(Exiting);

	Super::Logout(Exiting);
}

void AAGASSGameModeBase::HandlePlayerPawnFellOutOfWorld(APawn* FallenPawn)
{
	HandlePlayerPawnFallDeath(FallenPawn, TEXT("fell out of world"), false, false);
}

void AAGASSGameModeBase::HandlePlayerPawnTimedFallDeath(APawn* FallenPawn)
{
	HandlePlayerPawnFallDeath(FallenPawn, TEXT("dangerous fall timer"), true, true);
}

void AAGASSGameModeBase::CloseJoinInProgressForCompletedRun()
{
	if (UGameInstance* const GameInstance = GetGameInstance())
	{
		if (UAGASSSessionSubsystem* const SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			UE_LOG(LogAGASSGameModeFlow, Display, TEXT("Closing join-in-progress for completed run."));
			SessionSubsystem->SetHostedSessionJoinInProgressEnabled(false);
		}
	}
}

void AAGASSGameModeBase::StartReturnToFrontendFromCompletedRun()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PendingFrontendReturnTimer);
	UE_LOG(LogAGASSGameModeFlow, Display, TEXT("StartReturnToFrontendFromCompletedRun reason=\"%s\""), *PendingCompletedRunFrontendReason);

	if (UAGASSRunStateComponent* const RunStateComponent = GetRunStateComponent())
	{
		RunStateComponent->MarkReturningToFrontend();
	}

	CleanupTransientSessionStateBeforeFrontendReturn();

	if (UGameInstance* const GameInstance = GetGameInstance())
	{
		if (UAGASSSessionSubsystem* const SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			SessionSubsystem->ReturnToFrontend(PendingCompletedRunFrontendReason, true);
		}
	}
}

void AAGASSGameModeBase::CleanupTransientSessionStateBeforeFrontendReturn()
{
	ShutdownSessionEventRuntime();

	TArray<AController*> ControllersToCleanup;
	PendingRespawnTimers.GenerateKeyArray(ControllersToCleanup);

	for (const TPair<AController*, TWeakObjectPtr<APawn>>& PendingCorpsePair : PendingRespawnCorpses)
	{
		ControllersToCleanup.AddUnique(PendingCorpsePair.Key);
	}

	for (FConstPlayerControllerIterator ControllerIt = GetWorld()->GetPlayerControllerIterator(); ControllerIt; ++ControllerIt)
	{
		if (APlayerController* const PlayerController = ControllerIt->Get())
		{
			ControllersToCleanup.AddUnique(PlayerController);

			if (UAGASSInteractionComponent* const InteractionComponent = PlayerController->FindComponentByClass<UAGASSInteractionComponent>())
			{
				InteractionComponent->ForceReleaseHeldItem(true);
			}
		}
	}

	for (AController* const Controller : ControllersToCleanup)
	{
		if (Controller == nullptr)
		{
			continue;
		}

		if (FTimerHandle* const TimerHandle = PendingRespawnTimers.Find(Controller))
		{
			GetWorldTimerManager().ClearTimer(*TimerHandle);
			PendingRespawnTimers.Remove(Controller);
		}

		DestroyPendingRespawnCorpse(Controller);
	}

	UE_LOG(LogAGASSGameModeFlow, Display, TEXT("CleanupTransientSessionStateBeforeFrontendReturn cleanedControllers=%d"), ControllersToCleanup.Num());
}

void AAGASSGameModeBase::ShutdownSessionEventRuntime()
{
	const AAGASSGameStateBase* const AGASSGameState = Cast<AAGASSGameStateBase>(GameState);
	UAGASSSessionEventManagerComponent* const SessionEventManager = AGASSGameState != nullptr
		? AGASSGameState->GetSessionEventManagerComponent()
		: nullptr;
	if (SessionEventManager == nullptr)
	{
		return;
	}

	SessionEventManager->ShutdownEventRuntime();
}

void AAGASSGameModeBase::RedirectLateJoinerToFrontend(APlayerController* NewPlayer) const
{
	if (NewPlayer == nullptr)
	{
		return;
	}

	UE_LOG(LogAGASSGameModeFlow, Warning, TEXT("RedirectLateJoinerToFrontend player=%s"), *GetNameSafe(NewPlayer));
	NewPlayer->SetIgnoreMoveInput(true);
	NewPlayer->SetIgnoreLookInput(true);

	if (AAGASSPlayerController* const AGASSPlayerController = Cast<AAGASSPlayerController>(NewPlayer))
	{
		AGASSPlayerController->ClientReturnToFrontendForClosedRun(GetClosedRunJoinDeniedText());
		return;
	}

	NewPlayer->ClientReturnToMainMenuWithTextReason(GetClosedRunJoinDeniedText());
}

FText AAGASSGameModeBase::GetClosedRunJoinDeniedText() const
{
	return NSLOCTEXT("AGASSGameMode", "ClosedRunJoinDenied", "That tower run is already ending. Join a fresh active session instead.");
}

void AAGASSGameModeBase::HandlePlayerPawnFallDeath(APawn* FallenPawn, const TCHAR* CauseLabel, const bool bKeepCorpseInWorld, const bool bUseRagdollPresentation)
{
	if (!HasAuthority() || FallenPawn == nullptr)
	{
		return;
	}

	AController* const OwningController = FallenPawn->GetController();
	if (OwningController == nullptr || PendingRespawnTimers.Contains(OwningController))
	{
		return;
	}

	if (OwningController->GetPawn() == FallenPawn)
	{
		OwningController->UnPossess();
	}

	if (bKeepCorpseInWorld)
	{
		if (bUseRagdollPresentation)
		{
			if (AAGASSCharacter* const FallenCharacter = Cast<AAGASSCharacter>(FallenPawn))
			{
				FallenCharacter->ActivateDeathRagdoll();
			}
		}

		if (APlayerController* const OwningPlayerController = Cast<APlayerController>(OwningController))
		{
			OwningPlayerController->SetViewTarget(FallenPawn);
		}

		PendingRespawnCorpses.Add(OwningController, FallenPawn);
	}
	else
	{
		FallenPawn->Destroy();
	}

	QueueRespawn(OwningController);
}

void AAGASSGameModeBase::DestroyPendingRespawnCorpse(AController* Controller)
{
	if (Controller == nullptr)
	{
		return;
	}

	TWeakObjectPtr<APawn> PendingCorpse;
	if (!PendingRespawnCorpses.RemoveAndCopyValue(Controller, PendingCorpse))
	{
		return;
	}

	if (APawn* const CorpsePawn = PendingCorpse.Get())
	{
		CorpsePawn->Destroy();
	}
}

void AAGASSGameModeBase::QueueRespawn(AController* Controller)
{
	if (Controller == nullptr || PendingRespawnTimers.Contains(Controller))
	{
		return;
	}

	const float RespawnDelaySeconds = GetRespawnDelaySeconds();
	if (RespawnDelaySeconds <= 0.f)
	{
		FinishQueuedRespawn(Controller);
		return;
	}

	FTimerHandle RespawnTimerHandle;
	GetWorldTimerManager().SetTimer(
		RespawnTimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::FinishQueuedRespawn, Controller),
		RespawnDelaySeconds,
		false);
	PendingRespawnTimers.Add(Controller, RespawnTimerHandle);
}

void AAGASSGameModeBase::FinishQueuedRespawn(AController* Controller)
{
	if (Controller == nullptr)
	{
		return;
	}

	if (FTimerHandle* const TimerHandle = PendingRespawnTimers.Find(Controller))
	{
		GetWorldTimerManager().ClearTimer(*TimerHandle);
		PendingRespawnTimers.Remove(Controller);
	}

	DestroyPendingRespawnCorpse(Controller);

	if (Controller->GetPawn() == nullptr)
	{
		RestartPlayer(Controller);
	}
}

float AAGASSGameModeBase::GetCurrentServerWorldTimeSeconds() const
{
	if (const AGameStateBase* const CurrentGameState = GameState)
	{
		return CurrentGameState->GetServerWorldTimeSeconds();
	}

	return GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
}

int32 AAGASSGameModeBase::CountRemainingTowerPlaceables() const
{
	int32 PlaceableCount = 0;

	if (UWorld* const World = GetWorld())
	{
		for (TActorIterator<AAGASSPlaceableItemActor> It(World); It; ++It)
		{
			if (!It->IsHeldHidden())
			{
				++PlaceableCount;
			}
		}
	}

	return PlaceableCount;
}

UAGASSRunStateComponent* AAGASSGameModeBase::GetRunStateComponent() const
{
	if (const AAGASSGameStateBase* const AGASSGameState = GetGameState<AAGASSGameStateBase>())
	{
		return AGASSGameState->GetRunStateComponent();
	}

	return nullptr;
}

void AAGASSGameModeBase::ResolveTimedRunSelectionMetadata(FString& OutLeaderboardKey, FString& OutMapId, FString& OutOwningModId) const
{
	OutLeaderboardKey.Reset();
	OutMapId.Reset();
	OutOwningModId.Reset();

	if (const UGameInstance* const GameInstance = GetGameInstance())
	{
		if (const UAGASSModsSubsystem* const ModsSubsystem = GameInstance->GetSubsystem<UAGASSModsSubsystem>())
		{
			FAGASSResolvedContentSelection HostedSelection;
			FString FailureMessage;
			if (GetWorld() != nullptr
				&& ModsSubsystem->TryBuildRuntimeSelectionForWorld(*GetWorld(), HostedSelection, FailureMessage)
				&& HostedSelection.bIsValid)
			{
				OutLeaderboardKey = HostedSelection.BuildTimedRunLeaderboardKey();
				OutMapId = HostedSelection.SelectedMap.MapId;
				OutOwningModId = HostedSelection.SelectedMap.OwningModId;
			}
		}
	}

	if (OutMapId.IsEmpty())
	{
		OutMapId = GetWorld() != nullptr ? FPackageName::GetShortName(GetWorld()->GetOutermost()->GetName()) : FString(TEXT("unknown"));
	}

	if (OutLeaderboardKey.IsEmpty())
	{
		OutLeaderboardKey = FString::Printf(TEXT("timedrun_%s"), *AGASSGameModePrivate::BuildSafeTimedRunKeySuffix(OutMapId));
	}
}

float AAGASSGameModeBase::GetRespawnDelaySeconds() const
{
	if (const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get())
	{
		if (const UAGASSRespawnTuningData* const RespawnTuning = TowerSettings->DefaultRespawnTuning.LoadSynchronous())
		{
			return FMath::Max(RespawnTuning->RespawnDelaySeconds, 0.f);
		}
	}

	return 3.f;
}
