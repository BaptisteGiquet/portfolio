#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AGASSObjectiveRuntimeInterface.generated.h"

class AActor;
class AController;

UINTERFACE()
class AGASSTOWER_API UAGASSObjectiveRuntimeInterface : public UInterface
{
	GENERATED_BODY()
};

class AGASSTOWER_API IAGASSObjectiveRuntimeInterface
{
	GENERATED_BODY()

public:
	virtual bool HandleAGASSObjectiveCompleted(AController* CompletingController, AActor* ObjectiveActor) = 0;
};
