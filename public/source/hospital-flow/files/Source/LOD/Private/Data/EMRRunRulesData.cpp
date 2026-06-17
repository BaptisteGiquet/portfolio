#include "Data/EMRRunRulesData.h"

FPrimaryAssetId UEMRRunRulesData::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(GetRunRulesAssetType(), GetFName());
}

FPrimaryAssetType UEMRRunRulesData::GetRunRulesAssetType()
{
    return FPrimaryAssetType("RunRules");
}
