#include "Actors/AGASSItemSpawnerActor.h"

#include "Actors/AGASSPlaceableItemActor.h"
#include "Components/AGASSRunStateComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/AGASSItemSpawnerPoolData.h"
#include "Data/AGASSPlaceableItemRuntimeData.h"
#include "DrawDebugHelpers.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Settings/AGASSTowerDeveloperSettings.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSItemSpawner, Log, All);

namespace
{
	constexpr float LegacySpawnAreaRadiusDefault = 100.f;
	const FVector LegacySpawnAreaHalfExtentDefault(100.f, 100.f, 0.f);
}

AAGASSItemSpawnerActor::AAGASSItemSpawnerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SpawnerMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpawnerMesh"));
	SpawnerMeshComponent->SetupAttachment(SceneRoot);
	SpawnerMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SpawnerMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnerMeshComponent->SetGenerateOverlapEvents(false);
	SpawnerMeshComponent->SetCanEverAffectNavigation(false);
	SpawnerMeshComponent->SetSimulatePhysics(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		SpawnerMeshComponent->SetStaticMesh(CubeMesh.Object);
		SpawnerMeshComponent->SetRelativeScale3D(FVector(0.75f));
	}
}

void AAGASSItemSpawnerActor::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld() == nullptr)
	{
		return;
	}

	if (bDrawDebugSpawnAreaAtRuntime)
	{
		DrawDebugSpawnArea();
	}

	if (!HasAuthority())
	{
		return;
	}

	if (SpawnIntervalSeconds <= 0.f)
	{
		UE_LOG(LogAGASSItemSpawner, Warning, TEXT("Spawner '%s' will stay idle because SpawnIntervalSeconds is non-positive."), *GetNameSafe(this));
		return;
	}

	if (MaxActiveSpawnedItems <= 0)
	{
		UE_LOG(LogAGASSItemSpawner, Display, TEXT("Spawner '%s' is disabled because MaxActiveSpawnedItems is zero."), *GetNameSafe(this));
		return;
	}

	bSpawnerEnabled = true;
	RefreshRunStateBinding();
	HandleRunStateChanged();
}

void AAGASSItemSpawnerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRunStateBinding();
	StopSpawnTimer();
	TrackedSpawnedItems.Reset();

	Super::EndPlay(EndPlayReason);
}

void AAGASSItemSpawnerActor::PostLoad()
{
	Super::PostLoad();

	const bool bRadiusStillOnDefault = FMath::IsNearlyEqual(SpawnAreaRadius, LegacySpawnAreaRadiusDefault);
	const bool bLegacyExtentWasCustomized = !SpawnAreaHalfExtent.Equals(LegacySpawnAreaHalfExtentDefault, KINDA_SMALL_NUMBER);
	if (bRadiusStillOnDefault && bLegacyExtentWasCustomized)
	{
		SpawnAreaRadius = SpawnAreaHalfExtent.GetAbsMax();
	}
}

void AAGASSItemSpawnerActor::RefreshRunStateBinding()
{
	UAGASSRunStateComponent* const RunStateComponent = ResolveRunStateComponent();
	if (RunStateComponent == BoundRunStateComponent.Get())
	{
		return;
	}

	ClearRunStateBinding();
	if (RunStateComponent == nullptr)
	{
		UE_LOG(LogAGASSItemSpawner, Warning, TEXT("Spawner '%s' could not find a run state component and will stay idle."), *GetNameSafe(this));
		return;
	}

	BoundRunStateComponent = RunStateComponent;
	RunStateComponent->OnRunStateChanged().AddUObject(this, &ThisClass::HandleRunStateChanged);
}

void AAGASSItemSpawnerActor::ClearRunStateBinding()
{
	if (UAGASSRunStateComponent* const RunStateComponent = BoundRunStateComponent.Get())
	{
		RunStateComponent->OnRunStateChanged().RemoveAll(this);
	}

	BoundRunStateComponent.Reset();
}

void AAGASSItemSpawnerActor::HandleRunStateChanged()
{
	if (!bSpawnerEnabled)
	{
		return;
	}

	if (UAGASSRunStateComponent* const RunStateComponent = BoundRunStateComponent.Get())
	{
		if (RunStateComponent->IsRunActive())
		{
			StartSpawnTimer();
			return;
		}
	}

	StopSpawnTimer();
}

void AAGASSItemSpawnerActor::HandleSpawnTimerElapsed()
{
	PruneTrackedSpawnedItems();
	TrySpawnOneItem();
}

void AAGASSItemSpawnerActor::StartSpawnTimer()
{
	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(SpawnTimerHandle, this, &ThisClass::HandleSpawnTimerElapsed, SpawnIntervalSeconds, true);
}

void AAGASSItemSpawnerActor::StopSpawnTimer()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}
}

bool AAGASSItemSpawnerActor::TrySpawnOneItem()
{
	if (!HasAuthority() || GetWorld() == nullptr || GetTrackedSpawnedItemCount() >= MaxActiveSpawnedItems)
	{
		return false;
	}

	UStaticMesh* const SpawnMesh = ResolveRandomSpawnMesh();
	if (SpawnMesh == nullptr)
	{
		return false;
	}

	UClass* const SpawnClass = ResolveSpawnActorClass();
	if (SpawnClass == nullptr)
	{
		UE_LOG(LogAGASSItemSpawner, Warning, TEXT("Spawner '%s' could not resolve a valid spawn class."), *GetNameSafe(this));
		return false;
	}

	const FTransform SpawnTransform = ResolveSpawnTransform();
	AAGASSPlaceableItemActor* const SpawnedItem = GetWorld()->SpawnActorDeferred<AAGASSPlaceableItemActor>(
		SpawnClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (SpawnedItem == nullptr)
	{
		UE_LOG(LogAGASSItemSpawner, Warning, TEXT("Spawner '%s' failed to spawn one of its configured pool entries."), *GetNameSafe(this));
		return false;
	}

	FAGASSPlaceableItemRuntimeData RuntimeItemData;
	RuntimeItemData.bUseRuntimeData = true;
	RuntimeItemData.WorldStaticMesh = SpawnMesh;
	RuntimeItemData.SellValue = FMath::Max(SpawnedItemSellValue, 0);
	SpawnedItem->SetRuntimeItemData(RuntimeItemData);
	SpawnedItem->FinishSpawning(SpawnTransform);
	SpawnedItem->BeginSpawnFallCollisionSequence();
	SpawnedItem->OnDestroyed.AddDynamic(this, &ThisClass::HandleTrackedSpawnedItemDestroyed);
	TrackedSpawnedItems.Add(SpawnedItem);
	return true;
}

void AAGASSItemSpawnerActor::PruneTrackedSpawnedItems()
{
	TrackedSpawnedItems.RemoveAll([](const TWeakObjectPtr<AAGASSPlaceableItemActor>& ItemPtr)
	{
		return !ItemPtr.IsValid();
	});
}

int32 AAGASSItemSpawnerActor::GetTrackedSpawnedItemCount() const
{
	int32 ValidItemCount = 0;
	for (const TWeakObjectPtr<AAGASSPlaceableItemActor>& ItemPtr : TrackedSpawnedItems)
	{
		if (ItemPtr.IsValid())
		{
			++ValidItemCount;
		}
	}

	return ValidItemCount;
}

UAGASSRunStateComponent* AAGASSItemSpawnerActor::ResolveRunStateComponent() const
{
	const UWorld* const World = GetWorld();
	AGameStateBase* const GameState = World != nullptr ? World->GetGameState() : nullptr;
	return GameState != nullptr ? GameState->FindComponentByClass<UAGASSRunStateComponent>() : nullptr;
}

UStaticMesh* AAGASSItemSpawnerActor::ResolveRandomSpawnMesh() const
{
	UAGASSItemSpawnerPoolData* const SpawnPoolData = SpawnPool.LoadSynchronous();
	if (SpawnPoolData == nullptr)
	{
		if (!bHasLoggedMissingSpawnPool)
		{
			UE_LOG(LogAGASSItemSpawner, Warning, TEXT("Spawner '%s' has no valid spawn pool asset and will stay idle."), *GetNameSafe(this));
			const_cast<ThisClass*>(this)->bHasLoggedMissingSpawnPool = true;
		}

		return nullptr;
	}

	const_cast<ThisClass*>(this)->bHasLoggedMissingSpawnPool = false;

	UStaticMesh* const SpawnMesh = SpawnPoolData->ResolveRandomSpawnMesh();
	if (SpawnMesh == nullptr)
	{
		if (!bHasLoggedEmptySpawnPool)
		{
			UE_LOG(LogAGASSItemSpawner, Warning, TEXT("Spawner '%s' has a spawn pool with no valid direct-folder static meshes and will stay idle."), *GetNameSafe(this));
			const_cast<ThisClass*>(this)->bHasLoggedEmptySpawnPool = true;
		}

		return nullptr;
	}

	const_cast<ThisClass*>(this)->bHasLoggedEmptySpawnPool = false;

	return SpawnMesh;
}

UClass* AAGASSItemSpawnerActor::ResolveSpawnActorClass() const
{
	if (const UAGASSTowerDeveloperSettings* const TowerSettings = UAGASSTowerDeveloperSettings::Get())
	{
		if (UClass* const DefaultItemClass = TowerSettings->DefaultBootstrapItemClass.LoadSynchronous())
		{
			return DefaultItemClass;
		}
	}

	return AAGASSPlaceableItemActor::StaticClass();
}

FTransform AAGASSItemSpawnerActor::ResolveSpawnTransform() const
{
	const float RadiusScale = SpawnAreaRadius > 0.f ? FMath::Pow(FMath::FRand(), 1.f / 3.f) : 0.f;
	const FVector LocalOffset = FMath::VRand() * (SpawnAreaRadius * RadiusScale);

	const FTransform ActorTransform = GetActorTransform();
	const FRotator UprightSpawnRotation(0.f, ActorTransform.Rotator().Yaw, 0.f);
	return FTransform(
		UprightSpawnRotation.Quaternion(),
		ActorTransform.TransformPositionNoScale(LocalOffset),
		FVector::OneVector);
}

void AAGASSItemSpawnerActor::DrawDebugSpawnArea() const
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	DrawDebugSphere(World, GetActorLocation(), SpawnAreaRadius, 24, FColor::Green, true, -1.f, 0, 1.5f);
	DrawDebugSphere(World, GetActorLocation(), 12.f, 12, FColor::Emerald, true, -1.f, 0, 2.f);
}

void AAGASSItemSpawnerActor::HandleTrackedSpawnedItemDestroyed(AActor* DestroyedActor)
{
	TrackedSpawnedItems.RemoveAll([DestroyedActor](const TWeakObjectPtr<AAGASSPlaceableItemActor>& ItemPtr)
	{
		return !ItemPtr.IsValid() || ItemPtr.Get() == DestroyedActor;
	});
}
