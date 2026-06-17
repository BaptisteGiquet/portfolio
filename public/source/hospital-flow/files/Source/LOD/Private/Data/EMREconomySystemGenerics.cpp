#include "Data/EMREconomySystemGenerics.h"

FPrimaryAssetId UEMREconomySystemGenerics::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(GetEconomySystemGenericsAssetType(), GetFName());
}


FPrimaryAssetType UEMREconomySystemGenerics::GetEconomySystemGenericsAssetType()
{
	return FPrimaryAssetType("EconomySystemGenerics");
}
