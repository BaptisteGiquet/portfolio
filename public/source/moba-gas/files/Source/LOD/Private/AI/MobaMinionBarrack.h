
#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Actor.h"
#include "MobaMinionBarrack.generated.h"

class AMobaMinion;

UCLASS()
class AMobaMinionBarrack : public AActor
{
	GENERATED_BODY()
	
public:	
	AMobaMinionBarrack();
	
protected:
	virtual void BeginPlay() override;

private:
	void SpawnNewMinions(const int32 Amount);
	const APlayerStart* GetNextMinionSpawnSpot();

	void SpawnNewMinionWave();
	AMobaMinion* GetNextAvailableMinion() const;
	
	UPROPERTY(EditAnywhere, Category = "Spawn")
	FGenericTeamId BarrackTeamID;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TObjectPtr<AActor> Goal;
	
	UPROPERTY(VisibleAnywhere, Category = "Spawn")
	TArray<TObjectPtr<AMobaMinion>> MinionPool;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<AMobaMinion> MinionClass;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TArray<TObjectPtr<APlayerStart>> MinionSpawnSpots;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	int32 MinionPerWave = 5;
	
	UPROPERTY(EditAnywhere, Category = "Spawn")
	float WaveSpawningRate = 5.0f;
	
	int32 NextMinionSpawnSpotIndex = -1;

	FTimerHandle SpawnIntervalTimerHandle;
};
