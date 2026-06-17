#include "UI/Frontend/Data/EMRListDataObjectCollection.h"

#include "UI/Frontend/Data/EMRListDataObjectBase.h"


TArray<UEMRListDataObjectBase*> UEMRListDataObjectCollection::GetAllChildListData() const
{
	return ChildListDataArray;
}


bool UEMRListDataObjectCollection::HasAnyChildListData() const
{
	return !ChildListDataArray.IsEmpty();
}


void UEMRListDataObjectCollection::AddChildListData(UEMRListDataObjectBase* InChildListData)
{
	InChildListData->InitDataObject();
	InChildListData->SetParentData(this);
	
	ChildListDataArray.Add(InChildListData);
}
