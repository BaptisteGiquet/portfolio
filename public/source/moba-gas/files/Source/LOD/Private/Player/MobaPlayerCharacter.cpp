
#include "MobaPlayerCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "MobaCameraAimComponent.h"
#include "GAS/MobaAbilitySystemComponent.h"
#include "GAS/MobaGameplayTags.h"
#include "GAS/Attributes/MobaAttributeSet_Hero.h"
#include "Inventory/MobaInventoryComponent.h"
#include "LOD/MobaCustomCollisionChannels.h"


AMobaPlayerCharacter::AMobaPlayerCharacter()
{
	InitializeCameraRig();

	HeroAttributeSet = CreateDefaultSubobject<UMobaAttributeSet_Hero>(TEXT("HeroAttributeSet"));

	PlayerInventoryComponent = CreateDefaultSubobject<UMobaInventoryComponent>(TEXT("InventoryComponent"));
}




void AMobaPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	
	CachedPlayerController = GetController<APlayerController>();
	AddGameplayMappingContext();	
}




void AMobaPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!ensureMsgf(EnhancedInputComponent, TEXT("%s: EnhancedInputComponent is null"), *GetName()))
	{
		return;
	}

	const bool bAllInputActionsValid =
		ensureMsgf(JumpInputAction, TEXT("JumpInputAction not set on %s"), *GetName()) &
		ensureMsgf(LookInputAction, TEXT("LookInputAction not set on %s"), *GetName()) &
		ensureMsgf(MoveInputAction, TEXT("MoveInputAction not set on %s"), *GetName());

	if (!bAllInputActionsValid)
	{
		return;
	}

	
	EnhancedInputComponent->BindAction(JumpInputAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleLookInput);
	EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleMoveInput);
	
	EnhancedInputComponent->BindAction(UpgradeAbilityLeaderInputAction, ETriggerEvent::Started, this, &ThisClass::UpgradeAbilityLeaderDownInput);
	EnhancedInputComponent->BindAction(UpgradeAbilityLeaderInputAction, ETriggerEvent::Completed, this, &ThisClass::UpgradeAbilityLeaderUpInput);
	
	for (const TPair<EAbilityInputID, TObjectPtr<const UInputAction>>& InputActionPair : GameplayAbilityInputActions)
	{
		if (!InputActionPair.Value)
		{
			ensureMsgf(false, TEXT("Ability InputAction missing for %d on %s"), static_cast<int32>(InputActionPair.Key), *GetName());
			continue;
		}
		
		EnhancedInputComponent->BindAction(InputActionPair.Value, ETriggerEvent::Triggered, this, &ThisClass::HandleAbilityInput, InputActionPair.Key);
	}

	EnhancedInputComponent->BindAction(UseInventoryItemInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleUseInventoryItem);
}


void AMobaPlayerCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	Super::GetActorEyesViewPoint(OutLocation, OutRotation);

	if (Camera)
	{
		OutLocation = Camera->GetComponentLocation();
		OutRotation = GetBaseAimRotation();	
	}
}


void AMobaPlayerCharacter::InitializeCameraRig()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->ProbeChannel = ECC_SpringArm;
	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	
	// Character won't follow camera movement
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	// -1 in yaw make the rotation instant
	GetCharacterMovement()->RotationRate = FRotator(0.f, -1.f, 0.f);

	CameraAimComponent = CreateDefaultSubobject<UMobaCameraAimComponent>(TEXT("CameraAim"));
}




void AMobaPlayerCharacter::AddGameplayMappingContext() const
{
	if (CachedPlayerController.IsValid() && GameplayInputMappingContext)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = CachedPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		
		if (InputSubsystem)
		{
			constexpr int32 MappingPriority = 0;
			InputSubsystem->RemoveMappingContext(GameplayInputMappingContext);
			InputSubsystem->AddMappingContext(GameplayInputMappingContext, MappingPriority);	
		}
	}
}




void AMobaPlayerCharacter::HandleLookInput(const FInputActionValue& InputActionValue) 
{
	const FVector2D InputValue = InputActionValue.Get<FVector2D>();
	if (CachedPlayerController.IsValid() && !InputValue.IsNearlyZero())
	{
		AddControllerYawInput(InputValue.X);
		AddControllerPitchInput(InputValue.Y);
	}
}




void AMobaPlayerCharacter::HandleMoveInput(const FInputActionValue& InputActionValue)
{
	const FVector2D InputValue = InputActionValue.Get<FVector2D>();
	if (CachedPlayerController.IsValid() && !InputValue.IsNearlyZero())
	{
		const FRotator PlayerControllerRotation = CachedPlayerController->GetControlRotation();
		const FRotator PlayerControllerRotationYaw = FRotator(0.f, PlayerControllerRotation.Yaw, 0.f);
		
		const FVector PlayerControllerForwardDirection = FRotationMatrix(PlayerControllerRotationYaw).GetUnitAxis(EAxis::X);
		const FVector PlayerControllerRightDirection = FRotationMatrix(PlayerControllerRotationYaw).GetUnitAxis(EAxis::Y);
		
		AddMovementInput(PlayerControllerRightDirection, InputValue.X);
		AddMovementInput(PlayerControllerForwardDirection, InputValue.Y);
	}
}



void AMobaPlayerCharacter::UpgradeAbilityLeaderDownInput(const FInputActionValue& InputActionValue)
{
	bIsUpgradeAbilityLeaderDown = true;	
}



void AMobaPlayerCharacter::UpgradeAbilityLeaderUpInput(const FInputActionValue& InputActionValue)
{
	bIsUpgradeAbilityLeaderDown = false;
}


void AMobaPlayerCharacter::HandleUseInventoryItem(const FInputActionValue& InputActionValue)
{
	int32 InputActionScalarValue = FMath::RoundToInt(InputActionValue.Get<float>());
	PlayerInventoryComponent->TryActivateItemInSlot(InputActionScalarValue - 1 /* ItemSlots start at 0 */);
}


void AMobaPlayerCharacter::HandleAbilityInput(const FInputActionValue& InputActionValue, const EAbilityInputID AbilityInputID)
{
	if (UAbilitySystemComponent* PlayerAbilitySystemComponent = GetAbilitySystemComponent())
	{
		const bool bIsPressed = InputActionValue.Get<bool>();

		if (bIsPressed && bIsUpgradeAbilityLeaderDown)
		{
			// On MobaCharacter
			UpgradeAbilityWithInputID(AbilityInputID);
			return;
		}
		
		if (bIsPressed)
		{
			PlayerAbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(AbilityInputID));
			
			if (AbilityInputID == EAbilityInputID::BasicAttack)
			{
				PlayerAbilitySystemComponent->LocalInputConfirm();
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, MobaGameplayAbilityPressedTags::Ability_BasicAttack_Pressed, FGameplayEventData());
				Server_SendGameplayEvent(MobaGameplayAbilityPressedTags::Ability_BasicAttack_Pressed);
			}
		}
		else
		{
			if (AbilityInputID == EAbilityInputID::BasicAttack)
			{
				PlayerAbilitySystemComponent->LocalInputCancel();
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, MobaGameplayAbilityReleasedTags::Ability_BasicAttack_Released, FGameplayEventData());
				Server_SendGameplayEvent(MobaGameplayAbilityReleasedTags::Ability_BasicAttack_Released);
			}
			PlayerAbilitySystemComponent->AbilityLocalInputReleased(static_cast<int32>(AbilityInputID));
		}
	}
}




void AMobaPlayerCharacter::SetInputEnabled(const bool bIsEnabled)
{
	APlayerController* PlayerController = CachedPlayerController.Get();
	if (!PlayerController) { return; }
	
	if (bIsEnabled)
	{
		EnableInput(PlayerController);	
	}
	else
	{
		DisableInput(PlayerController);	
	}
}


void AMobaPlayerCharacter::Server_SendGameplayEvent_Implementation(FGameplayTag EventTag)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, FGameplayEventData());
}


void AMobaPlayerCharacter::OnStun()
{
	Super::OnStun();

	SetInputEnabled(false);
}




void AMobaPlayerCharacter::OnRecoverFromStun()
{
	Super::OnRecoverFromStun();

	if (IsDead()) { return; }
	
	SetInputEnabled(true);
}




void AMobaPlayerCharacter::OnDead()
{
	Super::OnDead();
	
	SetInputEnabled(false);
}




void AMobaPlayerCharacter::OnRespawn()
{
	Super::OnRespawn();
	
	SetInputEnabled(true);
}

