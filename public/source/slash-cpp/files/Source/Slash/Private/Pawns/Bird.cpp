
#include "Pawns/Bird.h"
#include "Slash/DebugMacros.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ABird::ABird()
{
	PrimaryActorTick.bCanEverTick = true;
	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	Capsule->SetCapsuleHalfHeight(20.f);
	Capsule->SetCapsuleRadius(15.f);
	SetRootComponent(Capsule);

	BirdSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BirdSkeletalMeshComponent"));
	BirdSkeletalMesh->SetupAttachment(GetRootComponent());

	BirdSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("BirdSpringArmComponent"));
	BirdSpringArm->SetupAttachment(GetRootComponent());

	BirdCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BirdCameraComponent"));
	BirdCamera->SetupAttachment(BirdSpringArm);
}


void ABird::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(BirdInputMappingContext, 0);	
		}
	}
}


void ABird::Move(const FInputActionValue& Value)
{
	const FVector2D MoveValue = Value.Get<FVector2D>();
	if ((GetController()) && !MoveValue.IsNearlyZero())
	{
		FVector ForwardDirection = GetActorForwardVector();
		AddMovementInput(ForwardDirection, MoveValue.Y);

		FVector RightDirection = GetActorRightVector();
		AddMovementInput(RightDirection, MoveValue.X);
	}
}

void ABird::Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();

	if ((GetController()) && !LookValue.IsNearlyZero())
	{
		AddControllerYawInput(LookValue.X);
		AddControllerPitchInput(LookValue.Y);
	}
	
}


void ABird::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABird::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABird::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABird::Look);
	}
}

