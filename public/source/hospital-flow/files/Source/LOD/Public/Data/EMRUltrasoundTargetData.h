#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EMRUltrasoundTargetData.generated.h"

/**
 * Data table row mapping a pathology to ultrasound socket targets on the patient skeletal mesh.
 */
USTRUCT(BlueprintType)
struct LOD_API FEMRUltrasoundTargetData : public FTableRowBase
{
    GENERATED_BODY()

    /** Pathology that requires these ultrasound socket targets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Ultrasound", meta = (Categories = "EMR.Patient.Pathology"))
    FGameplayTag PathologyTag;

    /** Socket names on the patient skeletal mesh to target. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Ultrasound")
    TArray<FName> SocketNames;
};
