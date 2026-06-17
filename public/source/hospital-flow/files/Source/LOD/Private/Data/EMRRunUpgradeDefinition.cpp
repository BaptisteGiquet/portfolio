#include "Data/EMRRunUpgradeDefinition.h"

FPrimaryAssetId UEMRRunUpgradeDefinition::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(GetRunUpgradeAssetType(), GetFName());
}

FPrimaryAssetType UEMRRunUpgradeDefinition::GetRunUpgradeAssetType()
{
    return FPrimaryAssetType(TEXT("RunUpgrade"));
}

