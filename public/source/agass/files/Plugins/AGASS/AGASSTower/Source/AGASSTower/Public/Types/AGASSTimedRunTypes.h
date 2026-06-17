#pragma once

#include "CoreMinimal.h"
#include "AGASSTimedRunTypes.generated.h"

UENUM(BlueprintType)
enum class EAGASSTimedRunState : uint8
{
	WaitingToStart,
	Active,
	Completed
};

USTRUCT(BlueprintType)
struct AGASSTOWER_API FAGASSTimedRunCompletionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AGASS|Timed Run")
	bool bWasVictory = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AGASS|Timed Run")
	FText ObjectiveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AGASS|Timed Run")
	FText CompletionTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AGASS|Timed Run")
	FText CompletionBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AGASS|Timed Run")
	FText ReturnToFrontendReason;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AGASS|Timed Run", meta = (ClampMin = "0.0"))
	float ReturnDelaySeconds = 8.f;
};
