#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "EMRLabAnalyzerValidationErrorWidget.generated.h"

class UCommonTextBlock;

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRLabAnalyzerValidationErrorWidget : public UCommonUserWidget
{
    GENERATED_BODY()

public:
    void ShowMessage(const FText& InMessage);
    void ClearMessage();

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_Message = nullptr;
};
