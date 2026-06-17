#include "Subsystems/EMRLocalizationDeveloperSettings.h"

namespace
{
	FName MakeStringTableIdFromSoftPtr(const TSoftObjectPtr<UStringTable>& StringTablePtr)
	{
		const FSoftObjectPath Path = StringTablePtr.ToSoftObjectPath();
		return Path.IsValid() ? FName(*Path.ToString()) : NAME_None;
	}
}

FName UEMRLocalizationDeveloperSettings::GetGameplayTagStringTableId() const
{
	return MakeStringTableIdFromSoftPtr(GameplayTagStringTable);
}

FName UEMRLocalizationDeveloperSettings::GetUIStringTableId() const
{
	return MakeStringTableIdFromSoftPtr(UIStringTable);
}
