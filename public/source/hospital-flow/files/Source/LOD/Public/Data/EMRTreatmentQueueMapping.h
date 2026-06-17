#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EMRTreatmentQueueMapping.generated.h"

USTRUCT(BlueprintType)
struct LOD_API FEMRTreatmentQueueMapping : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** Root tag of the Treatment **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Treatment", meta = (Categories = "EMR.Abilities.Treatment"))
	FGameplayTag TreatmentTag;

	/** ExamQueue tag to apply when patient waits for this Treatment **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Treatment", meta = (Categories = "EMR.Patient.TreatmentQueue"))
	FGameplayTag QueueTag;

	/** If true, matching uses hierarchical MatchesTag; otherwise tag must match exactly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	bool bMatchByHierarchy = false;

	/** Optional note for designers/debug. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Treatment")
	FString Notes;
};

