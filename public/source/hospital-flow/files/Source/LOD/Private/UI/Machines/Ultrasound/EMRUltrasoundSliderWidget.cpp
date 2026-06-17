#include "UI/Machines/Ultrasound/EMRUltrasoundSliderWidget.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/EMRUltrasoundMachine.h"
#include "AnalogSlider.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"

void UEMRUltrasoundSliderWidget::InitializeForMachine(AEMRUltrasoundMachine* InMachine)
{
    OwningMachine = InMachine;
    UpdateFromMachine();
}

void UEMRUltrasoundSliderWidget::UpdateFromMachine()
{
    const float MinValue = OwningMachine.IsValid() ? OwningMachine->GetSliderMinValue() : 0.0f;
    const float MaxValue = OwningMachine.IsValid() ? OwningMachine->GetSliderMaxValue() : 1.0f;
    const FVector Currents = OwningMachine.IsValid() ? OwningMachine->GetSliderCurrents() : FVector::ZeroVector;

    if (AnalogSlider_Slider1)
    {
        AnalogSlider_Slider1->SetMinValue(MinValue);
        AnalogSlider_Slider1->SetMaxValue(MaxValue);
        AnalogSlider_Slider1->SetValue(Currents.X);
        AnalogSlider_Slider1->SetIsEnabled(false);
    }

    if (AnalogSlider_Slider2)
    {
        AnalogSlider_Slider2->SetMinValue(MinValue);
        AnalogSlider_Slider2->SetMaxValue(MaxValue);
        AnalogSlider_Slider2->SetValue(Currents.Y);
        AnalogSlider_Slider2->SetIsEnabled(false);
    }

    if (AnalogSlider_Slider3)
    {
        AnalogSlider_Slider3->SetMinValue(MinValue);
        AnalogSlider_Slider3->SetMaxValue(MaxValue);
        AnalogSlider_Slider3->SetValue(Currents.Z);
        AnalogSlider_Slider3->SetIsEnabled(false);
    }
}

void UEMRUltrasoundSliderWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (CommonButton_Slider1)
    {
        CommonButton_Slider1->OnClicked().RemoveAll(this);
        CommonButton_Slider1->OnClicked().AddUObject(this, &ThisClass::HandleSlider1Clicked);
    }

    if (CommonButton_Slider2)
    {
        CommonButton_Slider2->OnClicked().RemoveAll(this);
        CommonButton_Slider2->OnClicked().AddUObject(this, &ThisClass::HandleSlider2Clicked);
    }

    if (CommonButton_Slider3)
    {
        CommonButton_Slider3->OnClicked().RemoveAll(this);
        CommonButton_Slider3->OnClicked().AddUObject(this, &ThisClass::HandleSlider3Clicked);
    }

    UpdateFromMachine();
}

void UEMRUltrasoundSliderWidget::HandleSlider1Clicked()
{
    HandleSliderClicked(0);
}

void UEMRUltrasoundSliderWidget::HandleSlider2Clicked()
{
    HandleSliderClicked(1);
}

void UEMRUltrasoundSliderWidget::HandleSlider3Clicked()
{
    HandleSliderClicked(2);
}

void UEMRUltrasoundSliderWidget::HandleSliderClicked(int32 SliderIndex)
{
    if (!OwningMachine.IsValid())
    {
        return;
    }

    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        UWorld* World = GetWorld();
        PlayerController = World ? World->GetFirstPlayerController() : nullptr;
    }

    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return;
    }

    AEMRPlayerCharacter* PlayerCharacter = Cast<AEMRPlayerCharacter>(PlayerController->GetPawn());
    if (!PlayerCharacter)
    {
        return;
    }

    PlayerCharacter->Server_SetUltrasoundActiveSlider(OwningMachine.Get(), SliderIndex);
}
