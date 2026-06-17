
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GenericTeamAgentInterface.h"
#include "GAS/Abilities/MobaGameplayTypes.h"
#include "MobaGameMode.generated.h"


class AMobaDestroyer;

UCLASS()
class AMobaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual APlayerController* SpawnPlayerController(ENetRole InRemoteRole, const FString& Options) override;
	virtual void StartPlay() override;

private:
	FGenericTeamId GetTeamIDForPlayer(const APlayerController* PlayerController) const;

	AActor* FindNextStartSpotForTeam(const FGenericTeamId& TeamID) const;

	AMobaDestroyer* GetDestroyer() const;

	void MatchFinished(AActor* ViewTarget, EMobaTeam WinningTeam); 
	
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TMap<FGenericTeamId, FName> TeamPlayerStartTagMap;
};

