#include "Components/AGASSRunStateComponent.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Settings/AGASSSettingsLocal.h"
#include "Subsystems/AGASSGameplayEventSubsystem.h"
#include "Subsystems/AGASSSessionSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSRunState, Log, All);

namespace
{
	const TCHAR* LexToString(const EAGASSRunPhase RunPhase)
	{
		switch (RunPhase)
		{
		case EAGASSRunPhase::Active:
			return TEXT("Active");
		case EAGASSRunPhase::Completed:
			return TEXT("Completed");
		case EAGASSRunPhase::ReturningToFrontend:
			return TEXT("ReturningToFrontend");
		default:
			return TEXT("Unknown");
		}
	}

	const TCHAR* LexToString(const EAGASSTimedRunState TimedRunState)
	{
		switch (TimedRunState)
		{
		case EAGASSTimedRunState::WaitingToStart:
			return TEXT("WaitingToStart");
		case EAGASSTimedRunState::Active:
			return TEXT("Active");
		case EAGASSTimedRunState::Completed:
			return TEXT("Completed");
		default:
			return TEXT("Unknown");
		}
	}
}

UAGASSRunStateComponent::UAGASSRunStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAGASSRunStateComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogAGASSRunState,
		Display,
		TEXT("BeginPlay owner=%s authority=%d phase=%s timedRunState=%s runStart=%.2f"),
		*GetNameSafe(GetOwner()),
		GetOwner() != nullptr && GetOwner()->HasAuthority() ? 1 : 0,
		LexToString(RunPhase),
		LexToString(TimedRunState),
		TimedRunStartTimeSeconds);

	EvaluateClientFrontendReturn();
}

void UAGASSRunStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, RunPhase);
	DOREPLIFETIME(ThisClass, RunSummary);
	DOREPLIFETIME(ThisClass, TimedRunStartTimeSeconds);
	DOREPLIFETIME(ThisClass, TimedRunState);
	DOREPLIFETIME(ThisClass, ReturnToFrontendServerTimeSeconds);
}

float UAGASSRunStateComponent::GetElapsedRunSeconds() const
{
	return static_cast<float>(GetCurrentElapsedTimedRunMilliseconds()) / 1000.f;
}

int32 UAGASSRunStateComponent::GetCurrentElapsedTimedRunMilliseconds() const
{
	if (TimedRunState == EAGASSTimedRunState::WaitingToStart)
	{
		return 0;
	}

	if (RunSummary.bHasOfficialTimedRunResult)
	{
		return FMath::Max(RunSummary.OfficialTimedRunMilliseconds, 0);
	}

	return FMath::Max(FMath::RoundToInt((GetCurrentServerWorldTimeSeconds() - TimedRunStartTimeSeconds) * 1000.f), 0);
}

float UAGASSRunStateComponent::GetRemainingReturnToFrontendSeconds() const
{
	if (RunPhase == EAGASSRunPhase::Active || ReturnToFrontendServerTimeSeconds <= 0.f)
	{
		return 0.f;
	}

	return FMath::Max(ReturnToFrontendServerTimeSeconds - GetCurrentServerWorldTimeSeconds(), 0.f);
}

void UAGASSRunStateComponent::MarkTimedRunStarted(const float NewTimedRunStartTimeSeconds)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || RunPhase != EAGASSRunPhase::Active || TimedRunState != EAGASSTimedRunState::WaitingToStart)
	{
		return;
	}

	TimedRunStartTimeSeconds = FMath::Max(NewTimedRunStartTimeSeconds, 0.f);
	TimedRunState = EAGASSTimedRunState::Active;

	UE_LOG(
		LogAGASSRunState,
		Display,
		TEXT("MarkTimedRunStarted owner=%s startTime=%.2f"),
		*GetNameSafe(GetOwner()),
		TimedRunStartTimeSeconds);
	RunStateChangedEvent.Broadcast();
}

void UAGASSRunStateComponent::MarkRunCompleted(const FAGASSRunSummaryData& NewRunSummary, const float NewReturnToFrontendServerTimeSeconds)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}

	RunSummary = NewRunSummary;
	ReturnToFrontendServerTimeSeconds = FMath::Max(NewReturnToFrontendServerTimeSeconds, 0.f);
	TimedRunState = EAGASSTimedRunState::Completed;
	RunPhase = EAGASSRunPhase::Completed;

	UE_LOG(
		LogAGASSRunState,
		Display,
		TEXT("MarkRunCompleted owner=%s objective=\"%s\" players=%d money=%d placeables=%d duration=%.2f officialMs=%d returnAt=%.2f delay=%.2f"),
		*GetNameSafe(GetOwner()),
		*RunSummary.ObjectiveName.ToString(),
		RunSummary.ConnectedPlayerCount,
		RunSummary.FinalTeamMoney,
		RunSummary.RemainingPlaceableCount,
		RunSummary.RunDurationSeconds,
		RunSummary.OfficialTimedRunMilliseconds,
		ReturnToFrontendServerTimeSeconds,
		RunSummary.ReturnDelaySeconds);

	RunSummaryChangedEvent.Broadcast();
	if (RunSummary.bHasOfficialTimedRunResult)
	{
		if (UAGASSSettingsLocal* const LocalSettings = UAGASSSettingsLocal::Get())
		{
			LocalSettings->UpdateTimedRunBestMilliseconds(
				RunSummary.TimedRunLeaderboardKey,
				RunSummary.TimedRunMapId,
				RunSummary.TimedRunOwningModId,
				RunSummary.OfficialTimedRunMilliseconds);
		}
	}
	BroadcastRunCompletedGameplayEvent();
	RunStateChangedEvent.Broadcast();
}

void UAGASSRunStateComponent::MarkReturningToFrontend()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority())
	{
		return;
	}

	RunPhase = EAGASSRunPhase::ReturningToFrontend;
	UE_LOG(
		LogAGASSRunState,
		Display,
		TEXT("MarkReturningToFrontend owner=%s returnAt=%.2f remaining=%.2f"),
		*GetNameSafe(GetOwner()),
		ReturnToFrontendServerTimeSeconds,
		GetRemainingReturnToFrontendSeconds());
	RunStateChangedEvent.Broadcast();
}

void UAGASSRunStateComponent::OnRep_RunPhase()
{
	UE_LOG(
		LogAGASSRunState,
		Display,
		TEXT("OnRep_RunPhase owner=%s newPhase=%s timedRunState=%s remaining=%.2f"),
		*GetNameSafe(GetOwner()),
		LexToString(RunPhase),
		LexToString(TimedRunState),
		GetRemainingReturnToFrontendSeconds());
	EvaluateClientFrontendReturn();
	RunStateChangedEvent.Broadcast();
}

void UAGASSRunStateComponent::OnRep_RunSummary()
{
	UE_LOG(
		LogAGASSRunState,
		Display,
		TEXT("OnRep_RunSummary owner=%s hasSummary=%d objective=\"%s\" title=\"%s\""),
		*GetNameSafe(GetOwner()),
		RunSummary.bHasRunSummary ? 1 : 0,
		*RunSummary.ObjectiveName.ToString(),
		*RunSummary.CompletionTitle.ToString());
	EvaluateClientFrontendReturn();
	if (RunSummary.bHasOfficialTimedRunResult)
	{
		if (UAGASSSettingsLocal* const LocalSettings = UAGASSSettingsLocal::Get())
		{
			LocalSettings->UpdateTimedRunBestMilliseconds(
				RunSummary.TimedRunLeaderboardKey,
				RunSummary.TimedRunMapId,
				RunSummary.TimedRunOwningModId,
				RunSummary.OfficialTimedRunMilliseconds);
		}
	}
	BroadcastRunCompletedGameplayEvent();
	RunSummaryChangedEvent.Broadcast();
}

void UAGASSRunStateComponent::OnRep_TimedRunState()
{
	UE_LOG(
		LogAGASSRunState,
		Display,
		TEXT("OnRep_TimedRunState owner=%s timedRunState=%s elapsedMs=%d"),
		*GetNameSafe(GetOwner()),
		LexToString(TimedRunState),
		GetCurrentElapsedTimedRunMilliseconds());
	RunStateChangedEvent.Broadcast();
}

void UAGASSRunStateComponent::OnRep_ReturnToFrontendServerTimeSeconds()
{
	UE_LOG(
		LogAGASSRunState,
		Display,
		TEXT("OnRep_ReturnToFrontendServerTimeSeconds owner=%s returnAt=%.2f remaining=%.2f"),
		*GetNameSafe(GetOwner()),
		ReturnToFrontendServerTimeSeconds,
		GetRemainingReturnToFrontendSeconds());
	EvaluateClientFrontendReturn();
	RunStateChangedEvent.Broadcast();
}

void UAGASSRunStateComponent::EvaluateClientFrontendReturn()
{
	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetNetMode() != NM_Client || bClientFrontendReturnTriggered)
	{
		return;
	}

	if (RunPhase == EAGASSRunPhase::Active)
	{
		StopClientFrontendReturnTimer();
		return;
	}

	const float RemainingSeconds = GetRemainingReturnToFrontendSeconds();
	if (RunPhase == EAGASSRunPhase::ReturningToFrontend || RemainingSeconds <= 0.05f)
	{
		UGameInstance* const GameInstance = World->GetGameInstance();
		UAGASSSessionSubsystem* const SessionSubsystem = GameInstance != nullptr ? GameInstance->GetSubsystem<UAGASSSessionSubsystem>() : nullptr;
		if (SessionSubsystem == nullptr)
		{
			UE_LOG(LogAGASSRunState, Warning, TEXT("EvaluateClientFrontendReturn could not find SessionSubsystem on client."));
			return;
		}

		bClientFrontendReturnTriggered = true;
		StopClientFrontendReturnTimer();

		const FString ReturnReason = !RunSummary.CompletionBody.IsEmpty()
			? RunSummary.CompletionBody.ToString()
			: NSLOCTEXT("AGASSTower", "ClientReturnFallbackReason", "Run complete. Returning to FrontendMap.").ToString();

		UE_LOG(
			LogAGASSRunState,
			Display,
			TEXT("EvaluateClientFrontendReturn triggering client frontend travel. phase=%s remaining=%.2f reason=\"%s\""),
			LexToString(RunPhase),
			RemainingSeconds,
			*ReturnReason);
		SessionSubsystem->ReturnToFrontend(ReturnReason, false);
		return;
	}

	StartClientFrontendReturnTimer();
}

void UAGASSRunStateComponent::HandleClientFrontendReturnTimerElapsed()
{
	EvaluateClientFrontendReturn();
}

void UAGASSRunStateComponent::StartClientFrontendReturnTimer()
{
	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetTimerManager().IsTimerActive(ClientFrontendReturnTimerHandle))
	{
		return;
	}

	UE_LOG(LogAGASSRunState, Display, TEXT("Starting client frontend return timer. remaining=%.2f"), GetRemainingReturnToFrontendSeconds());
	World->GetTimerManager().SetTimer(ClientFrontendReturnTimerHandle, this, &ThisClass::HandleClientFrontendReturnTimerElapsed, 0.25f, true);
}

void UAGASSRunStateComponent::StopClientFrontendReturnTimer()
{
	if (UWorld* const World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerActive(ClientFrontendReturnTimerHandle))
		{
			UE_LOG(LogAGASSRunState, Display, TEXT("Stopping client frontend return timer."));
		}
		World->GetTimerManager().ClearTimer(ClientFrontendReturnTimerHandle);
	}
}

float UAGASSRunStateComponent::GetCurrentServerWorldTimeSeconds() const
{
	if (const AGameStateBase* const OwningGameState = Cast<AGameStateBase>(GetOwner()))
	{
		return OwningGameState->GetServerWorldTimeSeconds();
	}

	return GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.f;
}

void UAGASSRunStateComponent::BroadcastRunCompletedGameplayEvent()
{
	if (bHasBroadcastRunCompletedGameplayEvent || !RunSummary.bHasRunSummary || RunPhase == EAGASSRunPhase::Active)
	{
		return;
	}

	bHasBroadcastRunCompletedGameplayEvent = true;
	UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(
		this,
		AGASSGameplayEventNames::RunCompleted(),
		RunSummary.OfficialTimedRunMilliseconds,
		RunSummary.RunDurationSeconds,
		RunSummary.bWasVictory);
}
