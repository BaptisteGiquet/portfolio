#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EMRXRayTargetData.generated.h"

/**
 * Data table row mapping a pathology to XRay socket targets on the patient skeletal mesh.
 */
USTRUCT(BlueprintType)
struct LOD_API FEMRXRayTargetData : public FTableRowBase
{
    GENERATED_BODY()

    /** Pathology that requires these XRay socket targets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|XRay", meta = (Categories = "EMR.Patient.Pathology"))
    FGameplayTag PathologyTag;

    /** Socket names on the patient skeletal mesh to target. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|XRay")
    TArray<FName> SocketNames;
};
