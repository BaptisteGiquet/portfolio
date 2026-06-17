#include "UI/Hub/EMRCommonHubNightShiftEntryWidget.h"

#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Data/EMRNightShiftDefinition.h"
#include "LocalizationLibrary.h"
#include "UI/Hub/EMRCommonHubNightShiftListDataObject.h"

namespace
{
	FText GetLocalizedDifficultyText(const EEMRNightShiftDifficultyTier DifficultyTier)
	{
		switch (DifficultyTier)
		{
			case EEMRNightShiftDifficultyTier::Calm:
				return GetLocalizedText(UIStringTableId, "UI.Hub.NightShift.Difficulty.Calm");
			case EEMRNightShiftDifficultyTier::Standard:
				return GetLocalizedText(UIStringTableId, "UI.Hub.NightShift.Difficulty.Standard");
			case EEMRNightShiftDifficultyTier::Intensifying:
				return GetLocalizedText(UIStringTableId, "UI.Hub.NightShift.Difficulty.Intensifying");
			case EEMRNightShiftDifficultyTier::Critical:
				return GetLocalizedText(UIStringTableId, "UI.Hub.NightShift.Difficulty.Critical");
			case EEMRNightShiftDifficultyTier::Catastrophic:
				return GetLocalizedText(UIStringTableId, "UI.Hub.NightShift.Difficulty.Catastrophic");
			default:
				if (const UEnum* DifficultyEnum = StaticEnum<EEMRNightShiftDifficultyTier>())
				{
					return DifficultyEnum->GetDisplayNameTextByValue(static_cast<int64>(DifficultyTier));
				}
				return FText::GetEmpty();
		}
	}

	FText GetLocalizedRewardText(const EEMRNightShiftRevenuePotential RevenuePotential)
	{
		switch (RevenuePotential)
		{
			case EEMRNightShiftRevenuePotential::Low:
				return GetLocalizedText(UIStringTableId, "UI.Hub.NightShift.Reward.Low");
			case EEMRNightShiftRevenuePotential::Medium:
				return GetLocalizedText(UIStringTableId, "UI.Hub.NightShift.Reward.Medium");
			case EEMRNightShiftRevenuePotential::High:
			case EEMRNightShiftRevenuePotential::VeryHigh:
				return GetLocalizedText(UIStringTableId, "UI.Hub.NightShift.Reward.High");
			default:
				if (const UEnum* RevenueEnum = StaticEnum<EEMRNightShiftRevenuePotential>())
				{
					return RevenueEnum->GetDisplayNameTextByValue(static_cast<int64>(RevenuePotential));
				}
				return FText::GetEmpty();
		}
	}
}

void UEMRCommonHubNightShiftEntryWidget::OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject)
{
	Super::OnOwningListDataObjectSet(InOwningListDataObject);

	CachedNightShiftData = Cast<UEMRCommonHubNightShiftListDataObject>(InOwningListDataObject);
	UpdateFromDataObject();
	UpdateSelectionBorder(IsListItemSelected());
}

void UEMRCommonHubNightShiftEntryWidget::OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason)
{
	UpdateFromDataObject();
}

void UEMRCommonHubNightShiftEntryWidget::NativeOnItemSelectionChanged(bool bIsSelected)
{
	Super::NativeOnItemSelectionChanged(bIsSelected);
	UpdateSelectionBorder(bIsSelected);
	BP_OnEntryVisualStateChanged(bIsUnlocked, bIsSelected);
}

void UEMRCommonHubNightShiftEntryWidget::UpdateFromDataObject()
{
	bIsUnlocked = CachedNightShiftData ? CachedNightShiftData->IsUnlocked() : false;

	const FText NameToShow = CachedNightShiftData
	? CachedNightShiftData->GetDataDisplayName()
	: FText::FromString(TEXT("Night Shift"));

	if (CommonTextBlock_NightShiftName)
	{
		CommonTextBlock_NightShiftName->SetText(NameToShow);
	}

	const UEMRNightShiftDefinition* Definition = CachedNightShiftData
	? CachedNightShiftData->GetDefinition()
	: nullptr;

	if (CommonTextBlock_Difficulty)
	{
		FText DifficultyText;
		if (Definition)
		{
			DifficultyText = GetLocalizedDifficultyText(Definition->DifficultyTier);
		}
		CommonTextBlock_Difficulty->SetText(DifficultyText);
	}

	if (CommonTextBlock_Reward)
	{
		FText RewardText;
		if (Definition)
		{
			RewardText = GetLocalizedRewardText(Definition->RevenuePotential);
		}
		CommonTextBlock_Reward->SetText(RewardText);
	}

	if (CommonTextBlock_Map)
	{
		FText MapText;
		if (Definition)
		{
			const FSoftObjectPath& MapPath = Definition->HospitalLevel.ToSoftObjectPath();
			if (!MapPath.IsNull())
			{
				const FString AssetName = MapPath.GetAssetName();
				MapText = FText::FromString(AssetName);
			}
		}
		CommonTextBlock_Map->SetText(MapText);
	}

	BP_OnEntryVisualStateChanged(bIsUnlocked, IsListItemSelected());
}

void UEMRCommonHubNightShiftEntryWidget::UpdateSelectionBorder(bool bIsSelected)
{
	if (!Border_Selection)
	{
		return;
	}

	Border_Selection->SetBrushColor(bIsSelected ? BorderColorSelected : BorderColorUnselected);
}
