#pragma once

#include "CoreMinimal.h"
#include "CommonInputBaseTypes.h"
#include "Blueprint/UserWidget.h"
#include "EMRGameplayKeybindHelperWidget.generated.h"

class UCommonInputSubsystem;
class UCommonTextBlock;
class UInputAction;

USTRUCT(BlueprintType)
struct FEMRGameplayKeybindEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Keybind Helper")
	FText DisplayLabel = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Keybind Helper")
	TObjectPtr<UInputAction> InputAction = nullptr;
};

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRGameplayKeybindHelperWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleControlMappingsRebuilt();

	void HandleInputMethodChanged(ECommonInputType NewInputType);
	void RefreshDisplayedBindings();
	FKey ResolveFirstKeyForCurrentInput(const UInputAction* InInputAction) const;
	FText BuildSummaryText() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Keybind Helper", meta = (AllowPrivateAccess = true))
	TArray<FEMRGameplayKeybindEntry> KeybindEntries;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = true))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_KeybindSummary = nullptr;

	TWeakObjectPtr<UCommonInputSubsystem> BoundCommonInputSubsystem;
	FDelegateHandle InputMethodChangedHandle;
};
