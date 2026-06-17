#include "LocalizationLibrary.h"

#include "Subsystems/EMRLocalizationDeveloperSettings.h"

FName GetUIStringTableId()
{
	if (const UEMRLocalizationDeveloperSettings* LocalizationSettings = GetDefault<UEMRLocalizationDeveloperSettings>())
	{
		const FName StringTableId = LocalizationSettings->GetUIStringTableId();
		if (!StringTableId.IsNone())
		{
			return StringTableId;
		}
	}

	static bool bLoggedMissingSettings = false;
	if (!bLoggedMissingSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("[LocalizationLibrary] UIStringTable is not configured in EMRLocalizationDeveloperSettings"));
		bLoggedMissingSettings = true;
	}

	return NAME_None;
}
