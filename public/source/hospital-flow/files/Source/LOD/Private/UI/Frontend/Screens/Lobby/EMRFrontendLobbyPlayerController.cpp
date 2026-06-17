#include "UI/Frontend/Screens/Lobby/EMRFrontendLobbyPlayerController.h"

#include "Camera/CameraActor.h"
#include "Characters/Player/EMRPlayerState.h"
#include "Framework/EMRLobbyGameMode.h"
#include "Framework/EMRLobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "LocalizationLibrary.h"
#include "Subsystems/EMRLobbySessionSubsystem.h"
#include "Subsystems/EMRLobbyInviteCodeSubsystem.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"
#include "UI/Frontend/Utils/DebugHelper.h"
#include "Utils/EMREndSessionTrace.h"


void AEMRFrontendLobbyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPC.OnPossess pc=%s pawn=%s local=%d authority=%d"),
		*GetNameSafe(this),
		*GetNameSafe(InPawn),
		IsLocalController() ? 1 : 0,
		HasAuthority() ? 1 : 0);

	if (IsLocalController())
	{
		TArray<AActor*> FoundCameras;
		UGameplayStatics::GetAllActorsOfClassWithTag(this, ACameraActor::StaticClass(), FName("Default"), FoundCameras);
		if (!FoundCameras.IsEmpty())
		{
			SetViewTarget(FoundCameras[0]);
		}

		if (UEMRLobbySessionSubsystem* LobbySessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbySessionSubsystem>() : nullptr)
		{
			EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPC.OnPossess requesting EnsureLobbyPresentation"));
			LobbySessionSubsystem->EnsureLobbyPresentation();
		}
	}
	
	

}


void AEMRFrontendLobbyPlayerController::AcknowledgePossession(class APawn* P)
{
	Super::AcknowledgePossession(P);

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPC.AcknowledgePossession pc=%s pawn=%s local=%d authority=%d"),
		*GetNameSafe(this),
		*GetNameSafe(P),
		IsLocalController() ? 1 : 0,
		HasAuthority() ? 1 : 0);

	if (IsLocalController())
	{
		TArray<AActor*> FoundCameras;
		UGameplayStatics::GetAllActorsOfClassWithTag(this, ACameraActor::StaticClass(), FName("Default"), FoundCameras);
		if (!FoundCameras.IsEmpty())
		{
			SetViewTarget(FoundCameras[0]);
		}

		if (UEMRLobbySessionSubsystem* LobbySessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbySessionSubsystem>() : nullptr)
		{
			EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPC.AcknowledgePossession requesting EnsureLobbyPresentation"));
			LobbySessionSubsystem->EnsureLobbyPresentation();
		}
	}
}

void AEMRFrontendLobbyPlayerController::ToggleLobbyReady()
{
	AEMRPlayerState* EMRPlayerState = GetPlayerState<AEMRPlayerState>();
	if (!EMRPlayerState)
	{
		return;
	}

	EMRPlayerState->SetLobbyReady(!EMRPlayerState->IsLobbyReady());
}

void AEMRFrontendLobbyPlayerController::RequestJoinLobbyByInviteCode(const FString& Code)
{
	if (UEMRLobbyInviteCodeSubsystem* InviteCodeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbyInviteCodeSubsystem>() : nullptr)
	{
		InviteCodeSubsystem->OnInviteJoinResult.RemoveAll(this);
		InviteCodeSubsystem->OnInviteJoinResult.AddUObject(this, &ThisClass::HandleInviteJoinResult);
		InviteCodeSubsystem->FindAndJoinSessionByInviteCode(Code, this);
		return;
	}

	HandleInviteJoinResult(ELobbyInviteJoinResult::SearchFailed);
}

void AEMRFrontendLobbyPlayerController::RequestRegenerateLobbyInviteCode()
{
	Server_RegenerateLobbyInviteCode();
}

void AEMRFrontendLobbyPlayerController::RequestStartGame()
{
	Server_RequestStartGame();
}

void AEMRFrontendLobbyPlayerController::RequestKickPlayer(APlayerState* TargetPlayerState)
{
	Server_RequestKickPlayer(TargetPlayerState);
}

void AEMRFrontendLobbyPlayerController::Server_RegenerateLobbyInviteCode_Implementation()
{
	if (!IsLocalController())
	{
		return;
	}

	UEMRLobbyInviteCodeSubsystem* InviteCodeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbyInviteCodeSubsystem>() : nullptr;
	if (!InviteCodeSubsystem)
	{
		return;
	}

	const FString NewCode = InviteCodeSubsystem->GenerateInviteCode();
	if (NewCode.IsEmpty())
	{
		return;
	}

	if (!InviteCodeSubsystem->UpdateSessionInviteCode(NewCode))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyPlayerController] Failed to update invite code in session."));
		return;
	}

	if (AEMRLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AEMRLobbyGameState>() : nullptr)
	{
		LobbyGameState->SetLobbyInviteCode(NewCode);
	}
}

void AEMRFrontendLobbyPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType, bool bIsSeamlessTravel)
{
	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPC.PreClientTravel pc=%s url=%s travelType=%d seamless=%d"),
		*GetNameSafe(this),
		*PendingURL,
		static_cast<int32>(TravelType),
		bIsSeamlessTravel ? 1 : 0);

	if (UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this))
	{
		UIManagerSubsystem->ClearPrimaryLayoutWidgets();
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	SetIgnoreLookInput(false);
	SetIgnoreMoveInput(false);

	Super::PreClientTravel(PendingURL, TravelType, bIsSeamlessTravel);
}

void AEMRFrontendLobbyPlayerController::Server_RequestStartGame_Implementation()
{
	if (!HasAuthority() || !IsLocalController())
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPC.Server_RequestStartGame rejected authority=%d local=%d"),
			HasAuthority() ? 1 : 0,
			IsLocalController() ? 1 : 0);
		return;
	}

	EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPC.Server_RequestStartGame accepted pc=%s"), *GetNameSafe(this));
	if (AEMRLobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEMRLobbyGameMode>() : nullptr)
	{
		LobbyGameMode->StartLobbyGame();
	}
	else
	{
		EMR_END_SESSION_TRACE(this, TEXT("[EndSessionLobbyTrace] LobbyPC.Server_RequestStartGame missing LobbyGameMode"));
	}
}

void AEMRFrontendLobbyPlayerController::Server_RequestKickPlayer_Implementation(APlayerState* TargetPlayerState)
{
	if (!HasAuthority() || !IsLocalController())
	{
		return;
	}

	if (!TargetPlayerState || TargetPlayerState == PlayerState)
	{
		return;
	}

	if (AEMRLobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEMRLobbyGameMode>() : nullptr)
	{
		LobbyGameMode->KickLobbyPlayer(TargetPlayerState);
	}
}

void AEMRFrontendLobbyPlayerController::Client_HandleKickedFromLobby_Implementation()
{
	if (UEMRLobbySessionSubsystem* LobbySessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRLobbySessionSubsystem>() : nullptr)
	{
		const FText KickedTitle = GetLocalizedText(UIStringTableId, TEXT("UI.Confirm.Title.LobbyKicked"));
		const FText KickedMessage = GetLocalizedText(UIStringTableId, TEXT("UI.Confirm.Message.LobbyKicked"));
		LobbySessionSubsystem->QueueKickedFromLobbyMessage(KickedTitle, KickedMessage);
		LobbySessionSubsystem->ReturnToFrontend(this);
	}
}

void AEMRFrontendLobbyPlayerController::HandleInviteJoinResult(ELobbyInviteJoinResult Result)
{
	if (Result == ELobbyInviteJoinResult::Success)
	{
		return;
	}

	if (UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this))
	{
	const FString MessageKey = [Result]()
	{
		switch (Result)
		{
		case ELobbyInviteJoinResult::InvalidCode:
			return FString(TEXT("UI.Confirm.Message.LobbyCodeInvalid"));
		case ELobbyInviteJoinResult::SearchFailed:
			return FString(TEXT("UI.Confirm.Message.LobbyCodeSearchFailed"));
		case ELobbyInviteJoinResult::LobbyFull:
			return FString(TEXT("UI.Confirm.Message.LobbyFull"));
		case ELobbyInviteJoinResult::JoinFailed:
			return FString(TEXT("UI.Confirm.Message.LobbyCodeJoinFailed"));
		case ELobbyInviteJoinResult::NotFound:
		default:
			return FString(TEXT("UI.Confirm.Message.LobbyCodeNotFound"));
		}
		}();

		UIManagerSubsystem->PushConfirmScreenToModalStackAsync(
			EConfirmScreenType::Ok,
			FText::GetEmpty(),
			GetLocalizedText(UIStringTableId, MessageKey),
			[](EConfirmScreenButtonType ClickedButtonType) {});
	}
}

