#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EMRFrontendLobbyPlayerController.generated.h"

enum class ELobbyInviteJoinResult : uint8;
class APlayerState;

/**
 * 
 */
UCLASS()
class LOD_API AEMRFrontendLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(class APawn* P) override;
	virtual void PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel) override;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void ToggleLobbyReady();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestKickPlayer(APlayerState* TargetPlayerState);

	UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
	void RequestJoinLobbyByInviteCode(const FString& Code);

	UFUNCTION(BlueprintCallable, Category = "Lobby|InviteCode")
	void RequestRegenerateLobbyInviteCode();

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestStartGame();
	
	UFUNCTION(Client, Reliable)
	void Client_HandleKickedFromLobby();

private:
	UFUNCTION(Server, Reliable)
	void Server_RegenerateLobbyInviteCode();

	UFUNCTION(Server, Reliable)
	void Server_RequestStartGame();

	UFUNCTION(Server, Reliable)
	void Server_RequestKickPlayer(APlayerState* TargetPlayerState);

	

	void HandleInviteJoinResult(ELobbyInviteJoinResult Result);
};
