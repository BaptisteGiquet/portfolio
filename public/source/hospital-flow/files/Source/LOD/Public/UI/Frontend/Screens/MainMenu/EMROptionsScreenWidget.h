

#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"
#include "EMROptionsScreenWidget.generated.h"

class UEMRListDataObjectBase;
class UEMROptionsDetailsViewWidget;
class UEMRFrontendCommonListView;
class UEMRFrontendCommonTabListWidgetBase;
class UEMROptionsDataRegistry;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMROptionsScreenWidget : public UEMRFrontendCommonActivatableWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	
private:
	UEMROptionsDataRegistry* GetOrCreateDataRegistry();
	
	void OnResetBoundActionTriggered();
	void OnBackBoundActionTriggered();

	UFUNCTION()
	void OnOptionsTabSelected(FName TabID);
	
	void OnListViewItemHoveredChanged(UObject* InHoveredItem, bool bWasHovered);
	void OnListViewItemSelectionChanged(UObject* InSelectedItem);
	
	FString TryGetEntryWidgetClassName(UObject* InOwningListItem) const;
	
	void OnListViewListDataModified(UEMRListDataObjectBase* ModifiedData, EOptionsListDataModifyReason ModifyReason);

	//***** Bound Widgets *****//
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonTabListWidgetBase> TabListWidget_OptionsTabs;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonListView> CommonListView_OptionsList;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMROptionsDetailsViewWidget> DetailsView_ListEntryInfo;
	//***** Bound Widgets *****//
	

	// Handle the creation of data in the Options screen. Direct access to this variable is forbidden.
	UPROPERTY(Transient)
	TObjectPtr<UEMROptionsDataRegistry> CreatedOwningDataRegistry;
	
	UPROPERTY(EditDefaultsOnly, Category = "EMR|Options Screen", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle ResetAction;

	FUIActionBindingHandle ResetActionHandle;
	FUIActionBindingHandle BackActionHandle;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UEMRListDataObjectBase>> ResettableDataArray;
	
	bool bIsResettingData = false;
};
