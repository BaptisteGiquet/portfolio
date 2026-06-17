#pragma once

#include "CoreMinimal.h"
#include "EMRFrontendCommonListEntryWidgetBase.h"
#include "EMRCommonListEntryWidgetKeyRemap.generated.h"

class UEMRListDataObjectKeyRemap;
class UEMRFrontendCommonButtonBase;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRCommonListEntryWidgetKeyRemap : public UEMRFrontendCommonListEntryWidgetBase
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	
	virtual void OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject) override;
	virtual void OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason) override;
	
	
private:
	void OnRemapKeyButtonClicked();
	void OnResetKeyBindingButtonClicked();
	
	void OnKeyToRemapPressed(const FKey& PressedKey);
	void OnKeyRemapCanceled(const FText& CanceledReason);
	
	//*****  Bound Widgets *****//
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_RemapKey;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = true))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_ResetKeyBinding;
	//*****  Bound Widgets *****//
	
	UPROPERTY(Transient)
	TObjectPtr<UEMRListDataObjectKeyRemap> CachedOwningKeyRemapDataObject;
};
