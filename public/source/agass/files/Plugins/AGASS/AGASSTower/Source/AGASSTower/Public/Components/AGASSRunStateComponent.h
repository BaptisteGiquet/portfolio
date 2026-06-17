#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "Types/AGASSTimedRunTypes.h"
#include "AGASSRunStateComponent.generated.h"

UENUM(BlueprintType)
enum class EAGASSRunPhase : uint8
{
	Active,
	Completed,
	ReturningToFrontend
};

USTRUCT(BlueprintType)
struct AGASSTOWER_API FAGASSRunSummaryData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	bool bHasRunSummary = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	bool bWasVictory = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	FText ObjectiveName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	FText CompletionTitle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	FText CompletionBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	int32 FinalTeamMoney = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	int32 ConnectedPlayerCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	int32 RemainingPlaceableCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	float RunDurationSeconds = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Run")
	float ReturnDelaySeconds = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Timed Run")
	bool bHasOfficialTimedRunResult = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Timed Run")
	int32 OfficialTimedRunMilliseconds = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Timed Run")
	FString TimedRunLeaderboardKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Timed Run")
	FString TimedRunMapId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Timed Run")
	FString TimedRunOwningModId;
};

DECLARE_MULTICAST_DELEGATE(FAGASSRunStateChangedEvent);
DECLARE_MULTICAST_DELEGATE(FAGASSRunSummaryChangedEvent);

UCLASS(ClassGroup = (AGASS), meta = (BlueprintSpawnableComponent))
class AGASSTOWER_API UAGASSRunStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAGASSRunStateComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	EAGASSRunPhase GetRunPhase() const
	{
		return RunPhase;
	}

	const FAGASSRunSummaryData& GetRunSummary() const
	{
		return RunSummary;
	}

	float GetRunStartTimeSeconds() const
	{
		return TimedRunStartTimeSeconds;
	}

	EAGASSTimedRunState GetTimedRunState() const
	{
		return TimedRunState;
	}

	float GetTimedRunStartTimeSeconds() const
	{
		return TimedRunStartTimeSeconds;
	}

	float GetReturnToFrontendServerTimeSeconds() const
	{
		return ReturnToFrontendServerTimeSeconds;
	}

	bool IsRunActive() const
	{
		return RunPhase == EAGASSRunPhase::Active && TimedRunState == EAGASSTimedRunState::Active;
	}

	bool HasTimedRunStarted() const
	{
		return TimedRunState != EAGASSTimedRunState::WaitingToStart;
	}

	bool IsJoinInProgressClosed() const
	{
		return RunPhase != EAGASSRunPhase::Active;
	}

	float GetElapsedRunSeconds() const;
	int32 GetCurrentElapsedTimedRunMilliseconds() const;
	float GetRemainingReturnToFrontendSeconds() const;

	void MarkTimedRunStarted(float NewTimedRunStartTimeSeconds);
	void MarkRunCompleted(const FAGASSRunSummaryData& NewRunSummary, float NewReturnToFrontendServerTimeSeconds);
	void MarkReturningToFrontend();

	FAGASSRunStateChangedEvent& OnRunStateChanged()
	{
		return RunStateChangedEvent;
	}

	FAGASSRunSummaryChangedEvent& OnRunSummaryChanged()
	{
		return RunSummaryChangedEvent;
	}

private:
	UFUNCTION()
	void OnRep_RunPhase();

	UFUNCTION()
	void OnRep_RunSummary();

	UFUNCTION()
	void OnRep_TimedRunState();

	UFUNCTION()
	void OnRep_ReturnToFrontendServerTimeSeconds();

	float GetCurrentServerWorldTimeSeconds() const;
	void EvaluateClientFrontendReturn();
	void HandleClientFrontendReturnTimerElapsed();
	void StartClientFrontendReturnTimer();
	void StopClientFrontendReturnTimer();
	void BroadcastRunCompletedGameplayEvent();

	UPROPERTY(ReplicatedUsing = OnRep_RunPhase)
	EAGASSRunPhase RunPhase = EAGASSRunPhase::Active;

	UPROPERTY(ReplicatedUsing = OnRep_RunSummary)
	FAGASSRunSummaryData RunSummary;

	UPROPERTY(Replicated)
	float TimedRunStartTimeSeconds = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_TimedRunState)
	EAGASSTimedRunState TimedRunState = EAGASSTimedRunState::WaitingToStart;

	UPROPERTY(ReplicatedUsing = OnRep_ReturnToFrontendServerTimeSeconds)
	float ReturnToFrontendServerTimeSeconds = 0.f;

	FAGASSRunStateChangedEvent RunStateChangedEvent;
	FAGASSRunSummaryChangedEvent RunSummaryChangedEvent;
	FTimerHandle ClientFrontendReturnTimerHandle;
	bool bClientFrontendReturnTriggered = false;
	bool bHasBroadcastRunCompletedGameplayEvent = false;
};
