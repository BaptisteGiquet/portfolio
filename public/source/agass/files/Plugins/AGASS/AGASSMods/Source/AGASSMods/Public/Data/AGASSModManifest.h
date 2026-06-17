#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AGASSModManifest.generated.h"

class UAGASSMapDefinition;

UCLASS(BlueprintType, Blueprintable)
class AGASSMODS_API UAGASSModManifest : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mod")
	FString ModId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mod")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mod")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mod")
	FString Version;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mod")
	FString CompatibilityVersion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mod")
	TArray<FString> CompatibilityTags;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "AGASS|Events",
		meta = (
			DisplayName = "Global Session Events",
			ToolTip = "Session events that become available whenever this mod is active, regardless of which selected map is being played.",
			AllowedClasses = "/Script/AGASSTower.AGASSSessionEventDefinition"))
	TArray<FSoftObjectPath> SessionEventDefinitions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Mod")
	TArray<TSoftObjectPtr<UAGASSMapDefinition>> MapDefinitions;

	FString GetEffectiveCompatibilityVersion() const
	{
		return CompatibilityVersion.IsEmpty() ? Version : CompatibilityVersion;
	}
};
