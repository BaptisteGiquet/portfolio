#include "UI/Frontend/Data/EMRListDataObjectString.h"

#include "LocalizationLibrary.h"
#include "UI/Frontend/Utils/DebugHelper.h"



void UEMRListDataObjectString::OnDataObjectInitialized()
{
	RefreshDynamicOptionsFromCallback();

	if (!AvailableOptionsStringArray.IsEmpty())
	{
		CurrentStringValue = AvailableOptionsStringArray[0];
	}

	if (HasDefaultValue())
	{
		CurrentStringValue = GetDefaultValueAsString();
	}

	if (DataDynamicGetter.IsValid())
	{
		if (!DataDynamicGetter->GetValueAsString().IsEmpty())
		{
			CurrentStringValue = DataDynamicGetter->GetValueAsString();
		}
	}

	if (!TrySetDisplayTextFromStringValue(CurrentStringValue))
	{
		CurrentDisplayText = FText::FromString(TEXT("Invalid option"));
	}
}


void UEMRListDataObjectString::AddDynamicOption(const FString& InStringValue, const FText& InDisplayText)
{
	AvailableOptionsStringArray.Add(InStringValue);
	AvailableOptionsTextArray.Add(InDisplayText);
}

void UEMRListDataObjectString::ResetDynamicOptions()
{
	AvailableOptionsStringArray.Reset();
	AvailableOptionsTextArray.Reset();
}

void UEMRListDataObjectString::SetDynamicOptionsRefreshFunction(TFunction<void(UEMRListDataObjectString&)> InRefreshFunction)
{
	DynamicOptionsRefreshFunction = MoveTemp(InRefreshFunction);
}

void UEMRListDataObjectString::RefreshDynamicOptionsFromCallback()
{
	if (DynamicOptionsRefreshFunction)
	{
		ResetDynamicOptions();
		DynamicOptionsRefreshFunction(*this);
	}
}


void UEMRListDataObjectString::AdvanceToNextOption()
{
	if (AvailableOptionsStringArray.IsEmpty() || AvailableOptionsTextArray.IsEmpty()) return;
	
	const int32 CurrentDisplayIndex = AvailableOptionsStringArray.IndexOfByKey(CurrentStringValue);
	const int32 NextIndexToDisplay = CurrentDisplayIndex + 1;
	
	const bool bIsNextIndexValid = AvailableOptionsStringArray.IsValidIndex(NextIndexToDisplay);
	if (bIsNextIndexValid)
	{
		CurrentStringValue = AvailableOptionsStringArray[NextIndexToDisplay];
	}
	else
	{
		CurrentStringValue = AvailableOptionsStringArray[0];
	}
	
	TrySetDisplayTextFromStringValue(CurrentStringValue);
	
	// Notify EMRGameUserSettings of the settings value change 
	if (DataDynamicSetter.IsValid())
	{
		DataDynamicSetter->SetValueFromString(CurrentStringValue);
		Debug::Print(TEXT("DataDynamicSetter is used. The latest value from getter: ") + DataDynamicGetter->GetValueAsString());
		
		NotifyListDataModified(this);
	}
}


void UEMRListDataObjectString::BackToPreviousOption()
{
	if (AvailableOptionsStringArray.IsEmpty() || AvailableOptionsTextArray.IsEmpty()) return;
	
	const int32 CurrentDisplayIndex = AvailableOptionsStringArray.IndexOfByKey(CurrentStringValue);
	const int32 PreviousIndexToDisplay = CurrentDisplayIndex - 1;
	
	const bool bIsPreviousIndexValid = AvailableOptionsStringArray.IsValidIndex(PreviousIndexToDisplay);
	if (bIsPreviousIndexValid)
	{
		CurrentStringValue = AvailableOptionsStringArray[PreviousIndexToDisplay];
	}
	else
	{
		CurrentStringValue = AvailableOptionsStringArray.Last();
	}
	
	TrySetDisplayTextFromStringValue(CurrentStringValue);
	
	// Notify EMRGameUserSettings of the settings value change 
	if (DataDynamicSetter.IsValid())
	{
		DataDynamicSetter->SetValueFromString(CurrentStringValue);
		
		Debug::Print(TEXT("DataDynamicSetter is used. The latest value from getter: ") + DataDynamicGetter->GetValueAsString());
		
		NotifyListDataModified(this);
	}
}


void UEMRListDataObjectString::OnRotatorInitiatedValueChanged(const FText& InNewSelectedText)
{
	const int32 FoundIndex = AvailableOptionsTextArray.IndexOfByPredicate([InNewSelectedText](const FText& AvailableText)->bool
	{
		return AvailableText.EqualTo(InNewSelectedText);
	});
	
	if (FoundIndex != INDEX_NONE && AvailableOptionsStringArray.IsValidIndex(FoundIndex))
	{
		CurrentDisplayText = InNewSelectedText;
		CurrentStringValue = AvailableOptionsStringArray[FoundIndex];
		
		if (DataDynamicSetter.IsValid())
		{
			DataDynamicSetter->SetValueFromString(CurrentStringValue);
			
			NotifyListDataModified(this);
		}
	}
}

void UEMRListDataObjectString::OnEditDependencyDataModified(UEMRListDataObjectBase* ModifiedDependencyData, EOptionsListDataModifyReason ModifiedReason)
{
	if (DynamicOptionsRefreshFunction)
	{
		RefreshDynamicOptionsFromCallback();

		FString DesiredStringValue = CurrentStringValue;
		if (DataDynamicGetter.IsValid())
		{
			const FString GetterValue = DataDynamicGetter->GetValueAsString();
			if (!GetterValue.IsEmpty())
			{
				DesiredStringValue = GetterValue;
			}
		}

		CurrentStringValue = DesiredStringValue;

		if (!TrySetDisplayTextFromStringValue(CurrentStringValue))
		{
			FString FallbackStringValue;
			if (HasDefaultValue() && AvailableOptionsStringArray.Contains(GetDefaultValueAsString()))
			{
				FallbackStringValue = GetDefaultValueAsString();
			}
			else if (!AvailableOptionsStringArray.IsEmpty())
			{
				FallbackStringValue = AvailableOptionsStringArray[0];
			}

			if (!FallbackStringValue.IsEmpty())
			{
				CurrentStringValue = FallbackStringValue;
				TrySetDisplayTextFromStringValue(CurrentStringValue);

				if (DataDynamicSetter.IsValid())
				{
					DataDynamicSetter->SetValueFromString(CurrentStringValue);
				}
			}
			else
			{
				CurrentDisplayText = FText::FromString(TEXT("Invalid option"));
			}
		}

		NotifyListDataModified(this, EOptionsListDataModifyReason::DependencyModified);
	}

	Super::OnEditDependencyDataModified(ModifiedDependencyData, ModifiedReason);
}


bool UEMRListDataObjectString::TrySetDisplayTextFromStringValue(const FString& InStringValue)
{
	const int32 CurrentFoundIndex = AvailableOptionsStringArray.IndexOfByKey(InStringValue);
	
	if (AvailableOptionsTextArray.IsValidIndex(CurrentFoundIndex))
	{
		CurrentDisplayText = AvailableOptionsTextArray[CurrentFoundIndex];
		return true;
	}
	
	return false;
}


bool UEMRListDataObjectString::CanResetBackToDefaultValue() const
{
	return HasDefaultValue() && CurrentStringValue != GetDefaultValueAsString();
}


bool UEMRListDataObjectString::TryResetBackToDefaultValue()
{
	if (CanResetBackToDefaultValue())
	{
		CurrentStringValue = GetDefaultValueAsString();
		
		TrySetDisplayTextFromStringValue(CurrentStringValue);
		
		if (DataDynamicSetter.IsValid())
		{
			DataDynamicSetter->SetValueFromString(CurrentStringValue);
			
			NotifyListDataModified(this, EOptionsListDataModifyReason::ResetToDefault);
			
			return true;
		}
		
		return false;
	}
	
	return false;
}


bool UEMRListDataObjectString::CanSetToForcedStringValue(const FString& InForcedValue) const
{
	return CurrentStringValue != InForcedValue;
}


void UEMRListDataObjectString::OnSetToForcedStringValue(const FString& InForcedValue)
{
	CurrentStringValue = InForcedValue;
	TrySetDisplayTextFromStringValue(CurrentStringValue);
	
	if (DataDynamicSetter)
	{
		DataDynamicSetter->SetValueFromString(CurrentStringValue);
		
		NotifyListDataModified(this, EOptionsListDataModifyReason::DependencyModified);
	}
}


//************** UEMRListDataObjectStringBool *****************//
void UEMRListDataObjectStringBool::OverrideTrueDisplayText(const FText& InNewTrueDisplayText)
{
	if (!AvailableOptionsStringArray.Contains(TrueString))
	{
		AddDynamicOption(TrueString, InNewTrueDisplayText);
	}
}


void UEMRListDataObjectStringBool::OverrideFalseDisplayText(const FText& InNewFalseDisplayText)
{
	if (!AvailableOptionsStringArray.Contains(FalseString))
	{
		AddDynamicOption(FalseString, InNewFalseDisplayText);
	}
}


void UEMRListDataObjectStringBool::SetTrueAsDefaultValue()
{
	SetDefaultValueFromString(TrueString);
}


void UEMRListDataObjectStringBool::SetFalseAsDefaultValue()
{
	SetDefaultValueFromString(FalseString);
}


void UEMRListDataObjectStringBool::OnDataObjectInitialized()
{
	TryInitBoolValues();
	
	Super::OnDataObjectInitialized();
}

void UEMRListDataObjectStringBool::TryInitBoolValues()
{
	if (!AvailableOptionsStringArray.Contains(TrueString))
	{
		AddDynamicOption(TrueString, FText::FromString(TEXT("ON")));
	}

	if (!AvailableOptionsStringArray.Contains(FalseString))
	{
		AddDynamicOption(FalseString, FText::FromString(TEXT("OFF")));
	}
}
//************** UEMRListDataObjectStringBool *****************//





//************** UEMRListDataObjectStringInteger *****************//
void UEMRListDataObjectStringInteger::AddIntegerOption(int32 InIntegerValue, const FText& InDisplayText)
{
	AddDynamicOption(LexToString(InIntegerValue), InDisplayText);
}


void UEMRListDataObjectStringInteger::OnDataObjectInitialized()
{
	Super::OnDataObjectInitialized();

	if (!TrySetDisplayTextFromStringValue(CurrentStringValue))
	{
		// Reach here when the string value is -1, means that the quality setting is "Custom"
		CurrentDisplayText = GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Custom");
	}
}


void UEMRListDataObjectStringInteger::OnEditDependencyDataModified(UEMRListDataObjectBase* ModifiedDependencyData, EOptionsListDataModifyReason ModifiedReason)
{
	if (DataDynamicGetter)
	{
		if (CurrentStringValue == DataDynamicGetter->GetValueAsString()) return;
		
		CurrentStringValue = DataDynamicGetter->GetValueAsString();
		
		if (!TrySetDisplayTextFromStringValue(CurrentStringValue))
		{
			// Reach here when the string value is -1, means that the quality setting is "Custom"
			CurrentDisplayText = GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Custom");
		}
		
		NotifyListDataModified(this, EOptionsListDataModifyReason::DependencyModified);
	}
	
	Super::OnEditDependencyDataModified(ModifiedDependencyData, ModifiedReason);
}
//************** UEMRListDataObjectStringInteger *****************//





