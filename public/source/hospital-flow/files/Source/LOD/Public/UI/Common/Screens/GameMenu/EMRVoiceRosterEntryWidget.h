#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Core/EMRFrontendCommonListEntryWidgetBase.h"
#include "EMRVoiceRosterEntryWidget.generated.h"

class UBorder;
class UButton;
class UEMRListDataObjectBase;
class UEMRFrontendCommonButtonBase;
class UEMRVoiceRosterEntryData;
class UAnalogSlider;
class USoundBase; 
class UTextBlock;

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRVoiceRosterEntryWidget : public UEMRFrontendCommonListEntryWidgetBase
{
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;
    virtual void OnOwningListDataObjectSet(UEMRListDataObjectBase* InOwningListDataObject) override;
    virtual void OnOwningListDataObjectModified(UEMRListDataObjectBase* OwningModifiedListData, EOptionsListDataModifyReason ModifyReason) override;
    virtual void NativeOnEntryReleased() override;
    virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
    virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
    virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

private:
    UFUNCTION()
    void HandleVolumeChanged(float InValue);

    UFUNCTION()
    void HandleMuteClicked();

    void ResolveNamedWidgetsByName();
    void ApplyEntryData();
    void RefreshMuteLabel() const;
    void RefreshTalkingState(bool bIsTalking) const;
    void RefreshSelectionState(bool bIsSelected) const;
    void RefreshSliderFocusFeedback();
    void ToggleMute();

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Voice|Feedback")
    TObjectPtr<USoundBase> SliderFocusSound = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Voice|Feedback")
    FLinearColor FocusedSliderBarColor = FLinearColor(0.82f, 0.94f, 1.0f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "EMR|Voice|Feedback")
    FLinearColor FocusedSliderHandleColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> Text_PlayerName = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> Text_PlayerPing = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UAnalogSlider> Slider_PlayerVolume = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_MuteToggle = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UButton> Button_MuteToggle = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> Text_MuteLabel = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UBorder> Border_TalkingState = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Border_RowCard = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UEMRVoiceRosterEntryData> CachedEntryData = nullptr;

    UPROPERTY(Transient)
    bool bApplyingVolume = false;

    UPROPERTY(Transient)
    bool bIsSelectedEntry = false;

    UPROPERTY(Transient)
    bool bIsInFocusPath = false;

    UPROPERTY(Transient)
    bool bWasSliderFeedbackActive = false;

    UPROPERTY(Transient)
    FLinearColor DefaultSliderBarColor = FLinearColor::White;

    UPROPERTY(Transient)
    FLinearColor DefaultSliderHandleColor = FLinearColor::White;
};
