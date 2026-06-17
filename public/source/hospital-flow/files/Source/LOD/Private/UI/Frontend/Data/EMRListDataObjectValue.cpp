 

#include "UI/Frontend/Data/EMRListDataObjectValue.h"


void UEMRListDataObjectValue::SetDataDynamicGetter(const TSharedPtr<FEMROptionsDataInteractionHelper>& InDynamicGetter)
{
	DataDynamicGetter = InDynamicGetter;	
}


void UEMRListDataObjectValue::SetDataDynamicSetter(const TSharedPtr<FEMROptionsDataInteractionHelper>& InDynamicSetter)
{
	DataDynamicSetter = InDynamicSetter;
}
