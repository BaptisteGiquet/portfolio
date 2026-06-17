#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"
#include "EMRCommonHubNightShiftEntryWidget.generated.h"

class UCommonTextBlock;
class UEMRCommonHubNightShiftListDataObject;

UCLASS()
class LOD_API UEMRCommonHubNightShiftEntryWidget : public UEMRFrontendCommonListEntryWidgetBase
{
	GENERATED_BODY()

public:
	bool IsUnlocked() const { return bIsUnlocked; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "EMR|NightShift|UI", meta = (DisplayName = "On Entry Visual State Changed"))
	void BP_OnEntryVisualStateChanged(bool bInUnlocked, bool bInSelected);

	virtual void OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject) override;
	virtual void OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

private:
	void UpdateFromDataObject();
	void UpdateSelectionBorder(bool bIsSelected);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_NightShiftName = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Difficulty = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Reward = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Map = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UBorder> Border_Selection = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UEMRCommonHubNightShiftListDataObject> CachedNightShiftData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|NightShift", meta = (AllowPrivateAccess = "true"))
	FLinearColor BorderColorSelected = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|NightShift", meta = (AllowPrivateAccess = "true"))
	FLinearColor BorderColorUnselected = FLinearColor::Transparent;

	bool bIsUnlocked = false;
};
