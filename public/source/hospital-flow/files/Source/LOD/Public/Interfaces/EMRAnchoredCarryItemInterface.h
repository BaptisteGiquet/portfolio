#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "EMRAnchoredCarryItemInterface.generated.h"

class AActor;

UINTERFACE(BlueprintType)
class UEMRAnchoredCarryItemInterface : public UInterface
{
	GENERATED_BODY()
};

class LOD_API IEMRAnchoredCarryItemInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Carry|Anchor")
	bool EMRReturnToAnchorFromTrace(const AActor* RequestingActor, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Carry|Anchor")
	void EMRReturnToAnchor();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Carry|Anchor")
	bool EMRIsCarriedBy(const AActor* Actor) const;
};
