#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProximityVoiceTypes.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRGameplayMenuWidget.generated.h"

class UEMRFrontendCommonButtonBase;
class UEMRFrontendCommonActivatableWidgetBase;
class UEMRFrontendCommonListView;
class UBorder;
class UProximityVoiceLocalPlayerSubsystem;
class UEMRVoiceRosterEntryData;
class UScrollBox;
class UTextBlock;
class UCommonTextBlock;
class AEMRNightShiftGameState;

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRGameplayMenuWidget : public UEMRFrontendCommonActivatableWidgetBase
{
    GENERATED_BODY()

protected:
    UEMRGameplayMenuWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeOnInitialized() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;

    UFUNCTION(BlueprintCallable, Category = "EMR|GameMenu|Voice")
    TArray<FProximityVoiceRemotePlayerView> GetVoiceRemotePlayers() const;

    UFUNCTION(BlueprintCallable, Category = "EMR|GameMenu|Voice")
    bool IsLocalMicActivityDetected() const;

    UFUNCTION(BlueprintCallable, Category = "EMR|GameMenu|Voice")
    void SetRemotePlayerVoiceVolume(const FString& RemotePlayerId, float InVolume);

    UFUNCTION(BlueprintCallable, Category = "EMR|GameMenu|Voice")
    void SetRemotePlayerVoiceMuted(const FString& RemotePlayerId, bool bMuted);

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|GameMenu|Voice")
    void BP_OnVoiceRosterUpdated();

    UFUNCTION(BlueprintImplementableEvent, Category = "EMR|GameMenu|Voice")
    void BP_OnLocalMicActivityChanged(bool bIsMicActive);

private:
    UProximityVoiceLocalPlayerSubsystem* ResolveVoiceSubsystem() const;
    AEMRNightShiftGameState* ResolveRunGameState() const;
    void BindVoiceDelegates();
    void UnbindVoiceDelegates();
    void BindInviteCodeDelegates();
    void UnbindInviteCodeDelegates();

    UFUNCTION()
    void HandleMainMenuClicked();

    UFUNCTION()
    void HandleOptionsClicked();

    UFUNCTION()
    void HandleBugFeedbackClicked();

    UFUNCTION()
    void HandleInviteSteamClicked();

    UFUNCTION()
    void HandleToggleInviteCodeClicked();

    void HandleVoiceRosterChanged();
    void HandleLocalMicActivityChanged(bool bIsMicActive);
    void HandleRunSessionInviteCodeChanged(const FString& NewInviteCode);
    void RebuildVoiceRosterWidgets();
    void RefreshSessionInviteCode();
    void RefreshInviteCodeToggleState();
    FString ResolveSessionInviteCodeForDisplay() const;
    void RefreshInviteSteamState();

    void OnBackBoundActionTriggered();

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_MainMenu = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Options = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_BugFeedback = nullptr;
	
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonListView> CommonListView_VoiceRoster = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonTextBlock> CommonTextBlock_InviteCode = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_ToggleInviteCode = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_InviteSteam = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|GameMenu|Invite")
    FText InviteCodeHiddenMaskText = FText::FromString(TEXT("*****"));

    UPROPERTY(EditDefaultsOnly, Category = "EMR|GameMenu|Invite")
    FText InviteCodeShowButtonText = FText::FromString(TEXT("Show Code"));

    UPROPERTY(EditDefaultsOnly, Category = "EMR|GameMenu|Invite")
    FText InviteCodeHideButtonText = FText::FromString(TEXT("Hide Code"));
	
    UPROPERTY(EditDefaultsOnly, Category = "EMR|GameMenu|Options", meta = (Categories = "EMR.UI.WidgetStack"))
    FGameplayTag OptionsScreenStackTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|GameMenu|Options")
    TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> OptionsScreenWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|GameMenu", meta = (Categories = "EMR.UI.WidgetStack"))
    FGameplayTag GameMenuStackTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|GameMenu|MainMenu")
    FText MainMenuConfirmTitle;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|GameMenu|MainMenu")
    FText MainMenuConfirmMessage;

    FUIActionBindingHandle BackActionHandle;
    FDelegateHandle VoiceRosterChangedHandle;
    FDelegateHandle VoiceMicActivityChangedHandle;
    FDelegateHandle SessionInviteCodeChangedHandle;
    bool bIsInviteCodeVisible = false;
    FString CachedInviteCodeForDisplay;

    TWeakObjectPtr<AEMRNightShiftGameState> BoundRunGameState;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UEMRVoiceRosterEntryData>> VoiceRosterItems;
};
