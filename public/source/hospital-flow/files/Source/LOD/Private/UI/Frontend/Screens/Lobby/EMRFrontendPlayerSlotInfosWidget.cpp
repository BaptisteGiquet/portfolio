#include "UI/Frontend/Screens/Lobby/EMRFrontendPlayerSlotInfosWidget.h"

#include "CommonTextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyPlayerController.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"

void UEMRFrontendPlayerSlotInfosWidget::SetPlayerInfo(const FText& PlayerName, bool bIsReady)
{
    if (CommonTextBlock_PlayerName)
    {
        CommonTextBlock_PlayerName->SetText(PlayerName);
    }

    if (CommonTextBlock_ReadyStatus)
    {
        CommonTextBlock_ReadyStatus->SetText(bIsReady ? ReadyText : NotReadyText);
    }

    RefreshKickVisibility();
}

void UEMRFrontendPlayerSlotInfosWidget::ClearPlayerInfo()
{
    if (CommonTextBlock_PlayerName)
    {
        CommonTextBlock_PlayerName->SetText(FText::GetEmpty());
    }

    if (CommonTextBlock_ReadyStatus)
    {
        CommonTextBlock_ReadyStatus->SetText(FText::GetEmpty());
    }

    AssignedPlayerState.Reset();
    RefreshKickVisibility();
}

void UEMRFrontendPlayerSlotInfosWidget::SetAssignedPlayerState(APlayerState* InPlayerState)
{
    AssignedPlayerState = InPlayerState;
    RefreshKickVisibility();
}

void UEMRFrontendPlayerSlotInfosWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (CommonButton_Kick)
    {
        CommonButton_Kick->OnClicked().RemoveAll(this);
        CommonButton_Kick->OnClicked().AddUObject(this, &ThisClass::HandleKickClicked);
    }
}

void UEMRFrontendPlayerSlotInfosWidget::RefreshKickVisibility()
{
    if (!CommonButton_Kick)
    {
        return;
    }

    const APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    const bool bIsHost = LocalPC && LocalPC->HasAuthority();
    const bool bIsSelf = LocalPC && AssignedPlayerState.IsValid() && LocalPC->PlayerState == AssignedPlayerState.Get();
    const bool bShouldShow = bIsHost && AssignedPlayerState.IsValid() && !bIsSelf;

    CommonButton_Kick->SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    CommonButton_Kick->SetIsEnabled(bShouldShow);
}

void UEMRFrontendPlayerSlotInfosWidget::HandleKickClicked()
{
    if (!AssignedPlayerState.IsValid())
    {
        return;
    }

    UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this);
    if (!UIManagerSubsystem)
    {
        return;
    }

    UIManagerSubsystem->PushConfirmScreenToModalStackAsync(
        EConfirmScreenType::YesNo,
        KickConfirmTitle,
        KickConfirmMessage,
        [this](EConfirmScreenButtonType ClickedButtonType)
        {
            if (ClickedButtonType != EConfirmScreenButtonType::Confirmed)
            {
                return;
            }

            APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
            if (AEMRFrontendLobbyPlayerController* LobbyPC = Cast<AEMRFrontendLobbyPlayerController>(LocalPC))
            {
                LobbyPC->RequestKickPlayer(AssignedPlayerState.Get());
            }
        });
}
