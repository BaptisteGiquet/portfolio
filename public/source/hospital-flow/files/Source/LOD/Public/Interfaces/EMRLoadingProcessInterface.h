#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "EMRLoadingProcessInterface.generated.h"

UINTERFACE(BlueprintType)
class LOD_API UEMRLoadingProcessInterface : public UInterface
{
	GENERATED_BODY()
};

class LOD_API IEMRLoadingProcessInterface
{
	GENERATED_BODY()

public:
	static bool ShouldShowLoadingScreen(UObject* TestObject, FString& OutReason);

	virtual bool ShouldShowLoadingScreen(FString& OutReason) const
	{
		return false;
	}
};
