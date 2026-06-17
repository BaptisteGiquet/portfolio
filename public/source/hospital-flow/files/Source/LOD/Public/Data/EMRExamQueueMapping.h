#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EMRExamQueueMapping.generated.h"

USTRUCT(BlueprintType)
struct LOD_API FEMRExamQueueMapping : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** Gameplay tag of the exam that should create a queue (e.g. EMR.Abilities.Exam.XRay). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exam", meta = (Categories = "EMR.Abilities.Exam"))
	FGameplayTag ExamTag;

	/** Machine type that can process the exam (e.g. EMR.Machine.XRay). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exam", meta = (Categories = "EMR.Machine"))
	FGameplayTag MachineTypeTag;

	/** ExamQueue tag to apply when patient waits for this exam (e.g. EMR.Patient.ExamQueue.WaitingForXRay). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exam", meta = (Categories = "EMR.Patient.ExamQueue"))
	FGameplayTag QueueTag;

	/** If true, matching uses hierarchical MatchesTag; otherwise tag must match exactly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exam")
	bool bMatchByHierarchy = false;

	/** Optional note for designers/debug. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exam")
	FString Notes;
};

