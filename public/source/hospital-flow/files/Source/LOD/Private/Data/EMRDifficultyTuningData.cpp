#include "Data/EMRDifficultyTuningData.h"

FPrimaryAssetId UEMRDifficultyTuningData::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(GetDifficultyTuningAssetType(), GetFName());
}

FPrimaryAssetType UEMRDifficultyTuningData::GetDifficultyTuningAssetType()
{
    return FPrimaryAssetType(TEXT("DifficultyTuning"));
}

int32 UEMRDifficultyTuningData::GetMaxTreatmentBedUpgradeCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const
{
    if (!NightShiftDefinition)
    {
        return 0;
    }

    const FSoftObjectPath SelectedHospitalLevelPath = NightShiftDefinition->HospitalLevel.ToSoftObjectPath();
    if (!SelectedHospitalLevelPath.IsValid())
    {
        return 0;
    }

    for (const FEMRTreatmentBedUpgradeMapCapacity& Entry : TreatmentBedUpgradeMapCapacities)
    {
        if (Entry.HospitalLevel.ToSoftObjectPath() == SelectedHospitalLevelPath)
        {
            return FMath::Max(Entry.MaxTreatmentBedUpgradeCount, 0);
        }
    }

    return 0;
}

int32 UEMRDifficultyTuningData::GetMaxCoffeeMachineUpgradeCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const
{
    if (!NightShiftDefinition)
    {
        return 0;
    }

    const FSoftObjectPath SelectedHospitalLevelPath = NightShiftDefinition->HospitalLevel.ToSoftObjectPath();
    if (!SelectedHospitalLevelPath.IsValid())
    {
        return 0;
    }

    for (const FEMRCoffeeMachineUpgradeMapCapacity& Entry : CoffeeMachineUpgradeMapCapacities)
    {
        if (Entry.HospitalLevel.ToSoftObjectPath() == SelectedHospitalLevelPath)
        {
            return FMath::Max(Entry.MaxCoffeeMachineUpgradeCount, 0);
        }
    }

    return 0;
}

int32 UEMRDifficultyTuningData::GetMaxSnackMachineUpgradeCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const
{
    if (!NightShiftDefinition)
    {
        return 0;
    }

    const FSoftObjectPath SelectedHospitalLevelPath = NightShiftDefinition->HospitalLevel.ToSoftObjectPath();
    if (!SelectedHospitalLevelPath.IsValid())
    {
        return 0;
    }

    for (const FEMRSnackMachineUpgradeMapCapacity& Entry : SnackMachineUpgradeMapCapacities)
    {
        if (Entry.HospitalLevel.ToSoftObjectPath() == SelectedHospitalLevelPath)
        {
            return FMath::Max(Entry.MaxSnackMachineUpgradeCount, 0);
        }
    }

    return 0;
}

int32 UEMRDifficultyTuningData::GetMaxMagicBoxUpgradeCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const
{
    if (!NightShiftDefinition)
    {
        return 0;
    }

    const FSoftObjectPath SelectedHospitalLevelPath = NightShiftDefinition->HospitalLevel.ToSoftObjectPath();
    if (!SelectedHospitalLevelPath.IsValid())
    {
        return 0;
    }

    for (const FEMRMagicBoxUpgradeMapCapacity& Entry : MagicBoxUpgradeMapCapacities)
    {
        if (Entry.HospitalLevel.ToSoftObjectPath() == SelectedHospitalLevelPath)
        {
            return FMath::Max(Entry.MaxMagicBoxUpgradeCount, 0);
        }
    }

    return 0;
}

int32 UEMRDifficultyTuningData::GetMaxOxygenMachineCountForNightShift(const UEMRNightShiftDefinition* NightShiftDefinition) const
{
    if (!NightShiftDefinition)
    {
        return 1;
    }

    const FSoftObjectPath SelectedHospitalLevelPath = NightShiftDefinition->HospitalLevel.ToSoftObjectPath();
    if (!SelectedHospitalLevelPath.IsValid())
    {
        return 1;
    }

    for (const FEMROxygenMachineUpgradeMapCapacity& Entry : OxygenMachineUpgradeMapCapacities)
    {
        if (Entry.HospitalLevel.ToSoftObjectPath() == SelectedHospitalLevelPath)
        {
            return FMath::Max(Entry.MaxOxygenMachineCount, 1);
        }
    }

    return 1;
}
