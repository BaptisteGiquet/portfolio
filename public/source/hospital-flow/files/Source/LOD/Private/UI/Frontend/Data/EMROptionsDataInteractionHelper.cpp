


#include "UI/Frontend/Data/EMROptionsDataInteractionHelper.h"

#include "UI/Frontend/EMRGameUserSettings.h"


FEMROptionsDataInteractionHelper::FEMROptionsDataInteractionHelper(const FString& InSetterOrGetterFuncPath)
	: CachedDynamicFunctionPath(InSetterOrGetterFuncPath)
{
	CachedWeakGameUserSettings = UEMRGameUserSettings::Get();
}


FString FEMROptionsDataInteractionHelper::GetValueAsString() const
{
	FString OutStringValue;
	PropertyPathHelpers::GetPropertyValueAsString(CachedWeakGameUserSettings.Get(), CachedDynamicFunctionPath, OutStringValue);
	
	return OutStringValue;
}


void FEMROptionsDataInteractionHelper::SetValueFromString(const FString& InStringValue)
{
	PropertyPathHelpers::SetPropertyValueFromString(CachedWeakGameUserSettings.Get(), CachedDynamicFunctionPath, InStringValue);
}
