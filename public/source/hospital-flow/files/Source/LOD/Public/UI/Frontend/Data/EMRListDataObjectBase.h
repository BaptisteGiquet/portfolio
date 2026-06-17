

#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/EMRFrontendStructTypes.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"
#include "UObject/Object.h"
#include "EMRListDataObjectBase.generated.h"

#define LIST_DATA_ACCESSOR(DataType, PropertyName) \
	DataType Get##PropertyName() const { return PropertyName; } \
	void Set##PropertyName(DataType In##PropertyName) { PropertyName = In##PropertyName; }

/**
 * 
 */
UCLASS(Abstract)
class LOD_API UEMRListDataObjectBase : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnListDataModifiedDelegate, UEMRListDataObjectBase*, EOptionsListDataModifyReason);
	FOnListDataModifiedDelegate OnListDataModified;
	FOnListDataModifiedDelegate OnDependencyDataModified;
	
	LIST_DATA_ACCESSOR(FName, DataID)
	LIST_DATA_ACCESSOR(FText, DataDisplayName)
	LIST_DATA_ACCESSOR(FText, DescriptionRichText)
	LIST_DATA_ACCESSOR(FText, DisabledRichText)
	LIST_DATA_ACCESSOR(TSoftObjectPtr<UTexture2D>, SoftDescriptionImage)
	LIST_DATA_ACCESSOR(TObjectPtr<UEMRListDataObjectBase>, ParentData)

	// Empty in base class. Child class ListDataObjectCollection should override it.
	virtual TArray<UEMRListDataObjectBase*> GetAllChildListData() const { return TArray<UEMRListDataObjectBase*>(); }
	virtual bool HasAnyChildListData() const { return false; }

	void InitDataObject();
	
	void SetShouldApplySettingsImmediately(bool bShouldApplySettingsRightAway) { bShouldApplySettingsImmediately = bShouldApplySettingsRightAway; }
	
	// The child class should override them to provide implementation for resetting the data
	virtual bool HasDefaultValue() const { return false; }
	virtual bool CanResetBackToDefaultValue() const { return false; }
	virtual bool TryResetBackToDefaultValue() { return false; }
	
	// Gets called from OptionsDataRegistry for adding edit condition for the constructed list data object
	void AddEditCondition(const FOptionsDataEditConditionDescriptor& InEditCondition);
	
	// Get called from OptionsDataRegistry to add in DependencyData
	void AddEditDependencyData(UEMRListDataObjectBase* InDependencyData);
	
	bool IsDataCurrentlyEditable();
	
	

protected:
	// The child classes should override it to handle the initialization needed accordingly.
	virtual void OnDataObjectInitialized();
	
	virtual void NotifyListDataModified(UEMRListDataObjectBase* ModifiedListData, EOptionsListDataModifyReason ModifiedReason = EOptionsListDataModifyReason::DirectlyModified);
	
	// The child classes should override it to allow the value to be set to the forced string value
	virtual bool CanSetToForcedStringValue(const FString& InForcedValue) const { return false; }
	
	// The child classes should override it to specify how to set the current string value to the force value
	virtual void OnSetToForcedStringValue(const FString& InForcedValue) {}
	
	// Called when the value of the dependency data has changed. The child class can override the function to handle the custom logic needed. Super call is expected.
	virtual void OnEditDependencyDataModified(UEMRListDataObjectBase* ModifiedDependencyData, EOptionsListDataModifyReason ModifiedReason = EOptionsListDataModifyReason::DirectlyModified);
	
private:
	FName DataID;
	FText DataDisplayName;
	FText DescriptionRichText;
	FText DisabledRichText;
	TSoftObjectPtr<UTexture2D> SoftDescriptionImage;

	UPROPERTY(Transient)
	TObjectPtr<UEMRListDataObjectBase> ParentData;
	
	bool bShouldApplySettingsImmediately = false;
	
	UPROPERTY(Transient)
	TArray<FOptionsDataEditConditionDescriptor> EditConditionDescriptors;
};
