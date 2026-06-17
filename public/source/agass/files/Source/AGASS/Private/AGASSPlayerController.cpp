#include "AGASSPlayerController.h"

#include "AGASSCharacter.h"
#include "Components/AGASSInteractionComponent.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Input/CommonUIActionRouterBase.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AGASSLocalPlayer.h"
#include "Settings/AGASSSettingsLocal.h"
#include "Subsystems/AGASSUIManagerSubsystem.h"
#include "Subsystems/AGASSSessionSubsystem.h"
#include "TimerManager.h"
#include "UserSettings/EnhancedInputUserSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSPlayerControllerFlow, Log, All);

AAGASSPlayerController::AAGASSPlayerController()
{
	InteractionComponent = CreateDefaultSubobject<UAGASSInteractionComponent>(TEXT("InteractionComponent"));
}

void AAGASSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogAGASSPlayerControllerFlow,
		Display,
		TEXT("BeginPlay controller=%s local=%d world=%s"),
		*GetNameSafe(this),
		IsLocalController() ? 1 : 0,
		*GetNameSafe(GetWorld()));

	ApplyGameplayInputMappingContext();
	QueueApplyGameplayViewportInputConfig();
}

void AAGASSPlayerController::ClientReturnToFrontendForClosedRun_Implementation(const FText& ReturnReason)
{
	UE_LOG(
		LogAGASSPlayerControllerFlow,
		Warning,
		TEXT("ClientReturnToFrontendForClosedRun controller=%s local=%d reason=\"%s\""),
		*GetNameSafe(this),
		IsLocalController() ? 1 : 0,
		*ReturnReason.ToString());

	if (UGameInstance* const GameInstance = GetGameInstance())
	{
		if (UAGASSSessionSubsystem* const SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			UE_LOG(LogAGASSPlayerControllerFlow, Display, TEXT("ClientReturnToFrontendForClosedRun forwarding to SessionSubsystem without destroying the client-side session first."));
			SessionSubsystem->ReturnToFrontend(ReturnReason.ToString(), false);
		}
		else
		{
			UE_LOG(LogAGASSPlayerControllerFlow, Warning, TEXT("ClientReturnToFrontendForClosedRun could not find SessionSubsystem."));
		}
	}
	else
	{
		UE_LOG(LogAGASSPlayerControllerFlow, Warning, TEXT("ClientReturnToFrontendForClosedRun could not find GameInstance."));
	}
}

void AAGASSPlayerController::ClientPlayObjectiveReachedSound_Implementation()
{
	PlayLocalSound2D(ObjectiveReachedSound);
}

void AAGASSPlayerController::RequestSessionInviteCodeRegeneration()
{
	if (UGameInstance* const GameInstance = GetGameInstance())
	{
		if (UAGASSSessionSubsystem* const SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			if (HasAuthority())
			{
				SessionSubsystem->RequestHostedSessionInviteCodeRegeneration();
				return;
			}

			ServerRequestRegenerateInviteCode();
			return;
		}
	}
}

void AAGASSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPendingInteractHold();
	RemoveGameplayInputMappingContext();

	Super::EndPlay(EndPlayReason);
}

void AAGASSPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	QueueApplyGameplayViewportInputConfig();
}

void AAGASSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!ensure(InputComponent != nullptr))
	{
		return;
	}

	if (ShouldUseEnhancedInput())
	{
		if (UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			if (UInputAction* const ResolvedMoveAction = MoveInputAction.LoadSynchronous())
			{
				EnhancedInputComponent->BindAction(ResolvedMoveAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMoveInput);
			}

			if (UInputAction* const ResolvedLookAction = LookInputAction.LoadSynchronous())
			{
				EnhancedInputComponent->BindAction(ResolvedLookAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLookInput);
			}

			if (UInputAction* const ResolvedJumpAction = JumpInputAction.LoadSynchronous())
			{
				EnhancedInputComponent->BindAction(ResolvedJumpAction, ETriggerEvent::Started, this, &ThisClass::HandleJumpStarted);
				EnhancedInputComponent->BindAction(ResolvedJumpAction, ETriggerEvent::Completed, this, &ThisClass::HandleJumpCompleted);
				EnhancedInputComponent->BindAction(ResolvedJumpAction, ETriggerEvent::Canceled, this, &ThisClass::HandleJumpCompleted);
			}

			if (UInputAction* const ResolvedInteractAction = InteractInputAction.LoadSynchronous())
			{
				EnhancedInputComponent->BindAction(ResolvedInteractAction, ETriggerEvent::Started, this, &ThisClass::HandleInteractStarted);
				EnhancedInputComponent->BindAction(ResolvedInteractAction, ETriggerEvent::Completed, this, &ThisClass::HandleInteractReleased);
				EnhancedInputComponent->BindAction(ResolvedInteractAction, ETriggerEvent::Canceled, this, &ThisClass::HandleInteractReleased);
			}

			if (UInputAction* const ResolvedCancelCarryAction = CancelCarryInputAction.LoadSynchronous())
			{
				EnhancedInputComponent->BindAction(ResolvedCancelCarryAction, ETriggerEvent::Started, this, &ThisClass::HandleCancelCarryPressed);
			}

			if (UInputAction* const ResolvedRotatePlacementAction = RotatePlacementInputAction.LoadSynchronous())
			{
				EnhancedInputComponent->BindAction(ResolvedRotatePlacementAction, ETriggerEvent::Started, this, &ThisClass::HandleTogglePlacementRotationPressed);
			}

			if (UInputAction* const ResolvedAdjustPlacementDistanceAction = AdjustPlacementDistanceInputAction.LoadSynchronous())
			{
				EnhancedInputComponent->BindAction(ResolvedAdjustPlacementDistanceAction, ETriggerEvent::Triggered, this, &ThisClass::HandleAdjustPlacementDistanceInput);
			}

			if (UInputAction* const ResolvedToggleGameMenuAction = ToggleGameMenuInputAction.LoadSynchronous())
			{
				EnhancedInputComponent->BindAction(ResolvedToggleGameMenuAction, ETriggerEvent::Started, this, &ThisClass::HandleToggleGameMenuPressed);
			}

			return;
		}
	}

	BindLegacyInput();
}

bool AAGASSPlayerController::ShouldUseEnhancedInput() const
{
	return !GameplayInputMappingContext.IsNull()
		&& !MoveInputAction.IsNull()
		&& !LookInputAction.IsNull()
		&& !JumpInputAction.IsNull()
		&& !InteractInputAction.IsNull()
		&& !CancelCarryInputAction.IsNull();
}

void AAGASSPlayerController::ApplyGameplayInputMappingContext()
{
	if (!IsLocalController() || !ShouldUseEnhancedInput() || AppliedGameplayInputMappingContext != nullptr)
	{
		return;
	}

	ULocalPlayer* const LocalPlayer = GetLocalPlayer();
	if (LocalPlayer == nullptr)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* const InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (InputSubsystem == nullptr)
	{
		return;
	}

	AppliedGameplayInputMappingContext = GameplayInputMappingContext.LoadSynchronous();
	if (AppliedGameplayInputMappingContext == nullptr)
	{
		return;
	}

	if (UEnhancedInputUserSettings* const UserSettings = InputSubsystem->GetUserSettings())
	{
		UserSettings->RegisterInputMappingContext(AppliedGameplayInputMappingContext);
	}

	FModifyContextOptions ModifyOptions;
	ModifyOptions.bNotifyUserSettings = true;
	InputSubsystem->AddMappingContext(AppliedGameplayInputMappingContext, GameplayInputMappingPriority, ModifyOptions);
}

void AAGASSPlayerController::RemoveGameplayInputMappingContext()
{
	if (AppliedGameplayInputMappingContext == nullptr)
	{
		return;
	}

	if (ULocalPlayer* const LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* const InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			InputSubsystem->RemoveMappingContext(AppliedGameplayInputMappingContext);
		}
	}

	AppliedGameplayInputMappingContext = nullptr;
}

void AAGASSPlayerController::QueueApplyGameplayViewportInputConfig()
{
	if (!IsLocalController())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ApplyGameplayViewportInputConfig);
	}
}

void AAGASSPlayerController::ApplyGameplayViewportInputConfig()
{
	if (!IsLocalController() || GetPawn() == nullptr)
	{
		return;
	}

	ULocalPlayer* const LocalPlayer = GetLocalPlayer();
	if (LocalPlayer == nullptr)
	{
		return;
	}

	if (UCommonUIActionRouterBase* const ActionRouter = LocalPlayer->GetSubsystem<UCommonUIActionRouterBase>())
	{
		FUIInputConfig GameplayInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
		GameplayInputConfig.bIgnoreMoveInput = false;
		GameplayInputConfig.bIgnoreLookInput = false;
		ActionRouter->SetActiveUIInputConfig(GameplayInputConfig, this);
	}

	FInputModeGameOnly GameplayInputMode;
	SetInputMode(GameplayInputMode);
	SetShowMouseCursor(false);
}

void AAGASSPlayerController::BindLegacyInput()
{
	InputComponent->BindAxis(TEXT("MoveForward"), this, &ThisClass::MoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"), this, &ThisClass::MoveRight);
	InputComponent->BindAxis(TEXT("Turn"), this, &ThisClass::Turn);
	InputComponent->BindAxis(TEXT("LookUp"), this, &ThisClass::LookUp);
	InputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ThisClass::StartJump);
	InputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ThisClass::StopJump);
	InputComponent->BindAction(TEXT("AGASS_Interact"), IE_Pressed, this, &ThisClass::HandleInteractStarted);
	InputComponent->BindAction(TEXT("AGASS_Interact"), IE_Released, this, &ThisClass::HandleInteractReleased);
	InputComponent->BindAction(TEXT("AGASS_CancelCarry"), IE_Pressed, this, &ThisClass::HandleCancelCarryPressed);
	InputComponent->BindAction(TEXT("AGASS_TogglePlacementRotate"), IE_Pressed, this, &ThisClass::HandleTogglePlacementRotationPressed);
	InputComponent->BindAction(TEXT("AGASS_ToggleGameMenu"), IE_Pressed, this, &ThisClass::HandleToggleGameMenuPressed);
	InputComponent->BindAxis(TEXT("MouseWheelAxis"), this, &ThisClass::AdjustPlacementDistance);
}

UAGASSSettingsLocal* AAGASSPlayerController::GetLocalSettings() const
{
	if (const UAGASSLocalPlayer* const AGASSLocalPlayer = Cast<UAGASSLocalPlayer>(GetLocalPlayer()))
	{
		return AGASSLocalPlayer->GetLocalSettings();
	}

	return UAGASSSettingsLocal::Get();
}

bool AAGASSPlayerController::IsUsingGamepadLookInput() const
{
	if (const UCommonInputSubsystem* const InputSubsystem = UCommonInputSubsystem::Get(GetLocalPlayer()))
	{
		return InputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad;
	}

	return false;
}

FVector2D AAGASSPlayerController::ApplyLookSettings(const FVector2D& RawInput) const
{
	if (const UAGASSSettingsLocal* const LocalSettings = GetLocalSettings())
	{
		return LocalSettings->ApplyLookInputSettings(RawInput, IsUsingGamepadLookInput());
	}

	return RawInput;
}

FVector2D AAGASSPlayerController::ApplyPlacementRotationSettings(const FVector2D& RawInput) const
{
	const float Scale = GetLocalSettings() != nullptr ? GetLocalSettings()->GetPlacementRotationSensitivity() : 1.0f;
	return ApplyLookSettings(RawInput) * Scale;
}

float AAGASSPlayerController::ApplyPlacementDistanceScale(const float RawInput) const
{
	const float Scale = GetLocalSettings() != nullptr ? GetLocalSettings()->GetPlacementDistanceSpeedScale() : 1.0f;
	return RawInput * Scale;
}

void AAGASSPlayerController::HandleMoveInput(const FInputActionValue& Value)
{
	if (AAGASSCharacter* const AGASSCharacter = Cast<AAGASSCharacter>(GetPawn()))
	{
		AGASSCharacter->ApplyMoveInput(Value.Get<FVector2D>());
	}
}

void AAGASSPlayerController::HandleLookInput(const FInputActionValue& Value)
{
	if (AAGASSCharacter* const AGASSCharacter = Cast<AAGASSCharacter>(GetPawn()))
	{
		const FVector2D RawLookInput = Value.Get<FVector2D>();
		if (InteractionComponent != nullptr && InteractionComponent->IsPlacementRotationModeActive())
		{
			InteractionComponent->ApplyPlacementRotationInput(ApplyPlacementRotationSettings(RawLookInput));
			return;
		}

		AGASSCharacter->ApplyLookInput(ApplyLookSettings(RawLookInput));
	}
}

void AAGASSPlayerController::HandleJumpStarted(const FInputActionValue& Value)
{
	StartJump();
}

void AAGASSPlayerController::HandleJumpCompleted(const FInputActionValue& Value)
{
	StopJump();
}

void AAGASSPlayerController::HandleInteractStarted()
{
	ClearPendingInteractHold();
	bInteractInputHeld = true;
	bPendingHoldInteractionConsumed = false;

	if (InteractionComponent == nullptr)
	{
		return;
	}

	AActor* HoldTargetActor = nullptr;
	float HoldDurationSeconds = 0.f;
	if (InteractionComponent->ResolveFocusedHoldInteract(HoldTargetActor, HoldDurationSeconds) && HoldTargetActor != nullptr)
	{
		PendingHoldInteractActor = HoldTargetActor;
		GetWorldTimerManager().SetTimer(
			InteractHoldTimerHandle,
			this,
			&ThisClass::HandleInteractHoldCompleted,
			HoldDurationSeconds,
			false);
	}
}

void AAGASSPlayerController::HandleInteractReleased()
{
	const bool bShouldRunQuickInteraction = bInteractInputHeld && !bPendingHoldInteractionConsumed;

	ClearPendingInteractHold();

	if (bShouldRunQuickInteraction && InteractionComponent != nullptr)
	{
		InteractionComponent->TryQuickInteract();
	}
}

void AAGASSPlayerController::HandleInteractHoldCompleted()
{
	if (!bInteractInputHeld || bPendingHoldInteractionConsumed || InteractionComponent == nullptr)
	{
		return;
	}

	AActor* const HoldTargetActor = PendingHoldInteractActor.Get();
	if (HoldTargetActor == nullptr || !InteractionComponent->IsFocusedInteractActor(HoldTargetActor))
	{
		return;
	}

	bPendingHoldInteractionConsumed = InteractionComponent->TryHoldInteract(HoldTargetActor);
}

void AAGASSPlayerController::ClearPendingInteractHold()
{
	GetWorldTimerManager().ClearTimer(InteractHoldTimerHandle);
	PendingHoldInteractActor.Reset();
	bInteractInputHeld = false;
	bPendingHoldInteractionConsumed = false;
}

void AAGASSPlayerController::HandleCancelCarryPressed()
{
	if (InteractionComponent != nullptr)
	{
		InteractionComponent->CancelHeldItem();
	}
}

void AAGASSPlayerController::HandleTogglePlacementRotationPressed()
{
	if (InteractionComponent != nullptr)
	{
		InteractionComponent->TogglePlacementRotationMode();
	}
}

void AAGASSPlayerController::HandleAdjustPlacementDistanceInput(const FInputActionValue& Value)
{
	if (InteractionComponent != nullptr)
	{
		InteractionComponent->AdjustPreviewDistance(ApplyPlacementDistanceScale(Value.Get<float>()));
	}
}

void AAGASSPlayerController::HandleToggleGameMenuPressed()
{
	if (!IsLocalController())
	{
		return;
	}

	if (UGameInstance* const GameInstance = GetGameInstance())
	{
		if (UAGASSUIManagerSubsystem* const UIManager = GameInstance->GetSubsystem<UAGASSUIManagerSubsystem>())
		{
			UIManager->ToggleGameMenu(GetLocalPlayer());
		}
	}
}

void AAGASSPlayerController::ServerRequestRegenerateInviteCode_Implementation()
{
	if (UGameInstance* const GameInstance = GetGameInstance())
	{
		if (UAGASSSessionSubsystem* const SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			SessionSubsystem->RequestHostedSessionInviteCodeRegeneration();
		}
	}
}

void AAGASSPlayerController::MoveForward(const float Value)
{
	if (AAGASSCharacter* const AGASSCharacter = Cast<AAGASSCharacter>(GetPawn()))
	{
		AGASSCharacter->ApplyMoveInput(FVector2D(0.f, Value));
	}
}

void AAGASSPlayerController::MoveRight(const float Value)
{
	if (AAGASSCharacter* const AGASSCharacter = Cast<AAGASSCharacter>(GetPawn()))
	{
		AGASSCharacter->ApplyMoveInput(FVector2D(Value, 0.f));
	}
}

void AAGASSPlayerController::Turn(const float Value)
{
	if (InteractionComponent != nullptr && InteractionComponent->IsPlacementRotationModeActive())
	{
		InteractionComponent->ApplyPlacementRotationInput(ApplyPlacementRotationSettings(FVector2D(Value, 0.f)));
		return;
	}

	if (AAGASSCharacter* const AGASSCharacter = Cast<AAGASSCharacter>(GetPawn()))
	{
		AGASSCharacter->ApplyLookInput(ApplyLookSettings(FVector2D(Value, 0.f)));
	}
}

void AAGASSPlayerController::LookUp(const float Value)
{
	if (InteractionComponent != nullptr && InteractionComponent->IsPlacementRotationModeActive())
	{
		InteractionComponent->ApplyPlacementRotationInput(ApplyPlacementRotationSettings(FVector2D(0.f, Value)));
		return;
	}

	if (AAGASSCharacter* const AGASSCharacter = Cast<AAGASSCharacter>(GetPawn()))
	{
		AGASSCharacter->ApplyLookInput(ApplyLookSettings(FVector2D(0.f, Value)));
	}
}

void AAGASSPlayerController::AdjustPlacementDistance(const float Value)
{
	if (InteractionComponent != nullptr)
	{
		InteractionComponent->AdjustPreviewDistance(ApplyPlacementDistanceScale(Value));
	}
}

void AAGASSPlayerController::StartJump()
{
	if (AAGASSCharacter* const AGASSCharacter = Cast<AAGASSCharacter>(GetPawn()))
	{
		AGASSCharacter->StartJumpInput();
	}
}

void AAGASSPlayerController::StopJump()
{
	if (AAGASSCharacter* const AGASSCharacter = Cast<AAGASSCharacter>(GetPawn()))
	{
		AGASSCharacter->StopJumpInput();
	}
}

void AAGASSPlayerController::PlayLocalSound2D(USoundBase* Sound) const
{
	if (Sound == nullptr || !IsLocalController())
	{
		return;
	}

	UGameplayStatics::PlaySound2D(this, Sound);
}
