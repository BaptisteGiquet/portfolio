

#pragma once

#include "CoreMinimal.h"
#include "EMRListDataObjectValue.h"
#include "EMRListDataObjectString.generated.h"

/**
 * 
 */
UCLASS()
class LOD_API UEMRListDataObjectString : public UEMRListDataObjectValue
{
	GENERATED_BODY()
	
public:
	void AddDynamicOption(const FString& InStringValue, const FText& InDisplayText);
	void ResetDynamicOptions();
	void SetDynamicOptionsRefreshFunction(TFunction<void(UEMRListDataObjectString&)> InRefreshFunction);
	
	void AdvanceToNextOption();
	void BackToPreviousOption();
	
	void OnRotatorInitiatedValueChanged(const FText& InNewSelectedText);
	
	const TArray<FText>& GetAvailableOptionsTextArray() const { return AvailableOptionsTextArray; }
	const FText& GetCurrentDisplayText() const { return CurrentDisplayText; }
	const FString& GetCurrentStringValue() const { return CurrentStringValue; }
	
protected:
	virtual void OnDataObjectInitialized() override;
	virtual void OnEditDependencyDataModified(UEMRListDataObjectBase* ModifiedDependencyData, EOptionsListDataModifyReason ModifiedReason = EOptionsListDataModifyReason::DirectlyModified) override;
	virtual bool CanResetBackToDefaultValue() const override;
	virtual bool TryResetBackToDefaultValue() override;
	virtual bool CanSetToForcedStringValue(const FString& InForcedValue) const override;
	virtual void OnSetToForcedStringValue(const FString& InForcedValue) override;
	
	bool TrySetDisplayTextFromStringValue(const FString& InStringValue);
	void RefreshDynamicOptionsFromCallback();
	
	FString CurrentStringValue;
	FText CurrentDisplayText;
	TArray<FString> AvailableOptionsStringArray;
	TArray<FText> AvailableOptionsTextArray;
	TFunction<void(UEMRListDataObjectString&)> DynamicOptionsRefreshFunction;
};


UCLASS()
class LOD_API UEMRListDataObjectStringBool : public UEMRListDataObjectString
{
	GENERATED_BODY()
	
public:
	void OverrideTrueDisplayText(const FText& InNewTrueDisplayText);
	void OverrideFalseDisplayText(const FText& InNewFalseDisplayText);
	
	void SetTrueAsDefaultValue();
	void SetFalseAsDefaultValue();
	
protected:
	virtual void OnDataObjectInitialized() override;
	
private:
	void TryInitBoolValues();

	const FString TrueString = TEXT("True");
	const FString FalseString = TEXT("False");
};



UCLASS()
class LOD_API UEMRListDataObjectStringEnum : public UEMRListDataObjectString
{
	GENERATED_BODY()
	
public:
	template <typename EnumType>
	void AddEnumOption(EnumType InEnumOption, const FText& InDisplayText)
	{
		const UEnum* StaticEnumOption =  StaticEnum<EnumType>();
		const FString ConvertedEnumString = StaticEnumOption->GetNameStringByValue(InEnumOption);
		
		AddDynamicOption(ConvertedEnumString, InDisplayText);
	}
	
	template <typename EnumType>
	EnumType GetCurrentValueAsEnum() const
	{
		const UEnum* StaticEnumOption =  StaticEnum<EnumType>();
		return static_cast<EnumType>(StaticEnumOption->GetValueByNameString(CurrentStringValue));
	}
	
	template <typename EnumType>
	void SetDefaultValueFromEnumOption(EnumType InEnumOption)
	{
		const UEnum* StaticEnumOption =  StaticEnum<EnumType>();
		const FString ConvertedEnumString = StaticEnumOption->GetNameStringByValue(InEnumOption);
		
		SetDefaultValueFromString(ConvertedEnumString);
	}

protected:
	
private:

};



UCLASS()
class LOD_API UEMRListDataObjectStringInteger : public UEMRListDataObjectString
{
	GENERATED_BODY()
	
public:
	void AddIntegerOption(int32 InIntegerValue, const FText& InDisplayText);
	
protected:
	virtual void OnDataObjectInitialized() override;
	virtual void OnEditDependencyDataModified(UEMRListDataObjectBase* ModifiedDependencyData, EOptionsListDataModifyReason ModifiedReason = EOptionsListDataModifyReason::DirectlyModified) override;
	
};

