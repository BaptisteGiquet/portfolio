#include "Data/AGASSItemSpawnerPoolData.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

bool UAGASSItemSpawnerPoolData::HasAnyResolvedSpawnMeshes() const
{
	return ResolvedStaticMeshes.ContainsByPredicate([](const TObjectPtr<UStaticMesh>& Mesh)
	{
		return Mesh != nullptr;
	});
}

UStaticMesh* UAGASSItemSpawnerPoolData::ResolveRandomSpawnMesh() const
{
	TArray<UStaticMesh*> ValidMeshes;
	ValidMeshes.Reserve(ResolvedStaticMeshes.Num());

	for (UStaticMesh* const Mesh : ResolvedStaticMeshes)
	{
		if (Mesh != nullptr)
		{
			ValidMeshes.Add(Mesh);
		}
	}

	if (ValidMeshes.IsEmpty())
	{
		return nullptr;
	}

	return ValidMeshes[FMath::RandHelper(ValidMeshes.Num())];
}

#if WITH_EDITOR
void UAGASSItemSpawnerPoolData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RefreshResolvedStaticMeshes();
}

void UAGASSItemSpawnerPoolData::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	RefreshResolvedStaticMeshes();

	Super::PreSave(ObjectSaveContext);
}

void UAGASSItemSpawnerPoolData::RefreshResolvedStaticMeshes()
{
	ResolvedStaticMeshes.Reset();

	if (StaticMeshFolder.Path.IsEmpty() || !FPackageName::IsValidLongPackageName(StaticMeshFolder.Path, true))
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.WaitForCompletion();

	TArray<FString> PathsToScan;
	PathsToScan.Add(StaticMeshFolder.Path);
	AssetRegistry.ScanPathsSynchronous(PathsToScan, true);

	FARFilter MeshFilter;
	MeshFilter.PackagePaths.Add(*StaticMeshFolder.Path);
	MeshFilter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	MeshFilter.bRecursivePaths = false;
	MeshFilter.bRecursiveClasses = true;

	TArray<FAssetData> MeshAssets;
	AssetRegistry.GetAssets(MeshFilter, MeshAssets);
	MeshAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.GetObjectPathString().Compare(Right.GetObjectPathString(), ESearchCase::IgnoreCase) < 0;
	});

	const FName ExpectedPackagePath(*StaticMeshFolder.Path);
	ResolvedStaticMeshes.Reserve(MeshAssets.Num());

	for (const FAssetData& MeshAsset : MeshAssets)
	{
		if (MeshAsset.PackagePath != ExpectedPackagePath)
		{
			continue;
		}

		if (UStaticMesh* const StaticMesh = Cast<UStaticMesh>(MeshAsset.GetAsset()))
		{
			ResolvedStaticMeshes.Add(StaticMesh);
		}
	}
}
#endif
