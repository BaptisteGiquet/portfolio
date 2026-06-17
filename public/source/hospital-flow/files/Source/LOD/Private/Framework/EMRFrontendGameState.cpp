#include "Framework/EMRFrontendGameState.h"

#include "Net/UnrealNetwork.h"

AEMRFrontendGameState::AEMRFrontendGameState()
{
}

void AEMRFrontendGameState::SetLobbyHosting(bool bHosting)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsLobbyHosting == bHosting)
	{
		return;
	}

	bIsLobbyHosting = bHosting;
	OnRep_LobbyHosting();
}

void AEMRFrontendGameState::OnRep_LobbyHosting()
{
	OnLobbyHostingChanged.Broadcast(bIsLobbyHosting);
}

void AEMRFrontendGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEMRFrontendGameState, bIsLobbyHosting);
}
