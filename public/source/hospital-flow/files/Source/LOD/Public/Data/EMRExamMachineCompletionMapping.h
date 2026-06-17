#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "EMRExamMachineCompletionMapping.generated.h"

/**
 * Declarative mapping between an exam machine tag and the gameplay tag that represents its completion state.
 */
USTRUCT(BlueprintType)
struct LOD_API FEMRExamMachineCompletionMapping : public FTableRowBase
{
    GENERATED_BODY()

public:
    /** The gameplay tag of the exam to complete (e.g. EMR.Abilities.Exam.CTScan). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR", meta = (Categories = "EMR.Abilities.Exam"))
    FGameplayTag ExamTag;

    /** The gameplay tag that should be granted when the exam has been successfully completed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR", meta = (Categories = "EMR.Patient.Exam.Completed"))
    FGameplayTag CompletionTag;

    /** Whether ExamTag should be matched using GameplayTag hierarchy comparisons. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR")
    bool bMatchExamByHierarchy = true;

    /** Whether CompletionTag should be matched using GameplayTag hierarchy comparisons. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR")
    bool bMatchCompletionByHierarchy = true;
};
