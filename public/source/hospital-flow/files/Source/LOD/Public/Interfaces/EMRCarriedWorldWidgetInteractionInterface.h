#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "EMRCarriedWorldWidgetInteractionInterface.generated.h"

UINTERFACE(BlueprintType)
class UEMRCarriedWorldWidgetInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

class LOD_API IEMRCarriedWorldWidgetInteractionInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "EMR|Carry|Widget")
	bool CanInteractWithWorldWidgetsWhileCarried() const;
};
