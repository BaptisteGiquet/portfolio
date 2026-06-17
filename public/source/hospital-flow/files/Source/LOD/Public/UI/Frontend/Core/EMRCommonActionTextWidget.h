#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EMRCommonActionTextWidget.generated.h"

class UCommonActionWidget;
class UCommonTextBlock;

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRCommonActionTextWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;

private:
    UFUNCTION()
    void HandleInputIconUpdated();

    UFUNCTION()
    void HandleInputMethodChanged(bool bUsingGamepad);

    void UpdateDisplayedText();

    //***** Bound Widgets *****//
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCommonActionWidget> CommonActionWidget_Action;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_ActionText;
    //***** Bound Widgets *****//

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Input", meta = (AllowPrivateAccess = true, RowType = "/Script/CommonUI.CommonInputActionDataBase"))
    FDataTableRowHandle ActionRow;
};
