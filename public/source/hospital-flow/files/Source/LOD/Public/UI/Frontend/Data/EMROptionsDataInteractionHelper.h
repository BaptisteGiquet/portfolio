

#pragma once

#include "CoreMinimal.h"
#include "PropertyPathHelpers.h"

class UEMRGameUserSettings;
/**
 * 
 */
class LOD_API FEMROptionsDataInteractionHelper
{
	
public:
	FEMROptionsDataInteractionHelper(const FString& InSetterOrGetterFuncPath);
	
	FString GetValueAsString() const;
	void SetValueFromString(const FString& InStringValue);
	
private:
	FCachedPropertyPath CachedDynamicFunctionPath;
	
	TWeakObjectPtr<UEMRGameUserSettings> CachedWeakGameUserSettings;
};
