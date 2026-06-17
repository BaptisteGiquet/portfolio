#pragma once

#include "CoreMinimal.h"
#include "EMRFrontendCommonListEntryWidgetBase.h"
#include "EMRCommonListEntryWidgetScalar.generated.h"

class UEMRListDataObjectScalar;
class UCommonNumericTextBlock;
class UAnalogSlider;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRCommonListEntryWidgetScalar : public UEMRFrontendCommonListEntryWidgetBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	
	virtual void OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason) override;
	virtual void OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject) override;

	
private:
	UFUNCTION()
	void OnSliderValueChanged(float Value);
	
	UFUNCTION()
	void OnSliderMouseCapturedBegin();

	UFUNCTION()
	void OnSliderControllerCapturedBegin();

	UFUNCTION()
	void OnSliderMouseCapturedEnd();

	UFUNCTION()
	void OnSliderControllerCapturedEnd();
	
	//***** Bound Widgets *****//
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UCommonNumericTextBlock> CommonNumericTextBlock_SettingValue;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UAnalogSlider> AnalogSlider_SettingSlider;
	//***** Bound Widgets *****//
	
	UPROPERTY(Transient)
	TObjectPtr<UEMRListDataObjectScalar> CachedOwningScalarDataObject;

	bool bIsSyncingSliderFromDataObject = false;
	bool bIsMouseCaptureActive = false;
	bool bIsControllerCaptureActive = false;
};
