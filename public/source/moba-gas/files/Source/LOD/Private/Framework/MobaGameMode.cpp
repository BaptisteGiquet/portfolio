
#include "Framework/MobaGameMode.h"

#include "EngineUtils.h"
#include "MobaDestroyer.h"
#include "GameFramework/PlayerStart.h"
#include "Player/MobaPlayerController.h"

APlayerController* AMobaGameMode::SpawnPlayerController(ENetRole InRemoteRole, const FString& Options)
{
	APlayerController* NewPlayerController =  Super::SpawnPlayerController(InRemoteRole, Options);
	IGenericTeamAgentInterface* NewPlayerTeamInterface = Cast<IGenericTeamAgentInterface>(NewPlayerController);

	const FGenericTeamId TeamID = GetTeamIDForPlayer(NewPlayerController);
	if (NewPlayerTeamInterface)
	{
		NewPlayerTeamInterface->SetGenericTeamId(TeamID);
	}

	NewPlayerController->StartSpot = FindNextStartSpotForTeam(TeamID);
	return NewPlayerController;
}


void AMobaGameMode::StartPlay()
{
	Super::StartPlay();

	AMobaDestroyer* Destroyer = GetDestroyer();
	if (Destroyer)
	{
		Destroyer->OnGoalReached.AddUObject(this, &ThisClass::MatchFinished);
	}
}


FGenericTeamId AMobaGameMode::GetTeamIDForPlayer(const APlayerController* PlayerController) const
{
	static int32 PlayerCount = 0;
	++PlayerCount;
	return FGenericTeamId(PlayerCount % 2);
}




AActor* AMobaGameMode::FindNextStartSpotForTeam(const FGenericTeamId& TeamID) const
{
	const FName CurrentPlayerStartTag = TeamPlayerStartTagMap.FindRef(TeamID);
	if (!CurrentPlayerStartTag.IsValid())
	{
		return nullptr;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	static const FName TakenTag(TEXT("Taken"));

	for (APlayerStart* WorldPlayerStart : TActorRange<APlayerStart>(World))
	{
		if (!WorldPlayerStart)
		{
			continue;
		}
		
		if (WorldPlayerStart->PlayerStartTag == CurrentPlayerStartTag)
		{
			WorldPlayerStart->PlayerStartTag = TakenTag;
			return WorldPlayerStart;
		}
	}

	return nullptr;
}


AMobaDestroyer* AMobaGameMode::GetDestroyer() const
{
	UWorld* World = GetWorld();
	if (World)
	{
		for (AMobaDestroyer* Destroyer : TActorRange<AMobaDestroyer>(World))
		{
			return Destroyer;
		}	
	}

	return nullptr;
}


void AMobaGameMode::MatchFinished(AActor* ViewTarget, EMobaTeam WinningTeam)
{
	UWorld* World = GetWorld();
	if (World)
	{
		for (AMobaPlayerController* PlayerController : TActorRange<AMobaPlayerController>(World))
		{
			PlayerController->MatchFinished(ViewTarget, WinningTeam);
		}	
	}
}
