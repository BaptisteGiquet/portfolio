#pragma once

#include "CoreMinimal.h"
#include "EMRListDataObjectBase.h"
#include "EMROptionsDataInteractionHelper.h"
#include "EMRListDataObjectValue.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class LOD_API UEMRListDataObjectValue : public UEMRListDataObjectBase
{
	GENERATED_BODY()
	
public:
	void SetDataDynamicGetter(const TSharedPtr<FEMROptionsDataInteractionHelper>& InDynamicGetter);
	void SetDataDynamicSetter(const TSharedPtr<FEMROptionsDataInteractionHelper>& InDynamicSetter);
	
	virtual bool HasDefaultValue() const override { return DefaultStringValue.IsSet(); }
	void SetDefaultValueFromString(const FString& InDefaultValue) { DefaultStringValue = InDefaultValue; }
	
protected:
	FString GetDefaultValueAsString() const { return DefaultStringValue.GetValue(); }
	
	TSharedPtr<FEMROptionsDataInteractionHelper> DataDynamicGetter;
	TSharedPtr<FEMROptionsDataInteractionHelper> DataDynamicSetter;
	
private:
	TOptional<FString> DefaultStringValue;
};
