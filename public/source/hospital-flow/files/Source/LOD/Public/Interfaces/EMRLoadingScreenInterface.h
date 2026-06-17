#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EMRLoadingScreenInterface.generated.h"

UINTERFACE(BlueprintType)
class UEMRLoadingScreenInterface : public UInterface
{
	GENERATED_BODY()
};

class LOD_API IEMRLoadingScreenInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void OnLoadingScreenActivated();
	
	UFUNCTION(BlueprintNativeEvent)
	void OnLoadingScreenDeactivated();
};
