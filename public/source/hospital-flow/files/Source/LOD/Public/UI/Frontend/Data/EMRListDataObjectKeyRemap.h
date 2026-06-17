

#pragma once

#include "CoreMinimal.h"
#include "EMRListDataObjectBase.h"
#include "CommonInputTypeEnum.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "EMRListDataObjectKeyRemap.generated.h"

class UEnhancedPlayerMappableKeyProfile;
class UEnhancedInputUserSettings;
/**
 * 
 */
UCLASS()
class LOD_API UEMRListDataObjectKeyRemap : public UEMRListDataObjectBase
{
	GENERATED_BODY()
	
public:
	void InitKeyRemapData(
		UEnhancedInputUserSettings* InOwningInputUserSettings,
		UEnhancedPlayerMappableKeyProfile* InKeyProfile,
		ECommonInputType InDesiredInputKeyType,
		const FPlayerKeyMapping& InOwningPlayerKeyMapping);
	
	FSlateBrush GetIconFromCurrentKey() const;
	FText GetDisplayNameFromCurrentKey() const;
	
	ECommonInputType GetDesiredInputTypeFromCurrentKey() const { return CachedDesiredInputKeyType; };
	
	void BindNewInputKey(const FKey& InNewKey);

	virtual bool HasDefaultValue() const override;
	virtual bool CanResetBackToDefaultValue() const override;
	virtual bool TryResetBackToDefaultValue() override;
	
protected:

	
	
private:
	FPlayerKeyMapping* GetOwningKeyMapping() const;
	
	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputUserSettings> CachedOwningEIUserSettings;
	
	UPROPERTY(Transient)
	TObjectPtr<UEnhancedPlayerMappableKeyProfile> CachedOwningKeyProfile;
	
	ECommonInputType CachedDesiredInputKeyType;
	
	FName CachedOwningMappingName;
	
	EPlayerMappableKeySlot CachedOwningMappableKeySlot;
};
