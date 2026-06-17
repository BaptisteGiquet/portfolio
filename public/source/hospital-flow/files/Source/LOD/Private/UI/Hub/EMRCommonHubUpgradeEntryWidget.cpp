#include "UI/Hub/EMRCommonHubUpgradeEntryWidget.h"

#include "CommonTextBlock.h"
#include "Data/EMRRunUpgradeDefinition.h"
#include "UI/Hub/EMRCommonHubUpgradeListDataObject.h"

void UEMRCommonHubUpgradeEntryWidget::OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject)
{
    Super::OnOwningListDataObjectSet(InOwningListDataObject);

    CachedUpgradeData = Cast<UEMRCommonHubUpgradeListDataObject>(InOwningListDataObject);
    RefreshFromData();
}

void UEMRCommonHubUpgradeEntryWidget::OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason)
{
    (void)OwningModifiedListData;
    (void)ModifyReason;
    RefreshFromData();
}

void UEMRCommonHubUpgradeEntryWidget::NativeOnItemSelectionChanged(bool bIsSelected)
{
    Super::NativeOnItemSelectionChanged(bIsSelected);
    BP_OnEntryVisualStateChanged(bIsSelected, CachedUpgradeData ? CachedUpgradeData->GetVoteCount() : 0);
}

void UEMRCommonHubUpgradeEntryWidget::RefreshFromData()
{
    const UEMRRunUpgradeDefinition* Definition = CachedUpgradeData ? CachedUpgradeData->GetDefinition() : nullptr;

    if (CommonTextBlock_UpgradeName)
    {
        CommonTextBlock_UpgradeName->SetText(CachedUpgradeData ? CachedUpgradeData->GetDataDisplayName() : FText::FromString(TEXT("Upgrade")));
    }

    if (CommonTextBlock_UpgradeDescription)
    {
        CommonTextBlock_UpgradeDescription->SetText(Definition ? Definition->Description : FText::GetEmpty());
    }

    if (CommonTextBlock_VoteCount)
    {
        CommonTextBlock_VoteCount->SetText(FText::AsNumber(CachedUpgradeData ? CachedUpgradeData->GetVoteCount() : 0));
    }

    BP_OnEntryVisualStateChanged(IsListItemSelected(), CachedUpgradeData ? CachedUpgradeData->GetVoteCount() : 0);
}

