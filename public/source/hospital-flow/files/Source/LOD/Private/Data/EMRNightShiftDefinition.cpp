#include "Data/EMRNightShiftDefinition.h"

FName UEMRNightShiftDefinition::GetEffectiveNightShiftId() const
{
	// If NightShiftId is set, use it. Otherwise, fall back to the asset name.
	if (!NightShiftId.IsNone())
	{
		return NightShiftId;
	}

	return GetPrimaryAssetId().PrimaryAssetName;
}
