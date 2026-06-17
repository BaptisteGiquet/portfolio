#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"
#include "EMRCommonHubUpgradeEntryWidget.generated.h"

class UCommonTextBlock;
class UEMRCommonHubUpgradeListDataObject;

UCLASS()
class LOD_API UEMRCommonHubUpgradeEntryWidget : public UEMRFrontendCommonListEntryWidgetBase
{
    GENERATED_BODY()

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|Hub|UpgradeVote|UI", meta = (DisplayName = "On Entry Visual State Changed"))
    void BP_OnEntryVisualStateChanged(bool bInSelected, int32 InVoteCount);

    virtual void OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject) override;
    virtual void OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason) override;
    virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

private:
    void RefreshFromData();

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_UpgradeName = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_UpgradeDescription = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_VoteCount = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEMRCommonHubUpgradeListDataObject> CachedUpgradeData = nullptr;
};

