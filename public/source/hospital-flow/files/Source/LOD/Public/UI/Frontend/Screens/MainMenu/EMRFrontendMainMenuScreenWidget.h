#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/SlateEnums.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "EMRFrontendMainMenuScreenWidget.generated.h"

class UEMRFrontendCommonButtonBase;
class UEMRFrontendCommonActivatableWidgetBase;
class UEditableTextBox;
class UWidget;

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRFrontendMainMenuScreenWidget : public UEMRFrontendCommonActivatableWidgetBase
{
    GENERATED_BODY()

protected:
    UEMRFrontendMainMenuScreenWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeOnInitialized() override;

private:
    UFUNCTION()
    void HandleLobbyButtonClicked();

    UFUNCTION()
    void HandleHostButtonClicked();

    UFUNCTION()
    void HandleJoinGameButtonClicked();

    UFUNCTION()
    void HandleJoinGameValidateButtonClicked();

    UFUNCTION()
    void HandleJoinGameInviteCodeCommitted(const FText& InText, ETextCommit::Type InCommitType);

    UFUNCTION()
    void HandleOptionsButtonClicked();

    UFUNCTION()
    void HandleCreditsButtonClicked();

    UFUNCTION()
    void HandleReportBugButtonClicked();

    UFUNCTION()
    void HandleQuitButtonClicked();

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Lobby;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Host;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_JoinGame;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_JoinGameValidate;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UEditableTextBox> EditableTextBox_InviteCode;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> Panel_JoinGame;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Options;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Credits;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_ReportBug;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Quit;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|MainMenu|Options", meta = (Categories = "EMR.UI.WidgetStack"))
    FGameplayTag OptionsScreenStackTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|MainMenu|Options")
    TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> OptionsScreenWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|MainMenu|Credits", meta = (Categories = "EMR.UI.WidgetStack"))
    FGameplayTag CreditsScreenStackTag;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|MainMenu|Credits")
    TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> CreditsScreenWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|MainMenu|Quit")
    FText QuitConfirmTitle;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|MainMenu|Quit")
    FText QuitConfirmMessage;

    void SetJoinGamePanelVisible(bool bVisible);
};
