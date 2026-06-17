


#include "UI/Frontend/Data/EMRListDataObjectBase.h"

#include "UI/Frontend/EMRGameUserSettings.h"


void UEMRListDataObjectBase::InitDataObject()
{
	OnDataObjectInitialized();
}


void UEMRListDataObjectBase::AddEditCondition(const FOptionsDataEditConditionDescriptor& InEditCondition)
{
	EditConditionDescriptors.Add(InEditCondition);
}


void UEMRListDataObjectBase::AddEditDependencyData(UEMRListDataObjectBase* InDependencyData)
{
	if (!InDependencyData) return;
	if (InDependencyData->OnListDataModified.IsBoundToObject(this)) return;
	
	InDependencyData->OnListDataModified.AddUObject(this, &ThisClass::OnEditDependencyDataModified);
}


bool UEMRListDataObjectBase::IsDataCurrentlyEditable()
{
	bool bIsEditable = true;
	
	if (EditConditionDescriptors.IsEmpty())
	{
		return bIsEditable;
	}
	
	TArray<FText> DisabledReasons;
	FText CachedDisabledRichReason;
	
	for (const FOptionsDataEditConditionDescriptor& EditCondition : EditConditionDescriptors)
	{
		if (!EditCondition.IsValid() || EditCondition.IsEditConditionMet()) continue;
		
		bIsEditable = false;

		const FText DisabledReason = EditCondition.GetDisabledRichReason();
		if (!DisabledReason.IsEmpty())
		{
			DisabledReasons.Add(DisabledReason);
		}
		
		const FText JoinedDisabledReasons = FText::Join(FText::FromString(TEXT("\n\n")), DisabledReasons);
		SetDisabledRichText(JoinedDisabledReasons);

		if (EditCondition.HasForcedStringValue())
		{
			const FString ForcedStringValue = EditCondition.GetDisabledForcedStringValue();
			
			// If the current value this data object has can be set to the forced value
			if (CanSetToForcedStringValue(ForcedStringValue))
			{
				OnSetToForcedStringValue(ForcedStringValue);
			}
		}
	}
	
	return bIsEditable;
}


void UEMRListDataObjectBase::OnDataObjectInitialized()
{
	
}


void UEMRListDataObjectBase::NotifyListDataModified(UEMRListDataObjectBase* ModifiedListData, EOptionsListDataModifyReason ModifiedReason)
{
	OnListDataModified.Broadcast(ModifiedListData, ModifiedReason);
	
	if (bShouldApplySettingsImmediately && ModifiedReason != EOptionsListDataModifyReason::DependencyModified)
	{
		if (UEMRGameUserSettings* Settings = UEMRGameUserSettings::Get())
		{
			const FName ModifiedDataID = ModifiedListData ? ModifiedListData->GetDataID() : NAME_None;
			Settings->ApplyImmediateSettingByDataID(ModifiedDataID);
		}
	}
}


void UEMRListDataObjectBase::OnEditDependencyDataModified(UEMRListDataObjectBase* ModifiedDependencyData, EOptionsListDataModifyReason ModifiedReason)
{
	OnDependencyDataModified.Broadcast(ModifiedDependencyData, ModifiedReason);	
}

