#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Interfaces/AGASSObjectiveRuntimeInterface.h"
#include "Interfaces/AGASSTimedRunRuntimeInterface.h"
#include "AGASSGameModeBase.generated.h"

class APawn;
class AController;
class AAGASSObjectiveActor;
class APlayerController;
class UAGASSRunStateComponent;
class UAGASSSessionEventManagerComponent;

UCLASS()
class AGASS_API AAGASSGameModeBase : public AGameModeBase, public IAGASSObjectiveRuntimeInterface, public IAGASSTimedRunRuntimeInterface
{
	GENERATED_BODY()

public:
	AAGASSGameModeBase();

	void HandlePlayerPawnFellOutOfWorld(APawn* FallenPawn);
	void HandlePlayerPawnTimedFallDeath(APawn* FallenPawn);
	virtual bool HandleAGASSObjectiveCompleted(AController* CompletingController, AActor* ObjectiveActor) override;
	virtual bool HandleAGASSTimedRunStarted(AController* StartingController, AActor* StartActor) override;
	virtual bool HandleAGASSTimedRunCompleted(
		AController* CompletingController,
		AActor* CompletionSourceActor,
		const FAGASSTimedRunCompletionData& CompletionData) override;

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

private:
	void CloseJoinInProgressForCompletedRun();
	void StartReturnToFrontendFromCompletedRun();
	void CleanupTransientSessionStateBeforeFrontendReturn();
	void ShutdownSessionEventRuntime();
	void RedirectLateJoinerToFrontend(APlayerController* NewPlayer) const;
	FText GetClosedRunJoinDeniedText() const;
	void HandlePlayerPawnFallDeath(APawn* FallenPawn, const TCHAR* CauseLabel, bool bKeepCorpseInWorld, bool bUseRagdollPresentation);
	void DestroyPendingRespawnCorpse(AController* Controller);
	void QueueRespawn(AController* Controller);
	void FinishQueuedRespawn(AController* Controller);
	float GetRespawnDelaySeconds() const;
	float GetCurrentServerWorldTimeSeconds() const;
	int32 CountRemainingTowerPlaceables() const;
	UAGASSRunStateComponent* GetRunStateComponent() const;
	void ResolveTimedRunSelectionMetadata(FString& OutLeaderboardKey, FString& OutMapId, FString& OutOwningModId) const;

	TMap<AController*, FTimerHandle> PendingRespawnTimers;
	TMap<AController*, TWeakObjectPtr<APawn>> PendingRespawnCorpses;
	FTimerHandle PendingFrontendReturnTimer;
	FString PendingCompletedRunFrontendReason;
};
