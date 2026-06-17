

#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"
#include "EMRCommonListEntryWidgetString.generated.h"

class UEMRListDataObjectString;
class UEMRFrontendCommonRotator;
class UEMRFrontendCommonButtonBase;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRCommonListEntryWidgetString : public UEMRFrontendCommonListEntryWidgetBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	
	virtual void OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject) override;
	virtual void OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason) override;
	virtual void OnToggleEditableState(bool bIsEditable) override;
	
private:
	void OnPreviousOptionButtonClicked();
	void OnNextOptionButtonClicked();
	
	void OnRotatorValueChanged(int32 Value, bool bUserInitiated);
	
	//***** Bound Widgets *****//
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_PreviousOption;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_NextOption;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonRotator> CommonRotator_AvailableOptions;
	//***** Bound Widgets *****//
	
	UPROPERTY(Transient)
	TObjectPtr<UEMRListDataObjectString> CachedOwningStringDataObject;
};
