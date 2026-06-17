

#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"
#include "EMRConfirmScreenWidget.generated.h"

class UDynamicEntryBox;
class UCommonTextBlock;

USTRUCT(BlueprintType)
struct FConfirmScreenButtonInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConfirmScreenButtonType ConfirmScreenButtonType = EConfirmScreenButtonType::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ButtonTextToDisplay;
};

UCLASS()
class LOD_API UConfirmScreenInfoObject : public UObject
{
	GENERATED_BODY()

public:
	static UConfirmScreenInfoObject* CreateOkScreen(const FText& InScreenTitle, const FText& InScreenMessage);
	static UConfirmScreenInfoObject* CreateYesNoScreen(const FText& InScreenTitle, const FText& InScreenMessage);
	static UConfirmScreenInfoObject* CreateOkCancelScreen(const FText& InScreenTitle, const FText& InScreenMessage);
	
	UPROPERTY(Transient)
	FText ScreenTitle;

	UPROPERTY(Transient)
	FText ScreenMessage;

	UPROPERTY(Transient)
	TArray<FConfirmScreenButtonInfo> AvailableScreenButtons;
};
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRConfirmScreenWidget : public UEMRFrontendCommonActivatableWidgetBase
{
	GENERATED_BODY()

public:
	// Gets called outside of the class when this widget is constructed and before it's pushed to the modal stack
	void InitConfirmScreen(UConfirmScreenInfoObject* InScreenInfoObject, TFunction<void(EConfirmScreenButtonType)> ClickedButtonCallback);
	
protected:
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Message;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UDynamicEntryBox> DynamicEntryBox_Buttons;
};
