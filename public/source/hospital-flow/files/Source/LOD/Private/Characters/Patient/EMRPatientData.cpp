#include "Characters/Patient/EMRPatientData.h"

#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

FPrimaryAssetId UEMRPatientData::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(GetPatientAssetType(), GetFName());
}


FPrimaryAssetType UEMRPatientData::GetPatientAssetType()
{
    return FPrimaryAssetType("Patient");
}


FText UEMRPatientData::GetRandomName() const
{
    if (!IdentityPool)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientData] IdentityPool not set for %s"), *GetName());
        return FText();
    }

    return IdentityPool->GetRandomName();
}


int32 UEMRPatientData::GetRandomAge() const
{
    if (!IdentityPool)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientData] IdentityPool not set for %s"), *GetName());
        return 0;
    }

    return IdentityPool->GetRandomAge();
}


FGameplayTag UEMRPatientData::GetRandomBloodType() const
{
    if (!IdentityPool)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientData] IdentityPool not set for %s"), *GetName());
        return FGameplayTag::EmptyTag;
    }

    return IdentityPool->GetRandomBloodType();
}



TArray<FEMRAttributeValue> UEMRPatientData::GetDefaultVitalStats() const
{
    static const FString Context(TEXT("PatientAttributesTable"));
    TArray<FEMRAttributeValue> Result;

    TArray<FPatientAttributeRow*> Rows;
    PatientAttributesTable->GetAllRows(Context, Rows);
    Result.Reserve(Rows.Num());

    for (const FPatientAttributeRow* Row : Rows)
    {
        if (!Row)
        {
            continue;
        }

        if (!Row->Attribute.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[EMRPatientData] Invalid attribute on row Id '%s' in table '%s' for patient data asset '%s'"),
                    *Row->Id.ToString(), *PatientAttributesTable->GetName(), *GetName());
            continue;
        }

        FEMRAttributeValue Value;
        Value.Attribute = Row->Attribute;
        Value.Value = Row->Value;

        Result.Add(Value);
    }
	

    return Result;
}


