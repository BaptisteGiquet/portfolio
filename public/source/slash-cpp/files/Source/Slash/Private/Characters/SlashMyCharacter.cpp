
#include "Characters/SlashMyCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Items/Item.h"
#include "GroomComponent.h"
#include "Items/Weapons/Weapon.h"
#include "Animation/AnimMontage.h"
#include "Components/AttributeComponent.h"
#include "HUD/SlashHUD.h"
#include "HUD/SlashOverlay.h"
#include "Items/Soul.h"
#include "Pickups/Treasure.h"
#include "Slash/DebugMacros.h"


ASlashMyCharacter::ASlashMyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetMesh()->SetCollisionObjectType(ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->bUsePawnControlRotation = true;
	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom);

	Hair = CreateDefaultSubobject<UGroomComponent>(TEXT("Hair"));
	Hair->SetupAttachment(GetMesh());
	Hair->AttachmentName = FString("head");

	Eyebrows = CreateDefaultSubobject<UGroomComponent>(TEXT("Eyebrows"));
	Eyebrows->SetupAttachment(GetMesh());
	Eyebrows->AttachmentName = FString("head");

	bIsEquiped = false;
}

void ASlashMyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (AttributeComponent && SlashOverlay)
	{
		AttributeComponent->RegenStamina(DeltaSeconds);
		SlashOverlay->SetStaminaBarPercent(AttributeComponent->GetStaminaPercent());
	}
}

void ASlashMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASlashMyCharacter::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASlashMyCharacter::Look);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ASlashMyCharacter::Jump);
	EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &ASlashMyCharacter::Equip);
	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ASlashMyCharacter::Attack);
	EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &ASlashMyCharacter::Dodge);
}

void ASlashMyCharacter::GetHit_Implementation(const FVector& InImpactPoint, AActor* Hitter)
{
	Super::GetHit_Implementation(InImpactPoint, Hitter);
	if (IsAlive())
	{
		SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
		CharacterState = ECharacterState::ECS_HitReacting;	
	}
}

float ASlashMyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	HandleDamage(DamageAmount);
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ASlashMyCharacter::SetOverlappingItem(AItem* InItem)
{
	IPickupInterface::SetOverlappingItem(InItem);
	OverlappingItem = InItem;
}

void ASlashMyCharacter::AddSouls(ASoul* InSoul)
{
	IPickupInterface::AddSouls(InSoul);
	int32 SoulsToAdd = InSoul->GetSoulsAmount();
	if (AttributeComponent)
	{
		AttributeComponent->AddSouls(SoulsToAdd);
		SlashOverlay->SetSoulsCountText(AttributeComponent->GetSouls());
	}
}

void ASlashMyCharacter::AddGold(ATreasure* InTreasure)
{
	IPickupInterface::AddGold(InTreasure);
	int32 GoldsToAdd = InTreasure->GetTreasureValue();
	if (AttributeComponent && SlashOverlay)
	{
		AttributeComponent->AddGold(GoldsToAdd);
		SlashOverlay->SetGoldCountText(AttributeComponent->GetGold());
	}
}

void ASlashMyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	PlayerController = Cast<APlayerController>(NewController);
}

void ASlashMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeSlashOverlay();
	AddMappingContext();

	Tags.Add(FName("EngageableTarget"));
}

void ASlashMyCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();
	if ((PlayerController) && !LookValue.IsNearlyZero())
	{
		AddControllerYawInput(LookValue.X);
		AddControllerPitchInput(LookValue.Y);
	}
}

void ASlashMyCharacter::Move(const FInputActionValue& Value)
{
	if (!IsUnoccupied()) return;
	const FVector2D MoveValue = Value.Get<FVector2D>();
	if ((PlayerController) && !MoveValue.IsNearlyZero())
	{
		const FRotator ControllerRotation = PlayerController->GetControlRotation();
		const FRotator ControllerYaw = FRotator(0.f, ControllerRotation.Yaw, 0.f);
		const FVector ControllerForwardDirection = FRotationMatrix(ControllerYaw).GetUnitAxis(EAxis::X);
		const FVector ControllerRightDirection = FRotationMatrix(ControllerYaw).GetUnitAxis(EAxis::Y);
		AddMovementInput(ControllerForwardDirection, MoveValue.Y);
		AddMovementInput(ControllerRightDirection, MoveValue.X);
	}
}

void ASlashMyCharacter::Jump()
{
	if (!IsUnoccupied()) return;
	Super::Jump();
}

void ASlashMyCharacter::Equip(const FInputActionValue& Value)
{
	AWeapon* OverlappingWeapon = Cast<AWeapon>(GetOverlappingItem());
	if (OverlappingWeapon)
	{
		EquipWeapon(OverlappingWeapon);
	}
	else
	{
		if (CanEquip())
		{
			Arm();
		}
		else if (CanUnequip())
		{
			Disarm();
		}
	}
	
}

void ASlashMyCharacter::Attack()
{
	Super::Attack();
	if (CanAttack())
	{
		PlayAttackMontage();
		CharacterState = ECharacterState::ECS_Attacking;
	}
}

void ASlashMyCharacter::Dodge()
{
	if (!IsUnoccupied() || !HasEnoughStamina() || !SlashOverlay) return;
	
	AttributeComponent->UseStamina(AttributeComponent->GetDodgeStaminaCost());
	SlashOverlay->SetStaminaBarPercent(AttributeComponent->GetStaminaPercent());
	CharacterState = ECharacterState::ECS_Dodging;
	PlayDodgeMontage();
}

void ASlashMyCharacter::HandleDamage(float DamageAmount)
{
	Super::HandleDamage(DamageAmount);
	UpdateHUDHealth();
}

void ASlashMyCharacter::UpdateHUDHealth()
{
	if (SlashOverlay && AttributeComponent)
	{
		const float HealthPercent = AttributeComponent->GetHealthPercent();
		SlashOverlay->SetHealthBarPercent(HealthPercent);
	}
}

bool ASlashMyCharacter::CanAttack()
{
	Super::CanAttack();
	return CharacterState != ECharacterState::ECS_Attacking && CharacterState != ECharacterState::ECS_HitReacting && EquipmentState != EEquipmentState::EES_Unequipped;
}

void ASlashMyCharacter::OnAttackEnd()
{
	Super::OnAttackEnd();
	CharacterState = ECharacterState::ECS_Unoccupied;
}

void ASlashMyCharacter::OnDodgeEnd()
{
	Super::OnDodgeEnd();
	CharacterState = ECharacterState::ECS_Unoccupied;
}

void ASlashMyCharacter::Disarm()
{
	PlayEquipMontage(FName("Unequip"));
	EquipmentState = EEquipmentState::EES_Unequipped;
	CharacterState = ECharacterState::ECS_Attacking;
}

void ASlashMyCharacter::Arm()
{
	PlayEquipMontage(FName("Equip"));
	EquipmentState = EEquipmentState::EES_EquippedOneHandedWeapon;
	CharacterState = ECharacterState::ECS_Attacking;
}

bool ASlashMyCharacter::CanEquip()
{
	return EquipmentState == EEquipmentState::EES_Unequipped && IsUnoccupied() && EquippedWeapon;
}

void ASlashMyCharacter::EquipWeapon(AWeapon*& Weapon)
{
	Weapon->Equip(this->GetMesh(), FName("RightHandSocket"),this, this);
	EquippedWeapon = Weapon;
	EquipmentState = EEquipmentState::EES_EquippedOneHandedWeapon;
	Weapon = nullptr;
}

bool ASlashMyCharacter::CanUnequip()
{
	return EquipmentState != EEquipmentState::EES_Unequipped && IsUnoccupied() && EquippedWeapon;
}

void ASlashMyCharacter::Die()
{
	Super::Die();
	DisableMeshCollision();
	CharacterState = ECharacterState::ECS_Dead;
	DisableCapsuleCollision();
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ASlashMyCharacter::HasEnoughStamina()
{
	return AttributeComponent && AttributeComponent->GetCurrentStamina() >= AttributeComponent->GetDodgeStaminaCost();
}

bool ASlashMyCharacter::IsUnoccupied()
{
	return CharacterState == ECharacterState::ECS_Unoccupied;
}

bool ASlashMyCharacter::IsDodging()
{
	return 	CharacterState == ECharacterState::ECS_Dodging;
}

void ASlashMyCharacter::InitializeSlashOverlay()
{
	if (PlayerController)
	{
		SlashHUD = Cast<ASlashHUD>(PlayerController->GetHUD());
		if (SlashHUD)
		{
			SlashOverlay = SlashHUD->GetSlashOverlay();
			if (SlashOverlay && AttributeComponent)
			{
				SlashOverlay->SetHealthBarPercent(AttributeComponent->GetHealthPercent());
				SlashOverlay->SetStaminaBarPercent(AttributeComponent->GetStaminaPercent());
				SlashOverlay->SetGoldCountText(0);
				SlashOverlay->SetSoulsCountText(0);
			}
		}
	}
}

void ASlashMyCharacter::AddMappingContext()
{
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(SlashCharacterMappingContext, 0);	
		}
	}
}

void ASlashMyCharacter::PlayEquipMontage(FName Section)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && EquipMontage)
	{
		AnimInstance->Montage_Play(EquipMontage);
		AnimInstance->Montage_JumpToSection(Section, EquipMontage);
	}
}

void ASlashMyCharacter::OnEquipEnd()
{
	CharacterState = ECharacterState::ECS_Unoccupied;	
}

void ASlashMyCharacter::OnHitReactEnd()
{
	CharacterState = ECharacterState::ECS_Unoccupied;
} 

void ASlashMyCharacter::AttachMeshToSocket(USceneComponent* InParent, FName InSocketName)
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachMeshToSocket(InParent, InSocketName);
	}
}










