#include "Data/EMRToiletConfig.h"

FPrimaryAssetId UEMRToiletConfig::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(GetToiletConfigAssetType(), GetFName());
}

FPrimaryAssetType UEMRToiletConfig::GetToiletConfigAssetType()
{
    return FPrimaryAssetType(TEXT("ToiletConfig"));
}
