#pragma once

#include "CoreMinimal.h"
#include "EMRListDataObjectValue.h"
#include "CommonNumericTextBlock.h"
#include "EMRListDataObjectScalar.generated.h"

/**
 * 
 */
UCLASS()
class LOD_API UEMRListDataObjectScalar : public UEMRListDataObjectValue
{
	GENERATED_BODY()
	
public:
	LIST_DATA_ACCESSOR(TRange<float>, DisplayValueRange)
	LIST_DATA_ACCESSOR(TRange<float>, OutputValueRange)
	LIST_DATA_ACCESSOR(float, SliderStepSize)
	LIST_DATA_ACCESSOR(ECommonNumericType, DisplayNumericType)
	LIST_DATA_ACCESSOR(FCommonNumberFormattingOptions, NumberFormattingOptions)
	
	static FCommonNumberFormattingOptions NoDecimal();
	static FCommonNumberFormattingOptions WithDecimal(int32 NumberOfFracDigit);
	
	float GetCurrentValue() const;
	
	void SetCurrentValueFromSlider(float InNewValue);
	
protected:
	virtual bool CanResetBackToDefaultValue() const override;
	virtual bool TryResetBackToDefaultValue() override;
	virtual void OnEditDependencyDataModified(UEMRListDataObjectBase* ModifiedDependencyData, EOptionsListDataModifyReason ModifiedReason = EOptionsListDataModifyReason::DirectlyModified) override;
	
private:
	float StringToFloat(const FString& String) const;
	
	TRange<float> DisplayValueRange = TRange<float>(0.f, 1.f);
	TRange<float> OutputValueRange = TRange<float>(0.f, 1.f);
	
	float SliderStepSize = 0.1f;
	ECommonNumericType DisplayNumericType = ECommonNumericType::Number;
	FCommonNumberFormattingOptions NumberFormattingOptions;
};
