#include "Blueprint/AGASSTimedRunBlueprintLibrary.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "Interfaces/AGASSTimedRunRuntimeInterface.h"

bool UAGASSTimedRunBlueprintLibrary::CompleteTimedRun(
	UObject* WorldContextObject,
	AController* CompletingController,
	FText ObjectiveName,
	FText CompletionTitle,
	FText CompletionBody,
	const float ReturnDelaySeconds,
	const bool bWasVictory,
	AActor* CompletionSourceActor)
{
	UWorld* const World = WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
	AGameModeBase* const AuthGameMode = World != nullptr ? World->GetAuthGameMode() : nullptr;
	IAGASSTimedRunRuntimeInterface* const TimedRunRuntime = Cast<IAGASSTimedRunRuntimeInterface>(AuthGameMode);
	if (TimedRunRuntime == nullptr)
	{
		return false;
	}

	FAGASSTimedRunCompletionData CompletionData;
	CompletionData.bWasVictory = bWasVictory;
	CompletionData.ObjectiveName = ObjectiveName;
	CompletionData.CompletionTitle = CompletionTitle;
	CompletionData.CompletionBody = CompletionBody;
	CompletionData.ReturnDelaySeconds = ReturnDelaySeconds;
	return TimedRunRuntime->HandleAGASSTimedRunCompleted(CompletingController, CompletionSourceActor, CompletionData);
}
