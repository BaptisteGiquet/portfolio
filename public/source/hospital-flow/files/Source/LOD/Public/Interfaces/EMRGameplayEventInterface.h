#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "EMRGameplayEventInterface.generated.h"

struct FGameplayEventData;

UINTERFACE(NotBlueprintable)
class UEMRGameplayEventInterface : public UInterface
{
	GENERATED_BODY()
};

class LOD_API IEMRGameplayEventInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EMR|Gameplay Events")
	virtual bool SendGameplayEventToActor(AActor* TargetActor, FGameplayTag EventTag, const FGameplayEventData& Payload) = 0;

	UFUNCTION(BlueprintCallable, Category = "EMR|Gameplay Events")
	virtual bool SendGameplayEvent(FGameplayTag EventTag, const FGameplayEventData& Payload) = 0;
};
