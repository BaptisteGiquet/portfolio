#include "Data/EMRSpecialEventDefinition.h"

FPrimaryAssetId UEMRSpecialEventDefinition::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(GetSpecialEventAssetType(), GetFName());
}

FPrimaryAssetType UEMRSpecialEventDefinition::GetSpecialEventAssetType()
{
    return FPrimaryAssetType(TEXT("SpecialEvent"));
}

bool UEMRSpecialEventDefinition::SupportsDifficulty(const EEMRNightShiftDifficultyTier DifficultyTier) const
{
    if (SupportedDifficulties.IsEmpty())
    {
        return true;
    }

    return SupportedDifficulties.Contains(DifficultyTier);
}
