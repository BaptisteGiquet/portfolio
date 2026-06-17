#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "AGASSItemSpawnerActor.generated.h"

class AAGASSPlaceableItemActor;
class UAGASSItemSpawnerPoolData;
class UAGASSRunStateComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class AGASSTOWER_API AAGASSItemSpawnerActor : public AActor
{
	GENERATED_BODY()

public:
	AAGASSItemSpawnerActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostLoad() override;

private:
	void RefreshRunStateBinding();
	void ClearRunStateBinding();
	void HandleRunStateChanged();
	void HandleSpawnTimerElapsed();
	void StartSpawnTimer();
	void StopSpawnTimer();
	bool TrySpawnOneItem();
	void PruneTrackedSpawnedItems();
	int32 GetTrackedSpawnedItemCount() const;
	UAGASSRunStateComponent* ResolveRunStateComponent() const;
	UStaticMesh* ResolveRandomSpawnMesh() const;
	UClass* ResolveSpawnActorClass() const;
	FTransform ResolveSpawnTransform() const;
	void DrawDebugSpawnArea() const;

	UFUNCTION()
	void HandleTrackedSpawnedItemDestroyed(AActor* DestroyedActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Spawner", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Spawner", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> SpawnerMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Spawner", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UAGASSItemSpawnerPoolData> SpawnPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Spawner", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float SpawnIntervalSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Spawner", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	int32 MaxActiveSpawnedItems = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Spawner", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	int32 SpawnedItemSellValue = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Spawner", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float SpawnAreaRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AGASS|Spawner|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDrawDebugSpawnAreaAtRuntime = false;

	UPROPERTY(EditAnywhere, Category = "AGASS|Spawner|Legacy", meta = (DeprecatedProperty, ClampMin = "0.0", AllowPrivateAccess = "true"))
	FVector SpawnAreaHalfExtent = FVector(100.f, 100.f, 0.f);

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AAGASSPlaceableItemActor>> TrackedSpawnedItems;

	UPROPERTY(Transient)
	bool bHasLoggedMissingSpawnPool = false;

	UPROPERTY(Transient)
	bool bHasLoggedEmptySpawnPool = false;

	UPROPERTY(Transient)
	bool bSpawnerEnabled = false;

	TWeakObjectPtr<UAGASSRunStateComponent> BoundRunStateComponent;
	FTimerHandle SpawnTimerHandle;
};
