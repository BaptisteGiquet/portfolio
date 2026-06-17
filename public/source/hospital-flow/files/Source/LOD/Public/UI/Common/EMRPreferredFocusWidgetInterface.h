#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EMRPreferredFocusWidgetInterface.generated.h"

class UWidget;

UINTERFACE(BlueprintType)
class UEMRPreferredFocusWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

class LOD_API IEMRPreferredFocusWidgetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EMR|UI")
	UWidget* GetPreferredFocusWidget() const;
};
