#include "UI/Frontend/Data/EMRListDataObjectScalar.h"


FCommonNumberFormattingOptions UEMRListDataObjectScalar::NoDecimal()
{
	FCommonNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;
	
	return Options;
}


FCommonNumberFormattingOptions UEMRListDataObjectScalar::WithDecimal(int32 NumberOfFracDigit)
{
	FCommonNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = NumberOfFracDigit;
	
	return Options;
}


float UEMRListDataObjectScalar::GetCurrentValue() const
{
	if (DataDynamicGetter)
	{
		return FMath::GetMappedRangeValueClamped(
			OutputValueRange,
			DisplayValueRange,
			StringToFloat(DataDynamicGetter->GetValueAsString()));
	}
	
	return 0.f;
}


void UEMRListDataObjectScalar::SetCurrentValueFromSlider(float InNewValue)
{
	if (DataDynamicSetter)
	{
		const float ClampedValue = FMath::GetMappedRangeValueClamped(
			DisplayValueRange,
			OutputValueRange,
			InNewValue);
		
		DataDynamicSetter->SetValueFromString(LexToString(ClampedValue));
		NotifyListDataModified(this);
	}
}


bool UEMRListDataObjectScalar::CanResetBackToDefaultValue() const
{
	if (HasDefaultValue() && DataDynamicGetter)
	{
		const float DefaultValue = StringToFloat(GetDefaultValueAsString());
		const float CurrentValue = StringToFloat(DataDynamicGetter->GetValueAsString());
		
		return !FMath::IsNearlyEqual(DefaultValue, CurrentValue, 0.01f);
	}
	
	return false;
}


bool UEMRListDataObjectScalar::TryResetBackToDefaultValue()
{
	if (CanResetBackToDefaultValue() && DataDynamicSetter)
	{
		DataDynamicSetter->SetValueFromString(GetDefaultValueAsString());
		
		NotifyListDataModified(this, EOptionsListDataModifyReason::ResetToDefault);
		return true;
	}
	
	return false;
}


void UEMRListDataObjectScalar::OnEditDependencyDataModified(UEMRListDataObjectBase* ModifiedDependencyData, EOptionsListDataModifyReason ModifiedReason)
{
	NotifyListDataModified(this, EOptionsListDataModifyReason::DependencyModified);
	
	Super::OnEditDependencyDataModified(ModifiedDependencyData, ModifiedReason);
}


float UEMRListDataObjectScalar::StringToFloat(const FString& String) const
{
	float OutConvertedValue = 0.f;
	LexFromString(OutConvertedValue, *String);
	
	return OutConvertedValue;
}
