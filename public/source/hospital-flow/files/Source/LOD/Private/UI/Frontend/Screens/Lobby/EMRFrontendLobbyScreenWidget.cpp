#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyScreenWidget.h"

#include "CommonTextBlock.h"
#include "Characters/Player/EMRPlayerState.h"
#include "Components/EditableTextBox.h"
#include "Framework/EMRLobbyGameState.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerState.h"
#include "GAS/EMRTags.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyPlayerController.h"
#include "Subsystems/EMRLobbySessionSubsystem.h"
#include "Subsystems/EMRLobbyCharacterSelectionSubsystem.h"
#include "Utils/EMREndSessionTrace.h"


UEMRFrontendLobbyScreenWidget::UEMRFrontendLobbyScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SocialsScreenStackTag = EMRTags::UI::WidgetStack::Frontend;
	CharactersScreenStackTag = EMRTags::UI::WidgetStack::Frontend;
}

void UEMRFrontendLobbyScreenWidget::FocusCharactersButton()
{
	if (CommonButton_Characters)
	{
		PendingFocusWidget = CommonButton_Characters;
	}
}

void UEMRFrontendLobbyScreenWidget::FocusInviteFriendsButton()
{
	if (CommonButton_InviteFriends)
	{
		PendingFocusWidget = CommonButton_InviteFriends;
	}
}


void UEMRFrontendLobbyScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.NativeOnInitialized widget=%s world=%s owningPlayer=%s local=%d authority=%d"),
		*GetNameSafe(this),
		GetWorld() ? *GetWorld()->GetMapName() : TEXT("<null>"),
		*GetNameSafe(GetOwningPlayer()),
		(GetOwningPlayer() && GetOwningPlayer()->IsLocalController()) ? 1 : 0,
		(GetOwningPlayer() && GetOwningPlayer()->HasAuthority()) ? 1 : 0);
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.NativeOnInitialized controllerClass=%s playerState=%s"),
		GetOwningPlayer() ? *GetNameSafe(GetOwningPlayer()->GetClass()) : TEXT("<null>"),
		GetOwningPlayer() ? *GetNameSafe(GetOwningPlayer()->PlayerState) : TEXT("<null>"));

	BackActionHandle = RegisterUIActionBinding(FBindUIActionArgs(ICommonInputModule::GetSettings().GetDefaultBackAction(),
		true,
		FSimpleDelegate::CreateUObject(this, &ThisClass::OnBackBoundActionTriggered)));

	if (CommonButton_InviteFriends)
	{
		CommonButton_InviteFriends->OnClicked().RemoveAll(this);
		CommonButton_InviteFriends->OnClicked().AddUObject(this, &ThisClass::HandleSocialsButtonClicked);
	}

	if (CommonButton_Characters)
	{
		CommonButton_Characters->OnClicked().RemoveAll(this);
		CommonButton_Characters->OnClicked().AddUObject(this, &ThisClass::HandleCharactersButtonClicked);
	}

	if (CommonButton_Ready)
	{
		CommonButton_Ready->OnClicked().RemoveAll(this);
		CommonButton_Ready->OnClicked().AddUObject(this, &ThisClass::HandleReadyButtonClicked);
	}

	if (CommonButton_StartGame)
	{
		CommonButton_StartGame->OnClicked().RemoveAll(this);
		CommonButton_StartGame->OnClicked().AddUObject(this, &ThisClass::HandleStartGameClicked);
	}

	if (CommonButton_ValidateInviteCode)
	{
		CommonButton_ValidateInviteCode->OnClicked().RemoveAll(this);
		CommonButton_ValidateInviteCode->OnClicked().AddUObject(this, &ThisClass::HandleValidateInviteCodeClicked);
	}

	if (CommonButton_RegenerateInviteCode)
	{
		CommonButton_RegenerateInviteCode->OnClicked().RemoveAll(this);
		CommonButton_RegenerateInviteCode->OnClicked().AddUObject(this, &ThisClass::HandleRegenerateInviteCodeClicked);
	}
	

	if (AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr)
	{
		LobbyGameState->OnLobbyInviteCodeChanged.AddUObject(this, &ThisClass::RefreshInviteCodeDisplay);
		RefreshInviteCodeDisplay(LobbyGameState->GetLobbyInviteCode());
	}

	if (CommonButton_RegenerateInviteCode)
	{
		const bool bIsHost = GetOwningPlayer() && GetOwningPlayer()->HasAuthority();
		CommonButton_RegenerateInviteCode->SetIsEnabled(bIsHost);
	}

	BindToLobbyGameState();
	RefreshStartGameState();

	if (UEMRLobbyCharacterSelectionSubsystem* SelectionSubsystem = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UEMRLobbyCharacterSelectionSubsystem>() : nullptr)
	{
		SelectionSubsystem->ApplySavedSelectionToPlayerState();
	}

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.NativeOnInitialized completed widget=%s active=%d visibility=%d inViewport=%d"),
		*GetNameSafe(this),
		IsActivated() ? 1 : 0,
		static_cast<int32>(GetVisibility()),
		IsInViewport() ? 1 : 0);
}

void UEMRFrontendLobbyScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.NativeOnActivated widget=%s owningPlayer=%s active=%d visibility=%d inViewport=%d"),
		*GetNameSafe(this),
		*GetNameSafe(GetOwningPlayer()),
		IsActivated() ? 1 : 0,
		static_cast<int32>(GetVisibility()),
		IsInViewport() ? 1 : 0);
}

void UEMRFrontendLobbyScreenWidget::NativeOnDeactivated()
{
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.NativeOnDeactivated widget=%s owningPlayer=%s active=%d visibility=%d inViewport=%d"),
		*GetNameSafe(this),
		*GetNameSafe(GetOwningPlayer()),
		IsActivated() ? 1 : 0,
		static_cast<int32>(GetVisibility()),
		IsInViewport() ? 1 : 0);

	Super::NativeOnDeactivated();
}

UWidget* UEMRFrontendLobbyScreenWidget::NativeGetDesiredFocusTarget() const
{
	if (PendingFocusWidget.IsValid())
	{
		UWidget* PendingWidget = PendingFocusWidget.Get();
		PendingFocusWidget.Reset();
		return PendingWidget;
	}

	if (CommonButton_InviteFriends)
	{
		return CommonButton_InviteFriends;
	}

	return Super::NativeGetDesiredFocusTarget();
}

void UEMRFrontendLobbyScreenWidget::NativeDestruct()
{
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.NativeDestruct widget=%s world=%s"),
		*GetNameSafe(this),
		GetWorld() ? *GetWorld()->GetMapName() : TEXT("<null>"));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LobbyGameStateBindRetryHandle);
	}
	LobbyGameStateBindRetryCount = 0;

	if (BoundLobbyGameState.IsValid())
	{
		BoundLobbyGameState->OnLobbyPlayerStateAdded.RemoveAll(this);
		BoundLobbyGameState->OnLobbyPlayerStateRemoved.RemoveAll(this);
	}

	for (const TWeakObjectPtr<AEMRPlayerState>& PlayerState : BoundLobbyPlayerStates)
	{
		if (AEMRPlayerState* EMRPlayerState = PlayerState.Get())
		{
			EMRPlayerState->OnLobbyPlayerInfoChanged.RemoveDynamic(this, &ThisClass::HandleLobbyPlayerInfoChanged);
		}
	}

	BoundLobbyPlayerStates.Reset();
	BoundLobbyGameState.Reset();

	Super::NativeDestruct();
}


void UEMRFrontendLobbyScreenWidget::HandleSocialsButtonClicked()
{
	if (SocialsScreenWidgetClass.IsNull())
	{
		return;
	}

	UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this);
	if (!UIManagerSubsystem)
	{
		return;
	}

	if (LastPushedSocialsWidget.IsValid())
	{
		UEMRFrontendCommonActivatableWidgetBase* ExistingWidget = LastPushedSocialsWidget.Get();
		if (ExistingWidget->IsActivated())
		{
			UIManagerSubsystem->RemoveWidgetFromStack(SocialsScreenStackTag, ExistingWidget);
			LastPushedSocialsWidget = nullptr;
			return;
		}

		LastPushedSocialsWidget = nullptr;
	}

	UIManagerSubsystem->PushSoftWidgetToStackAsync(
		SocialsScreenStackTag,
		SocialsScreenWidgetClass,
		[this](EAsyncPushWidgetState PushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
		{
			if (PushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				LastPushedSocialsWidget = PushedWidget;
				OnSocialsWidgetPushed.Broadcast(PushedWidget);
			}
		});
}

void UEMRFrontendLobbyScreenWidget::HandleCharactersButtonClicked()
{
	if (CharactersScreenWidgetClass.IsNull())
	{
		return;
	}

	UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this);
	if (!UIManagerSubsystem)
	{
		return;
	}

	if (LastPushedCharactersWidget.IsValid())
	{
		UEMRFrontendCommonActivatableWidgetBase* ExistingWidget = LastPushedCharactersWidget.Get();
		if (ExistingWidget->IsActivated())
		{
			UIManagerSubsystem->RemoveWidgetFromStack(CharactersScreenStackTag, ExistingWidget);
			LastPushedCharactersWidget = nullptr;
			return;
		}

		LastPushedCharactersWidget = nullptr;
	}

	UIManagerSubsystem->PushSoftWidgetToStackAsync(
		CharactersScreenStackTag,
		CharactersScreenWidgetClass,
		[this](EAsyncPushWidgetState PushState, UEMRFrontendCommonActivatableWidgetBase* PushedWidget)
		{
			if (PushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				LastPushedCharactersWidget = PushedWidget;
			}
		});
}

void UEMRFrontendLobbyScreenWidget::HandleReadyButtonClicked()
{
	if (AEMRFrontendLobbyPlayerController* FrontendPlayerController = GetOwningPlayer<AEMRFrontendLobbyPlayerController>())
	{
		FrontendPlayerController->ToggleLobbyReady();
	}
}

void UEMRFrontendLobbyScreenWidget::HandleStartGameClicked()
{
	if (AEMRFrontendLobbyPlayerController* FrontendPlayerController = GetOwningPlayer<AEMRFrontendLobbyPlayerController>())
	{
		FrontendPlayerController->RequestStartGame();
	}
}

void UEMRFrontendLobbyScreenWidget::HandleValidateInviteCodeClicked()
{
	if (!EditableTextBox_InviteCode)
	{
		return;
	}

	const FString Code = EditableTextBox_InviteCode->GetText().ToString();
	if (AEMRFrontendLobbyPlayerController* FrontendPlayerController = GetOwningPlayer<AEMRFrontendLobbyPlayerController>())
	{
		FrontendPlayerController->RequestJoinLobbyByInviteCode(Code);
	}
}

void UEMRFrontendLobbyScreenWidget::HandleRegenerateInviteCodeClicked()
{
	if (AEMRFrontendLobbyPlayerController* FrontendPlayerController = GetOwningPlayer<AEMRFrontendLobbyPlayerController>())
	{
		FrontendPlayerController->RequestRegenerateLobbyInviteCode();
	}
}

void UEMRFrontendLobbyScreenWidget::HandleBackToMenuClicked()
{
	UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this);
	if (!UIManagerSubsystem)
	{
		return;
	}

	const FText FallbackTitle = FText::FromString(TEXT("Return to Main Menu"));
	const FText FallbackMessage = FText::FromString(TEXT("Do you want to leave the lobby and return to the main menu?"));
	const FText Title = BackConfirmTitle.IsEmpty() ? FallbackTitle : BackConfirmTitle;
	const FText Message = BackConfirmMessage.IsEmpty() ? FallbackMessage : BackConfirmMessage;

	UIManagerSubsystem->PushConfirmScreenToModalStackAsync(
		EConfirmScreenType::YesNo,
		Title,
		Message,
		[this](EConfirmScreenButtonType ClickedButtonType)
		{
			if (ClickedButtonType != EConfirmScreenButtonType::Confirmed)
			{
				return;
			}

			if (UEMRLobbySessionSubsystem* LobbySessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbySessionSubsystem>() : nullptr)
			{
				LobbySessionSubsystem->ReturnToFrontend(GetOwningPlayer());
			}
		});
}

void UEMRFrontendLobbyScreenWidget::OnBackBoundActionTriggered()
{
	HandleBackToMenuClicked();
}

void UEMRFrontendLobbyScreenWidget::HandleLobbyPlayerInfoChanged(const FLobbyPlayerInfo& Info)
{
	RefreshStartGameState();
}

void UEMRFrontendLobbyScreenWidget::HandleLobbyPlayerStateAdded(APlayerState* PlayerState)
{
	AEMRPlayerState* EMRPlayerState = Cast<AEMRPlayerState>(PlayerState);
	if (!EMRPlayerState)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.HandleLobbyPlayerStateAdded ignored non-EMR player=%s"),
			*GetNameSafe(PlayerState));
		return;
	}

	if (BoundLobbyPlayerStates.Contains(EMRPlayerState))
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.HandleLobbyPlayerStateAdded duplicate player=%s"),
			*GetNameSafe(EMRPlayerState));
		return;
	}

	BoundLobbyPlayerStates.Add(EMRPlayerState);
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.HandleLobbyPlayerStateAdded player=%s trackedPlayers=%d"),
		*GetNameSafe(EMRPlayerState),
		BoundLobbyPlayerStates.Num());
	EMRPlayerState->OnLobbyPlayerInfoChanged.AddDynamic(this, &ThisClass::HandleLobbyPlayerInfoChanged);
	RefreshStartGameState();
}

void UEMRFrontendLobbyScreenWidget::HandleLobbyPlayerStateRemoved(APlayerState* PlayerState)
{
	AEMRPlayerState* EMRPlayerState = Cast<AEMRPlayerState>(PlayerState);
	if (!EMRPlayerState)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.HandleLobbyPlayerStateRemoved ignored non-EMR player=%s"),
			*GetNameSafe(PlayerState));
		return;
	}

	EMRPlayerState->OnLobbyPlayerInfoChanged.RemoveDynamic(this, &ThisClass::HandleLobbyPlayerInfoChanged);
	BoundLobbyPlayerStates.Remove(EMRPlayerState);
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.HandleLobbyPlayerStateRemoved player=%s trackedPlayers=%d"),
		*GetNameSafe(EMRPlayerState),
		BoundLobbyPlayerStates.Num());
	RefreshStartGameState();
}

void UEMRFrontendLobbyScreenWidget::BindToLobbyGameState()
{
	AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr;
	if (!LobbyGameState)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.BindToLobbyGameState missing LobbyGameState world=%s"),
			GetWorld() ? *GetWorld()->GetMapName() : TEXT("<null>"));
		ScheduleBindToLobbyGameStateRetry();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LobbyGameStateBindRetryHandle);
	}
	LobbyGameStateBindRetryCount = 0;

	if (BoundLobbyGameState.Get() != LobbyGameState)
	{
		if (BoundLobbyGameState.IsValid())
		{
			BoundLobbyGameState->OnLobbyPlayerStateAdded.RemoveAll(this);
			BoundLobbyGameState->OnLobbyPlayerStateRemoved.RemoveAll(this);
		}

		BoundLobbyGameState = LobbyGameState;
		LobbyGameState->OnLobbyPlayerStateAdded.AddUObject(this, &ThisClass::HandleLobbyPlayerStateAdded);
		LobbyGameState->OnLobbyPlayerStateRemoved.AddUObject(this, &ThisClass::HandleLobbyPlayerStateRemoved);
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.BindToLobbyGameState bound GameState=%s players=%d"),
			*GetNameSafe(LobbyGameState),
			LobbyGameState->PlayerArray.Num());
	}

	for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
	{
		HandleLobbyPlayerStateAdded(PlayerState);
	}
}

void UEMRFrontendLobbyScreenWidget::ScheduleBindToLobbyGameStateRetry()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	constexpr int32 MaxRetries = 100;
	if (LobbyGameStateBindRetryCount >= MaxRetries)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.BindToLobbyGameState retry timeout"));
		return;
	}

	++LobbyGameStateBindRetryCount;
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.BindToLobbyGameState retry scheduled count=%d"),
		LobbyGameStateBindRetryCount);
	World->GetTimerManager().SetTimer(
		LobbyGameStateBindRetryHandle,
		this,
		&ThisClass::BindToLobbyGameState,
		0.1f,
		false);
}

void UEMRFrontendLobbyScreenWidget::RefreshStartGameState()
{
	if (!CommonButton_StartGame)
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.RefreshStartGameState missing StartGame button"));
		return;
	}

	const bool bIsHost = GetOwningPlayer() && GetOwningPlayer()->HasAuthority();
	CommonButton_StartGame->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.RefreshStartGameState host=%d visibility=%d world=%s"),
		bIsHost ? 1 : 0,
		static_cast<int32>(CommonButton_StartGame->GetVisibility()),
		GetWorld() ? *GetWorld()->GetMapName() : TEXT("<null>"));
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.RefreshStartGameState owner=%s ownerClass=%s ownerState=%s widgetVisible=%d widgetInViewport=%d active=%d"),
		*GetNameSafe(GetOwningPlayer()),
		GetOwningPlayer() ? *GetNameSafe(GetOwningPlayer()->GetClass()) : TEXT("<null>"),
		GetOwningPlayer() ? *GetNameSafe(GetOwningPlayer()->PlayerState) : TEXT("<null>"),
		static_cast<int32>(GetVisibility()),
		IsInViewport() ? 1 : 0,
		IsActivated() ? 1 : 0);

	if (!bIsHost)
	{
		return;
	}

	if (!BoundLobbyGameState.IsValid())
	{
		BindToLobbyGameState();
	}

	const bool bReadyToStart = AreNonHostPlayersReady();
	CommonButton_StartGame->SetIsEnabled(bReadyToStart);
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyScreen.RefreshStartGameState host enabled=%d trackedPlayers=%d"),
		bReadyToStart ? 1 : 0,
		BoundLobbyPlayerStates.Num());
}

bool UEMRFrontendLobbyScreenWidget::AreNonHostPlayersReady() const
{
	AEMRLobbyGameState* LobbyGameState = BoundLobbyGameState.IsValid()
	? BoundLobbyGameState.Get()
	: (GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr);
	if (!LobbyGameState)
	{
		return false;
	}

	APlayerController* OwningController = GetOwningPlayer();
	APlayerState* HostPlayerState = OwningController ? OwningController->PlayerState : nullptr;

	for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
	{
		if (!PlayerState || PlayerState == HostPlayerState)
		{
			continue;
		}

		const AEMRPlayerState* EMRPlayerState = Cast<AEMRPlayerState>(PlayerState);
		if (!EMRPlayerState || !EMRPlayerState->IsLobbyReady())
		{
			return false;
		}
	}

	return true;
}

void UEMRFrontendLobbyScreenWidget::RefreshInviteCodeDisplay(const FString& NewCode)
{
	if (!CommonTextBlock_InviteCode)
	{
		return;
	}

	CommonTextBlock_InviteCode->SetText(FText::FromString(NewCode));
}

