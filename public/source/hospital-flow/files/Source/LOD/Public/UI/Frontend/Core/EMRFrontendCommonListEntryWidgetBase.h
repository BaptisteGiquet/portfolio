

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"
#include "EMRFrontendCommonListEntryWidgetBase.generated.h"

class UEMRListDataObjectBase;
class UCommonTextBlock;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendCommonListEntryWidgetBase : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On List Entry Widget Hovered"))
	void BP_OnListEntryWidgetHovered(bool bWasHovered, bool bIsEntryWidgetStillSelected);
	void NativeOnListEntryWidgetHovered(bool bWasHovered);

	
protected:
	// The child widget blueprint should override this function for the gamepad interaction to function properly
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName="Get Widget To Focus For Gamepad"))
	UWidget* BP_GetWidgetToFocusForGamepad() const;
	
	// The child widget blueprint should override it to handle the highlight state when this entry widget is hovered or selected
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName="On Toggle Entry Widget Highlight State"))
	void BP_OnToggleEntryWidgetHighlightState(bool ShouldHighlight) const;
	
	virtual void NativeOnEntryReleased() override;
	
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

	// The child class should override this function to handle the initialization needed. Super call is expected.
	virtual void OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject);
	
	// The child class should override this function to update the UI Values after the list data object has been modified. Super call is not needed.
	virtual void OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason);
	
	virtual void OnOwningDependencyDataObjectModified(UEMRListDataObjectBase* OwningModifiedDependencyData, EOptionsListDataModifyReason ModifyReason);
	
	// The child class should override this to change the editable state of the widget it owns. The super call is expected.
	virtual void OnToggleEditableState(bool bIsEditable);
	
	void SelectThisEntryWidget();
private:
	//***** Bound Widgets *****//
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_SettingDisplayName;
	//***** Bound Widgets *****//
	
	
	UPROPERTY(Transient)
	TObjectPtr<UEMRListDataObjectBase> CachedOwningDataObject;
};
