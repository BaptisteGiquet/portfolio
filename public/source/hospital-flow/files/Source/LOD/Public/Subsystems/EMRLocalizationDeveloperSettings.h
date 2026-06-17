#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "EMRLocalizationDeveloperSettings.generated.h"

class UDataTable;
class UStringTable;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Localization Settings"))
class LOD_API UEMRLocalizationDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "EMR|Localization")
	FName GetGameplayTagStringTableId() const;

	UFUNCTION(BlueprintPure, Category = "EMR|Localization")
	FName GetUIStringTableId() const;

	const TSoftObjectPtr<UDataTable>& GetGameplayTagLocalizationTable() const
	{
		return GameplayTagLocalizationTable;
	}

private:
	UPROPERTY(Config, EditAnywhere, Category = "EMR|Localization")
	TSoftObjectPtr<UDataTable> GameplayTagLocalizationTable;

	UPROPERTY(Config, EditAnywhere, Category = "EMR|Localization")
	TSoftObjectPtr<UStringTable> GameplayTagStringTable;

	UPROPERTY(Config, EditAnywhere, Category = "EMR|Localization")
	TSoftObjectPtr<UStringTable> UIStringTable;
};
