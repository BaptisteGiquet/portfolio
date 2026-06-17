#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AGASSTimedRunBlueprintLibrary.generated.h"

class AActor;
class AController;

UCLASS()
class AGASSTOWER_API UAGASSTimedRunBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "AGASS|Timed Run", meta = (WorldContext = "WorldContextObject"))
	static bool CompleteTimedRun(
		UObject* WorldContextObject,
		AController* CompletingController,
		FText ObjectiveName,
		FText CompletionTitle,
		FText CompletionBody,
		float ReturnDelaySeconds = 8.f,
		bool bWasVictory = true,
		AActor* CompletionSourceActor = nullptr);
};
