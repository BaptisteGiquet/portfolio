#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EMRLobbyGameMode.generated.h"

class AEMRLobbyGameState;
class AEMRLobbyPlayerSlot;
class APlayerState;
class UWorld;

UCLASS()
class LOD_API AEMRLobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEMRLobbyGameMode();

    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    UFUNCTION(BlueprintCallable, Category = "Lobby|Travel")
    bool StartLobbyGame();

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    bool KickLobbyPlayer(APlayerState* TargetPlayerState);

private:
    void HandleLobbySlotRegistered(AEMRLobbyPlayerSlot* SlotActor);
    void AssignSlotsForExistingPlayers();
    void RemovePendingPlayerState(APlayerState* PlayerState);
    void TryAssignPendingPlayers();
    void ResetRunProgressionForFreshSession() const;
    bool TravelToHubLevel();
    void NotifyControllersOfPendingTravel(const FString& TravelURL);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Travel")
    TSoftObjectPtr<UWorld> HubLevel;

    UPROPERTY()
    TArray<TWeakObjectPtr<APlayerState>> PendingPlayerStates;

    AEMRLobbyGameState* GetLobbyGameState() const;
};
