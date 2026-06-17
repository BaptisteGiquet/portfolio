#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "EMRExamRequirementsForPathologyMapping.generated.h"

/**
 * Declarative mapping between a pathology and the list of exams that must be completed for it.
 */
USTRUCT(BlueprintType)
struct LOD_API FEMRExamRequirementsForPathologyMapping : public FTableRowBase
{
    GENERATED_BODY()

public:
    /** Pathology that this row describes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR", meta = (Categories = "EMR.Patient.Pathology"))
    FGameplayTag Pathology;

    /** Exams (gameplay ability exam tags) that must be completed for the pathology. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR", meta = (Categories = "EMR.Abilities.Exam"))
    FGameplayTagContainer RequiredExams;

    /** Whether the pathology should match using the GameplayTag hierarchy. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR")
    bool bMatchByHierarchy = false;
};
