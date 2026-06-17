
#include "Subsystems/EMRSubsystemConfig.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Data/EMRSpecialEventDefinition.h"
#include "Data/EMRToiletConfig.h"


FPrimaryAssetId UEMRSubsystemConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(GetSubsystemConfigAssetType(), GetFName());
}


FPrimaryAssetType UEMRSubsystemConfig::GetSubsystemConfigAssetType()
{
	return FPrimaryAssetType("SubsystemConfig");
}


const UEMRDifficultyTuningData* UEMRSubsystemConfig::GetDifficultyTuning() const
{
    PreloadRuntimeAssets();
    return CachedDifficultyTuningAsset;
}

const UEMRToiletConfig* UEMRSubsystemConfig::GetToiletConfig() const
{
    return ToiletConfig.LoadSynchronous();
}

void UEMRSubsystemConfig::PreloadRuntimeAssets() const
{
    if (bRuntimeAssetsPreloaded && CachedDifficultyTuningAsset)
    {
        return;
    }

    if (bRuntimeAssetsPreloaded && !CachedDifficultyTuningAsset)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[SubsystemConfig] Runtime assets were marked preloaded but DifficultyTuning is null. Retrying load for %s."),
            *GetName());
    }

    const FString DifficultyTuningPath = DifficultyTuning.ToSoftObjectPath().ToString();
    if (DifficultyTuningPath.IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[SubsystemConfig] DifficultyTuning soft reference is empty on %s."),
            *GetName());
    }

    CachedDifficultyTuningAsset = DifficultyTuning.LoadSynchronous();
    UE_LOG(
        LogTemp,
        Log,
        TEXT("[SubsystemConfig] DifficultyTuning load on %s path='%s' result=%s"),
        *GetName(),
        DifficultyTuningPath.IsEmpty() ? TEXT("<empty>") : *DifficultyTuningPath,
        *GetNameSafe(CachedDifficultyTuningAsset.Get()));

    CachedSpecialEventDefinitions.Reset();
    CachedSpecialEventDefinitions.Reserve(SpecialEventDefinitions.Num());
    for (const TSoftObjectPtr<UEMRSpecialEventDefinition>& DefinitionPtr : SpecialEventDefinitions)
    {
        if (UEMRSpecialEventDefinition* Definition = DefinitionPtr.LoadSynchronous())
        {
            CachedSpecialEventDefinitions.Add(Definition);
        }
        else if (!DefinitionPtr.IsNull())
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[SubsystemConfig] Failed to load SpecialEvent '%s' from %s"),
                *DefinitionPtr.ToSoftObjectPath().ToString(),
                *GetName());
        }
    }

    bRuntimeAssetsPreloaded = CachedDifficultyTuningAsset != nullptr || DifficultyTuning.IsNull();
}

void UEMRSubsystemConfig::GetSpecialEventDefinitions(TArray<const UEMRSpecialEventDefinition*>& OutSpecialEventDefinitions) const
{
    PreloadRuntimeAssets();

    for (const UEMRSpecialEventDefinition* Definition : CachedSpecialEventDefinitions)
    {
        if (Definition)
        {
            OutSpecialEventDefinitions.AddUnique(Definition);
        }
    }
}
