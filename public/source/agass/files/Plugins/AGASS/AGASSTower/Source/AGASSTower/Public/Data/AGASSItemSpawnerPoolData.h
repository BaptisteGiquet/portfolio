#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectSaveContext.h"
#include "AGASSItemSpawnerPoolData.generated.h"

class UStaticMesh;
struct FPropertyChangedEvent;

UCLASS(BlueprintType, Blueprintable)
class AGASSTOWER_API UAGASSItemSpawnerPoolData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	bool HasAnyResolvedSpawnMeshes() const;
	UStaticMesh* ResolveRandomSpawnMesh() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
#endif

private:
#if WITH_EDITOR
	void RefreshResolvedStaticMeshes();
#endif

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Spawner", meta = (AllowPrivateAccess = "true", ContentDir, LongPackageName, ForceShowPluginContent))
	FDirectoryPath StaticMeshFolder;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Spawner", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UStaticMesh>> ResolvedStaticMeshes;
};
