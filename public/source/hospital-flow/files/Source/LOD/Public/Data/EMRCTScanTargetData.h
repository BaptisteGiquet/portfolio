#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EMRCTScanTargetData.generated.h"

/**
 * Data table row mapping a pathology to CT scan target points on the patient silhouette.
 * Positions are expected to be expressed in normalized widget space (0..1).
 */
USTRUCT(BlueprintType)
struct FEMRCTScanTargetData : public FTableRowBase
{
    GENERATED_BODY()

    /** Pathology that requires this CT scan target layout. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CTScan", meta = (Categories = "EMR.Patient.Pathology"))
    FGameplayTag PathologyTag;

    /** Socket name on the patient skeletal mesh to target. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CTScan")
    FName SocketName = NAME_None;

    /** Normalized target points on the body silhouette. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CTScan")
    TArray<FVector2D> TargetPoints;
};
