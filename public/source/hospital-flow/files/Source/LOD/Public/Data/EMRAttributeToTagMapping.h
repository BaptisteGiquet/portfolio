#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "EMRAttributeToTagMapping.generated.h"

USTRUCT(BlueprintType)
struct LOD_API FEMRAttributeToTagMapping : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "EMR")
	FGameplayAttribute Attribute;

	UPROPERTY(EditDefaultsOnly, Category = "EMR", meta = (Categories = "EMR.Attributes"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	bool bMatchByHierarchy = false;
};