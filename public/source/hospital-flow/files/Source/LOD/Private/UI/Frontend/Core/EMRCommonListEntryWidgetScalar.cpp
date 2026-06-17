#include "UI/Frontend/Core/EMRCommonListEntryWidgetScalar.h"

#include "AnalogSlider.h"
#include "UI/Frontend/EMRGameUserSettings.h"
#include "UI/Frontend/Data/EMRListDataObjectScalar.h"


void UEMRCommonListEntryWidgetScalar::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	AnalogSlider_SettingSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::OnSliderValueChanged);
	AnalogSlider_SettingSlider->OnMouseCaptureBegin.AddUniqueDynamic(this, &ThisClass::OnSliderMouseCapturedBegin);
	AnalogSlider_SettingSlider->OnControllerCaptureBegin.AddUniqueDynamic(this, &ThisClass::OnSliderControllerCapturedBegin);
	AnalogSlider_SettingSlider->OnMouseCaptureEnd.AddUniqueDynamic(this, &ThisClass::OnSliderMouseCapturedEnd);
	AnalogSlider_SettingSlider->OnControllerCaptureEnd.AddUniqueDynamic(this, &ThisClass::OnSliderControllerCapturedEnd);
}


void UEMRCommonListEntryWidgetScalar::OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject)
{
	Super::OnOwningListDataObjectSet(InOwningListDataObject);
	
	CachedOwningScalarDataObject = CastChecked<UEMRListDataObjectScalar>(InOwningListDataObject);
	
	CommonNumericTextBlock_SettingValue->SetNumericType(CachedOwningScalarDataObject->GetDisplayNumericType());
	CommonNumericTextBlock_SettingValue->FormattingSpecification = CachedOwningScalarDataObject->GetNumberFormattingOptions();
	CommonNumericTextBlock_SettingValue->SetCurrentValue(CachedOwningScalarDataObject->GetCurrentValue());
	
	AnalogSlider_SettingSlider->SetMinValue(CachedOwningScalarDataObject->GetDisplayValueRange().GetLowerBoundValue());
	AnalogSlider_SettingSlider->SetMaxValue(CachedOwningScalarDataObject->GetDisplayValueRange().GetUpperBoundValue());
	AnalogSlider_SettingSlider->SetStepSize(CachedOwningScalarDataObject->GetSliderStepSize());
	bIsSyncingSliderFromDataObject = true;
	AnalogSlider_SettingSlider->SetValue(CachedOwningScalarDataObject->GetCurrentValue());
	bIsSyncingSliderFromDataObject = false;
}


void UEMRCommonListEntryWidgetScalar::OnSliderValueChanged(float Value)
{
	if (bIsSyncingSliderFromDataObject)
	{
		return;
	}

	if (CachedOwningScalarDataObject)
	{
		CachedOwningScalarDataObject->SetCurrentValueFromSlider(Value);
	}
}


void UEMRCommonListEntryWidgetScalar::OnSliderMouseCapturedBegin()
{
	bIsMouseCaptureActive = true;
	SelectThisEntryWidget();
}

void UEMRCommonListEntryWidgetScalar::OnSliderControllerCapturedBegin()
{
	bIsControllerCaptureActive = true;
}

void UEMRCommonListEntryWidgetScalar::OnSliderMouseCapturedEnd()
{
	bIsMouseCaptureActive = false;
}

void UEMRCommonListEntryWidgetScalar::OnSliderControllerCapturedEnd()
{
	bIsControllerCaptureActive = false;
	OnSliderMouseCapturedEnd();
}


void UEMRCommonListEntryWidgetScalar::OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason)
{
	Super::OnOwningListDataObjectModified(OwningModifiedListData, ModifyReason);
	
	if (CachedOwningScalarDataObject)
	{
		CommonNumericTextBlock_SettingValue->SetCurrentValue(CachedOwningScalarDataObject->GetCurrentValue());
		bIsSyncingSliderFromDataObject = true;
		AnalogSlider_SettingSlider->SetValue(CachedOwningScalarDataObject->GetCurrentValue());
		bIsSyncingSliderFromDataObject = false;
	}
}


