#include "UI/Hub/EMRCommonHubUpgradeListDataObject.h"

#include "Data/EMRRunUpgradeDefinition.h"

void UEMRCommonHubUpgradeListDataObject::Init(UEMRRunUpgradeDefinition* InDefinition, const int32 InOfferIndex, const int32 InVoteCount)
{
    Definition = InDefinition;
    OfferIndex = InOfferIndex;
    VoteCount = FMath::Max(0, InVoteCount);

    const FName DataName = (Definition && Definition->UpgradeId != NAME_None)
    ? Definition->UpgradeId
    : (Definition ? Definition->GetFName() : NAME_None);

    SetDataID(DataName);
    SetDataDisplayName(Definition && !Definition->DisplayName.IsEmpty()
        ? Definition->DisplayName
        : FText::FromString(TEXT("Upgrade")));

    InitDataObject();
}

void UEMRCommonHubUpgradeListDataObject::OnDataObjectInitialized()
{
    if (Definition && GetDataDisplayName().IsEmpty())
    {
        SetDataDisplayName(Definition->DisplayName);
    }
}

