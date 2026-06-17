// Copyright Lorenzo Deluca

#include "Characters/Patient/EMRPatientIdentityPoolData.h"

FPrimaryAssetId UEMRPatientIdentityPoolData::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(GetIdentityPoolAssetType(), GetFName());
}


FPrimaryAssetType UEMRPatientIdentityPoolData::GetIdentityPoolAssetType()
{
    return FPrimaryAssetType("PatientIdentityPool");
}


FText UEMRPatientIdentityPoolData::GetRandomName() const
{
    if (AvailableNames.IsEmpty())
        {
                return FText();
    }

    const int32 RandomIndex = FMath::RandRange(0, AvailableNames.Num() - 1);
    return AvailableNames[RandomIndex];
}


int32 UEMRPatientIdentityPoolData::GetRandomAge() const
{
    const int32 AgeMin = FMath::Min(MinAge, MaxAge);
    const int32 AgeMax = FMath::Max(MinAge, MaxAge);

    if (AgeMax <= 0)
        {
                return 0;
    }

    return FMath::RandRange(AgeMin, AgeMax);
}


FGameplayTag UEMRPatientIdentityPoolData::GetRandomBloodType() const
{
    TArray<FGameplayTag> BloodTypeArray;
    AvailableBloodTypes.GetGameplayTagArray(BloodTypeArray);

    if (BloodTypeArray.IsEmpty())
        {
                return FGameplayTag::EmptyTag;
    }

    const int32 RandomIndex = FMath::RandRange(0, BloodTypeArray.Num() - 1);
    return BloodTypeArray[RandomIndex];
}
