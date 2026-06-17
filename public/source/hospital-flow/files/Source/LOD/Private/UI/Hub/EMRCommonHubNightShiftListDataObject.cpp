#include "UI/Hub/EMRCommonHubNightShiftListDataObject.h"

#include "Data/EMRNightShiftDefinition.h"

void UEMRCommonHubNightShiftListDataObject::Init(UEMRNightShiftDefinition* InDefinition, bool bInUnlocked, int32 InOfferIndex)
{
	Definition = InDefinition;
	bUnlocked = bInUnlocked;
	OfferIndex = InOfferIndex;

	SetDataID(Definition ? Definition->GetFName() : NAME_None);
	SetDataDisplayName(Definition && !Definition->DisplayName.IsEmpty()
		? Definition->DisplayName
		: FText::FromString(TEXT("Night Shift")));

	InitDataObject();
}

void UEMRCommonHubNightShiftListDataObject::OnDataObjectInitialized()
{
	if (Definition && GetDataDisplayName().IsEmpty())
	{
		SetDataDisplayName(Definition->DisplayName);
	}
}
