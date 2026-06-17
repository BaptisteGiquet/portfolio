
#include "AI/MobaMinionBarrack.h"

#include "AI/MobaMinion.h"
#include "GameFramework/PlayerStart.h"

AMobaMinionBarrack::AMobaMinionBarrack()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMobaMinionBarrack::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(SpawnIntervalTimerHandle, this,	&AMobaMinionBarrack::SpawnNewMinionWave, WaveSpawningRate, true);
	}
}

void AMobaMinionBarrack::SpawnNewMinions(const int32 Amount)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(MinionClass))
	{
		return;
	}

	for (int32 MinionSpawned = 0; MinionSpawned < Amount; ++MinionSpawned)
	{
		const APlayerStart* NextSpawnSpot = GetNextMinionSpawnSpot();
		const FTransform MinionSpawnSpotTransform = NextSpawnSpot ? NextSpawnSpot->GetTransform() : GetActorTransform();
		
		
		AMobaMinion* NewMinion = World->SpawnActorDeferred<AMobaMinion>(MinionClass, MinionSpawnSpotTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!IsValid(NewMinion))
		{
			continue;
		}
		
		NewMinion->SetGenericTeamId(BarrackTeamID);
		NewMinion->FinishSpawning(MinionSpawnSpotTransform);
		NewMinion->SetGoal(Goal);
		
		MinionPool.Add(NewMinion);
	}
}

const APlayerStart* AMobaMinionBarrack::GetNextMinionSpawnSpot()
{
	if (MinionSpawnSpots.IsEmpty())
	{
		return nullptr;
	}

	++NextMinionSpawnSpotIndex;

	if (NextMinionSpawnSpotIndex >= MinionSpawnSpots.Num())
	{
		NextMinionSpawnSpotIndex = 0;
	}

	return MinionSpawnSpots[NextMinionSpawnSpotIndex];
}




void AMobaMinionBarrack::SpawnNewMinionWave()
{
	for (int32 MinionSpawned = 0; MinionSpawned < MinionPerWave; ++MinionSpawned)
	{
		AMobaMinion* AvailableMinion = GetNextAvailableMinion();
		if (!IsValid(AvailableMinion))
		{
			const int32 MinionRemaining = MinionPerWave - MinionSpawned;
			SpawnNewMinions(MinionRemaining);
			break;
		}

		const APlayerStart* NextSpawnSpot = GetNextMinionSpawnSpot();
		const FTransform MinionSpawnSpotTransform = NextSpawnSpot ? NextSpawnSpot->GetTransform() : GetActorTransform();

		
		AvailableMinion->SetActorTransform(MinionSpawnSpotTransform);
		AvailableMinion->Activate();
	}
}

AMobaMinion* AMobaMinionBarrack::GetNextAvailableMinion() const
{
	for (AMobaMinion* Minion : MinionPool)
	{
		if (Minion->IsActive() || !Minion->IsReadyToRespawn())
		{
			continue;
		}

		return Minion;
	}

	// No minion available in the pool
	return nullptr;
}



