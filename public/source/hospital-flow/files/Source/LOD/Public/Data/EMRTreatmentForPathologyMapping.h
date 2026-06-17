#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "EMRTreatmentForPathologyMapping.generated.h"

USTRUCT(BlueprintType)
struct LOD_API FEMRTreatmentForPathologyMapping : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "EMR", meta = (Categories = "EMR.Patient.Pathology"))
	FGameplayTag Pathology;

	UPROPERTY(EditDefaultsOnly, Category = "EMR", meta = (Categories = "EMR.Abilities.Treatment"))
	FGameplayTagContainer Treatments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	bool bMatchByHierarchy = false;
};

