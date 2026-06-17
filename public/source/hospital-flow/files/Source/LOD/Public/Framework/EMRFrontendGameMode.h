#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EMRFrontendGameMode.generated.h"

class AEMRFrontendGameState;
class AEMRLobbyPlayerSlot;
class APlayerState;

UCLASS()
class LOD_API AEMRFrontendGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEMRFrontendGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

private:
	void HandleLobbySlotRegistered(AEMRLobbyPlayerSlot* SlotActor);
	void RemovePendingPlayerState(APlayerState* PlayerState);
	void TryAssignPendingPlayers();

	UPROPERTY()
	TArray<TWeakObjectPtr<APlayerState>> PendingPlayerStates;

	AEMRFrontendGameState* GetFrontendGameState() const;
};
