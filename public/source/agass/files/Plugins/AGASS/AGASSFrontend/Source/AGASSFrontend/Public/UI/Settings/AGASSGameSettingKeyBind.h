#pragma once

#include "CoreMinimal.h"
#include "GameSettingValue.h"
#include "AGASSGameSettingKeyBind.generated.h"

class UEnhancedInputUserSettings;
class ULocalPlayer;
struct FPlayerKeyMapping;
struct FSlateBrush;

UENUM()
enum class EAGASSRebindInputType : uint8
{
	MouseAndKeyboard,
	Gamepad
};

UCLASS()
class AGASSFRONTEND_API UAGASSGameSettingKeyBind : public UGameSettingValue
{
	GENERATED_BODY()

public:
	void InitializeInputData(ULocalPlayer* InLocalPlayer, FName InMappingName, EAGASSRebindInputType InInputType, bool bInAllowAxisKeys = false, bool bInRequireMouseWheelAxis = false);

	FText GetCurrentKeyText() const;
	bool TryGetCurrentKeyBrush(FSlateBrush& OutBrush) const;
	bool IsCustomized() const;
	bool HasDuplicateBindingForKey(FKey InKey, FText& OutConflictText) const;
	bool ChangeBinding(FKey InKey, FText& OutFailureReason);

	virtual void StoreInitial() override;
	virtual void ResetToDefault() override;
	virtual void RestoreToInitial() override;

	EAGASSRebindInputType GetInputType() const
	{
		return InputType;
	}

	bool AllowsAxisKeys() const
	{
		return bAllowAxisKeys;
	}

	bool RequiresMouseWheelAxis() const
	{
		return bRequireMouseWheelAxis;
	}

	FName GetMappingName() const
	{
		return MappingName;
	}

protected:
	virtual void OnApply() override;

private:
	UEnhancedInputUserSettings* GetUserSettings() const;
	const FPlayerKeyMapping* FindCurrentMapping() const;
	FKey GetCurrentKey() const;
	bool ValidateKeyForRow(FKey InKey, FText& OutFailureReason) const;
	FText GetDisplayNameForMapping(FName InMappingName) const;

	UPROPERTY(Transient)
	TObjectPtr<ULocalPlayer> RebindLocalPlayer;

	FName MappingName = NAME_None;
	EAGASSRebindInputType InputType = EAGASSRebindInputType::MouseAndKeyboard;
	FKey InitialKey = EKeys::Invalid;
	bool bAllowAxisKeys = false;
	bool bRequireMouseWheelAxis = false;
};
