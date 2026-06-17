
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"

#include "EMRTagsLocalizationTable.generated.h"


USTRUCT(BlueprintType)
struct FEMRGameplayTagLocalizationEntry : public FTableRowBase
{
	GENERATED_BODY()

	/** The gameplay tag to localize */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Localization")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Localization")
	FString LocalizationKey;
};