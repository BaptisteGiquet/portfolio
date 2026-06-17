


#include "UI/Frontend/Data/EMRListDataObjectKeyRemap.h"

#include "CommonInputSubsystem.h"

void UEMRListDataObjectKeyRemap::InitKeyRemapData(UEnhancedInputUserSettings* InOwningInputUserSettings, UEnhancedPlayerMappableKeyProfile* InKeyProfile, ECommonInputType InDesiredInputKeyType, const FPlayerKeyMapping& InOwningPlayerKeyMapping)
{
	CachedOwningEIUserSettings = InOwningInputUserSettings;
	CachedOwningKeyProfile = InKeyProfile;
	CachedDesiredInputKeyType = InDesiredInputKeyType;
	CachedOwningMappingName = InOwningPlayerKeyMapping.GetMappingName();
	CachedOwningMappableKeySlot = InOwningPlayerKeyMapping.GetSlot();
}


FSlateBrush UEMRListDataObjectKeyRemap::GetIconFromCurrentKey() const
{
	check(CachedOwningEIUserSettings);
	
	UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(CachedOwningEIUserSettings->GetLocalPlayer());
	check(CommonInputSubsystem)
	
	FSlateBrush OutBrush;
	
	UCommonInputPlatformSettings::Get()->TryGetInputBrush(
		OutBrush,
		GetOwningKeyMapping()->GetCurrentKey(),
		CachedDesiredInputKeyType,
		CommonInputSubsystem->GetCurrentGamepadName()
		);
	
	return OutBrush;
}


FText UEMRListDataObjectKeyRemap::GetDisplayNameFromCurrentKey() const
{
	const FKey CurrentKey = GetOwningKeyMapping()->GetCurrentKey();
	if (!CurrentKey.IsValid())
	{
		return FText::GetEmpty();
	}

	return CurrentKey.GetDisplayName();
}


void UEMRListDataObjectKeyRemap::BindNewInputKey(const FKey& InNewKey)
{
	check(CachedOwningEIUserSettings);
	
	FMapPlayerKeyArgs KeyArgs;
	KeyArgs.MappingName = CachedOwningMappingName;
	KeyArgs.Slot = CachedOwningMappableKeySlot;
	KeyArgs.NewKey = InNewKey;
	
	FGameplayTagContainer Container;
	
	CachedOwningEIUserSettings->MapPlayerKey(KeyArgs, Container);
	CachedOwningEIUserSettings->SaveSettings();
	
	NotifyListDataModified(this);
}


bool UEMRListDataObjectKeyRemap::HasDefaultValue() const
{
	return GetOwningKeyMapping()->GetDefaultKey().IsValid();
}


bool UEMRListDataObjectKeyRemap::CanResetBackToDefaultValue() const
{
	return HasDefaultValue() && GetOwningKeyMapping()->IsCustomized();
}


bool UEMRListDataObjectKeyRemap::TryResetBackToDefaultValue()
{
	if (CanResetBackToDefaultValue())
	{
		check(CachedOwningEIUserSettings)
		
		GetOwningKeyMapping()->ResetToDefault();
		CachedOwningEIUserSettings->SaveSettings();
		
		NotifyListDataModified(this, EOptionsListDataModifyReason::ResetToDefault);
		return true;
	}
	
	return false;
}


FPlayerKeyMapping* UEMRListDataObjectKeyRemap::GetOwningKeyMapping() const
{
	check(CachedOwningKeyProfile)
	
	FMapPlayerKeyArgs KeyArgs;
	KeyArgs.MappingName = CachedOwningMappingName;
	KeyArgs.Slot = CachedOwningMappableKeySlot;
	
	return CachedOwningKeyProfile->FindKeyMapping(KeyArgs);
}
