#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EMRLabAnalyzerTestData.generated.h"

USTRUCT(BlueprintType)
struct LOD_API FEMRLabAnalyzerTestDefinition : public FTableRowBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|LabAnalyzer", meta = (Categories = "EMR.Abilities.Exam"))
    FGameplayTag TestTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    FLinearColor TestColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|LabAnalyzer")
    int32 SortOrder = 0;
};
