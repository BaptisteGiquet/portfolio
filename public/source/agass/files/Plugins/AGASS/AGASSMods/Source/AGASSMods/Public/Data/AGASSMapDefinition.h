#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AGASSMapDefinition.generated.h"

class UWorld;

UCLASS(BlueprintType, Blueprintable)
class AGASSMODS_API UAGASSMapDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Map")
	FString MapId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Map")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Map")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Map")
	TSoftObjectPtr<UWorld> WorldAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Map")
	FString Version;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Map")
	FString CompatibilityVersion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Map")
	TArray<FString> CompatibilityTags;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "AGASS|Events",
		meta = (
			DisplayName = "Map Session Events",
			ToolTip = "Session events that become available only when this map is the selected live session map.",
			AllowedClasses = "/Script/AGASSTower.AGASSSessionEventDefinition"))
	TArray<FSoftObjectPath> SessionEventDefinitions;

	FString GetEffectiveCompatibilityVersion() const
	{
		return CompatibilityVersion.IsEmpty() ? Version : CompatibilityVersion;
	}
};
