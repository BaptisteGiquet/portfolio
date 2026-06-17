

#pragma once

#include "CoreMinimal.h"
#include "CommonRotator.h"
#include "EMRFrontendCommonRotator.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendCommonRotator : public UCommonRotator
{
	GENERATED_BODY()
	
public:
	void SetSelectedOptionByText(const FText& InTextOption);
};
