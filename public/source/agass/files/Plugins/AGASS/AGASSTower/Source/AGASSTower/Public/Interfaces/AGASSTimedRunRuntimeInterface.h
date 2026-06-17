#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/AGASSTimedRunTypes.h"
#include "AGASSTimedRunRuntimeInterface.generated.h"

class AActor;
class AController;

UINTERFACE()
class AGASSTOWER_API UAGASSTimedRunRuntimeInterface : public UInterface
{
	GENERATED_BODY()
};

class AGASSTOWER_API IAGASSTimedRunRuntimeInterface
{
	GENERATED_BODY()

public:
	virtual bool HandleAGASSTimedRunStarted(AController* StartingController, AActor* StartActor) = 0;
	virtual bool HandleAGASSTimedRunCompleted(
		AController* CompletingController,
		AActor* CompletionSourceActor,
		const FAGASSTimedRunCompletionData& CompletionData) = 0;
};
