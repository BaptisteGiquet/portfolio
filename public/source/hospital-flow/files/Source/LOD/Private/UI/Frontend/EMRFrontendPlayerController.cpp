

#include "UI/Frontend/EMRFrontendPlayerController.h"

#include "Camera/CameraActor.h"
#include "Characters/Player/EMRPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/EMRFrontendGameState.h"
#include "GAS/EMRTags.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "LocalizationLibrary.h"
#include "Subsystems/EMRLobbyInviteCodeSubsystem.h"
#include "UI/Common/Subsystems/EMRUIManagerSubsystem.h"
#include "UI/Frontend/Core/EMRCommonPrimaryLayoutWidget.h"
#include "UI/Frontend/Core/EMRFrontendCommonActivatableWidgetBase.h"
#include "UI/Frontend/EMRGameUserSettings.h"
#include "UI/Frontend/Utils/EMRFrontendEnumTypes.h"
#include "UI/Frontend/Utils/EMRFrontendFunctionLibrary.h"
#include "UserSettings/EnhancedInputUserSettings.h"

AEMRFrontendPlayerController::AEMRFrontendPlayerController()
{
	FrontendPrimaryLayoutWidgetClass = TSoftClassPtr<UEMRCommonPrimaryLayoutWidget>(
		FSoftObjectPath(TEXT("/Game/EmergencyRoom/UI/Frontend/Core/WBP_CUW_PrimaryLayout.WBP_CUW_PrimaryLayout_C")));
	ControlsRemapInputMappingContext = TSoftObjectPtr<UInputMappingContext>(
		FSoftObjectPath(TEXT("/Game/EmergencyRoom/Inputs/IMC_Gameplay.IMC_Gameplay")));
}

void AEMRFrontendPlayerController::BeginPlay()
{
	Super::BeginPlay();
	RegisterControlsRemapInputMappingContext();
	TryBootstrapFrontendUI();
}

void AEMRFrontendPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClassWithTag(this, ACameraActor::StaticClass(), FName("Default"), FoundCameras);

	if (!FoundCameras.IsEmpty())
	{
		SetViewTarget(FoundCameras[0]);
	}
	
	UEMRGameUserSettings* GameUserSettings = UEMRGameUserSettings::Get();
	if (!GameUserSettings) return;
	
	if (GameUserSettings->GetLastCPUBenchmarkResult() == -1.f || GameUserSettings->GetLastGPUBenchmarkResult() == -1.f) // -1.f means no hardwarebenchmark has been run before
	{
		GameUserSettings->RunHardwareBenchmark();
		GameUserSettings->ApplyHardwareBenchmarkResults();
	}

	RegisterControlsRemapInputMappingContext();
	TryBootstrapFrontendUI();
}

void AEMRFrontendPlayerController::ToggleLobbyReady()
{
	AEMRPlayerState* EMRPlayerState = GetPlayerState<AEMRPlayerState>();
	if (!EMRPlayerState)
	{
		return;
	}

	EMRPlayerState->SetLobbyReady(!EMRPlayerState->IsLobbyReady());
}

void AEMRFrontendPlayerController::RequestJoinLobbyByInviteCode(const FString& Code)
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

void AEMRFrontendPlayerController::RequestRegenerateLobbyInviteCode()
{
	Server_RegenerateLobbyInviteCode();
}

void AEMRFrontendPlayerController::Server_RegenerateLobbyInviteCode_Implementation()
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
		UE_LOG(LogTemp, Warning, TEXT("[FrontendPlayerController] Failed to update invite code in session."));
		return;
	}

	if (AEMRFrontendGameState* FrontendGameState = GetWorld() ? GetWorld()->GetGameState<AEMRFrontendGameState>() : nullptr)
	{
		FrontendGameState->SetLobbyInviteCode(NewCode);
	}
}

void AEMRFrontendPlayerController::HandleInviteJoinResult(ELobbyInviteJoinResult Result)
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
			[](EConfirmScreenButtonType) {});
	}
}

void AEMRFrontendPlayerController::TryBootstrapFrontendUI()
{
	if (bFrontendUIBootstrapped || !IsLocalController())
	{
		return;
	}

	UEMRUIManagerSubsystem* UIManagerSubsystem = UEMRUIManagerSubsystem::Get(this);
	if (!UIManagerSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FrontendPlayerController] Frontend bootstrap skipped: UIManager subsystem missing."));
		return;
	}

	const TSubclassOf<UEMRCommonPrimaryLayoutWidget> PrimaryLayoutClass = ResolveFrontendPrimaryLayoutWidgetClass();
	if (!PrimaryLayoutClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FrontendPlayerController] Frontend bootstrap skipped: PrimaryLayout class not configured."));
		return;
	}

	UEMRCommonPrimaryLayoutWidget* PrimaryLayout = UIManagerSubsystem->EnsurePrimaryLayout(this, PrimaryLayoutClass);
	if (!PrimaryLayout || !PrimaryLayout->IsInViewport())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FrontendPlayerController] Frontend bootstrap skipped: PrimaryLayout unavailable."));
		return;
	}

	const TSoftClassPtr<UEMRFrontendCommonActivatableWidgetBase> MainMenuClass =
	UEMRFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(EMRTags::UI::Widgets::MainMenu);
	if (MainMenuClass.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FrontendPlayerController] Frontend bootstrap skipped: MainMenu widget class missing from settings."));
		return;
	}

	UIManagerSubsystem->PushOrReplaceSoftWidgetToStackAsync(
		EMRTags::UI::WidgetStack::Frontend,
		MainMenuClass,
		[](EAsyncPushWidgetState, UEMRFrontendCommonActivatableWidgetBase*)
		{
		});

	bFrontendUIBootstrapped = true;
	UE_LOG(LogTemp, Log, TEXT("[FrontendPlayerController] Frontend bootstrap completed."));
}

TSubclassOf<UEMRCommonPrimaryLayoutWidget> AEMRFrontendPlayerController::ResolveFrontendPrimaryLayoutWidgetClass() const
{
	if (FrontendPrimaryLayoutWidgetClass.IsNull())
	{
		return nullptr;
	}

	if (FrontendPrimaryLayoutWidgetClass.IsValid())
	{
		return FrontendPrimaryLayoutWidgetClass.Get();
	}

	return FrontendPrimaryLayoutWidgetClass.LoadSynchronous();
}

void AEMRFrontendPlayerController::RegisterControlsRemapInputMappingContext()
{
	if (!IsLocalController())
	{
		return;
	}

	const UInputMappingContext* MappingContext = ResolveControlsRemapInputMappingContext();
	if (!MappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!InputSubsystem)
	{
		return;
	}

	if (UEnhancedInputUserSettings* UserSettings = InputSubsystem->GetUserSettings())
	{
		UserSettings->RegisterInputMappingContext(MappingContext);
		InputSubsystem->OnPostUserSettingsInitialized.RemoveDynamic(this, &ThisClass::HandlePostUserSettingsInitialized);
		return;
	}

	if (!InputSubsystem->OnPostUserSettingsInitialized.IsAlreadyBound(this, &ThisClass::HandlePostUserSettingsInitialized))
	{
		InputSubsystem->OnPostUserSettingsInitialized.AddDynamic(this, &ThisClass::HandlePostUserSettingsInitialized);
	}
}

const UInputMappingContext* AEMRFrontendPlayerController::ResolveControlsRemapInputMappingContext() const
{
	if (ControlsRemapInputMappingContext.IsNull())
	{
		return nullptr;
	}

	if (ControlsRemapInputMappingContext.IsValid())
	{
		return ControlsRemapInputMappingContext.Get();
	}

	return ControlsRemapInputMappingContext.LoadSynchronous();
}

void AEMRFrontendPlayerController::HandlePostUserSettingsInitialized(const UEnhancedInputUserSettings* Settings)
{
	if (!Settings)
	{
		return;
	}

	RegisterControlsRemapInputMappingContext();
}
