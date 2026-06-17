#include "UI/Frontend/Screens/MainMenu/EMRFrontendMainMenuScreenWidget.h"

#include "Components/EditableTextBox.h"
#include "Components/Widget.h"
#include "GAS/EMRTags.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LocalizationLibrary.h"
#include "Subsystems/EMRLobbySessionSubsystem.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/EMRFrontendPlayerController.h"

UEMRFrontendMainMenuScreenWidget::UEMRFrontendMainMenuScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    OptionsScreenStackTag = EMRTags::UI::WidgetStack::Frontend;
    CreditsScreenStackTag = EMRTags::UI::WidgetStack::Frontend;
}

void UEMRFrontendMainMenuScreenWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (CommonButton_Lobby)
    {
        CommonButton_Lobby->OnClicked().RemoveAll(this);
        CommonButton_Lobby->OnClicked().AddUObject(this, &ThisClass::HandleHostButtonClicked);
    }

    if (CommonButton_Host)
    {
        CommonButton_Host->OnClicked().RemoveAll(this);
        CommonButton_Host->OnClicked().AddUObject(this, &ThisClass::HandleHostButtonClicked);
    }

    if (CommonButton_JoinGame)
    {
        CommonButton_JoinGame->OnClicked().RemoveAll(this);
        CommonButton_JoinGame->OnClicked().AddUObject(this, &ThisClass::HandleJoinGameButtonClicked);
    }

    if (CommonButton_JoinGameValidate)
    {
        CommonButton_JoinGameValidate->OnClicked().RemoveAll(this);
        CommonButton_JoinGameValidate->OnClicked().AddUObject(this, &ThisClass::HandleJoinGameValidateButtonClicked);
    }

    if (EditableTextBox_InviteCode)
    {
        EditableTextBox_InviteCode->OnTextCommitted.RemoveAll(this);
        EditableTextBox_InviteCode->OnTextCommitted.AddDynamic(this, &ThisClass::HandleJoinGameInviteCodeCommitted);
    }

    if (CommonButton_Options)
    {
        CommonButton_Options->OnClicked().RemoveAll(this);
        CommonButton_Options->OnClicked().AddUObject(this, &ThisClass::HandleOptionsButtonClicked);
    }

    if (CommonButton_Credits)
    {
        CommonButton_Credits->OnClicked().RemoveAll(this);
        CommonButton_Credits->OnClicked().AddUObject(this, &ThisClass::HandleCreditsButtonClicked);
    }

    if (CommonButton_ReportBug)
    {
        CommonButton_ReportBug->OnClicked().RemoveAll(this);
        CommonButton_ReportBug->OnClicked().AddUObject(this, &ThisClass::HandleReportBugButtonClicked);
    }

    if (CommonButton_Quit)
    {
        CommonButton_Quit->OnClicked().RemoveAll(this);
        CommonButton_Quit->OnClicked().AddUObject(this, &ThisClass::HandleQuitButtonClicked);
    }

    SetJoinGamePanelVisible(false);
}

void UEMRFrontendMainMenuScreenWidget::HandleLobbyButtonClicked()
{
    HandleHostButtonClicked();
}

void UEMRFrontendMainMenuScreenWidget::HandleHostButtonClicked()
{
    if (UEMRLobbySessionSubsystem* LobbySessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbySessionSubsystem>() : nullptr)
    {
        const ELobbySessionState CurrentState = LobbySessionSubsystem->GetLobbySessionState();
        if (CurrentState == ELobbySessionState::Creating || CurrentState == ELobbySessionState::Hosting)
        {
            return;
        }

        if (APlayerController* OwningPlayer = GetOwningPlayer())
        {
            LobbySessionSubsystem->HostLobbySession(OwningPlayer);
        }
    }
}

void UEMRFrontendMainMenuScreenWidget::HandleJoinGameButtonClicked()
{
    const bool bShowJoinPanel = !Panel_JoinGame || !Panel_JoinGame->IsVisible();
    SetJoinGamePanelVisible(bShowJoinPanel);
}

void UEMRFrontendMainMenuScreenWidget::HandleJoinGameValidateButtonClicked()
{
    const FString InviteCode = EditableTextBox_InviteCode
    ? EditableTextBox_InviteCode->GetText().ToString().TrimStartAndEnd()
    : FString();

    if (AEMRFrontendPlayerController* FrontendPlayerController = Cast<AEMRFrontendPlayerController>(GetOwningPlayer()))
    {
        FrontendPlayerController->RequestJoinLobbyByInviteCode(InviteCode);
    }
}

void UEMRFrontendMainMenuScreenWidget::HandleJoinGameInviteCodeCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
    if (InCommitType != ETextCommit::OnEnter)
    {
        return;
    }

    HandleJoinGameValidateButtonClicked();
}

void UEMRFrontendMainMenuScreenWidget::HandleOptionsButtonClicked()
{
    if (OptionsScreenWidgetClass.IsNull() || !OptionsScreenStackTag.IsValid())
    {
        return;
    }

    if (UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        UIManagerSubsystem->PushSoftWidgetToStackAsync(
            OptionsScreenStackTag,
            OptionsScreenWidgetClass,
            [](EAsyncPushWidgetState, UEMRFrontendCommonActivatableWidgetBase*)
            {
            });
    }
}

void UEMRFrontendMainMenuScreenWidget::HandleCreditsButtonClicked()
{
    if (CreditsScreenWidgetClass.IsNull() || !CreditsScreenStackTag.IsValid())
    {
        return;
    }

    if (UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        UIManagerSubsystem->PushSoftWidgetToStackAsync(
            CreditsScreenStackTag,
            CreditsScreenWidgetClass,
            [](EAsyncPushWidgetState, UEMRFrontendCommonActivatableWidgetBase*)
            {
            });
    }
}

void UEMRFrontendMainMenuScreenWidget::HandleReportBugButtonClicked()
{
}

void UEMRFrontendMainMenuScreenWidget::HandleQuitButtonClicked()
{
    if (UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        UIManagerSubsystem->PushConfirmScreenToModalStackAsync(
            EConfirmScreenType::YesNo,
            QuitConfirmTitle,
            QuitConfirmMessage,
            [this](EConfirmScreenButtonType ClickedButtonType)
            {
                if (ClickedButtonType == EConfirmScreenButtonType::Confirmed)
                {
                    if (APlayerController* OwningPlayer = GetOwningPlayer())
                    {
                        UKismetSystemLibrary::QuitGame(this, OwningPlayer, EQuitPreference::Quit, true);
                    }
                }
            });
    }
}

void UEMRFrontendMainMenuScreenWidget::SetJoinGamePanelVisible(const bool bVisible)
{
    if (!Panel_JoinGame)
    {
        return;
    }

    Panel_JoinGame->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
