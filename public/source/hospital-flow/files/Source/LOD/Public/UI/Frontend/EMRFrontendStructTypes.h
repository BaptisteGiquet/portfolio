#pragma once

#include "CoreMinimal.h"

#include "EMRFrontendStructTypes.generated.h"


USTRUCT()
struct FOptionsDataEditConditionDescriptor
{
	GENERATED_BODY()
	
public:
	void SetEditConditionFunction(TFunction<bool()> InEditConditionFunction)
	{
		EditConditionFunction = InEditConditionFunction;
	}
	
	bool IsValid() const
	{
		return EditConditionFunction != nullptr;
	}
	
	bool IsEditConditionMet() const
	{
		if (IsValid())
		{
			return EditConditionFunction();
		}
		
		return true;
	}
	
	FText GetDisabledRichReason() const
	{
		return DisabledRichReason;
	}
	
	void SetDisabledRichReason(const FText& InDisabledRichReason)
	{
		DisabledRichReason = InDisabledRichReason;
	}
	
	bool HasForcedStringValue() const
	{
		return DisabledForcedStringValue.IsSet();
	}
	
	FString GetDisabledForcedStringValue() const
	{
		return DisabledForcedStringValue.GetValue();
	}
	
	void SetDisabledForcedStringValue(const FString& InDisabledForcedStringValue)
	{
		DisabledForcedStringValue = InDisabledForcedStringValue;
	}
	
private:
	TFunction<bool()> EditConditionFunction;
	FText DisabledRichReason;
	TOptional<FString> DisabledForcedStringValue;
};
