#include "Framework/EMRFrontendGameMode.h"

#include "Framework/EMRFrontendGameState.h"
#include "Framework/EMRLobbyPlayerSlot.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UI/Frontend/EMRFrontendPlayerController.h"

AEMRFrontendGameMode::AEMRFrontendGameMode()
{
	GameStateClass = AEMRFrontendGameState::StaticClass();
	PlayerControllerClass = AEMRFrontendPlayerController::StaticClass();
}

void AEMRFrontendGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (AEMRFrontendGameState* FrontendGameState = GetFrontendGameState())
	{
		FrontendGameState->OnLobbySlotRegistered.AddUObject(this, &ThisClass::HandleLobbySlotRegistered);
	}
}

void AEMRFrontendGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!HasAuthority() || !NewPlayer)
	{
		return;
	}

	APlayerState* PlayerState = NewPlayer->PlayerState;
	if (!PlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FrontendGameMode] PostLogin - PlayerState missing for %s"), *GetNameSafe(NewPlayer));
		return;
	}

	if (AEMRFrontendGameState* FrontendGameState = GetFrontendGameState())
	{
		if (!FrontendGameState->AssignSlotToPlayer(PlayerState))
		{
			PendingPlayerStates.AddUnique(PlayerState);
		}
	}
}

void AEMRFrontendGameMode::Logout(AController* Exiting)
{
	APlayerState* PlayerState = Exiting ? Exiting->PlayerState : nullptr;

	if (HasAuthority() && PlayerState)
	{
		RemovePendingPlayerState(PlayerState);
		if (AEMRFrontendGameState* FrontendGameState = GetFrontendGameState())
		{
			FrontendGameState->ClearSlotForPlayer(PlayerState);
		}
	}

	Super::Logout(Exiting);
}

AEMRFrontendGameState* AEMRFrontendGameMode::GetFrontendGameState() const
{
	return GetGameState<AEMRFrontendGameState>();
}

void AEMRFrontendGameMode::HandleLobbySlotRegistered(AEMRLobbyPlayerSlot* SlotActor)
{
	if (!HasAuthority() || !SlotActor)
	{
		return;
	}

	TryAssignPendingPlayers();
}

void AEMRFrontendGameMode::RemovePendingPlayerState(APlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	PendingPlayerStates.Remove(PlayerState);
}

void AEMRFrontendGameMode::TryAssignPendingPlayers()
{
	if (!HasAuthority())
	{
		return;
	}

	AEMRFrontendGameState* FrontendGameState = GetFrontendGameState();
	if (!FrontendGameState)
	{
		return;
	}

	for (int32 Index = PendingPlayerStates.Num() - 1; Index >= 0; --Index)
	{
		APlayerState* PlayerState = PendingPlayerStates[Index].Get();
		if (!PlayerState)
		{
			PendingPlayerStates.RemoveAt(Index);
			continue;
		}

		if (FrontendGameState->AssignSlotToPlayer(PlayerState))
		{
			PendingPlayerStates.RemoveAt(Index);
		}
	}
}
