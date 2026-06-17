#include "UI/Frontend/Data/EMRDataListEntryMapping.h"

#include "UI/Frontend/Data/EMRListDataObjectBase.h"


TSubclassOf<UEMRFrontendCommonListEntryWidgetBase> UEMRDataListEntryMapping::FindEntryWidgetClassByDataObject(UEMRListDataObjectBase* InDataObject) const
{
	check(InDataObject);
	
	for (UClass* DataObjectClass = InDataObject->GetClass(); DataObjectClass != nullptr; DataObjectClass = DataObjectClass->GetSuperClass())
	{
		TSubclassOf<UEMRListDataObjectBase> ConvertedDataObjectClass = TSubclassOf<UEMRListDataObjectBase>(DataObjectClass);
		if (DataObjectListEntryMap.Contains(ConvertedDataObjectClass))
		{
			return DataObjectListEntryMap.FindRef(ConvertedDataObjectClass);
		}
	}
	
	return nullptr;
}
