#include "UI/Common/Screens/GameMenu/EMRVoiceRosterEntryWidget.h"

#include "AnalogSlider.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "ProximityVoiceLocalPlayerSubsystem.h"
#include "Sound/SoundBase.h"
#include "UI/Common/Screens/GameMenu/EMRVoiceRosterEntryData.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/Data/EMRListDataObjectBase.h"

void UEMRVoiceRosterEntryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    SetIsFocusable(true);
    ResolveNamedWidgetsByName();

    if (Slider_PlayerVolume)
    {
        Slider_PlayerVolume->SetMinValue(0.0f);
        Slider_PlayerVolume->SetMaxValue(2.0f);
        Slider_PlayerVolume->SetStepSize(0.05f);
        Slider_PlayerVolume->IsFocusable = true;
        Slider_PlayerVolume->SynchronizeProperties();
        DefaultSliderBarColor = Slider_PlayerVolume->GetSliderBarColor();
        DefaultSliderHandleColor = Slider_PlayerVolume->GetSliderHandleColor();
        Slider_PlayerVolume->OnValueChanged.RemoveAll(this);
        Slider_PlayerVolume->OnValueChanged.AddDynamic(this, &ThisClass::HandleVolumeChanged);
    }

    if (CommonButton_MuteToggle)
    {
        CommonButton_MuteToggle->OnClicked().RemoveAll(this);
        CommonButton_MuteToggle->OnClicked().AddUObject(this, &ThisClass::HandleMuteClicked);
    }
    else if (Button_MuteToggle)
    {
        Button_MuteToggle->OnClicked.RemoveAll(this);
        Button_MuteToggle->OnClicked.AddDynamic(this, &ThisClass::HandleMuteClicked);
    }

    RefreshSliderFocusFeedback();
}

void UEMRVoiceRosterEntryWidget::OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject)
{
    Super::OnOwningListDataObjectSet(InOwningListDataObject);

    if (Slider_PlayerVolume)
    {
        Slider_PlayerVolume->SetStepSize(0.05f);
        Slider_PlayerVolume->IsFocusable = true;
        Slider_PlayerVolume->SynchronizeProperties();
    }

    CachedEntryData = Cast<UEMRVoiceRosterEntryData>(InOwningListDataObject);
    ApplyEntryData();
}

void UEMRVoiceRosterEntryWidget::OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason)
{
    CachedEntryData = Cast<UEMRVoiceRosterEntryData>(OwningModifiedListData);
    ApplyEntryData();
}

void UEMRVoiceRosterEntryWidget::NativeOnEntryReleased()
{
    Super::NativeOnEntryReleased();
    CachedEntryData = nullptr;
    bIsSelectedEntry = false;
    bIsInFocusPath = false;
    bWasSliderFeedbackActive = false;
    RefreshSelectionState(false);
    RefreshSliderFocusFeedback();
}

void UEMRVoiceRosterEntryWidget::NativeOnItemSelectionChanged(bool bIsSelected)
{
    Super::NativeOnItemSelectionChanged(bIsSelected);
    bIsSelectedEntry = bIsSelected;
    RefreshSelectionState(bIsSelected);
    RefreshSliderFocusFeedback();
}

void UEMRVoiceRosterEntryWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
    Super::NativeOnAddedToFocusPath(InFocusEvent);
    bIsInFocusPath = true;
    RefreshSliderFocusFeedback();
}

void UEMRVoiceRosterEntryWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
    Super::NativeOnRemovedFromFocusPath(InFocusEvent);
    bIsInFocusPath = false;
    RefreshSliderFocusFeedback();
}

void UEMRVoiceRosterEntryWidget::HandleVolumeChanged(float InValue)
{
    if (bApplyingVolume
        || !CachedEntryData
        || !CachedEntryData->GetVoiceSubsystem()
        || CachedEntryData->GetPlayerId().IsEmpty()
        || CachedEntryData->IsLocalPlayer())
    {
        return;
    }

    const float ClampedValue = FMath::Clamp(InValue, 0.0f, 2.0f);
    CachedEntryData->SetVolume(ClampedValue);
    CachedEntryData->GetVoiceSubsystem()->SetRemotePlayerVolume(CachedEntryData->GetPlayerId(), ClampedValue);
}

void UEMRVoiceRosterEntryWidget::HandleMuteClicked()
{
    ToggleMute();
}

void UEMRVoiceRosterEntryWidget::ResolveNamedWidgetsByName()
{
    if (!WidgetTree)
    {
        return;
    }

    if (!Text_PlayerName)
    {
        Text_PlayerName = WidgetTree->FindWidget<UTextBlock>(TEXT("Text_PlayerName"));
    }

    if (!Text_PlayerPing)
    {
        Text_PlayerPing = WidgetTree->FindWidget<UTextBlock>(TEXT("Text_PlayerPing"));
    }

    if (!Slider_PlayerVolume)
    {
        Slider_PlayerVolume = WidgetTree->FindWidget<UAnalogSlider>(TEXT("Slider_PlayerVolume"));
    }

    if (!CommonButton_MuteToggle)
    {
        CommonButton_MuteToggle = WidgetTree->FindWidget<UEMRFrontendCommonButtonBase>(TEXT("CommonButton_MuteToggle"));
    }

    if (!Button_MuteToggle)
    {
        Button_MuteToggle = WidgetTree->FindWidget<UButton>(TEXT("Button_MuteToggle"));
    }

    if (!Text_MuteLabel)
    {
        Text_MuteLabel = WidgetTree->FindWidget<UTextBlock>(TEXT("Text_MuteLabel"));
    }

    if (!Border_TalkingState)
    {
        Border_TalkingState = WidgetTree->FindWidget<UBorder>(TEXT("Border_TalkingState"));
    }

    if (!Border_RowCard)
    {
        Border_RowCard = WidgetTree->FindWidget<UBorder>(TEXT("Border_RowCard"));
    }
}

void UEMRVoiceRosterEntryWidget::ApplyEntryData()
{
    ResolveNamedWidgetsByName();

    if (!CachedEntryData)
    {
        return;
    }

    const int32 SafePingMs = CachedEntryData->GetPingMs() >= 0 ? CachedEntryData->GetPingMs() : 0;
    const FText PingText = FText::FromString(FString::Printf(TEXT("%d ms"), SafePingMs));

    if (Text_PlayerName)
    {
        if (Text_PlayerPing)
        {
            Text_PlayerName->SetText(CachedEntryData->GetDisplayName());
        }
        else
        {
            const FString NameWithPing = FString::Printf(
                TEXT("%s (%s)"),
                *CachedEntryData->GetDisplayName().ToString(),
                *PingText.ToString());
            Text_PlayerName->SetText(FText::FromString(NameWithPing));
        }
    }

    if (Text_PlayerPing)
    {
        Text_PlayerPing->SetText(PingText);
    }

    if (Slider_PlayerVolume)
    {
        bApplyingVolume = true;
        Slider_PlayerVolume->SetValue(CachedEntryData->GetVolume());
        bApplyingVolume = false;
        Slider_PlayerVolume->SetIsEnabled(!CachedEntryData->IsLocalPlayer());
    }

    if (CommonButton_MuteToggle)
    {
        CommonButton_MuteToggle->SetIsEnabled(!CachedEntryData->IsLocalPlayer());
    }

    if (Button_MuteToggle)
    {
        Button_MuteToggle->SetIsEnabled(!CachedEntryData->IsLocalPlayer());
    }

    RefreshMuteLabel();
    RefreshTalkingState(CachedEntryData->IsTalking());
}

void UEMRVoiceRosterEntryWidget::RefreshMuteLabel() const
{
    if (!CachedEntryData)
    {
        return;
    }

    const FText MuteText = CachedEntryData->IsLocalPlayer()
    ? FText::FromString(TEXT("Local"))
    : (CachedEntryData->IsMuted() ? FText::FromString(TEXT("Unmute")) : FText::FromString(TEXT("Mute")));
    if (CommonButton_MuteToggle)
    {
        CommonButton_MuteToggle->SetButtonText(MuteText);
    }

    if (Text_MuteLabel)
    {
        Text_MuteLabel->SetText(MuteText);
    }
}

void UEMRVoiceRosterEntryWidget::RefreshTalkingState(bool bIsTalking) const
{
    if (!Border_TalkingState)
    {
        return;
    }

    Border_TalkingState->SetBrushColor(bIsTalking
        ? FLinearColor(0.12f, 0.82f, 0.28f, 1.0f)
        : FLinearColor(0.22f, 0.22f, 0.22f, 1.0f));
}

void UEMRVoiceRosterEntryWidget::RefreshSelectionState(bool bIsSelected) const
{
    if (!Border_RowCard)
    {
        return;
    }

    Border_RowCard->SetBrushColor(bIsSelected
        ? FLinearColor(0.09f, 0.20f, 0.29f, 0.98f)
        : FLinearColor(0.04f, 0.11f, 0.16f, 0.95f));
}

void UEMRVoiceRosterEntryWidget::RefreshSliderFocusFeedback()
{
    if (!Slider_PlayerVolume)
    {
        return;
    }

    const bool bSliderFeedbackActive = bIsSelectedEntry && bIsInFocusPath;

    Slider_PlayerVolume->SetSliderBarColor(bSliderFeedbackActive ? FocusedSliderBarColor : DefaultSliderBarColor);
    Slider_PlayerVolume->SetSliderHandleColor(bSliderFeedbackActive ? FocusedSliderHandleColor : DefaultSliderHandleColor);
    Slider_PlayerVolume->SetRenderScale(bSliderFeedbackActive ? FVector2D(1.05f, 1.05f) : FVector2D(1.0f, 1.0f));

    if (bSliderFeedbackActive && !bWasSliderFeedbackActive && SliderFocusSound)
    {
        UGameplayStatics::PlaySound2D(this, SliderFocusSound);
    }

    bWasSliderFeedbackActive = bSliderFeedbackActive;
}

void UEMRVoiceRosterEntryWidget::ToggleMute()
{
    if (!CachedEntryData
        || !CachedEntryData->GetVoiceSubsystem()
        || CachedEntryData->GetPlayerId().IsEmpty()
        || CachedEntryData->IsLocalPlayer())
    {
        return;
    }

    CachedEntryData->SetMuted(!CachedEntryData->IsMuted());
    CachedEntryData->GetVoiceSubsystem()->SetRemotePlayerMuted(CachedEntryData->GetPlayerId(), CachedEntryData->IsMuted());
    RefreshMuteLabel();
}
