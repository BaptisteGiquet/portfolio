#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRUltrasoundSliderWidget.generated.h"

class UEMRFrontendCommonButtonBase;
class AEMRUltrasoundMachine;
class UAnalogSlider;

UCLASS()
class LOD_API UEMRUltrasoundSliderWidget : public UEMRFrontendCommonActivatableWidgetBase
{
    GENERATED_BODY()

public:
    void InitializeForMachine(AEMRUltrasoundMachine* InMachine);
    void UpdateFromMachine();

protected:
    virtual void NativeConstruct() override;

    UFUNCTION()
    void HandleSlider1Clicked();

    UFUNCTION()
    void HandleSlider2Clicked();

    UFUNCTION()
    void HandleSlider3Clicked();

private:
    void HandleSliderClicked(int32 SliderIndex);

    TWeakObjectPtr<AEMRUltrasoundMachine> OwningMachine;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Slider1 = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Slider2 = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Slider3 = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAnalogSlider> AnalogSlider_Slider1 = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAnalogSlider> AnalogSlider_Slider2 = nullptr;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UAnalogSlider> AnalogSlider_Slider3 = nullptr;
};
