#include "UI/Common/Screens/GameMenu/EMRGameplayMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Characters/Player/EMRPlayerController.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Engine/LocalPlayer.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/EMRTags.h"
#include "GameFramework/PlayerController.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "OnlineSubsystem.h"
#include "ProximityVoiceLocalPlayerSubsystem.h"
#include "Subsystems/EMRLobbyInviteCodeSubsystem.h"
#include "Subsystems/EMRLobbySessionSubsystem.h"
#include "UI/Common/Screens/GameMenu/EMRVoiceRosterEntryData.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/Core/EMRFrontendCommonListView.h"

#include "AdvancedSteamFriendsLibrary.h"

namespace
{
	bool IsSteamInviteRuntimeAvailable(UWorld* World)
	{
		IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(World);
		if (!OnlineSubsystem || OnlineSubsystem->GetSubsystemName() != FName(TEXT("STEAM")))
		{
			return false;
		}

		return UAdvancedSteamFriendsLibrary::IsOverlayEnabled();
	}
}

UEMRGameplayMenuWidget::UEMRGameplayMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    OptionsScreenStackTag = EMRTags::UI::WidgetStack::GameMenu;
    GameMenuStackTag = EMRTags::UI::WidgetStack::GameMenu;
}

void UEMRGameplayMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    BackActionHandle = RegisterUIActionBinding(FBindUIActionArgs(ICommonInputModule::GetSettings().GetDefaultBackAction(),
        true,
        FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)));

    if (CommonButton_MainMenu)
    {
        CommonButton_MainMenu->OnClicked().RemoveAll(this);
        CommonButton_MainMenu->OnClicked().AddUObject(this, &ThisClass::HandleMainMenuClicked);
    }

    if (CommonButton_Options)
    {
        CommonButton_Options->OnClicked().RemoveAll(this);
        CommonButton_Options->OnClicked().AddUObject(this, &ThisClass::HandleOptionsClicked);
    }

    if (CommonButton_BugFeedback)
    {
        CommonButton_BugFeedback->OnClicked().RemoveAll(this);
        CommonButton_BugFeedback->OnClicked().AddUObject(this, &ThisClass::HandleBugFeedbackClicked);
    }

    if (CommonButton_InviteSteam)
    {
        CommonButton_InviteSteam->OnClicked().RemoveAll(this);
        CommonButton_InviteSteam->OnClicked().AddUObject(this, &ThisClass::HandleInviteSteamClicked);
    }

    if (CommonButton_ToggleInviteCode)
    {
        CommonButton_ToggleInviteCode->OnClicked().RemoveAll(this);
        CommonButton_ToggleInviteCode->OnClicked().AddUObject(this, &ThisClass::HandleToggleInviteCodeClicked);
    }
}

void UEMRGameplayMenuWidget::NativeOnActivated()
{
    Super::NativeOnActivated();

    bIsInviteCodeVisible = false;
    CachedInviteCodeForDisplay.Reset();
    BindVoiceDelegates();
    BindInviteCodeDelegates();
    RefreshSessionInviteCode();
    RefreshInviteSteamState();
}

void UEMRGameplayMenuWidget::NativeOnDeactivated()
{
    UnbindInviteCodeDelegates();
    UnbindVoiceDelegates();
    Super::NativeOnDeactivated();
}

TArray<FProximityVoiceRemotePlayerView> UEMRGameplayMenuWidget::GetVoiceRemotePlayers() const
{
    if (UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = ResolveVoiceSubsystem())
    {
        return VoiceSubsystem->GetRemotePlayersView();
    }

    return TArray<FProximityVoiceRemotePlayerView>();
}

bool UEMRGameplayMenuWidget::IsLocalMicActivityDetected() const
{
    if (UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = ResolveVoiceSubsystem())
    {
        return VoiceSubsystem->IsLocalMicActive();
    }

    return false;
}

void UEMRGameplayMenuWidget::SetRemotePlayerVoiceVolume(const FString& RemotePlayerId, float InVolume)
{
    if (UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = ResolveVoiceSubsystem())
    {
        VoiceSubsystem->SetRemotePlayerVolume(RemotePlayerId, InVolume);
    }
}

void UEMRGameplayMenuWidget::SetRemotePlayerVoiceMuted(const FString& RemotePlayerId, bool bMuted)
{
    if (UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = ResolveVoiceSubsystem())
    {
        VoiceSubsystem->SetRemotePlayerMuted(RemotePlayerId, bMuted);
    }
}

UProximityVoiceLocalPlayerSubsystem* UEMRGameplayMenuWidget::ResolveVoiceSubsystem() const
{
    if (const APlayerController* OwningPlayer = GetOwningPlayer())
    {
        if (ULocalPlayer* LocalPlayer = OwningPlayer->GetLocalPlayer())
        {
            return LocalPlayer->GetSubsystem<UProximityVoiceLocalPlayerSubsystem>();
        }
    }

    return nullptr;
}

AEMRNightShiftGameState* UEMRGameplayMenuWidget::ResolveRunGameState() const
{
    return GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
}

void UEMRGameplayMenuWidget::BindVoiceDelegates()
{
    UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = ResolveVoiceSubsystem();
    if (!VoiceSubsystem)
    {
        return;
    }

    if (!VoiceRosterChangedHandle.IsValid())
    {
        VoiceRosterChangedHandle = VoiceSubsystem->OnVoiceRosterChanged().AddUObject(this, &ThisClass::HandleVoiceRosterChanged);
    }

    if (!VoiceMicActivityChangedHandle.IsValid())
    {
        VoiceMicActivityChangedHandle = VoiceSubsystem->OnLocalMicActivityChanged().AddUObject(this, &ThisClass::HandleLocalMicActivityChanged);
    }

    HandleVoiceRosterChanged();
    HandleLocalMicActivityChanged(VoiceSubsystem->IsLocalMicActive());
}

void UEMRGameplayMenuWidget::UnbindVoiceDelegates()
{
    UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = ResolveVoiceSubsystem();
    if (!VoiceSubsystem)
    {
        return;
    }

    if (VoiceRosterChangedHandle.IsValid())
    {
        VoiceSubsystem->OnVoiceRosterChanged().Remove(VoiceRosterChangedHandle);
        VoiceRosterChangedHandle.Reset();
    }

    if (VoiceMicActivityChangedHandle.IsValid())
    {
        VoiceSubsystem->OnLocalMicActivityChanged().Remove(VoiceMicActivityChangedHandle);
        VoiceMicActivityChangedHandle.Reset();
    }
}

void UEMRGameplayMenuWidget::BindInviteCodeDelegates()
{
    AEMRNightShiftGameState* RunGameState = ResolveRunGameState();
    if (!RunGameState)
    {
        UnbindInviteCodeDelegates();
        return;
    }

    if (BoundRunGameState.Get() != RunGameState)
    {
        UnbindInviteCodeDelegates();
        BoundRunGameState = RunGameState;
    }

    if (!SessionInviteCodeChangedHandle.IsValid())
    {
        SessionInviteCodeChangedHandle = RunGameState->OnSessionInviteCodeChanged().AddUObject(this, &ThisClass::HandleRunSessionInviteCodeChanged);
    }
}

void UEMRGameplayMenuWidget::UnbindInviteCodeDelegates()
{
    if (SessionInviteCodeChangedHandle.IsValid())
    {
        if (AEMRNightShiftGameState* RunGameState = BoundRunGameState.Get())
        {
            RunGameState->OnSessionInviteCodeChanged().Remove(SessionInviteCodeChangedHandle);
        }

        SessionInviteCodeChangedHandle.Reset();
    }

    BoundRunGameState.Reset();
}

void UEMRGameplayMenuWidget::RefreshSessionInviteCode()
{
    if (!BoundRunGameState.IsValid())
    {
        BindInviteCodeDelegates();
    }

    if (!CommonTextBlock_InviteCode)
    {
        RefreshInviteCodeToggleState();
        return;
    }

    const FString InviteCodeValue = ResolveSessionInviteCodeForDisplay();
    if (!InviteCodeValue.IsEmpty())
    {
        CachedInviteCodeForDisplay = InviteCodeValue;
    }

    const FString CodeToDisplay = !InviteCodeValue.IsEmpty() ? InviteCodeValue : CachedInviteCodeForDisplay;
    if (!bIsInviteCodeVisible)
    {
        CommonTextBlock_InviteCode->SetText(InviteCodeHiddenMaskText);
    }
    else if (CodeToDisplay.IsEmpty())
    {
        // Avoid rendering a blank field while waiting for the replicated/session value.
        CommonTextBlock_InviteCode->SetText(InviteCodeHiddenMaskText);
    }
    else
    {
        CommonTextBlock_InviteCode->SetText(FText::FromString(CodeToDisplay));
    }

    RefreshInviteCodeToggleState();
}

void UEMRGameplayMenuWidget::RefreshInviteCodeToggleState()
{
    if (!CommonButton_ToggleInviteCode)
    {
        return;
    }

    CommonButton_ToggleInviteCode->SetButtonText(
        bIsInviteCodeVisible ? InviteCodeHideButtonText : InviteCodeShowButtonText);
}

void UEMRGameplayMenuWidget::RefreshInviteSteamState()
{
    if (!CommonButton_InviteSteam)
    {
        return;
    }

    CommonButton_InviteSteam->SetVisibility(ESlateVisibility::Visible);
    CommonButton_InviteSteam->SetIsEnabled(IsSteamInviteRuntimeAvailable(GetWorld()));
}

void UEMRGameplayMenuWidget::HandleVoiceRosterChanged()
{
    RebuildVoiceRosterWidgets();
    BP_OnVoiceRosterUpdated();
}

void UEMRGameplayMenuWidget::HandleLocalMicActivityChanged(bool bIsMicActive)
{
    BP_OnLocalMicActivityChanged(bIsMicActive);
}

void UEMRGameplayMenuWidget::HandleRunSessionInviteCodeChanged(const FString& NewInviteCode)
{
    (void)NewInviteCode;
    RefreshSessionInviteCode();
}

FString UEMRGameplayMenuWidget::ResolveSessionInviteCodeForDisplay() const
{
    const AEMRNightShiftGameState* RunGameState = ResolveRunGameState();
    FString InviteCodeValue = RunGameState ? RunGameState->GetSessionInviteCode().TrimStartAndEnd() : FString();
    if (!InviteCodeValue.IsEmpty())
    {
        return InviteCodeValue;
    }

    if (const UEMRLobbyInviteCodeSubsystem* InviteCodeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbyInviteCodeSubsystem>() : nullptr)
    {
        FString SessionSettingsInviteCode;
        if (InviteCodeSubsystem->GetCurrentSessionInviteCode(SessionSettingsInviteCode))
        {
            return SessionSettingsInviteCode;
        }
    }

    return FString();
}

void UEMRGameplayMenuWidget::HandleToggleInviteCodeClicked()
{
    bIsInviteCodeVisible = !bIsInviteCodeVisible;

    if (bIsInviteCodeVisible)
    {
        const FString InviteCodeValue = ResolveSessionInviteCodeForDisplay();
        if (InviteCodeValue.IsEmpty())
        {
            const AEMRNightShiftGameState* RunGameState = ResolveRunGameState();
            const FString GameStateCode = RunGameState ? RunGameState->GetSessionInviteCode().TrimStartAndEnd() : FString();
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[GameMenuInviteCode] Reveal requested but invite code is unavailable. RunGameState=%s GameStateCodeLen=%d"),
                *GetNameSafe(RunGameState),
                GameStateCode.Len());
        }
    }

    RefreshSessionInviteCode();
}

void UEMRGameplayMenuWidget::RebuildVoiceRosterWidgets()
{
    if (!CommonListView_VoiceRoster)
    {
        return;
    }

    UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = ResolveVoiceSubsystem();
    if (!VoiceSubsystem)
    {
        VoiceRosterItems.Reset();
        CommonListView_VoiceRoster->ClearListItems();
        CommonListView_VoiceRoster->RequestRefresh();
        return;
    }

    const TArray<FProximityVoiceRemotePlayerView> RemotePlayers = VoiceSubsystem->GetRemotePlayersView();
    TMap<FString, UEMRVoiceRosterEntryData*> ExistingItemsByPlayerId;
    ExistingItemsByPlayerId.Reserve(VoiceRosterItems.Num());
    for (UEMRVoiceRosterEntryData* ExistingItem : VoiceRosterItems)
    {
        if (!ExistingItem)
        {
            continue;
        }

        ExistingItemsByPlayerId.Add(ExistingItem->GetPlayerId(), ExistingItem);
    }

    TArray<TObjectPtr<UEMRVoiceRosterEntryData>> UpdatedVoiceRosterItems;
    UpdatedVoiceRosterItems.Reserve(RemotePlayers.Num());
    TArray<UObject*> ListItems;
    ListItems.Reserve(RemotePlayers.Num());
    bool bNeedsListRebuild = VoiceRosterItems.Num() != RemotePlayers.Num();

    for (const FProximityVoiceRemotePlayerView& PlayerView : RemotePlayers)
    {
        UEMRVoiceRosterEntryData* EntryData = ExistingItemsByPlayerId.FindRef(PlayerView.PlayerId);
        if (EntryData)
        {
            EntryData->UpdateFromPlayerView(PlayerView);
        }
        else
        {
            EntryData = NewObject<UEMRVoiceRosterEntryData>(this);
            EntryData->Initialize(VoiceSubsystem, PlayerView);
            bNeedsListRebuild = true;
        }

        UpdatedVoiceRosterItems.Add(EntryData);
        ListItems.Add(EntryData);
    }

    if (!bNeedsListRebuild)
    {
        if (VoiceRosterItems.Num() != UpdatedVoiceRosterItems.Num())
        {
            bNeedsListRebuild = true;
        }
        else
        {
            for (int32 ItemIndex = 0; ItemIndex < VoiceRosterItems.Num(); ++ItemIndex)
            {
                if (VoiceRosterItems[ItemIndex] != UpdatedVoiceRosterItems[ItemIndex])
                {
                    bNeedsListRebuild = true;
                    break;
                }
            }
        }
    }

    VoiceRosterItems = MoveTemp(UpdatedVoiceRosterItems);

    if (bNeedsListRebuild)
    {
        CommonListView_VoiceRoster->SetListItems(ListItems);
        CommonListView_VoiceRoster->RequestRefresh();
    }
}


void UEMRGameplayMenuWidget::HandleMainMenuClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[GameMenu] MainMenu clicked. OwnerPC=%s"), *GetNameSafe(GetOwningPlayer()));

    if (UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        if (!UIManagerSubsystem->GetPrimaryLayoutWidget())
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMenu] MainMenu click: PrimaryLayoutWidget is null; confirm screen may not appear."));
        }

        UE_LOG(LogTemp, Log, TEXT("[GameMenu] MainMenu click: pushing confirm screen (Title='%s', Message='%s')."),
            *MainMenuConfirmTitle.ToString(),
            *MainMenuConfirmMessage.ToString());

        UIManagerSubsystem->PushConfirmScreenToModalStackAsync(
            EConfirmScreenType::YesNo,
            MainMenuConfirmTitle,
            MainMenuConfirmMessage,
            [this](EConfirmScreenButtonType ClickedButtonType)
            {
                UE_LOG(LogTemp, Log, TEXT("[GameMenu] Confirm result: %d"), static_cast<int32>(ClickedButtonType));

                if (ClickedButtonType != EConfirmScreenButtonType::Confirmed)
                {
                    UE_LOG(LogTemp, Log, TEXT("[GameMenu] Confirm canceled."));
                    return;
                }

                if (AEMRPlayerController* EMRPlayerController = Cast<AEMRPlayerController>(GetOwningPlayer()))
                {
                    UE_LOG(LogTemp, Log, TEXT("[GameMenu] Requesting ReturnToFrontend with telemetry flush."));
                    EMRPlayerController->RequestReturnToFrontendWithNightShiftTelemetry();
                }
                else if (UEMRLobbySessionSubsystem* LobbySessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbySessionSubsystem>() : nullptr)
                {
                    UE_LOG(LogTemp, Log, TEXT("[GameMenu] Returning to frontend via LobbySessionSubsystem (fallback path)."));
                    LobbySessionSubsystem->ReturnToFrontend(GetOwningPlayer());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[GameMenu] LobbySessionSubsystem missing; cannot return to frontend."));
                }
            });
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMenu] MainMenu click: UIManagerSubsystem missing; confirm not pushed."));
    }
}

void UEMRGameplayMenuWidget::HandleOptionsClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[GameMenu] Options clicked. StackTagValid=%d ClassNull=%d"),
        OptionsScreenStackTag.IsValid() ? 1 : 0,
        OptionsScreenWidgetClass.IsNull() ? 1 : 0);

    if (OptionsScreenWidgetClass.IsNull() || !OptionsScreenStackTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMenu] Options click ignored: invalid stack tag or widget class."));
        return;
    }

    if (UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        if (!UIManagerSubsystem->GetPrimaryLayoutWidget())
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMenu] Options click: PrimaryLayoutWidget is null; options may not appear."));
        }

        UE_LOG(LogTemp, Log, TEXT("[GameMenu] Options click: pushing Options screen."));
        UIManagerSubsystem->PushSoftWidgetToStackAsync(
            OptionsScreenStackTag,
            OptionsScreenWidgetClass,
            [](EAsyncPushWidgetState InPushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
            {
                UE_LOG(LogTemp, Log, TEXT("[GameMenu] Options push state=%d widget=%s"),
                    static_cast<int32>(InPushState),
                    *GetNameSafe(PushedWidget));
            });
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMenu] Options click: UIManagerSubsystem missing; options not pushed."));
    }
}

void UEMRGameplayMenuWidget::HandleBugFeedbackClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[GameMenu] Bug/Feedback clicked (no-op)."));
}

void UEMRGameplayMenuWidget::HandleInviteSteamClicked()
{
    APlayerController* OwningPlayer = GetOwningPlayer();
    if (!OwningPlayer)
    {
        return;
    }

    if (!IsSteamInviteRuntimeAvailable(GetWorld()))
    {
        RefreshInviteSteamState();
        return;
    }

    UWorld* World = GetWorld();
    IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(World);
    IOnlineExternalUIPtr ExternalUIInterface = OnlineSubsystem ? OnlineSubsystem->GetExternalUIInterface() : nullptr;
    ULocalPlayer* LocalPlayer = OwningPlayer ? OwningPlayer->GetLocalPlayer() : nullptr;
    if (!ExternalUIInterface.IsValid() || !LocalPlayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMenu] Steam invite UI unavailable (ExternalUI or LocalPlayer missing)."));
        return;
    }

    ExternalUIInterface->ShowInviteUI(LocalPlayer->GetControllerId(), NAME_GameSession);
}

void UEMRGameplayMenuWidget::OnBackBoundActionTriggered()
{
    const bool bVoiceRosterFocused = CommonListView_VoiceRoster
    && (CommonListView_VoiceRoster->HasAnyUserFocus() || CommonListView_VoiceRoster->HasFocusedDescendants());
    if (bVoiceRosterFocused && CommonButton_MainMenu)
    {
        if (APlayerController* OwningPlayer = GetOwningPlayer())
        {
            CommonButton_MainMenu->SetUserFocus(OwningPlayer);
        }

        CommonButton_MainMenu->SetFocus();
        UE_LOG(LogTemp, Log, TEXT("[GameMenu] Back action moved focus from voice roster to MainMenu."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[GameMenu] Back action triggered; removing menu from stack. Tag=%s"), *GameMenuStackTag.ToString());
	
    if (UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this))
    {
        UIManagerSubsystem->RemoveWidgetFromStack(GameMenuStackTag, this);
    }
}
