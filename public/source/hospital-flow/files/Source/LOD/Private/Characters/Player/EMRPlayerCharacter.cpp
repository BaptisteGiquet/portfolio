#include "Characters/Player/EMRPlayerCharacter.h"

#include "Characters/Player/EMRPlayerController.h"
#include "Characters/Player/EMRPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/EMRInteractableDetectionComponent.h"
#include "Components/EMRPlayerMachineInputComponent.h"
#include "Components/EMRPlayerWidgetInteractionComponent.h"
#include "Components/EMRPlayerViewStateComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRFixedPlacementComponent.h"
#include "Interaction/EMRItemPlacementComponent.h"
#include "Interaction/EMRHubTabletActor.h"
#include "Interaction/EMRReceptionMachine.h"
#include "Interaction/EMRXRayMachine.h"
#include "Interaction/EMRUltrasoundMachine.h"
#include "Interaction/EMRCTScanMachine.h"
#include "Interaction/EMRIntravenousMedicationStand.h"
#include "Interaction/EMRLabAnalyzerMachine.h"
#include "Interaction/EMRLabAnalyzerLid.h"
#include "Interaction/EMRLabAnalyzerTube.h"
#include "Interaction/EMRItemDispenser.h"
#include "Interaction/EMRLabAnalyzerTrash.h"
#include "Interaction/EMROxygenMachine.h"
#include "Interaction/EMROxygenMask.h"
#include "Interaction/EMRToiletCleaner.h"
#include "Interaction/EMROvertimeStopTerminal.h"
#include "Interaction/EMRTreatmentBedFolder.h"
#include "Interfaces/EMRAnchoredCarryItemInterface.h"
#include "Interfaces/EMRCarriedWorldWidgetInteractionInterface.h"
#include "Interfaces/EMRFirstPersonCarryViewInterface.h"
#include "EMRLabAnalyzerLog.h"
#include "GAS/Abilities/EMRGameplayAbility_PerformXRay.h"
#include "GAS/Abilities/EMRGameplayAbility_Interact.h"
#include "GAS/Abilities/EMRGameplayAbility_PickItem.h"
#include "GAS/EMRTags.h"
#include "Shop/EMRItemActor.h"
#include "Shop/EMRItemData.h"
#include "Blueprint/UserWidget.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "TimerManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "CommonInputSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "UI/Frontend/EMRGameUserSettings.h"
#include "Components/CapsuleComponent.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Interfaces/EMRGameplayEventInterface.h"
#include "InputCoreTypes.h"
#include "Components/WidgetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "GameplayEffectTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ReferenceSkeleton.h"
#include "EngineUtils.h"
#include "LOD/EMRCollisionChannels.h"
#include "ProximityVoiceLocalPlayerSubsystem.h"
#include "Kismet/GameplayStatics.h"

#define EMR_CONFIRM_LOG(...)

namespace
{
    constexpr TCHAR PlayerCharacterUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");

    void GatherAttachedActors(AActor* RootActor, TArray<AActor*>& OutActors)
    {
        if (!RootActor)
        {
            return;
        }

        TArray<AActor*> Stack;
        Stack.Add(RootActor);

        while (!Stack.IsEmpty())
        {
            AActor* Current = Stack.Pop(EAllowShrinking::No);
            if (!Current || OutActors.Contains(Current))
            {
                continue;
            }

            OutActors.Add(Current);

            TArray<AActor*> Attached;
            Current->GetAttachedActors(Attached);
            for (AActor* Child : Attached)
            {
                if (Child && !OutActors.Contains(Child))
                {
                    Stack.Add(Child);
                }
            }
        }
    }

    AEMRLabAnalyzerMachine* ResolveLabAnalyzerFromHit(const UPrimitiveComponent* HitComponent)
    {
        if (!HitComponent)
        {
            return nullptr;
        }

        if (AEMRLabAnalyzerMachine* Machine = Cast<AEMRLabAnalyzerMachine>(HitComponent->GetOwner()))
        {
            return Machine;
        }

        if (const AEMRLabAnalyzerLid* LidActor = Cast<AEMRLabAnalyzerLid>(HitComponent->GetOwner()))
        {
            return LidActor->GetOwningMachine();
        }

        return nullptr;
    }

    bool IsLabAnalyzerOperator(const AEMRPlayerCharacter* Player, AEMRLabAnalyzerMachine*& OutMachine)
    {
        OutMachine = nullptr;
        if (!Player)
        {
            return false;
        }

        UWorld* World = Player->GetWorld();
        if (!World)
        {
            return false;
        }

        for (TActorIterator<AEMRLabAnalyzerMachine> It(World); It; ++It)
        {
            if (It->GetCurrentOperator() == Player)
            {
                OutMachine = *It;
                return true;
            }
        }

        return false;
    }

    const TCHAR* GetLocalRoleLabel(ENetRole Role)
    {
        switch (Role)
        {
        case ROLE_Authority:
            return TEXT("Authority");
        case ROLE_AutonomousProxy:
            return TEXT("AutonomousProxy");
        case ROLE_SimulatedProxy:
            return TEXT("SimulatedProxy");
        default:
            return TEXT("None");
        }
    }

    const TCHAR* GetDispenserButtonTypeLabel(EEMRItemDispenserButtonType ButtonType)
    {
        switch (ButtonType)
        {
        case EEMRItemDispenserButtonType::Digit:
            return TEXT("Digit");
        case EEMRItemDispenserButtonType::Confirm:
            return TEXT("Confirm");
        case EEMRItemDispenserButtonType::Reset:
            return TEXT("Reset");
        case EEMRItemDispenserButtonType::Increase:
            return TEXT("Increase");
        case EEMRItemDispenserButtonType::Decrease:
            return TEXT("Decrease");
        default:
            return TEXT("Unknown");
        }
    }

    void PlayWorldSoundForPlayers(
        UWorld* World,
        USoundBase* Sound,
        const FVector& SoundLocation)
    {
        if (!World || !Sound)
        {
            return;
        }

        for (TActorIterator<AEMRPlayerCharacter> It(World); It; ++It)
        {
            AEMRPlayerCharacter* TargetPlayer = *It;
            if (!TargetPlayer)
            {
                continue;
            }

            TargetPlayer->Client_PlayWorldSoundAtLocation(Sound, SoundLocation);
        }
    }

    bool CanUseDisabledIVStandTarget(const AActor* CarriedActor, const AActor* TargetActor)
    {
        const AEMRIntravenousMedicationStand* Stand = Cast<AEMRIntravenousMedicationStand>(TargetActor);
        const AEMRItemActor* CarriedItem = Cast<AEMRItemActor>(CarriedActor);
        const UEMRItemData* ItemData = CarriedItem ? CarriedItem->GetItemData() : nullptr;
        if (!Stand || !ItemData)
        {
            return false;
        }

        return ItemData->GetUseItemTag().MatchesTagExact(EMRTags::Event::Intravenous::UseBag);
    }
}

static const TCHAR* GetNetModeLabel(ENetMode NetMode)
{
	switch (NetMode)
	{
	case NM_Client:
		return TEXT("Client");
	case NM_ListenServer:
		return TEXT("ListenServer");
	case NM_DedicatedServer:
		return TEXT("DedicatedServer");
	default:
		return TEXT("Standalone");
	}
}

AEMRPlayerCharacter::AEMRPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    SetActorTickEnabled(false);

    // Character won't follow camera movement
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = true;
    // -1 in yaw make the rotation instant
    GetCharacterMovement()->RotationRate = FRotator(0.f, -1.f, 0.f);

    InteractableDetectionComponent = CreateDefaultSubobject<UEMRInteractableDetectionComponent>(TEXT("InteractableDetectionComponent"));
    PlayerMachineInputComponent = CreateDefaultSubobject<UEMRPlayerMachineInputComponent>(TEXT("PlayerMachineInputComponent"));
    PlayerWidgetInteractionComponent = CreateDefaultSubobject<UEMRPlayerWidgetInteractionComponent>(TEXT("PlayerWidgetInteractionComponent"));
    PlayerViewStateComponent = CreateDefaultSubobject<UEMRPlayerViewStateComponent>(TEXT("PlayerViewStateComponent"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->TargetArmLength = 0.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom);
	
	LookTarget = CreateDefaultSubobject<USceneComponent>(TEXT("LookTarget"));
	LookTarget->SetupAttachment(Camera);

    HubTabletComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("HubTabletComponent"));
    HubTabletComponent->SetupAttachment(GetRootComponent());
	
	WorldWidgetInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("WorldWidgetInteraction"));
	WorldWidgetInteraction->SetupAttachment(GetRootComponent());
	WorldWidgetInteraction->InteractionSource = EWidgetInteractionSource::CenterScreen;
	WorldWidgetInteraction->TraceChannel = WorldWidgetInteractionTraceChannel;
	WorldWidgetInteraction->InteractionDistance = WorldWidgetInteractionDistance;
	WorldWidgetInteraction->bEnableHitTesting = true;
	WorldWidgetInteraction->bShowDebug = false;

    FirstPersonCarriedItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonCarriedItemMesh"));
    if (FirstPersonCarriedItemMesh)
    {
    	FirstPersonCarriedItemMesh->SetupAttachment(Camera);
        FirstPersonCarriedItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FirstPersonCarriedItemMesh->SetGenerateOverlapEvents(false);
        FirstPersonCarriedItemMesh->SetHiddenInGame(true);
        FirstPersonCarriedItemMesh->SetOnlyOwnerSee(true);
        FirstPersonCarriedItemMesh->SetCastShadow(false);
        FirstPersonCarriedItemMesh->bCastDynamicShadow = false;
    }

    FirstPersonCarriedItemFilledOverlayMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonCarriedItemFilledOverlayMesh"));
    if (FirstPersonCarriedItemFilledOverlayMesh)
    {
        FirstPersonCarriedItemFilledOverlayMesh->SetupAttachment(FirstPersonCarriedItemMesh);
        FirstPersonCarriedItemFilledOverlayMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FirstPersonCarriedItemFilledOverlayMesh->SetGenerateOverlapEvents(false);
        FirstPersonCarriedItemFilledOverlayMesh->SetHiddenInGame(true);
        FirstPersonCarriedItemFilledOverlayMesh->SetVisibility(false, true);
        FirstPersonCarriedItemFilledOverlayMesh->SetOnlyOwnerSee(true);
        FirstPersonCarriedItemFilledOverlayMesh->SetCastShadow(false);
        FirstPersonCarriedItemFilledOverlayMesh->bCastDynamicShadow = false;
    }

	FirstPersonCarriedItemWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("FirstPersonCarriedItemWidget"));
	if (FirstPersonCarriedItemWidget)
	{
		FirstPersonCarriedItemWidget->SetupAttachment(FirstPersonCarriedItemMesh);
		FirstPersonCarriedItemWidget->SetWidgetSpace(EWidgetSpace::World);
		FirstPersonCarriedItemWidget->SetDrawAtDesiredSize(true);
		FirstPersonCarriedItemWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FirstPersonCarriedItemWidget->SetGenerateOverlapEvents(false);
		FirstPersonCarriedItemWidget->SetHiddenInGame(true);
		FirstPersonCarriedItemWidget->SetOnlyOwnerSee(true);
		FirstPersonCarriedItemWidget->SetCastShadow(false);
		FirstPersonCarriedItemWidget->bCastDynamicShadow = false;
		FirstPersonCarriedItemWidget->SetWindowFocusable(false);
	}

	ThirdPersonWatchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThirdPersonWatchMesh"));
	if (ThirdPersonWatchMesh)
	{
		ThirdPersonWatchMesh->SetupAttachment(GetRootComponent());
		ThirdPersonWatchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ThirdPersonWatchMesh->SetGenerateOverlapEvents(false);
	}

	WatchWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WatchWidgetComponent"));
	if (WatchWidgetComponent)
	{
		WatchWidgetComponent->SetupAttachment(ThirdPersonWatchMesh);
		WatchWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
		WatchWidgetComponent->SetDrawAtDesiredSize(true);
		WatchWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WatchWidgetComponent->SetGenerateOverlapEvents(false);
		WatchWidgetComponent->SetWindowFocusable(false);
	}


	ItemPlacementComponent = CreateDefaultSubobject<UEMRItemPlacementComponent>(TEXT("ItemPlacementComponent"));

	PlacementPreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlacementPreviewMesh"));
	if (PlacementPreviewMesh)
	{
		PlacementPreviewMesh->SetupAttachment(GetRootComponent());
		PlacementPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PlacementPreviewMesh->SetGenerateOverlapEvents(false);
		PlacementPreviewMesh->SetHiddenInGame(true);
		PlacementPreviewMesh->SetOnlyOwnerSee(true);
		PlacementPreviewMesh->SetCastShadow(false);
		PlacementPreviewMesh->bCastDynamicShadow = false;
	}
}

void AEMRPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bCameraTransitionActive)
    {
        UpdateCameraTransition(DeltaSeconds);
    }

    if (bWatchSocketBlendActive)
    {
        UpdateThirdPersonWatchBlend(DeltaSeconds);
    }

    if (bIsSeated && IsLocallyControlled())
    {
        UpdateLookTargetLocation(false);
    }

	if (bPlacementPreviewActive && IsLocallyControlled())
	{
		UpdatePlacementPreview();
	}

    if (IsLocallyControlled()
        && FirstPersonCarriedItemMesh
        && FirstPersonCarriedItemFilledOverlayMesh)
    {
        const AEMRItemActor* CarriedItemActor = Cast<AEMRItemActor>(CachedCarriedActor.Get());
        const bool bShouldShowBaseMesh = CarriedItemActor
            && FirstPersonCarriedItemMesh->GetStaticMesh() != nullptr
            && !FirstPersonCarriedItemMesh->bHiddenInGame;
        RefreshFirstPersonCarriedItemFilledOverlay(CarriedItemActor, bShouldShowBaseMesh);
    }

}


void AEMRPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UCapsuleComponent* PlayerCapsuleComponent = GetCapsuleComponent())
    {
        PlayerCapsuleComponent->SetCapsuleSize(FixedCapsuleRadius, FixedCapsuleHalfHeight, true);
    }

    CachedPlayerController = GetController<APlayerController>();

    if (InteractableDetectionComponent && IsLocallyControlled())
    {
        InteractableDetectionComponent->OnTargetChanged.AddDynamic(this, &ThisClass::HandleInteractableTargetChanged);
    }

    CacheDefaultCameraBoomTransform();
    HandleSeatedCameraStateChanged();
    UpdateTickState();

    if (CameraBoom)
    {
        AddTickPrerequisiteComponent(CameraBoom);
    }

    InitializeFirstPersonCarriedItemMesh();
	InitializeThirdPersonWatchMesh();
	RefreshAbilityActorInfoMesh();
    InitializeHubTabletComponent();
    BindToHubTabletState();
}

void AEMRPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromHubTabletState();
	Super::EndPlay(EndPlayReason);
}


void AEMRPlayerCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();

    CachedAbilitySystemComponent = nullptr;
    CachedPlayerController = GetController<APlayerController>();

    AddGameplayMappingContext();

    CacheDefaultCameraBoomTransform();
    HandleSeatedCameraStateChanged();
    UpdateTickState();

    if (CameraBoom)
    {
        AddTickPrerequisiteComponent(CameraBoom);
    }

    InitializeFirstPersonCarriedItemMesh();
	InitializeThirdPersonWatchMesh();
	RefreshAbilityActorInfoMesh();
    InitializeHubTabletComponent();
    BindToHubTabletState();
}


void AEMRPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
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
    EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleMoveInput);

    if (ToggleGameMenuInputAction)
    {
        EnhancedInputComponent->BindAction(ToggleGameMenuInputAction, ETriggerEvent::Started, this, &ThisClass::HandleToggleGameMenuInput);
    }

    if (ToggleDeveloperToolsInputAction)
    {
        EnhancedInputComponent->BindAction(ToggleDeveloperToolsInputAction, ETriggerEvent::Started, this, &ThisClass::HandleToggleDeveloperToolsInput);
    }

    if (PushToTalkInputAction)
    {
        EnhancedInputComponent->BindAction(PushToTalkInputAction, ETriggerEvent::Started, this, &ThisClass::HandlePushToTalkStarted);
        EnhancedInputComponent->BindAction(PushToTalkInputAction, ETriggerEvent::Completed, this, &ThisClass::HandlePushToTalkCompleted);
    }

    if (XRayMoveLeftInputAction)
    {
        EnhancedInputComponent->BindAction(XRayMoveLeftInputAction, ETriggerEvent::Started, this, &ThisClass::HandleXRayMoveLeftStarted);
        EnhancedInputComponent->BindAction(XRayMoveLeftInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleXRayMoveLeftCompleted);
    }

    if (XRayMoveRightInputAction)
    {
        EnhancedInputComponent->BindAction(XRayMoveRightInputAction, ETriggerEvent::Started, this, &ThisClass::HandleXRayMoveRightStarted);
        EnhancedInputComponent->BindAction(XRayMoveRightInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleXRayMoveRightCompleted);
    }

    if (XRayMoveForwardInputAction)
    {
        EnhancedInputComponent->BindAction(XRayMoveForwardInputAction, ETriggerEvent::Started, this, &ThisClass::HandleXRayMoveForwardStarted);
        EnhancedInputComponent->BindAction(XRayMoveForwardInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleXRayMoveForwardCompleted);
    }

    if (XRayMoveBackwardInputAction)
    {
        EnhancedInputComponent->BindAction(XRayMoveBackwardInputAction, ETriggerEvent::Started, this, &ThisClass::HandleXRayMoveBackwardStarted);
        EnhancedInputComponent->BindAction(XRayMoveBackwardInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleXRayMoveBackwardCompleted);
    }

    if (UltrasoundMoveLeftInputAction)
    {
        EnhancedInputComponent->BindAction(UltrasoundMoveLeftInputAction, ETriggerEvent::Started, this, &ThisClass::HandleUltrasoundMoveLeftStarted);
        EnhancedInputComponent->BindAction(UltrasoundMoveLeftInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleUltrasoundMoveLeftCompleted);
    }

    if (UltrasoundMoveRightInputAction)
    {
        EnhancedInputComponent->BindAction(UltrasoundMoveRightInputAction, ETriggerEvent::Started, this, &ThisClass::HandleUltrasoundMoveRightStarted);
        EnhancedInputComponent->BindAction(UltrasoundMoveRightInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleUltrasoundMoveRightCompleted);
    }

    if (UltrasoundMoveForwardInputAction)
    {
        EnhancedInputComponent->BindAction(UltrasoundMoveForwardInputAction, ETriggerEvent::Started, this, &ThisClass::HandleUltrasoundMoveForwardStarted);
        EnhancedInputComponent->BindAction(UltrasoundMoveForwardInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleUltrasoundMoveForwardCompleted);
    }

    if (UltrasoundMoveBackwardInputAction)
    {
        EnhancedInputComponent->BindAction(UltrasoundMoveBackwardInputAction, ETriggerEvent::Started, this, &ThisClass::HandleUltrasoundMoveBackwardStarted);
        EnhancedInputComponent->BindAction(UltrasoundMoveBackwardInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleUltrasoundMoveBackwardCompleted);
    }

    if (UltrasoundSliderAdjustInputAction)
    {
        EnhancedInputComponent->BindAction(UltrasoundSliderAdjustInputAction, ETriggerEvent::Triggered, this, &ThisClass::HandleUltrasoundAdjustInputTriggered);
        EnhancedInputComponent->BindAction(UltrasoundSliderAdjustInputAction, ETriggerEvent::Completed, this, &ThisClass::HandleUltrasoundAdjustInputCompleted);
    }

    for (const TPair<EAbilityInputID, TObjectPtr<const UInputAction>>& InputActionPair : GameplayAbilityInputActions)
    {
		if (!InputActionPair.Value)
		{
			ensureMsgf(false, TEXT("Ability InputAction missing for %d on %s"), static_cast<int32>(InputActionPair.Key), *GetName());
			continue;
		}
		
		EnhancedInputComponent->BindAction(InputActionPair.Value, ETriggerEvent::Started, this, &ThisClass::HandleAbilityInput, InputActionPair.Key);
	}
}


UAbilitySystemComponent* AEMRPlayerCharacter::GetAbilitySystemComponent() const
{
	if (!CachedAbilitySystemComponent)
	{
		const AEMRPlayerState* PS = GetPlayerState<AEMRPlayerState>();
		if (PS)
		{
			CachedAbilitySystemComponent = PS->GetAbilitySystemComponent();
		}
	}

	return CachedAbilitySystemComponent;
}


bool AEMRPlayerCharacter::TryHandleHoveredWidgetInteract(UWidgetComponent* TargetWidget)
{
	if (!IsLocallyControlled() || !TargetWidget || !WorldWidgetInteraction)
	{
		return false;
	}

	if (WorldWidgetInteraction->GetHoveredWidgetComponent() != TargetWidget)
	{
		return false;
	}

	WorldWidgetInteraction->PressPointerKey(EKeys::LeftMouseButton);
	WorldWidgetInteraction->ReleasePointerKey(EKeys::LeftMouseButton);
	return true;
}

void AEMRPlayerCharacter::HandleToggleWatchAbility()
{
	if (!IsLocallyControlled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Watch] HandleToggleWatchAbility ignored: not locally controlled."));
		return;
	}

	if (!CanUseWatch())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Watch] HandleToggleWatchAbility blocked: CanUseWatch false."));
		return;
	}

	const bool bDesired = !bLookingAtWatch;
	UE_LOG(LogTemp, Log, TEXT("[Watch] Toggle requested. Desired=%d Authority=%d"), bDesired ? 1 : 0, HasAuthority() ? 1 : 0);
	if (HasAuthority())
	{
		SetLookingAtWatch(bDesired);
	}
	else
	{
		ServerSetLookingAtWatch(bDesired);
	}
}

void AEMRPlayerCharacter::SetLookingAtWatch(bool bEnabled)
{
	if (bLookingAtWatch == bEnabled)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Watch] SetLookingAtWatch no-op. Value=%d"), bEnabled ? 1 : 0);
		return;
	}

	bLookingAtWatch = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("[Watch] SetLookingAtWatch -> %d"), bEnabled ? 1 : 0);
	RefreshThirdPersonWatchMesh();
}

void AEMRPlayerCharacter::SetWatchSocketLookActive(bool bEnabled)
{
	if (bWatchSocketLookActive == bEnabled)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Watch] SetWatchSocketLookActive no-op. Value=%d"), bEnabled ? 1 : 0);
		return;
	}

	bWatchSocketLookActive = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("[Watch] SetWatchSocketLookActive -> %d"), bEnabled ? 1 : 0);
	RefreshThirdPersonWatchMesh();
}

void AEMRPlayerCharacter::CancelActiveMontagesForSeatedChange()
{
	USkeletalMeshComponent* MeshComponent = ResolvePlayerMeshComponent();
	if (!MeshComponent)
	{
		MeshComponent = GetMesh();
	}

	if (!MeshComponent)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance || !AnimInstance->IsAnyMontagePlaying())
	{
		return;
	}

	AnimInstance->Montage_Stop(0.1f);
}

void AEMRPlayerCharacter::ServerSetLookingAtWatch_Implementation(bool bInLookingAtWatch)
{
	if (!CanUseWatch())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Watch] ServerSetLookingAtWatch blocked: CanUseWatch false."));
		SetLookingAtWatch(false);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Watch] ServerSetLookingAtWatch -> %d"), bInLookingAtWatch ? 1 : 0);
	SetLookingAtWatch(bInLookingAtWatch);
}

void AEMRPlayerCharacter::ServerSetWatchSocketLookActive_Implementation(bool bInWatchSocketLookActive)
{
	if (!CanUseWatch())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Watch] ServerSetWatchSocketLookActive blocked: CanUseWatch false."));
		SetWatchSocketLookActive(false);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Watch] ServerSetWatchSocketLookActive -> %d"), bInWatchSocketLookActive ? 1 : 0);
	SetWatchSocketLookActive(bInWatchSocketLookActive);
}

void AEMRPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	CachedAbilitySystemComponent = nullptr;
	InitializeThirdPersonWatchMesh();
	RefreshAbilityActorInfoMesh();
	BindToHubTabletState();
}

void AEMRPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRPlayerCharacter, bIsSeated);
    DOREPLIFETIME(AEMRPlayerCharacter, SeatedAnimationTag);
    DOREPLIFETIME_CONDITION(AEMRPlayerCharacter, LookTargetLocation, COND_SkipOwner);
	DOREPLIFETIME(AEMRPlayerCharacter, bLookingAtWatch);
	DOREPLIFETIME(AEMRPlayerCharacter, bWatchSocketLookActive);
}

void AEMRPlayerCharacter::SetIsSeated(bool bInIsSeated)
{
    if (bIsSeated == bInIsSeated)
    {
        return;
    }

    bIsSeated = bInIsSeated;
    if (PlayerViewStateComponent)
    {
        PlayerViewStateComponent->ResetLookTargetReplicationState();
    }
    UE_LOG(LogTemp, Log, TEXT("[ReceptionSeat] %s SetIsSeated=%d (Local=%d)"),
        *GetName(), bIsSeated ? 1 : 0, IsLocallyControlled() ? 1 : 0);
	CancelActiveMontagesForSeatedChange();
    RefreshSeatedRenderState();
    HandleSeatedCameraStateChanged();
    UpdateTickState();
	RefreshThirdPersonWatchMesh();

    if (bIsSeated && IsLocallyControlled())
    {
        UpdateLookTargetLocation(true);
    }
    else if (!bIsSeated && bSeatedRotateBodyWithCamera)
    {
        SetSeatedRotateBodyWithCamera(false);
    }

    if (!bIsSeated)
    {
        SeatedAnimationTag = FGameplayTag();
    }

    RefreshCurrentTargetInteractionFeedback();
}

void AEMRPlayerCharacter::RequestInstantSeatedCameraTransition()
{
    if (!IsLocallyControlled())
    {
        return;
    }

    bInstantSeatedCameraTransitionRequested = true;
}

void AEMRPlayerCharacter::ServerSetIsSeated_Implementation(bool bInIsSeated)    
{
    SetIsSeated(bInIsSeated);
}

void AEMRPlayerCharacter::SetSeatedAnimationTag_Implementation(const FGameplayTag& InTag)
{
    SeatedAnimationTag = InTag;
}

FGameplayTag AEMRPlayerCharacter::GetSeatedAnimationTag_Implementation() const
{
    return SeatedAnimationTag;
}

USkeletalMeshComponent* AEMRPlayerCharacter::GetPlayerMeshComponentForSeating() const
{
    return ResolvePlayerMeshComponent();
}


void AEMRPlayerCharacter::SetSeatedRotateBodyWithCamera(bool bEnable)
{
    if (bSeatedRotateBodyWithCamera == bEnable)
    {
        return;
    }

    bSeatedRotateBodyWithCamera = bEnable;
    UE_LOG(LogTemp, Log, TEXT("[ReceptionSeat] %s SetSeatedRotateBodyWithCamera=%d (bUseControllerRotationYaw=%d)"),
        *GetName(), bSeatedRotateBodyWithCamera ? 1 : 0, bUseControllerRotationYaw ? 1 : 0);

    if (bSeatedRotateBodyWithCamera)
    {
        if (!bHasSavedSeatedRotation)
        {
            bSavedUseControllerRotationYaw = bUseControllerRotationYaw;
            bHasSavedSeatedRotation = true;
        }

        bUseControllerRotationYaw = true;
    }
    else
    {
        if (bHasSavedSeatedRotation)
        {
            bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
            bHasSavedSeatedRotation = false;
        }
    }

}

void AEMRPlayerCharacter::ServerSetLookTargetLocation_Implementation(const FVector_NetQuantize10& InLocation)
{
    if (!HasAuthority() || !bIsSeated)
    {
        return;
    }

    LookTargetLocation = InLocation;
}

void AEMRPlayerCharacter::ServerSetLookTargetLocationUnreliable_Implementation(const FVector_NetQuantize10& InLocation)
{
    if (!HasAuthority() || !bIsSeated)
    {
        return;
    }

    LookTargetLocation = InLocation;
}

void AEMRPlayerCharacter::OnRep_IsSeated()
{
    if (IsLocallyControlled())
    {
        bInstantSeatedCameraTransitionRequested = true;
    }

	CancelActiveMontagesForSeatedChange();
    RefreshSeatedRenderState();
    HandleSeatedCameraStateChanged();
    const bool bShouldRotateBodyWithCamera = bIsSeated && IsLocallyControlled() && ShouldRotateBodyWithCameraForCurrentSeat();
    if (bShouldRotateBodyWithCamera)
    {
        SetSeatedRotateBodyWithCamera(true);
        UpdateLookTargetLocation(true);
    }
    else if (bSeatedRotateBodyWithCamera)
    {
        SetSeatedRotateBodyWithCamera(false);
    }
    UpdateTickState();
	RefreshThirdPersonWatchMesh();
    RefreshCurrentTargetInteractionFeedback();
}

void AEMRPlayerCharacter::OnRep_SeatedAnimationTag()
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (bIsSeated)
    {
        SetSeatedRotateBodyWithCamera(ShouldRotateBodyWithCameraForCurrentSeat());
    }
    else if (bSeatedRotateBodyWithCamera)
    {
        SetSeatedRotateBodyWithCamera(false);
    }
}

void AEMRPlayerCharacter::OnRep_LookingAtWatch()
{
	RefreshThirdPersonWatchMesh();
}

void AEMRPlayerCharacter::OnRep_WatchSocketLookActive()
{
	RefreshThirdPersonWatchMesh();
}

void AEMRPlayerCharacter::RefreshSeatedRenderState()
{
    TArray<AActor*> ActorHierarchy;
    GatherAttachedActors(this, ActorHierarchy);
    for (AActor* HierarchyActor : ActorHierarchy)
    {
        if (!HierarchyActor)
        {
            continue;
        }

        TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(HierarchyActor);
        for (UPrimitiveComponent* Component : PrimitiveComponents)
        {
            if (!Component)
            {
                continue;
            }

            if (USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Component))
            {
                Skinned->InvalidateCachedBounds();
            }

            Component->UpdateBounds();
            Component->MarkRenderTransformDirty();
            Component->MarkRenderDynamicDataDirty();
        }
    }
}

USkeletalMeshComponent* AEMRPlayerCharacter::ResolveSeatedCameraMesh() const
{
    UActorComponent* Component = SeatedCameraMesh.GetComponent(const_cast<AEMRPlayerCharacter*>(this));
    return Cast<USkeletalMeshComponent>(Component);
}

USkeletalMeshComponent* AEMRPlayerCharacter::ResolvePlayerMeshComponent() const
{
    UActorComponent* Component = PlayerMeshComponent.GetComponent(const_cast<AEMRPlayerCharacter*>(this));
    return Cast<USkeletalMeshComponent>(Component);
}

bool AEMRPlayerCharacter::CanUseSeatedCameraSocket(const USkeletalMeshComponent* InMesh) const
{
    return InMesh && InMesh->DoesSocketExist(SeatedCameraSocketName);
}

void AEMRPlayerCharacter::HandleSeatedCameraStateChanged()
{
    if (!IsLocallyControlled() || !CameraBoom)
    {
        return;
    }

    if (!bCameraTransitionActive && bIsSeated == bCameraAttachedToSeat)
    {
        return;
    }

    StartCameraTransition(bIsSeated);
}

float AEMRPlayerCharacter::ResolveSeatedCameraBlendDuration()
{
    const float Duration = bInstantSeatedCameraTransitionRequested ? 0.f : SeatedCameraBlendDuration;
    bInstantSeatedCameraTransitionRequested = false;
    return Duration;
}

void AEMRPlayerCharacter::StartCameraTransition(bool bToSeated)
{
    if (!CameraBoom)
    {
        return;
    }

    if (bToSeated)
    {
        USkeletalMeshComponent* TargetMesh = ResolveSeatedCameraMesh();
        if (!CanUseSeatedCameraSocket(TargetMesh))
        {
            return;
        }

        CameraTransitionTargetWorld = TargetMesh->GetSocketTransform(SeatedCameraSocketName, RTS_World);
    }
    else
    {
        CameraTransitionTargetWorld = GetDefaultCameraBoomWorldTransform();
    }

    CameraBoom->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

    CameraTransitionStartWorld = CameraBoom->GetComponentTransform();
    CameraTransitionElapsed = 0.f;
    const float EffectiveBlendDuration = ResolveSeatedCameraBlendDuration();
    ActiveSeatedCameraBlendDuration = EffectiveBlendDuration;
    bCameraTransitionActive = true;
    bCameraTransitionToSeated = bToSeated;
    UpdateTickState();

    if (EffectiveBlendDuration <= 0.f)
    {
        UpdateCameraTransition(0.f);
    }
}

void AEMRPlayerCharacter::UpdateCameraTransition(float DeltaSeconds)
{
    if (!bCameraTransitionActive || !CameraBoom)
    {
        return;
    }

    CameraTransitionElapsed += DeltaSeconds;
    const float Duration = FMath::Max(ActiveSeatedCameraBlendDuration, 0.f);
    const float Alpha = Duration > 0.f ? FMath::Clamp(CameraTransitionElapsed / Duration, 0.f, 1.f) : 1.f;

    const FVector StartLocation = CameraTransitionStartWorld.GetLocation();
    const FVector TargetLocation = CameraTransitionTargetWorld.GetLocation();
    const FVector NewLocation = FMath::Lerp(StartLocation, TargetLocation, Alpha);

    const FQuat StartRotation = CameraTransitionStartWorld.GetRotation();
    const FQuat TargetRotation = CameraTransitionTargetWorld.GetRotation();
    const FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, Alpha).GetNormalized();

    CameraBoom->SetWorldLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::None);

    if (Alpha >= 1.f)
    {
        FinishCameraTransition();
    }
}

void AEMRPlayerCharacter::FinishCameraTransition()
{
    bCameraTransitionActive = false;
    UpdateTickState();

    if (!CameraBoom)
    {
        return;
    }

    if (bCameraTransitionToSeated)
    {
        USkeletalMeshComponent* TargetMesh = ResolveSeatedCameraMesh();
        if (!CanUseSeatedCameraSocket(TargetMesh))
        {
            return;
        }

        CameraBoom->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SeatedCameraSocketName);
        bCameraAttachedToSeat = true;
    }
    else
    {
        if (USceneComponent* RootComp = GetRootComponent())
        {
            CameraBoom->AttachToComponent(RootComp, FAttachmentTransformRules::KeepWorldTransform);
            CameraBoom->SetRelativeTransform(DefaultCameraBoomRelativeTransform);
        }

        bCameraAttachedToSeat = false;
    }
} 

FTransform AEMRPlayerCharacter::GetDefaultCameraBoomWorldTransform() const
{
    if (USceneComponent* RootComp = GetRootComponent())
    {
        return DefaultCameraBoomRelativeTransform * RootComp->GetComponentTransform();
    }

    return CameraBoom ? CameraBoom->GetComponentTransform() : FTransform::Identity;
}

void AEMRPlayerCharacter::CacheDefaultCameraBoomTransform()
{
    if (CameraBoom)
    {
        DefaultCameraBoomRelativeTransform = CameraBoom->GetRelativeTransform();
    }
}

bool AEMRPlayerCharacter::ShouldRotateBodyWithCameraForCurrentSeat() const
{
    return !SeatedAnimationTag.MatchesTagExact(EMRTags::SeatAnimation::Hub);
}


void AEMRPlayerCharacter::UpdateLookTargetLocation(bool bForceReplication)
{
    if (!IsLocallyControlled() || !bIsSeated)
    {
        return;
    }
	

    const FVector NewLocation = LookTarget->GetComponentLocation();
    const UWorld* World = GetWorld();
    const float Now = World ? World->GetTimeSeconds() : 0.0f;
    bool bUseReliableRpc = bForceReplication;
    const bool bShouldReplicate = PlayerViewStateComponent
    ? PlayerViewStateComponent->ShouldSendLookTargetUpdate(
            Now,
            NewLocation,
            LookTargetLocation,
            bForceReplication,
            LookTargetReplicationDistanceThreshold,
            LookTargetReplicationMinInterval,
            bUseReliableRpc)
    : (bForceReplication || !NewLocation.Equals(LookTargetLocation, FMath::Max(0.01f, LookTargetReplicationDistanceThreshold)));

    LookTargetLocation = NewLocation;

    if (bShouldReplicate)
    {
        if (HasAuthority())
        {
            LookTargetLocation = NewLocation;
        }
        else
        {
            if (bUseReliableRpc)
            {
                ServerSetLookTargetLocation(NewLocation);
            }
            else
            {
                ServerSetLookTargetLocationUnreliable(NewLocation);
            }
        }
    }
}

void AEMRPlayerCharacter::UpdateTickState()
{
    const bool bShouldTick = bCameraTransitionActive
    || bWatchSocketBlendActive
    || (bIsSeated && IsLocallyControlled())
    || (bPlacementPreviewActive && IsLocallyControlled());
    SetActorTickEnabled(bShouldTick);
}



void AEMRPlayerCharacter::AddGameplayMappingContext() const
{
	if (CachedPlayerController.IsValid() && GameplayInputMappingContext)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = CachedPlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		
		if (InputSubsystem)
		{
			constexpr int32 MappingPriority = 0;
			InputSubsystem->RemoveMappingContext(GameplayInputMappingContext);

			FModifyContextOptions AddMappingOptions;
			AddMappingOptions.bNotifyUserSettings = true;
			InputSubsystem->AddMappingContext(GameplayInputMappingContext, MappingPriority, AddMappingOptions);	
		}
	}
}


void AEMRPlayerCharacter::HandleLookInput(const FInputActionValue& InputActionValue) 
{
	FVector2D InputValue = InputActionValue.Get<FVector2D>();
	if (CachedPlayerController.IsValid() && !InputValue.IsNearlyZero())
	{
		const UEMRGameUserSettings* GameUserSettings = UEMRGameUserSettings::Get();
		const UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(CachedPlayerController->GetLocalPlayer());
		const ECommonInputType CurrentInputType = CommonInputSubsystem ? CommonInputSubsystem->GetCurrentInputType() : ECommonInputType::MouseAndKeyboard;

		float Sensitivity = LookSensitivity;
		bool bInvertX = false;
		bool bInvertY = false;

		if (GameUserSettings)
		{
			if (CurrentInputType == ECommonInputType::Gamepad)
			{
				Sensitivity *= GameUserSettings->GetGamepadSensitivity();
				bInvertX = GameUserSettings->GetGamepadInvertX();
				bInvertY = GameUserSettings->GetGamepadInvertY();
			}
			else
			{
				Sensitivity *= GameUserSettings->GetMouseSensitivity();
				bInvertX = GameUserSettings->GetMouseInvertX();
				bInvertY = GameUserSettings->GetMouseInvertY();
			}
		}

		InputValue *= Sensitivity;
		InputValue.X *= bInvertX ? -1.f : 1.f;
		InputValue.Y *= bInvertY ? -1.f : 1.f;

		AddControllerYawInput(InputValue.X);
		AddControllerPitchInput(InputValue.Y);
	}
}


void AEMRPlayerCharacter::HandleMoveInput(const FInputActionValue& InputActionValue)
{
    const FVector2D InputValue = InputActionValue.Get<FVector2D>();
    LastMoveInputAxis = InputValue;

    if (bIsSeated)
    {
        LastMoveInputAxis = FVector2D::ZeroVector;
        return;
    }

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

void AEMRPlayerCharacter::HandleToggleGameMenuInput(const FInputActionValue& InputActionValue)
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (AEMRPlayerController* EMRPlayerController = Cast<AEMRPlayerController>(GetController()))
    {
        EMRPlayerController->ToggleGameplayMenu();
    }
}

void AEMRPlayerCharacter::HandleToggleDeveloperToolsInput(const FInputActionValue& InputActionValue)
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (AEMRPlayerController* EMRPlayerController = Cast<AEMRPlayerController>(GetController()))
    {
        EMRPlayerController->ToggleDeveloperTools();
    }
}

void AEMRPlayerCharacter::HandlePushToTalkStarted(const FInputActionValue& InputActionValue)
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (const APlayerController* LocalController = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = LocalController->GetLocalPlayer())
        {
            if (UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = LocalPlayer->GetSubsystem<UProximityVoiceLocalPlayerSubsystem>())
            {
                VoiceSubsystem->SetPushToTalkActive(true);
            }
        }
    }
}

void AEMRPlayerCharacter::HandlePushToTalkCompleted(const FInputActionValue& InputActionValue)
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (const APlayerController* LocalController = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = LocalController->GetLocalPlayer())
        {
            if (UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = LocalPlayer->GetSubsystem<UProximityVoiceLocalPlayerSubsystem>())
            {
                VoiceSubsystem->SetPushToTalkActive(false);
            }
        }
    }
}


void AEMRPlayerCharacter::HandleInteractableTargetChanged(AActor* NewTarget)
{
    if (PreviousInteractableTarget.IsValid())
    {
        if (UEMRInteractableComponent* PreviousInteractableComponent = PreviousInteractableTarget->FindComponentByClass<UEMRInteractableComponent>())
        {
            PreviousInteractableComponent->ClearHighlight();
            PreviousInteractableComponent->HideNameWidget();
        }
    }

    PreviousInteractableTarget = NewTarget;

    if (!IsValid(NewTarget))
    {
        return;
    }

    if (!NewTarget->GetClass()->ImplementsInterface(UEMRInteractableInterface::StaticClass()))
    {
        return;
    }

    if (UEMRInteractableComponent* InteractableComponent = NewTarget->FindComponentByClass<UEMRInteractableComponent>())
    {
        if (InteractableComponent->GetOutlineProfile() == EEMRInteractableOutlineProfile::GAInteract
            && IsGAInteractTargetCurrentlyActiveInteraction(NewTarget))
        {
            InteractableComponent->ClearHighlight();
            InteractableComponent->HideNameWidget();
            return;
        }

        InteractableComponent->ApplyHighlight();
        InteractableComponent->ShowNameWidget();
    }
}

bool AEMRPlayerCharacter::IsGAInteractTargetCurrentlyActiveInteraction(const AActor* Target) const
{
    if (!Target)
    {
        return false;
    }

    if (bIsSeated)
    {
        if (const AEMRReceptionMachine* SeatedReceptionMachine = FindSeatedReceptionMachine())
        {
            if (Target == SeatedReceptionMachine)
            {
                return true;
            }
        }
    }

    return Target == ActiveXRayMachine.Get()
    || Target == ActiveUltrasoundMachine.Get()
    || Target == ActiveCTScanMachine.Get()
    || Target == ActiveIVStand.Get()
    || Target == ActiveOxygenMachine.Get()
    || Target == ActiveLabAnalyzerMachine.Get();
}

void AEMRPlayerCharacter::RefreshCurrentTargetInteractionFeedback()
{
    if (!IsLocallyControlled() || !InteractableDetectionComponent)
    {
        return;
    }

    HandleInteractableTargetChanged(InteractableDetectionComponent->GetCurrentTarget());
}

bool AEMRPlayerCharacter::IsGAInteractInput(EAbilityInputID AbilityInputID) const
{
    const TSubclassOf<UGameplayAbility>* AbilityClass = StartupAbilities.Find(AbilityInputID);
    return AbilityClass
    && AbilityClass->Get()
    && AbilityClass->Get()->IsChildOf(UEMRGameplayAbility_Interact::StaticClass());
}

bool AEMRPlayerCharacter::TryHandleActiveGAInteractCancel()
{
    constexpr EAbilityInputID CancelInput = EAbilityInputID::Cancel;

    if (ActiveXRayMachine.IsValid())
    {
        return TryHandleXRayCancelInput(CancelInput);
    }

    if (ActiveUltrasoundMachine.IsValid())
    {
        return TryHandleUltrasoundCancelInput(CancelInput);
    }

    if (ActiveCTScanMachine.IsValid())
    {
        return TryHandleCTScanCancelInput(CancelInput);
    }

    if (ActiveIVStand.IsValid())
    {
        return TryHandleIVStandCancelInput(CancelInput);
    }

    if (ActiveOxygenMachine.IsValid())
    {
        return TryHandleOxygenMachineCancelInput(CancelInput);
    }

    if (ActiveLabAnalyzerMachine.IsValid())
    {
        return TryHandleLabAnalyzerCancelInput(CancelInput);
    }

    if (bIsSeated && FindSeatedReceptionMachine())
    {
        return TryHandleReceptionCancelInput(CancelInput);
    }

    return false;
}

bool AEMRPlayerCharacter::TryHandleGAInteractStopInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || !IsGAInteractInput(AbilityInputID))
    {
        return false;
    }

    return TryHandleActiveGAInteractCancel();
}


void AEMRPlayerCharacter::HandleAbilityInput(EAbilityInputID AbilityInputID)    
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerCharacter] HandleAbilityInput - ASC not available"));
        return;
    }

    if (AbilityInputID == EAbilityInputID::Cancel && TryHandleAnchoredCarryItemCancelInput(AbilityInputID))
    {
        return;
    }

    if (TryHandleGAInteractStopInput(AbilityInputID))
    {
        return;
    }

    if (AbilityInputID == EAbilityInputID::Cancel && TryHandleActiveGAInteractCancel())
    {
        return;
    }

    if (AbilityInputID == EAbilityInputID::Confirm)
    {
        if (!IsLocallyControlled())
        {
            EMR_CONFIRM_LOG(Warning, "Ignored non-local confirm player=%s netmode=%s role=%s auth=%d",
                *GetNameSafe(this), GetNetModeLabel(GetNetMode()), GetLocalRoleLabel(GetLocalRole()), HasAuthority() ? 1 : 0);
            return;
        }

        AActor* CarriedActor = FindCarriedActor();
        const bool bHasCarriedItem = CarriedActor != nullptr;
        FVector ViewLocation = Camera ? Camera->GetComponentLocation() : GetPawnViewLocation();
        FRotator ViewRotation = Camera ? Camera->GetComponentRotation() : GetViewRotation();
        if (APlayerController* PlayerController = CachedPlayerController.Get())
        {
            PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
        }
        const FVector ViewDirection = ViewRotation.Vector();

        EMR_CONFIRM_LOG(Warning,
            "Pressed player=%s netmode=%s role=%s auth=%d carried=%s hasCarried=%d active(ultra=%s ct=%s iv=%s oxy=%s lab=%s) viewLoc=%s viewDir=%s",
            *GetNameSafe(this),
            GetNetModeLabel(GetNetMode()),
            GetLocalRoleLabel(GetLocalRole()),
            HasAuthority() ? 1 : 0,
            *GetNameSafe(CarriedActor),
            bHasCarriedItem ? 1 : 0,
            *GetNameSafe(ActiveUltrasoundMachine.Get()),
            *GetNameSafe(ActiveCTScanMachine.Get()),
            *GetNameSafe(ActiveIVStand.Get()),
            *GetNameSafe(ActiveOxygenMachine.Get()),
            *GetNameSafe(ActiveLabAnalyzerMachine.Get()),
            *ViewLocation.ToCompactString(),
            *ViewDirection.ToCompactString());

        if (bHasCarriedItem)
        {
            const bool bCanUseWorldWidgetsWhileCarried =
                CarriedActor->GetClass()->ImplementsInterface(UEMRCarriedWorldWidgetInteractionInterface::StaticClass()) &&
                IEMRCarriedWorldWidgetInteractionInterface::Execute_CanInteractWithWorldWidgetsWhileCarried(CarriedActor);

            FHitResult WidgetHit;
            if (HasWorldWidgetUnderCrosshair(WidgetHit))
            {
                if (bCanUseWorldWidgetsWhileCarried)
                {
                    if (TryHandleWorldWidgetClick(EAbilityInputID::Confirm))
                    {
                        EMR_CONFIRM_LOG(Warning, "Consumed by carried-item world widget click player=%s carried=%s widgetActor=%s component=%s dist=%.2f",
                            *GetNameSafe(this),
                            *GetNameSafe(CarriedActor),
                            *GetNameSafe(WidgetHit.GetActor()),
                            *GetNameSafe(WidgetHit.GetComponent()),
                            WidgetHit.Distance);
                        return;
                    }
                }
                else
                {
                    EMR_CONFIRM_LOG(Warning, "Consumed by carried-item widget block player=%s carried=%s widgetActor=%s component=%s dist=%.2f",
                        *GetNameSafe(this),
                        *GetNameSafe(CarriedActor),
                        *GetNameSafe(WidgetHit.GetActor()),
                        *GetNameSafe(WidgetHit.GetComponent()),
                        WidgetHit.Distance);
                    return;
                }
            }

            if (!bCanUseWorldWidgetsWhileCarried)
            {
                EMR_CONFIRM_LOG(Warning, "Sending Server_HandleConfirmInteraction for carried item player=%s carried=%s",
                    *GetNameSafe(this), *GetNameSafe(CarriedActor));
                Server_HandleConfirmInteraction(ViewLocation, ViewDirection);
                return;
            }

            EMR_CONFIRM_LOG(Warning, "Carried item allows confirm passthrough player=%s carried=%s",
                *GetNameSafe(this), *GetNameSafe(CarriedActor));
        }

        if (ActiveUltrasoundMachine.IsValid())
        {
            if (TryHandleUltrasoundSliderSelect(EAbilityInputID::Confirm))
            {
                EMR_CONFIRM_LOG(Warning, "Consumed by ultrasound slider select player=%s machine=%s",
                    *GetNameSafe(this), *GetNameSafe(ActiveUltrasoundMachine.Get()));
                return;
            }

            const bool bHandledByWidget = TryHandleWorldWidgetClick(EAbilityInputID::Confirm);
            EMR_CONFIRM_LOG(Warning, "Ultrasound branch world-widget result player=%s machine=%s handled=%d",
                *GetNameSafe(this), *GetNameSafe(ActiveUltrasoundMachine.Get()), bHandledByWidget ? 1 : 0);
            return;
        }

        if (ActiveCTScanMachine.IsValid())
        {
            if (TryHandleCTScanButtonSelect(EAbilityInputID::Confirm))
            {
                EMR_CONFIRM_LOG(Warning, "Consumed by CT button select player=%s machine=%s",
                    *GetNameSafe(this), *GetNameSafe(ActiveCTScanMachine.Get()));
                return;
            }

            const bool bHandledByWidget = TryHandleWorldWidgetClick(EAbilityInputID::Confirm);
            EMR_CONFIRM_LOG(Warning, "CT branch world-widget result player=%s machine=%s handled=%d",
                *GetNameSafe(this), *GetNameSafe(ActiveCTScanMachine.Get()), bHandledByWidget ? 1 : 0);
            return;
        }

        if (ActiveIVStand.IsValid())
        {
            if (TryHandleIVStandConfirm(EAbilityInputID::Confirm))
            {
                EMR_CONFIRM_LOG(Warning, "Consumed by IV stand confirm player=%s stand=%s",
                    *GetNameSafe(this), *GetNameSafe(ActiveIVStand.Get()));
                return;
            }

            EMR_CONFIRM_LOG(Warning, "IV stand active but confirm not consumed player=%s stand=%s",
                *GetNameSafe(this), *GetNameSafe(ActiveIVStand.Get()));
            return;
        }

        if (TryHandleItemDispenserButtonSelect(EAbilityInputID::Confirm))
        {
            EMR_CONFIRM_LOG(Warning, "Consumed by item dispenser button selection player=%s", *GetNameSafe(this));
            return;
        }

        if (TryHandleLabAnalyzerButtonSelect(EAbilityInputID::Confirm))
        {
            EMR_CONFIRM_LOG(Warning, "Consumed by lab analyzer button selection player=%s machine=%s",
                *GetNameSafe(this), *GetNameSafe(ActiveLabAnalyzerMachine.Get()));
            return;
        }

        if (TryHandleOvertimeStopTerminalButtonSelect(EAbilityInputID::Confirm))
        {
            return;
        }

        FHitResult ItemHit;
        if (FindBestItemHit(ViewLocation, ViewDirection, ItemHit))
        {
            EMR_CONFIRM_LOG(Warning, "Sending Server_HandleConfirmInteraction for local item hit player=%s actor=%s component=%s dist=%.2f",
                *GetNameSafe(this), *GetNameSafe(ItemHit.GetActor()), *GetNameSafe(ItemHit.GetComponent()), ItemHit.Distance);
            Server_HandleConfirmInteraction(ViewLocation, ViewDirection);
            return;
        }

        if (TryHandleWorldWidgetClick(EAbilityInputID::Confirm))
        {
            EMR_CONFIRM_LOG(Warning, "Consumed by world widget click player=%s", *GetNameSafe(this));
            return;
        }

        // Fallback to authoritative interaction resolution on the server.
        // This keeps item pickup reliable when a newly replicated item is not yet
        // fully traceable on the local proxy.
        EMR_CONFIRM_LOG(Warning, "Fallback Server_HandleConfirmInteraction player=%s", *GetNameSafe(this));
        Server_HandleConfirmInteraction(ViewLocation, ViewDirection);
        return;
    }

    if (TryHandleOxygenMaskCancelInput(AbilityInputID))
    {
        return;
    }

    if (TryHandleWorldWidgetClick(AbilityInputID))
    {
        UE_LOG(LogTemp, Log, TEXT("[PlayerCharacter] Confirm consumed by world widget click player=%s"),
            *GetNameSafe(this));
        return;
    }

    ASC->AbilityLocalInputPressed(static_cast<int32>(AbilityInputID));      
}

bool AEMRPlayerCharacter::TryHandleReceptionCancelInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Cancel || !bIsSeated)
    {
        return false;
    }

    AEMRReceptionMachine* ReceptionMachine = FindSeatedReceptionMachine();
    if (!ReceptionMachine)
    {
        return false;
    }

    APlayerController* PlayerController = CachedPlayerController.Get();
    if (!PlayerController)
    {
        PlayerController = Cast<APlayerController>(GetController());
    }

    if (!PlayerController)
    {
        return false;
    }

    if (!HasAuthority())
    {
        Server_RequestReceptionUnseat(ReceptionMachine);
        return true;
    }

    if (!ReceptionMachine->GetClass()->ImplementsInterface(UEMRInteractableInterface::StaticClass()))
    {
        return false;
    }

    FGameplayEventData EventData = IEMRInteractableInterface::Execute_GetInteractionEventData(ReceptionMachine, this);
    if (!EventData.OptionalObject)
    {
        EventData.OptionalObject = ReceptionMachine;
    }

    if (IEMRGameplayEventInterface* GameplayEventInterface = Cast<IEMRGameplayEventInterface>(PlayerController))
    {
        return GameplayEventInterface->SendGameplayEvent(EventData.EventTag, EventData);
    }

    return false;
}

bool AEMRPlayerCharacter::TryHandleXRayCancelInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Cancel)
    {
        return false;
    }

    if (!ActiveXRayMachine.IsValid())
    {
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[XRayInput] Cancel pressed by %s"), *GetNameSafe(this));
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Cancel input local player=%s machine=%s"),
        *GetNameSafe(this), *GetNameSafe(ActiveXRayMachine.Get()));
    ResetXRayMoveInput();
    Server_RequestXRayExit(ActiveXRayMachine.Get());
    return true;
}

bool AEMRPlayerCharacter::TryHandleUltrasoundCancelInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Cancel)
    {
        return false;
    }

    if (!ActiveUltrasoundMachine.IsValid())
    {
        return false;
    }

    ResetUltrasoundMoveInput();
    ResetUltrasoundAdjustInput();
    Server_RequestUltrasoundExit(ActiveUltrasoundMachine.Get());
    return true;
}

bool AEMRPlayerCharacter::TryHandleCTScanCancelInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Cancel)
    {
        return false;
    }

    if (!ActiveCTScanMachine.IsValid())
    {
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[CTScanInput] Cancel pressed by %s"), *GetNameSafe(this));
    Server_RequestCTScanExit(ActiveCTScanMachine.Get());
    return true;
}

bool AEMRPlayerCharacter::TryHandleIVStandCancelInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Cancel)
    {
        return false;
    }

    if (!ActiveIVStand.IsValid())
    {
        return false;
    }

    Server_RequestIVStandExit(ActiveIVStand.Get());
    return true;
}

bool AEMRPlayerCharacter::TryHandleOxygenMachineCancelInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Cancel)
    {
        return false;
    }

    if (!ActiveOxygenMachine.IsValid())
    {
        return false;
    }

    Server_RequestOxygenMachineExit(ActiveOxygenMachine.Get());
    return true;
}

bool AEMRPlayerCharacter::TryHandleLabAnalyzerCancelInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Cancel)
    {
        return false;
    }

    if (!ActiveLabAnalyzerMachine.IsValid())
    {
        return false;
    }

    EMR_LAB_LOG(Log, "Player cancel lab exam player=%s", *GetNameSafe(this));
    Server_RequestLabAnalyzerExit(ActiveLabAnalyzerMachine.Get());
    return true;
}

bool AEMRPlayerCharacter::TryHandleToiletCleanerCancelInput(EAbilityInputID AbilityInputID)
{
    return TryHandleAnchoredCarryItemCancelInput(AbilityInputID);
}

bool AEMRPlayerCharacter::TryHandleAnchoredCarryItemCancelInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Cancel)
    {
        return false;
    }

    AActor* CarriedActor = FindCarriedActor();
    if (!CarriedActor || !CarriedActor->GetClass()->ImplementsInterface(UEMRAnchoredCarryItemInterface::StaticClass()))
    {
        return false;
    }

    Server_RequestAnchoredCarryItemReturn(CarriedActor);
    return true;
}

void AEMRPlayerCharacter::Server_RequestReceptionUnseat_Implementation(AEMRReceptionMachine* ReceptionMachine)
{
    if (!HasAuthority() || !ReceptionMachine)
    {
        return;
    }

    if (ReceptionMachine->GetSeatedPlayer() != this)
    {
        return;
    }

    ReceptionMachine->UnseatPlayer(this);
}

void AEMRPlayerCharacter::Server_SetXRayMoveInput_Implementation(AEMRXRayMachine* Machine, float InputX, float InputY)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    Machine->SetMoveInput(this, InputX, InputY);
}

void AEMRPlayerCharacter::Server_SetUltrasoundMoveInput_Implementation(AEMRUltrasoundMachine* Machine, float InputX, float InputY)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    Machine->SetMoveInput(this, InputX, InputY);
}

void AEMRPlayerCharacter::Server_SetUltrasoundActiveSlider_Implementation(AEMRUltrasoundMachine* Machine, int32 SliderIndex)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    Machine->SetActiveSlider(this, SliderIndex);
}

void AEMRPlayerCharacter::Server_SetUltrasoundSliderAdjust_Implementation(AEMRUltrasoundMachine* Machine, float InputValue)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    Machine->SetSliderAdjustInput(this, InputValue);
}

void AEMRPlayerCharacter::Server_RequestXRayExit_Implementation(AEMRXRayMachine* Machine)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[XRayInput] Server_RequestXRayExit: %s -> %s"), *GetNameSafe(this), *GetNameSafe(Machine));
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Server_RequestXRayExit operator=%s machine=%s"),
        *GetNameSafe(this), *GetNameSafe(Machine));
    Machine->StopExam(this, true);
}

void AEMRPlayerCharacter::Server_RequestUltrasoundExit_Implementation(AEMRUltrasoundMachine* Machine)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    Machine->StopExam(this, true);
}

void AEMRPlayerCharacter::Server_RequestCTScanExit_Implementation(AEMRCTScanMachine* Machine)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    Machine->StopExam(this, true);
}

void AEMRPlayerCharacter::Server_RequestIVStandExit_Implementation(AEMRIntravenousMedicationStand* Stand)
{
    if (!HasAuthority() || !Stand)
    {
        return;
    }

    if (Stand->GetCurrentOperator() != this)
    {
        return;
    }

    Stand->StopTreatment(this, true);
}

void AEMRPlayerCharacter::Server_RequestOxygenMachineExit_Implementation(AEMROxygenMachine* Machine)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentMover() != this)
    {
        return;
    }

    Machine->StopMove(this);
}

void AEMRPlayerCharacter::Server_RequestOxygenMaskReturn_Implementation(AEMROxygenMask* Mask)
{
    if (!HasAuthority() || !Mask)
    {
        return;
    }

    if (Mask->IsMaskAttached())
    {
        return;
    }

    if (!Mask->IsCarriedBy(this))
    {
        return;
    }

    Mask->ReturnToMachine();
}

void AEMRPlayerCharacter::Server_RequestToiletCleanerReturn_Implementation(AEMRToiletCleaner* Cleaner)
{
    if (!HasAuthority() || !Cleaner)
    {
        return;
    }

    if (!Cleaner->IsCarriedBy(this))
    {
        return;
    }

    Cleaner->ReturnToAnchor();
}

void AEMRPlayerCharacter::Server_RequestAnchoredCarryItemReturn_Implementation(AActor* AnchoredItemActor)
{
    if (!HasAuthority() || !AnchoredItemActor || !AnchoredItemActor->GetClass()->ImplementsInterface(UEMRAnchoredCarryItemInterface::StaticClass()))
    {
        return;
    }

    if (!IEMRAnchoredCarryItemInterface::Execute_EMRIsCarriedBy(AnchoredItemActor, this))
    {
        return;
    }

    IEMRAnchoredCarryItemInterface::Execute_EMRReturnToAnchor(AnchoredItemActor);
}

void AEMRPlayerCharacter::Server_HandleCTScanButtonPressed_Implementation(AEMRCTScanMachine* Machine, int32 ButtonIndex)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    if (ButtonIndex == 0)
    {
        Machine->HandleStartButtonPressed(this);
        return;
    }

    if (ButtonIndex == 1)
    {
        Machine->HandleValidateButtonPressed(this);
    }
}

void AEMRPlayerCharacter::Server_HandleIVStandConfirm_Implementation(AEMRIntravenousMedicationStand* Stand)
{
    if (!HasAuthority() || !Stand)
    {
        return;
    }

    if (Stand->GetCurrentOperator() != this)
    {
        return;
    }

    Stand->HandleConfirmPressed(this);
}

void AEMRPlayerCharacter::Server_RequestLabAnalyzerExit_Implementation(AEMRLabAnalyzerMachine* Machine)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    EMR_LAB_LOG(Log, "Server_RequestLabAnalyzerExit player=%s machine=%s", *GetNameSafe(this), *GetNameSafe(Machine));
    Machine->StopExam(this, true);
}

void AEMRPlayerCharacter::Server_HandleLabAnalyzerStartPressed_Implementation(AEMRLabAnalyzerMachine* Machine)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    EMR_LAB_LOG(Log, "Server_StartPressed player=%s machine=%s", *GetNameSafe(this), *GetNameSafe(Machine));
    Machine->HandleStartButtonPressed(this);
}

void AEMRPlayerCharacter::Server_HandleLabAnalyzerLidPressed_Implementation(AEMRLabAnalyzerMachine* Machine)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    EMR_LAB_LOG(Log, "Server_LidPressed player=%s machine=%s", *GetNameSafe(this), *GetNameSafe(Machine));
    Machine->HandleLidTogglePressed(this);
}

void AEMRPlayerCharacter::Server_HandleItemDispenserButtonPressed_Implementation(AEMRItemDispenser* Dispenser, EEMRItemDispenserButtonType ButtonType, int32 Digit)
{
    if (!HasAuthority() || !Dispenser)
    {
        EMR_CONFIRM_LOG(Warning, "Server_HandleItemDispenserButtonPressed ignored player=%s authority=%d dispenser=%s type=%s digit=%d",
            *GetNameSafe(this),
            HasAuthority() ? 1 : 0,
            *GetNameSafe(Dispenser),
            GetDispenserButtonTypeLabel(ButtonType),
            Digit);
        return;
    }

    EMR_CONFIRM_LOG(Warning, "Server_HandleItemDispenserButtonPressed received player=%s dispenser=%s type=%s digit=%d",
        *GetNameSafe(this), *GetNameSafe(Dispenser), GetDispenserButtonTypeLabel(ButtonType), Digit);

    switch (ButtonType)
    {
    case EEMRItemDispenserButtonType::Digit:
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Server button press Digit=%d player=%s dispenser=%s"),
            Digit, *GetNameSafe(this), *GetNameSafe(Dispenser));
        Dispenser->HandleDigitPressed(Digit, this);
        break;
    case EEMRItemDispenserButtonType::Confirm:
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Server button press Confirm player=%s dispenser=%s"),
            *GetNameSafe(this), *GetNameSafe(Dispenser));
        Dispenser->HandleConfirmPressed(this);
        break;
    case EEMRItemDispenserButtonType::Reset:
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Server button press Reset player=%s dispenser=%s"),
            *GetNameSafe(this), *GetNameSafe(Dispenser));
        Dispenser->HandleResetPressed(this);
        break;
    case EEMRItemDispenserButtonType::Increase:
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Server button press Increase player=%s dispenser=%s"),
            *GetNameSafe(this), *GetNameSafe(Dispenser));
        Dispenser->HandleIncreasePressed(this);
        break;
    case EEMRItemDispenserButtonType::Decrease:
        UE_LOG(LogTemp, Warning, TEXT("[ItemDispenser] Server button press Decrease player=%s dispenser=%s"),
            *GetNameSafe(this), *GetNameSafe(Dispenser));
        Dispenser->HandleDecreasePressed(this);
        break;
    default:
        break;
    }
}

void AEMRPlayerCharacter::Server_HandleOvertimeStopTerminalButtonPressed_Implementation(AEMROvertimeStopTerminal* Terminal)
{
    if (!HasAuthority() || !Terminal || !Terminal->IsConfirmButtonInteractionEnabled())
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    IEMRGameplayEventInterface* GameplayEventInterface = Cast<IEMRGameplayEventInterface>(PlayerController);
    if (!GameplayEventInterface)
    {
        return;
    }

    FGameplayEventData InteractionData;
    InteractionData.EventTag = EMRTags::Machine::OvertimeStopTerminal;
    InteractionData.Instigator = this;
    InteractionData.Target = Terminal;
    InteractionData.OptionalObject = Terminal;
    GameplayEventInterface->SendGameplayEvent(InteractionData.EventTag, InteractionData);
}

void AEMRPlayerCharacter::Server_HandleConfirmInteraction_Implementation(const FVector_NetQuantize& ViewLocation, const FVector_NetQuantizeNormal& ViewDirection)
{
    if (!HasAuthority())
    {
        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction ignored: no authority player=%s", *GetNameSafe(this));
        return;
    }

    const FVector SafeViewDirection = ViewDirection.GetSafeNormal();
    if (SafeViewDirection.IsNearlyZero())
    {
        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction ignored: zero direction player=%s viewDir=%s",
            *GetNameSafe(this), *ViewDirection.ToString());
        return;
    }

    EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction received player=%s viewLoc=%s viewDir=%s carried=%s",
        *GetNameSafe(this),
        *ViewLocation.ToString(),
        *SafeViewDirection.ToCompactString(),
        *GetNameSafe(FindCarriedActor()));

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    IEMRGameplayEventInterface* GameplayEventInterface = Cast<IEMRGameplayEventInterface>(PlayerController);
    if (!GameplayEventInterface)
    {
        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction aborted: missing gameplay event interface player=%s controller=%s",
            *GetNameSafe(this), *GetNameSafe(PlayerController));
        return;
    }

    AActor* CarriedActor = FindCarriedActor();
    if (CarriedActor)
    {
        UEMRCarryableComponent* Carryable = CarriedActor->FindComponentByClass<UEMRCarryableComponent>();
        if (!Carryable || !Carryable->IsCarried())
        {
            EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction carried path aborted player=%s carried=%s carryable=%d isCarried=%d",
                *GetNameSafe(this), *GetNameSafe(CarriedActor), Carryable ? 1 : 0, Carryable && Carryable->IsCarried() ? 1 : 0);
            return;
        }

        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction carried path player=%s carried=%s",
            *GetNameSafe(this), *GetNameSafe(CarriedActor));

        if (AEMROxygenMask* OxygenMask = Cast<AEMROxygenMask>(CarriedActor))
        {
            if (AEMROxygenMachine* OxygenMachine = OxygenMask->OwningMachine)
            {
                float TraceDistance = 900.f;
                if (InteractableDetectionComponent)
                {
                    TraceDistance = InteractableDetectionComponent->TraceDistance;
                }

                if (OxygenMachine->TryReturnMaskFromTrace(this, ViewLocation, SafeViewDirection, TraceDistance))
                {
                    return;
                }
            }
        }

        // Prioritize explicit anchored return for anchored carry items before generic use-target handling.
        if (CarriedActor->GetClass()->ImplementsInterface(UEMRAnchoredCarryItemInterface::StaticClass()))
        {
            float TraceDistance = 900.f;
            if (InteractableDetectionComponent)
            {
                TraceDistance = InteractableDetectionComponent->TraceDistance;
            }

            if (IEMRAnchoredCarryItemInterface::Execute_EMRReturnToAnchorFromTrace(CarriedActor, this, ViewLocation, SafeViewDirection, TraceDistance))
            {
                return;
            }
        }

        FHitResult TargetHit;
        if (FindBestUsableTargetHit(ViewLocation, SafeViewDirection, CarriedActor, TargetHit))
        {
            if (AEMRLabAnalyzerTube* LabTube = Cast<AEMRLabAnalyzerTube>(CarriedActor))
            {
                if (Cast<AEMRLabAnalyzerTrash>(TargetHit.GetActor()))
                {
                    AEMRLabAnalyzerMachine* OperatorMachine = nullptr;
                    const bool bIsOperator = IsLabAnalyzerOperator(this, OperatorMachine);
                    if (!bIsOperator)
                    {
                        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction blocked trashing tube player=%s tube=%s isOperator=%d",
                            *GetNameSafe(this), *GetNameSafe(LabTube), bIsOperator ? 1 : 0);
                        return;
                    }
                }
            }

            FGameplayEventData UseEvent;
            if (Carryable->BuildUseObjectEventData(this, UseEvent, TargetHit.GetActor()))
            {
                if (TargetHit.bBlockingHit)
                {
                    UseEvent.TargetData = FGameplayAbilityTargetDataHandle(new FGameplayAbilityTargetData_SingleTargetHit(TargetHit));
                }
                EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction send use event player=%s carried=%s target=%s tag=%s",
                    *GetNameSafe(this), *GetNameSafe(CarriedActor), *GetNameSafe(TargetHit.GetActor()), *UseEvent.EventTag.ToString());
                GameplayEventInterface->SendGameplayEvent(UseEvent.EventTag, UseEvent);
            }
            else
            {
                EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction failed to build use event player=%s carried=%s target=%s",
                    *GetNameSafe(this), *GetNameSafe(CarriedActor), *GetNameSafe(TargetHit.GetActor()));
            }

            return;
        }

        FEMRItemPlacementResult PlacementResult;
        bool bHasPlacementHit = false;
        if (AEMRItemActor* CarriedItem = Cast<AEMRItemActor>(CarriedActor))
        {
            if (ItemPlacementComponent)
            {
                const bool bQuerySucceeded = ItemPlacementComponent->QueryPlacement(*this, *CarriedItem, ViewLocation, SafeViewDirection, PlacementResult);
                if (bQuerySucceeded && PlacementResult.bHasHit)
                {
                    bHasPlacementHit = true;
                }
            }
        }

        if (AEMRLabAnalyzerTube* LabTube = Cast<AEMRLabAnalyzerTube>(CarriedActor))
        {
            if (bHasPlacementHit)
            {
                if (AEMRLabAnalyzerTrash* Trash = Cast<AEMRLabAnalyzerTrash>(PlacementResult.SurfaceHit.GetActor()))
                {
                    AEMRLabAnalyzerMachine* OperatorMachine = nullptr;
                    const bool bIsOperator = IsLabAnalyzerOperator(this, OperatorMachine);
                    if (!bIsOperator)
                    {
                        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction blocked placement trashing tube player=%s tube=%s isOperator=%d",
                            *GetNameSafe(this), *GetNameSafe(LabTube), bIsOperator ? 1 : 0);
                        return;
                    }

                    FGameplayEventData UseEvent;
                    if (Carryable->BuildUseObjectEventData(this, UseEvent, Trash))
                    {
                        UseEvent.TargetData = FGameplayAbilityTargetDataHandle(new FGameplayAbilityTargetData_SingleTargetHit(PlacementResult.SurfaceHit));
                        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction send tube-trash use event player=%s tube=%s trash=%s tag=%s",
                            *GetNameSafe(this), *GetNameSafe(LabTube), *GetNameSafe(Trash), *UseEvent.EventTag.ToString());
                        GameplayEventInterface->SendGameplayEvent(UseEvent.EventTag, UseEvent);
                    }
                    else
                    {
                        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction failed build tube-trash use event player=%s tube=%s trash=%s",
                            *GetNameSafe(this), *GetNameSafe(LabTube), *GetNameSafe(Trash));
                    }

                    return;
                }
            }
        }

        FGameplayEventData PlaceEvent;
        if (Carryable->BuildPlaceObjectEventData(this, PlaceEvent))
        {
            if (bHasPlacementHit)
            {
                PlaceEvent.TargetData = FGameplayAbilityTargetDataHandle(new FGameplayAbilityTargetData_SingleTargetHit(PlacementResult.SurfaceHit));
            }

            EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction send place event player=%s carried=%s hasPlacementHit=%d tag=%s target=%s",
                *GetNameSafe(this),
                *GetNameSafe(CarriedActor),
                bHasPlacementHit ? 1 : 0,
                *PlaceEvent.EventTag.ToString(),
                bHasPlacementHit ? *GetNameSafe(PlacementResult.SurfaceHit.GetActor()) : TEXT("None"));
            GameplayEventInterface->SendGameplayEvent(PlaceEvent.EventTag, PlaceEvent);
        }
        else
        {
            EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction failed to build place event player=%s carried=%s",
                *GetNameSafe(this), *GetNameSafe(CarriedActor));
        }

        return;
    }

    FHitResult ItemHit;
    if (!FindBestItemHit(ViewLocation, SafeViewDirection, ItemHit))
    {
        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction no eligible item hit player=%s", *GetNameSafe(this));
        return;
    }

    AActor* ItemActor = ItemHit.GetActor();
    if (!ItemActor || !ItemActor->GetClass()->ImplementsInterface(UEMRInteractableInterface::StaticClass()))
    {
        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction item invalid/interfaced player=%s item=%s",
            *GetNameSafe(this), *GetNameSafe(ItemActor));
        return;
    }

    if (!IEMRInteractableInterface::Execute_IsInteractableEnabled(ItemActor))
    {
        EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction item disabled player=%s item=%s",
            *GetNameSafe(this), *GetNameSafe(ItemActor));
        return;
    }

    FGameplayEventData InteractionData = IEMRInteractableInterface::Execute_GetInteractionEventData(ItemActor, this);
    if (ItemHit.bBlockingHit)
    {
        InteractionData.TargetData = FGameplayAbilityTargetDataHandle(new FGameplayAbilityTargetData_SingleTargetHit(ItemHit));
    }

    if (!InteractionData.OptionalObject)
    {
        InteractionData.OptionalObject = ItemActor;
    }

    EMR_CONFIRM_LOG(Warning, "Server_HandleConfirmInteraction send item interaction event player=%s item=%s tag=%s hitDist=%.2f",
        *GetNameSafe(this), *GetNameSafe(ItemActor), *InteractionData.EventTag.ToString(), ItemHit.Distance);
    GameplayEventInterface->SendGameplayEvent(InteractionData.EventTag, InteractionData);
}

void AEMRPlayerCharacter::Server_PlayWatchSoundForAllPlayers_Implementation(USoundBase* Sound, FVector_NetQuantize SoundLocation)
{
    if (!HasAuthority())
    {
        return;
    }

    PlayWorldSoundForAllPlayers(Sound, SoundLocation);
}

void AEMRPlayerCharacter::Server_NotifyUltrasoundCompletionCueFinished_Implementation(AEMRUltrasoundMachine* Machine, bool bCanceled)
{
    if (!HasAuthority() || !Machine)
    {
        return;
    }

    if (Machine->GetCurrentOperator() != this)
    {
        return;
    }

    Machine->HandleCompletionCueFinished(this, bCanceled);
}

void AEMRPlayerCharacter::Client_PlayWorldSoundAtLocation_Implementation(USoundBase* Sound, FVector_NetQuantize Location)
{
    if (!IsLocallyControlled() || !Sound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
}

void AEMRPlayerCharacter::Client_PlayWatchReputationFailureSound_Implementation()
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (!WatchReputationFailureSound)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchReputationFailureCue] Missing WatchReputationFailureSound on %s"), *GetNameSafe(this));
        return;
    }

    const FVector SoundLocation = (WatchWidgetComponent
        ? WatchWidgetComponent->GetComponentLocation()
        : (ThirdPersonWatchMesh ? ThirdPersonWatchMesh->GetComponentLocation() : GetActorLocation()));

    UGameplayStatics::PlaySoundAtLocation(this, WatchReputationFailureSound, SoundLocation);
}

void AEMRPlayerCharacter::PlayWorldSoundForAllPlayers(USoundBase* Sound, FVector_NetQuantize Location) const
{
    if (!HasAuthority())
    {
        return;
    }

    PlayWorldSoundForPlayers(GetWorld(), Sound, Location);
}


void AEMRPlayerCharacter::Client_BeginXRayExam_Implementation(AEMRXRayMachine* Machine, UInputMappingContext* ExamIMC)
{
    ActiveXRayMachine = Machine;
    ActiveXRayInputMappingContext = ExamIMC;
    RefreshCurrentTargetInteractionFeedback();

    ResetXRayMoveInput();

    UE_LOG(LogTemp, Log, TEXT("[XRayInput] Client_BeginXRayExam: machine=%s, imc=%s"),
        *GetNameSafe(Machine), *GetNameSafe(ExamIMC));
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Client_BeginXRayExam player=%s machine=%s"),
        *GetNameSafe(this), *GetNameSafe(Machine));

    if (!IsLocallyControlled())
    {
        return;
    }

    APlayerController* PlayerController = CachedPlayerController.Get();
    if (!PlayerController)
    {
        PlayerController = Cast<APlayerController>(GetController());
    }

    if (!PlayerController)
    {
        return;
    }

    {
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = false;
    }

    if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (GameplayInputMappingContext)
            {
                InputSubsystem->RemoveMappingContext(GameplayInputMappingContext);
            }

            if (ExamIMC)
            {
                constexpr int32 XRayMappingPriority = 1;
                InputSubsystem->AddMappingContext(ExamIMC, XRayMappingPriority);
            }
            else if (GameplayInputMappingContext)
            {
                InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
            }
        }
    }
}

void AEMRPlayerCharacter::Client_EndXRayExam_Implementation(AEMRXRayMachine* Machine)
{
    ResetXRayMoveInput();

    UE_LOG(LogTemp, Log, TEXT("[XRayInput] Client_EndXRayExam: machine=%s"), *GetNameSafe(Machine));
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Client_EndXRayExam player=%s machine=%s"),
        *GetNameSafe(this), *GetNameSafe(Machine));

    if (IsLocallyControlled())
    {
        APlayerController* PlayerController = CachedPlayerController.Get();
        if (!PlayerController)
        {
            PlayerController = Cast<APlayerController>(GetController());
        }

        if (PlayerController)
        {
            FInputModeGameOnly InputMode;
            PlayerController->SetInputMode(InputMode);
            PlayerController->bShowMouseCursor = false;

            if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
            {
                if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                {
                    if (ActiveXRayInputMappingContext)
                    {
                        InputSubsystem->RemoveMappingContext(ActiveXRayInputMappingContext);
                    }

                    if (GameplayInputMappingContext)
                    {
                        InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
                    }
                }
            }
        }
    }

    ActiveXRayMachine.Reset();
    ActiveXRayInputMappingContext = nullptr;
    RefreshCurrentTargetInteractionFeedback();
}

void AEMRPlayerCharacter::Client_BeginUltrasoundExam_Implementation(AEMRUltrasoundMachine* Machine, UInputMappingContext* ExamIMC)
{
    ActiveUltrasoundMachine = Machine;
    ActiveUltrasoundInputMappingContext = ExamIMC;
    RefreshCurrentTargetInteractionFeedback();

    ResetUltrasoundMoveInput();
    ResetUltrasoundAdjustInput();

    if (!IsLocallyControlled())
    {
        return;
    }

    APlayerController* PlayerController = CachedPlayerController.Get();
    if (!PlayerController)
    {
        PlayerController = Cast<APlayerController>(GetController());
    }

    if (!PlayerController)
    {
        return;
    }

    if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (GameplayInputMappingContext)
            {
                InputSubsystem->RemoveMappingContext(GameplayInputMappingContext);
            }

            if (ExamIMC)
            {
                constexpr int32 UltrasoundMappingPriority = 1;
                InputSubsystem->AddMappingContext(ExamIMC, UltrasoundMappingPriority);
            }

            UE_LOG(LogTemp, Log, TEXT("[UltrasoundIMC] Begin exam player=%s examIMC=%s gameplayIMC=%s"),
                *GetNameSafe(this),
                *GetNameSafe(ExamIMC),
                *GetNameSafe(GameplayInputMappingContext));
        }
    }

    if (WorldWidgetInteraction)
    {
        WorldWidgetInteraction->SetActive(true);
        WorldWidgetInteraction->bEnableHitTesting = true;
    }

}

void AEMRPlayerCharacter::Client_EndUltrasoundExam_Implementation(AEMRUltrasoundMachine* Machine)
{
    ResetUltrasoundMoveInput();
    ResetUltrasoundAdjustInput();

    if (IsLocallyControlled())
    {
        APlayerController* PlayerController = CachedPlayerController.Get();
        if (!PlayerController)
        {
            PlayerController = Cast<APlayerController>(GetController());
        }

        if (PlayerController)
        {
            if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
            {
                if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                {
                    if (ActiveUltrasoundInputMappingContext)
                    {
                        InputSubsystem->RemoveMappingContext(ActiveUltrasoundInputMappingContext);
                    }

                    if (GameplayInputMappingContext)
                    {
                        InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
                    }

                    UE_LOG(LogTemp, Log, TEXT("[UltrasoundIMC] End exam player=%s examIMC=%s gameplayIMC=%s"),
                        *GetNameSafe(this),
                        *GetNameSafe(ActiveUltrasoundInputMappingContext),
                        *GetNameSafe(GameplayInputMappingContext));
                }
            }
        }
    }

    if (WorldWidgetInteraction)
    {
        WorldWidgetInteraction->SetActive(false);
        WorldWidgetInteraction->bEnableHitTesting = false;
    }

    ActiveUltrasoundMachine.Reset();
    ActiveUltrasoundInputMappingContext = nullptr;
    RefreshCurrentTargetInteractionFeedback();
}

void AEMRPlayerCharacter::Client_BeginCTScanExam_Implementation(AEMRCTScanMachine* Machine, UInputMappingContext* ExamIMC)
{
    ActiveCTScanMachine = Machine;
    ActiveCTScanInputMappingContext = ExamIMC;
    RefreshCurrentTargetInteractionFeedback();

    if (!IsLocallyControlled())
    {
        return;
    }

    APlayerController* PlayerController = CachedPlayerController.Get();
    if (!PlayerController)
    {
        PlayerController = Cast<APlayerController>(GetController());
    }

    if (!PlayerController)
    {
        return;
    }

    {
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = false;
    }

    if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (GameplayInputMappingContext)
            {
                InputSubsystem->RemoveMappingContext(GameplayInputMappingContext);
            }

            if (ExamIMC)
            {
                constexpr int32 CTScanMappingPriority = 1;
                InputSubsystem->AddMappingContext(ExamIMC, CTScanMappingPriority);
            }
            else if (GameplayInputMappingContext)
            {
                InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
            }
        }
    }
}

void AEMRPlayerCharacter::Client_EndCTScanExam_Implementation(AEMRCTScanMachine* Machine)
{
    if (IsLocallyControlled())
    {
        APlayerController* PlayerController = CachedPlayerController.Get();
        if (!PlayerController)
        {
            PlayerController = Cast<APlayerController>(GetController());
        }

        if (PlayerController)
        {
            FInputModeGameOnly InputMode;
            PlayerController->SetInputMode(InputMode);
            PlayerController->bShowMouseCursor = false;

            if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
            {
                if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                {
                    if (ActiveCTScanInputMappingContext)
                    {
                        InputSubsystem->RemoveMappingContext(ActiveCTScanInputMappingContext);
                    }

                    if (GameplayInputMappingContext)
                    {
                        InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
                    }
                }
            }
        }
    }

    ActiveCTScanMachine.Reset();
    ActiveCTScanInputMappingContext = nullptr;
    RefreshCurrentTargetInteractionFeedback();
}

void AEMRPlayerCharacter::Client_BeginIVTreatment_Implementation(AEMRIntravenousMedicationStand* Stand, UInputMappingContext* ExamIMC)
{
    ActiveIVStand = Stand;
    ActiveIVInputMappingContext = ExamIMC;
    RefreshCurrentTargetInteractionFeedback();

    if (!IsLocallyControlled())
    {
        return;
    }

    APlayerController* PlayerController = CachedPlayerController.Get();
    if (!PlayerController)
    {
        PlayerController = Cast<APlayerController>(GetController());
    }

    if (!PlayerController)
    {
        return;
    }

    {
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = false;
    }

    if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (GameplayInputMappingContext)
            {
                InputSubsystem->RemoveMappingContext(GameplayInputMappingContext);
            }

            if (ExamIMC)
            {
                constexpr int32 IVStandMappingPriority = 1;
                InputSubsystem->AddMappingContext(ExamIMC, IVStandMappingPriority);
            }
            else if (GameplayInputMappingContext)
            {
                InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
            }
        }
    }
}

void AEMRPlayerCharacter::Client_EndIVTreatment_Implementation(AEMRIntravenousMedicationStand* Stand)
{
    if (IsLocallyControlled())
    {
        APlayerController* PlayerController = CachedPlayerController.Get();
        if (!PlayerController)
        {
            PlayerController = Cast<APlayerController>(GetController());
        }

        if (PlayerController)
        {
            FInputModeGameOnly InputMode;
            PlayerController->SetInputMode(InputMode);
            PlayerController->bShowMouseCursor = false;

            if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
            {
                if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                {
                    if (ActiveIVInputMappingContext)
                    {
                        InputSubsystem->RemoveMappingContext(ActiveIVInputMappingContext);
                    }

                    if (GameplayInputMappingContext)
                    {
                        InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
                    }
                }
            }
        }
    }

    ActiveIVStand.Reset();
    ActiveIVInputMappingContext = nullptr;
    RefreshCurrentTargetInteractionFeedback();
}

void AEMRPlayerCharacter::Client_BeginOxygenMove_Implementation(AEMROxygenMachine* Machine)
{
    ActiveOxygenMachine = Machine;
    RefreshCurrentTargetInteractionFeedback();

    // Keep autonomous client prediction aligned with server speed reduction to avoid rollback corrections.
    if (Machine && IsLocallyControlled() && !HasAuthority())
    {
        if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
        {
            if (CachedLocalOxygenMoveMaxWalkSpeed <= 0.0f)
            {
                CachedLocalOxygenMoveMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
            }

            MovementComponent->MaxWalkSpeed = MovementComponent->MaxWalkSpeed * Machine->GetMoveSpeedMultiplier();
        }
    }

    if (Machine)
    {
        Machine->BeginLocalMoveDriver(this);
    }
}

void AEMRPlayerCharacter::Client_EndOxygenMove_Implementation(AEMROxygenMachine* Machine)
{
    if (Machine)
    {
        Machine->EndLocalMoveDriver(this);
    }

    if (IsLocallyControlled() && !HasAuthority() && CachedLocalOxygenMoveMaxWalkSpeed > 0.0f)
    {
        if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
        {
            MovementComponent->MaxWalkSpeed = CachedLocalOxygenMoveMaxWalkSpeed;
        }
        CachedLocalOxygenMoveMaxWalkSpeed = 0.0f;
    }

    if (ActiveOxygenMachine.Get() == Machine)
    {
        ActiveOxygenMachine.Reset();
    }
    RefreshCurrentTargetInteractionFeedback();
}

void AEMRPlayerCharacter::Client_BeginLabAnalyzerExam_Implementation(AEMRLabAnalyzerMachine* Machine, UInputMappingContext* ExamIMC)
{
    ActiveLabAnalyzerMachine = Machine;
    ActiveLabAnalyzerInputMappingContext = ExamIMC;
    RefreshCurrentTargetInteractionFeedback();

    EMR_LAB_LOG(Log, "Client_BeginLabAnalyzerExam player=%s machine=%s", *GetNameSafe(this), *GetNameSafe(Machine));
    if (!IsLocallyControlled())
    {
        return;
    }

    APlayerController* PlayerController = CachedPlayerController.Get();
    if (!PlayerController)
    {
        PlayerController = Cast<APlayerController>(GetController());
    }

    if (!PlayerController)
    {
        return;
    }

    {
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = false;
    }

    if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (GameplayInputMappingContext)
            {
                InputSubsystem->RemoveMappingContext(GameplayInputMappingContext);
            }

            if (ExamIMC)
            {
                constexpr int32 LabAnalyzerMappingPriority = 1;
                InputSubsystem->AddMappingContext(ExamIMC, LabAnalyzerMappingPriority);
            }
            else if (GameplayInputMappingContext)
            {
                InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
            }
        }
    }
}

void AEMRPlayerCharacter::Client_EndLabAnalyzerExam_Implementation(AEMRLabAnalyzerMachine* Machine)
{
    EMR_LAB_LOG(Log, "Client_EndLabAnalyzerExam player=%s machine=%s", *GetNameSafe(this), *GetNameSafe(Machine));
    if (IsLocallyControlled())
    {
        APlayerController* PlayerController = CachedPlayerController.Get();
        if (!PlayerController)
        {
            PlayerController = Cast<APlayerController>(GetController());
        }

        if (PlayerController)
        {
            FInputModeGameOnly InputMode;
            PlayerController->SetInputMode(InputMode);
            PlayerController->bShowMouseCursor = false;

            if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
            {
                if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                {
                    if (ActiveLabAnalyzerInputMappingContext)
                    {
                        InputSubsystem->RemoveMappingContext(ActiveLabAnalyzerInputMappingContext);
                    }

                    if (GameplayInputMappingContext)
                    {
                        InputSubsystem->AddMappingContext(GameplayInputMappingContext, 0);
                    }
                }
            }
        }
    }

    ActiveLabAnalyzerMachine.Reset();
    ActiveLabAnalyzerInputMappingContext = nullptr;
    RefreshCurrentTargetInteractionFeedback();
}

void AEMRPlayerCharacter::HandleXRayMoveLeftStarted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetXRayMoveLeft(true, ActiveXRayMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleXRayMoveLeftCompleted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetXRayMoveLeft(false, ActiveXRayMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleXRayMoveRightStarted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetXRayMoveRight(true, ActiveXRayMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleXRayMoveRightCompleted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetXRayMoveRight(false, ActiveXRayMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleXRayMoveForwardStarted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetXRayMoveForward(true, ActiveXRayMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleXRayMoveForwardCompleted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetXRayMoveForward(false, ActiveXRayMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleXRayMoveBackwardStarted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetXRayMoveBackward(true, ActiveXRayMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleXRayMoveBackwardCompleted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetXRayMoveBackward(false, ActiveXRayMachine.Get());
    }
}

void AEMRPlayerCharacter::ResetXRayMoveInput()
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->ResetXRayMoveInput(ActiveXRayMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundMoveLeftStarted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetUltrasoundMoveLeft(true, ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundMoveLeftCompleted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetUltrasoundMoveLeft(false, ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundMoveRightStarted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetUltrasoundMoveRight(true, ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundMoveRightCompleted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetUltrasoundMoveRight(false, ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundMoveForwardStarted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetUltrasoundMoveForward(true, ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundMoveForwardCompleted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetUltrasoundMoveForward(false, ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundMoveBackwardStarted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetUltrasoundMoveBackward(true, ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundMoveBackwardCompleted(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->SetUltrasoundMoveBackward(false, ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::ResetUltrasoundMoveInput()
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->ResetUltrasoundMoveInput(ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundAdjustInputTriggered(const FInputActionValue& InputActionValue)
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->HandleUltrasoundAdjustInput(InputActionValue.Get<float>(), ActiveUltrasoundMachine.Get());
    }
}

void AEMRPlayerCharacter::HandleUltrasoundAdjustInputCompleted(const FInputActionValue& InputActionValue)
{
    ResetUltrasoundAdjustInput();
}

void AEMRPlayerCharacter::ResetUltrasoundAdjustInput()
{
    if (PlayerMachineInputComponent)
    {
        PlayerMachineInputComponent->ResetUltrasoundAdjustInput(ActiveUltrasoundMachine.Get());
    }
}

AEMRReceptionMachine* AEMRPlayerCharacter::FindSeatedReceptionMachine() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AEMRReceptionMachine> It(World); It; ++It)
    {
        AEMRReceptionMachine* ReceptionMachine = *It;
        if (!ReceptionMachine)
        {
            continue;
        }

        if (ReceptionMachine->GetSeatedPlayer() == this)
        {
            return ReceptionMachine;
        }
    }

    return nullptr;
}


void AEMRPlayerCharacter::SetInputEnabled(const bool bIsEnabled)
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


void AEMRPlayerCharacter::InitializeFirstPersonCarriedItemMesh()
{
    if (FirstPersonCarriedItemMesh)
    {
        FirstPersonCarriedItemMesh->SetHiddenInGame(true);
    }

    ClearFirstPersonCarriedItemFilledOverlay();

	if (FirstPersonCarriedItemWidget)
	{
		FirstPersonCarriedItemWidget->SetHiddenInGame(true);
		FirstPersonCarriedItemWidget->SetVisibility(false);
	}

    if (IsLocallyControlled())
    {
        StartCarriedItemRefreshTimer();
        RefreshFirstPersonCarriedItem();
    }
}

void AEMRPlayerCharacter::InitializeThirdPersonWatchMesh()
{
	if (!ThirdPersonWatchMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Watch] Third-person watch mesh component missing; skipping init."));
		return;
	}

	USkeletalMeshComponent* SourceMesh = ResolvePlayerMeshComponent();
	if (!SourceMesh)
	{
		SourceMesh = GetMesh();
	}

	if (!SourceMesh)
	{
		if (!bHasWarnedMissingThirdPersonWatchMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Watch] Missing player mesh for third-person watch on %s."), *GetNameSafe(this));
			bHasWarnedMissingThirdPersonWatchMesh = true;
		}
		return;
	}

	bHasWarnedMissingThirdPersonWatchMesh = false;
	RefreshThirdPersonWatchMesh(true);
}

void AEMRPlayerCharacter::RefreshAbilityActorInfoMesh()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !ASC->AbilityActorInfo.IsValid())
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = ResolvePlayerMeshComponent();
	if (!MeshComponent)
	{
		MeshComponent = GetMesh();
	}

	if (!MeshComponent)
	{
		return;
	}

	if (ASC->AbilityActorInfo->SkeletalMeshComponent != MeshComponent)
	{
		ASC->AbilityActorInfo->SkeletalMeshComponent = MeshComponent;
	}
}

void AEMRPlayerCharacter::RefreshThirdPersonWatchMesh(bool bForce)
{
	if (!ThirdPersonWatchMesh)
	{
		return;
	}

	USkeletalMeshComponent* SourceMesh = ResolvePlayerMeshComponent();
	if (!SourceMesh)
	{
		SourceMesh = GetMesh();
	}

	if (!SourceMesh)
	{
		return;
	}

	const EEMRWatchSocketState DesiredState = ResolveWatchSocketState();
	if (!bForce && bHasActiveWatchSocketState && DesiredState == ActiveWatchSocketState && !bWatchSocketBlendActive)
	{
		return;
	}

	const FEMRWatchSocketConfig Config = GetWatchSocketConfig(DesiredState);
	if (!IsWatchSocketValid(SourceMesh, Config, DesiredState))
	{
		return;
	}

	if (!bHasActiveWatchSocketState || WatchSocketBlendDuration <= 0.f)
	{
		AttachThirdPersonWatchMesh(SourceMesh, Config.SocketName, Config.RelativeTransform);
		ActiveWatchSocketState = DesiredState;
		bHasActiveWatchSocketState = true;
		bWatchSocketBlendActive = false;
		UpdateTickState();
		return;
	}

	BeginWatchSocketBlend(SourceMesh, DesiredState, Config);
}

void AEMRPlayerCharacter::UpdateThirdPersonWatchBlend(float DeltaSeconds)
{
	if (!bWatchSocketBlendActive || !ThirdPersonWatchMesh)
	{
		return;
	}

	USkeletalMeshComponent* SourceMesh = ResolvePlayerMeshComponent();
	if (!SourceMesh)
	{
		SourceMesh = GetMesh();
	}

	if (!SourceMesh)
	{
		return;
	}

	if (PendingWatchSocketName.IsNone() || !SourceMesh->DoesSocketExist(PendingWatchSocketName))
	{
		FEMRWatchSocketConfig TempConfig;
		TempConfig.SocketName = PendingWatchSocketName;
		TempConfig.RelativeTransform = PendingWatchSocketTransform;
		IsWatchSocketValid(SourceMesh, TempConfig, PendingWatchSocketState);
		bWatchSocketBlendActive = false;
		UpdateTickState();
		return;
	}

	const FTransform SocketTransform = SourceMesh->GetSocketTransform(PendingWatchSocketName, RTS_World);
	const FTransform TargetWorld = PendingWatchSocketTransform * SocketTransform;
	const float Duration = FMath::Max(WatchSocketBlendDuration, KINDA_SMALL_NUMBER);
	WatchSocketBlendElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(WatchSocketBlendElapsed / Duration, 0.f, 1.f);

	FTransform Blended;
	Blended.Blend(WatchSocketBlendStart, TargetWorld, Alpha);
	ThirdPersonWatchMesh->SetWorldTransform(Blended, false, nullptr, ETeleportType::TeleportPhysics);

	if (Alpha >= 1.f)
	{
		AttachThirdPersonWatchMesh(SourceMesh, PendingWatchSocketName, PendingWatchSocketTransform);
		ActiveWatchSocketState = PendingWatchSocketState;
		bHasActiveWatchSocketState = true;
		bWatchSocketBlendActive = false;
		UpdateTickState();
	}
}

EEMRWatchSocketState AEMRPlayerCharacter::ResolveWatchSocketState() const
{
	if (bIsSeated)
	{
		return bWatchSocketLookActive ? EEMRWatchSocketState::SeatedWatch : EEMRWatchSocketState::SeatedIdle;
	}

	return bWatchSocketLookActive ? EEMRWatchSocketState::StandingWatch : EEMRWatchSocketState::StandingIdle;
}

FEMRWatchSocketConfig AEMRPlayerCharacter::GetWatchSocketConfig(EEMRWatchSocketState State) const
{
	FEMRWatchSocketConfig Result;
	switch (State)
	{
	case EEMRWatchSocketState::StandingWatch:
		Result = WatchSocketStandingLook;
		break;
	case EEMRWatchSocketState::SeatedIdle:
		Result = WatchSocketSeatedIdle;
		break;
	case EEMRWatchSocketState::SeatedWatch:
		Result = WatchSocketSeatedLook;
		break;
	case EEMRWatchSocketState::StandingIdle:
	default:
		Result = WatchSocketStandingIdle;
		break;
	}

	if (Result.SocketName.IsNone() && !ThirdPersonWatchSocketName.IsNone())
	{
		Result.SocketName = ThirdPersonWatchSocketName;
		Result.RelativeTransform = ThirdPersonWatchMeshTransform;
	}

	return Result;
}

bool AEMRPlayerCharacter::IsWatchSocketValid(USkeletalMeshComponent* SourceMesh, const FEMRWatchSocketConfig& Config, EEMRWatchSocketState State)
{
	if (!SourceMesh || Config.SocketName.IsNone())
	{
		return false;
	}

	if (!SourceMesh->DoesSocketExist(Config.SocketName))
	{
		const uint8 StateIndex = static_cast<uint8>(State);
		const uint8 Mask = static_cast<uint8>(1 << StateIndex);
		if ((MissingWatchSocketWarningMask & Mask) == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Watch] Third-person watch socket '%s' missing for state %d on %s."),
				*Config.SocketName.ToString(),
				static_cast<int32>(State),
				*GetNameSafe(SourceMesh));
			MissingWatchSocketWarningMask |= Mask;
		}
		return false;
	}

	return true;
}

void AEMRPlayerCharacter::BeginWatchSocketBlend(USkeletalMeshComponent* SourceMesh, EEMRWatchSocketState State, const FEMRWatchSocketConfig& Config)
{
	if (!ThirdPersonWatchMesh || !SourceMesh)
	{
		return;
	}

	PendingWatchSocketState = State;
	PendingWatchSocketName = Config.SocketName;
	PendingWatchSocketTransform = Config.RelativeTransform;
	WatchSocketBlendElapsed = 0.f;
	WatchSocketBlendStart = ThirdPersonWatchMesh->GetComponentTransform();
	bWatchSocketBlendActive = true;

	ThirdPersonWatchMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	if (USceneComponent* RootComp = GetRootComponent())
	{
		ThirdPersonWatchMesh->AttachToComponent(RootComp, FAttachmentTransformRules::KeepWorldTransform);
	}

	UpdateTickState();
}

void AEMRPlayerCharacter::AttachThirdPersonWatchMesh(USkeletalMeshComponent* SourceMesh, const FName& SocketName, const FTransform& RelativeTransform)
{
	if (!ThirdPersonWatchMesh || !SourceMesh)
	{
		return;
	}

	ThirdPersonWatchMesh->AttachToComponent(
		SourceMesh,
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		SocketName);
	ThirdPersonWatchMesh->SetRelativeTransform(RelativeTransform);

	if (WatchWidgetComponent)
	{
		if (WatchWidgetClass && WatchWidgetComponent->GetWidgetClass() != WatchWidgetClass)
		{
			WatchWidgetComponent->SetWidgetClass(WatchWidgetClass);
			WatchWidgetComponent->InitWidget();
		}
		else if (!WatchWidgetComponent->GetUserWidgetObject())
		{
			WatchWidgetComponent->InitWidget();
		}

		WatchWidgetComponent->AttachToComponent(ThirdPersonWatchMesh, FAttachmentTransformRules::KeepRelativeTransform);
		WatchWidgetComponent->SetRelativeTransform(WatchWidgetTransform);
	}
}


void AEMRPlayerCharacter::StartCarriedItemRefreshTimer()
{
    if (!IsLocallyControlled() || !GetWorld())
    {
        return;
    }

    if (!GetWorldTimerManager().IsTimerActive(CarriedItemRefreshHandle))
    {
        const float Interval = FMath::Max(FirstPersonCarriedItemRefreshInterval, 0.05f);
        GetWorldTimerManager().SetTimer(CarriedItemRefreshHandle, this, &ThisClass::RefreshFirstPersonCarriedItem, Interval, true);
    }
}

void AEMRPlayerCharacter::NotifyCarriedItemStateChanged()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	StartCarriedItemRefreshTimer();
	RefreshFirstPersonCarriedItem();
}


void AEMRPlayerCharacter::StopCarriedItemRefreshTimer()
{
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(CarriedItemRefreshHandle);
    }
}


void AEMRPlayerCharacter::RefreshFirstPersonCarriedItem()
{
    if (!FirstPersonCarriedItemMesh)
    {
        return;
    }

    if (!IsLocallyControlled())
    {
        FirstPersonCarriedItemMesh->SetStaticMesh(nullptr);
        FirstPersonCarriedItemMesh->SetHiddenInGame(true);
        ClearFirstPersonCarriedItemFilledOverlay();
		if (FirstPersonCarriedItemWidget)
		{
			FirstPersonCarriedItemWidget->SetHiddenInGame(true);
			FirstPersonCarriedItemWidget->SetVisibility(false);
		}
        CachedCarriedActor = nullptr;
		CachedFirstPersonWidgetActor = nullptr;
		bPlacementPreviewActive = false;
		SetPlacementPreviewVisible(false);
        StopCarriedItemRefreshTimer();
        return;
    }
	

    AActor* CarriedActor = FindCarriedActor();
    CachedCarriedActor = CarriedActor;

	const bool bWantsPlacementPreview = IsLocallyControlled()
	&& Cast<AEMRItemActor>(CarriedActor) != nullptr
	&& (!CarriedActor || !CarriedActor->GetClass()->ImplementsInterface(UEMRAnchoredCarryItemInterface::StaticClass()))
	&& Cast<AEMRTreatmentBedFolder>(CarriedActor) == nullptr;
	if (bPlacementPreviewActive != bWantsPlacementPreview)
	{
		bPlacementPreviewActive = bWantsPlacementPreview;
		UpdateTickState();
		if (!bPlacementPreviewActive)
		{
			SetPlacementPreviewVisible(false);
		}
		else
		{
			UpdatePlacementPreview();
		}
	}

    UStaticMesh* CarriedMesh = nullptr;
    FVector RelativeLocation = FVector::ZeroVector;
    FRotator RelativeRotation = FRotator::ZeroRotator;
    FVector RelativeScale = FVector::OneVector;
    const AEMRItemActor* CarriedItemActor = Cast<AEMRItemActor>(CarriedActor);
    if (CarriedItemActor)
    {
        if (const UEMRItemData* ItemData = CarriedItemActor->GetItemData())
        {
            CarriedMesh = ItemData->GetWorldItemMesh();
            RelativeLocation = ItemData->GetFirstPersonMeshRelativeLocation();
            RelativeRotation = ItemData->GetFirstPersonMeshRelativeRotation();
            RelativeScale = ItemData->GetFirstPersonMeshRelativeScale();
        }

        if (!CarriedMesh)
        {
            if (const UStaticMeshComponent* StaticMeshComponent = CarriedItemActor->FindComponentByClass<UStaticMeshComponent>())
            {
                CarriedMesh = StaticMeshComponent->GetStaticMesh();
            }
        }
    }
    else if (const UStaticMeshComponent* StaticMeshComponent = CarriedActor ? CarriedActor->FindComponentByClass<UStaticMeshComponent>() : nullptr)
    {
        CarriedMesh = StaticMeshComponent->GetStaticMesh();
    }

    FirstPersonCarriedItemMesh->SetStaticMesh(CarriedMesh);
    FirstPersonCarriedItemMesh->SetRelativeLocation(RelativeLocation);
    FirstPersonCarriedItemMesh->SetRelativeRotation(RelativeRotation);
    FirstPersonCarriedItemMesh->SetRelativeScale3D(RelativeScale);

    const bool bShouldShowMesh = CarriedMesh != nullptr && CarriedActor != nullptr;
    FirstPersonCarriedItemMesh->SetHiddenInGame(!bShouldShowMesh);
    RefreshFirstPersonCarriedItemFilledOverlay(CarriedItemActor, bShouldShowMesh);

	RefreshFirstPersonCarriedItemWidget(CarriedActor);

    if (AEMRLabAnalyzerTube* Tube = Cast<AEMRLabAnalyzerTube>(CarriedActor))
    {
        RefreshFirstPersonTubeDecals(Tube);
    }
    else
    {
        ClearFirstPersonTubeDecals();
    }
}

void AEMRPlayerCharacter::RefreshFirstPersonCarriedItemFilledOverlay(const AEMRItemActor* CarriedItemActor, const bool bShouldShowBaseMesh)
{
    if (!FirstPersonCarriedItemFilledOverlayMesh)
    {
        return;
    }

    if (!CarriedItemActor || !bShouldShowBaseMesh)
    {
        ClearFirstPersonCarriedItemFilledOverlay();
        return;
    }

    UStaticMesh* CachedOverlayMesh = FirstPersonCarriedItemFilledOverlayCachedMesh.Get();
    UMaterialInterface* CachedOverlayMaterial = FirstPersonCarriedItemFilledOverlayCachedMaterial.Get();
    UMaterialInstanceDynamic* PreviewOverlayMID = FirstPersonCarriedItemFilledOverlayMID.Get();
    CarriedItemActor->ApplyFilledOverlayToFirstPersonPreview(
        FirstPersonCarriedItemFilledOverlayMesh,
        PreviewOverlayMID,
        CachedOverlayMesh,
        CachedOverlayMaterial);
    FirstPersonCarriedItemFilledOverlayMID = PreviewOverlayMID;
    FirstPersonCarriedItemFilledOverlayCachedMesh = CachedOverlayMesh;
    FirstPersonCarriedItemFilledOverlayCachedMaterial = CachedOverlayMaterial;
}

void AEMRPlayerCharacter::ClearFirstPersonCarriedItemFilledOverlay()
{
    UStaticMesh* CachedOverlayMesh = FirstPersonCarriedItemFilledOverlayCachedMesh.Get();
    UMaterialInterface* CachedOverlayMaterial = FirstPersonCarriedItemFilledOverlayCachedMaterial.Get();
    UMaterialInstanceDynamic* PreviewOverlayMID = FirstPersonCarriedItemFilledOverlayMID.Get();
    AEMRItemActor::ClearFilledOverlayFirstPersonPreview(
        FirstPersonCarriedItemFilledOverlayMesh,
        PreviewOverlayMID,
        CachedOverlayMesh,
        CachedOverlayMaterial);
    FirstPersonCarriedItemFilledOverlayMID = PreviewOverlayMID;
    FirstPersonCarriedItemFilledOverlayCachedMesh = CachedOverlayMesh;
    FirstPersonCarriedItemFilledOverlayCachedMaterial = CachedOverlayMaterial;
}

void AEMRPlayerCharacter::RefreshFirstPersonCarriedItemWidget(AActor* CarriedActor)
{
	if (!FirstPersonCarriedItemWidget)
	{
		return;
	}

	AActor* PreviousActor = CachedFirstPersonWidgetActor.Get();
	if (PreviousActor && PreviousActor != CarriedActor && PreviousActor->GetClass()->ImplementsInterface(UEMRFirstPersonCarryViewInterface::StaticClass()))
	{
		UUserWidget* PrevWidget = FirstPersonCarriedItemWidget->GetUserWidgetObject();
		IEMRFirstPersonCarryViewInterface::Execute_ResetFirstPersonCarryWidget(PreviousActor, PrevWidget);
	}

	CachedFirstPersonWidgetActor = CarriedActor;

	if (!CarriedActor || !CarriedActor->GetClass()->ImplementsInterface(UEMRFirstPersonCarryViewInterface::StaticClass()))
	{
		FirstPersonCarriedItemWidget->SetHiddenInGame(true);
		FirstPersonCarriedItemWidget->SetVisibility(false);
		return;
	}

	FEMRFirstPersonCarryWidgetSettings Settings;
	if (!IEMRFirstPersonCarryViewInterface::Execute_GetFirstPersonCarryWidgetSettings(CarriedActor, Settings) || !Settings.WidgetClass)
	{
		FirstPersonCarriedItemWidget->SetHiddenInGame(true);
		FirstPersonCarriedItemWidget->SetVisibility(false);
		return;
	}

	if (FirstPersonCarriedItemWidget->GetWidgetClass() != Settings.WidgetClass)
	{
		FirstPersonCarriedItemWidget->SetWidgetClass(Settings.WidgetClass);
		FirstPersonCarriedItemWidget->InitWidget();
	}
	else if (!FirstPersonCarriedItemWidget->GetUserWidgetObject())
	{
		FirstPersonCarriedItemWidget->InitWidget();
	}

	FirstPersonCarriedItemWidget->SetDrawAtDesiredSize(Settings.bDrawAtDesiredSize);
	FirstPersonCarriedItemWidget->SetRelativeTransform(Settings.RelativeTransform);

	const bool bShouldShow = Settings.bShouldShow;
	FirstPersonCarriedItemWidget->SetHiddenInGame(!bShouldShow);
	FirstPersonCarriedItemWidget->SetVisibility(bShouldShow);

	if (UUserWidget* Widget = FirstPersonCarriedItemWidget->GetUserWidgetObject())
	{
		IEMRFirstPersonCarryViewInterface::Execute_UpdateFirstPersonCarryWidget(CarriedActor, Widget);
	}
}

void AEMRPlayerCharacter::RefreshFirstPersonTubeDecals(AEMRLabAnalyzerTube* Tube)
{
    if (!Tube || !FirstPersonCarriedItemMesh)
    {
        ClearFirstPersonTubeDecals();
        return;
    }

    if (!FirstPersonCarriedItemMesh->GetStaticMesh())
    {
        ClearFirstPersonTubeDecals();
        return;
    }

    TArray<UDecalComponent*> TubeDecals;
    Tube->GetTestDecalComponents(TubeDecals);
    if (TubeDecals.IsEmpty())
    {
        ClearFirstPersonTubeDecals();
        return;
    }

    if (CachedPreviewTube.Get() != Tube || FirstPersonTubeDecals.Num() != TubeDecals.Num())
    {
        BuildFirstPersonTubeDecals(Tube);
    }

    Tube->UpdateDecalMarkersForComponents(FirstPersonTubeDecals, FirstPersonTubeDecalMIDs);
}

void AEMRPlayerCharacter::BuildFirstPersonTubeDecals(AEMRLabAnalyzerTube* Tube)
{
    ClearFirstPersonTubeDecals();

    if (!Tube || !FirstPersonCarriedItemMesh)
    {
        return;
    }

    TArray<UDecalComponent*> TubeDecals;
    Tube->GetTestDecalComponents(TubeDecals);
    if (TubeDecals.IsEmpty())
    {
        return;
    }

    const UStaticMeshComponent* TubeMesh = Tube->FindComponentByClass<UStaticMeshComponent>();
    const FTransform TubeMeshTransform = TubeMesh ? TubeMesh->GetComponentTransform() : Tube->GetActorTransform();

    FirstPersonTubeDecals.Reserve(TubeDecals.Num());
    FirstPersonTubeDecalMIDs.SetNumZeroed(TubeDecals.Num());

    for (UDecalComponent* SourceDecal : TubeDecals)
    {
        if (!SourceDecal)
        {
            FirstPersonTubeDecals.Add(nullptr);
            continue;
        }

        UDecalComponent* PreviewDecal = NewObject<UDecalComponent>(this);
        if (!PreviewDecal)
        {
            FirstPersonTubeDecals.Add(nullptr);
            continue;
        }

        PreviewDecal->RegisterComponent();
        PreviewDecal->AttachToComponent(FirstPersonCarriedItemMesh, FAttachmentTransformRules::KeepRelativeTransform);
        PreviewDecal->DecalSize = SourceDecal->DecalSize;
        PreviewDecal->FadeScreenSize = SourceDecal->FadeScreenSize;
        PreviewDecal->SortOrder = SourceDecal->SortOrder;
        PreviewDecal->SetDecalMaterial(SourceDecal->GetDecalMaterial());

        const FTransform RelativeTransform = SourceDecal->GetComponentTransform().GetRelativeTransform(TubeMeshTransform);
        PreviewDecal->SetRelativeTransform(RelativeTransform);

        PreviewDecal->SetHiddenInGame(true);
        PreviewDecal->SetVisibility(false);

        FirstPersonTubeDecals.Add(PreviewDecal);
    }

    CachedPreviewTube = Tube;
}

void AEMRPlayerCharacter::ClearFirstPersonTubeDecals()
{
    for (UDecalComponent* Decal : FirstPersonTubeDecals)
    {
        if (Decal)
        {
            Decal->DestroyComponent();
        }
    }

    FirstPersonTubeDecals.Reset();
    FirstPersonTubeDecalMIDs.Reset();
    CachedPreviewTube = nullptr;
}

void AEMRPlayerCharacter::UpdatePlacementPreview()
{
	if (!PlacementPreviewMesh || !ItemPlacementComponent)
	{
		SetPlacementPreviewVisible(false);
		return;
	}

	AActor* CarriedActor = CachedCarriedActor.Get();
	if (!CarriedActor)
	{
		CarriedActor = FindCarriedActor();
	}

	AEMRItemActor* CarriedItem = Cast<AEMRItemActor>(CarriedActor);
	if (!CarriedItem)
	{
		SetPlacementPreviewVisible(false);
		return;
	}

	if (Cast<AEMRTreatmentBedFolder>(CarriedActor))
	{
		SetPlacementPreviewVisible(false);
		return;
	}

	if (CarriedActor->GetClass()->ImplementsInterface(UEMRAnchoredCarryItemInterface::StaticClass()))
	{
		SetPlacementPreviewVisible(false);
		return;
	}

	bool bForceOutOfRangeInvalidPreview = false;
	const AEMROxygenMask* OxygenMask = Cast<AEMROxygenMask>(CarriedItem);
	if (OxygenMask)
	{
		const AEMROxygenMachine* OxygenMachine = OxygenMask->OwningMachine;
		bForceOutOfRangeInvalidPreview = !OxygenMachine
			|| !OxygenMachine->IsMaskWithinDistanceLimit(OxygenMask->GetActorLocation());
	}

	FVector ViewLocation = Camera ? Camera->GetComponentLocation() : GetPawnViewLocation();
	FRotator ViewRotation = Camera ? Camera->GetComponentRotation() : GetViewRotation();
	if (APlayerController* PlayerController = CachedPlayerController.Get())
	{
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	const FVector ViewDirection = ViewRotation.Vector();

	FEMRItemPlacementResult PlacementResult;
	FHitResult UsableTargetHit;
	const bool bHasUsableTargetHit = FindBestUsableTargetHit(ViewLocation, ViewDirection, CarriedActor, UsableTargetHit);
	if (OxygenMask)
	{
		const AEMROxygenMachine* OxygenMachine = OxygenMask->OwningMachine;
		const AEMRPatient* TargetPatient = bHasUsableTargetHit ? Cast<AEMRPatient>(UsableTargetHit.GetActor()) : nullptr;
		if (!OxygenMachine || !TargetPatient || !OxygenMachine->IsMaskPlacementSocketAimed(TargetPatient, UsableTargetHit))
		{
			SetPlacementPreviewVisible(false);
			return;
		}
	}
	else if (bHasUsableTargetHit && !bForceOutOfRangeInvalidPreview)
	{
		SetPlacementPreviewVisible(false);
		return;
	}

	bool bHasPlacementResult = false;
	if (bHasUsableTargetHit && bForceOutOfRangeInvalidPreview)
	{
		bHasPlacementResult = ItemPlacementComponent->BuildPlacementFromHit(*this, *CarriedItem, UsableTargetHit, PlacementResult);
	}
	else
	{
		bHasPlacementResult = ItemPlacementComponent->QueryPlacement(*this, *CarriedItem, ViewLocation, ViewDirection, PlacementResult);
	}

	if (!bHasPlacementResult || !PlacementResult.bHasHit)
	{
		SetPlacementPreviewVisible(false);
		return;
	}

	if (bForceOutOfRangeInvalidPreview)
	{
		PlacementResult.bIsValid = false;
	}

	UStaticMesh* DesiredMesh = nullptr;
	const UStaticMeshComponent* SourceMeshComponent = nullptr;
	if (const UEMRItemData* ItemData = CarriedItem->GetItemData())
	{
		DesiredMesh = ItemData->GetWorldItemMesh();
	}
	if (!DesiredMesh)
	{
		if (const UStaticMeshComponent* StaticMeshComponent = CarriedItem->FindComponentByClass<UStaticMeshComponent>())
		{
			SourceMeshComponent = StaticMeshComponent;
			DesiredMesh = StaticMeshComponent->GetStaticMesh();
		}
	}
	else
	{
		SourceMeshComponent = CarriedItem->FindComponentByClass<UStaticMeshComponent>();
	}

	if (!DesiredMesh)
	{
		SetPlacementPreviewVisible(false);
		return;
	}

	if (PlacementPreviewMesh->GetStaticMesh() != DesiredMesh)
	{
		PlacementPreviewMesh->SetStaticMesh(DesiredMesh);
		PlacementPreviewMID = nullptr;
	}

	if (PlacementPreviewMaterial && !PlacementPreviewMID)
	{
		PlacementPreviewMID = UMaterialInstanceDynamic::Create(PlacementPreviewMaterial, this);
		if (PlacementPreviewMID)
		{
			const int32 MaterialCount = PlacementPreviewMesh->GetNumMaterials();
			for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
			{
				PlacementPreviewMesh->SetMaterial(MaterialIndex, PlacementPreviewMID);
			}
		}
	}

	if (PlacementPreviewMID)
	{
		const FLinearColor TargetColor = PlacementResult.bIsValid ? PlacementPreviewValidColor : PlacementPreviewInvalidColor;
		PlacementPreviewMID->SetVectorParameterValue(PlacementPreviewColorParam, TargetColor);
	}

	FTransform PreviewTransform = PlacementResult.Transform;
	if (SourceMeshComponent)
	{
		const FTransform MeshRelative = SourceMeshComponent->GetRelativeTransform();
		PreviewTransform = MeshRelative * PlacementResult.Transform;
	}
	else
	{
		PreviewTransform.SetScale3D(CarriedItem->GetActorScale3D());
	}

	PlacementPreviewMesh->SetWorldTransform(PreviewTransform);
	SetPlacementPreviewVisible(true);
}

void AEMRPlayerCharacter::SetPlacementPreviewVisible(bool bVisible)
{
	if (PlacementPreviewMesh)
	{
		PlacementPreviewMesh->SetHiddenInGame(!bVisible);
	}
}


AActor* AEMRPlayerCharacter::FindCarriedActor() const
{
    TArray<AActor*> AttachedActors;
    GetAttachedActors(AttachedActors);

    for (AActor* Attached : AttachedActors)
    {
        if (!Attached)
        {
            continue;
        }

        if (UEMRCarryableComponent* Carryable = Attached->FindComponentByClass<UEMRCarryableComponent>())
        {
            if (Carryable->IsCarried() && Attached->GetAttachParentActor() == this)
            {
                return Attached;
            }
        }
    }

    return nullptr;
}

void AEMRPlayerCharacter::InitializeHubTabletComponent()
{
	if (!HubTabletComponent)
	{
		return;
	}

	if (HubTabletClass && HubTabletComponent->GetChildActorClass() != HubTabletClass)
	{
		HubTabletComponent->SetChildActorClass(HubTabletClass);
	}

    if (USkeletalMeshComponent* MeshComponent = ResolvePlayerMeshComponent())
        {
                if (MeshComponent->DoesSocketExist(HubTabletSocketName))
                {
                        HubTabletComponent->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HubTabletSocketName);
                }
                else
                {
                        HubTabletComponent->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
                        UE_LOG(LogTemp, Warning, TEXT("[HubTablet] Socket %s missing on %s"), *HubTabletSocketName.ToString(), *GetNameSafe(MeshComponent));
                }

                if (WorldWidgetInteraction)
                {
                        WorldWidgetInteraction->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
                }
    }

	if (WorldWidgetInteraction)
	{
		WorldWidgetInteraction->InteractionSource = EWidgetInteractionSource::CenterScreen;
		WorldWidgetInteraction->TraceChannel = WorldWidgetInteractionTraceChannel;
			WorldWidgetInteraction->InteractionDistance = WorldWidgetInteractionDistance;
		WorldWidgetInteraction->bEnableHitTesting = false;
		WorldWidgetInteraction->SetActive(false);

		if (!IsLocallyControlled())
		{
			WorldWidgetInteraction->SetActive(false);
			WorldWidgetInteraction->bEnableHitTesting = false;
		}
	}

	if (AEMRHubTabletActor* TabletActor = ResolveOwnedHubTablet())
	{
		TabletActor->SetOwner(this);
		TabletActor->RefreshOwnerPlayer();
	}
}

void AEMRPlayerCharacter::BindToHubTabletState()
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s PlayerCharacter BindToHubTabletState skipped: world missing."), PlayerCharacterUpgradesFlowLogPrefix);
		return;
	}

	if (AEMRNightShiftGameState* RunGS = GetWorld()->GetGameState<AEMRNightShiftGameState>())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s PlayerCharacter BindToHubTabletState binding RunGS=%s"),
			PlayerCharacterUpgradesFlowLogPrefix,
			*GetNameSafe(RunGS));
		CachedNightShiftGameState = RunGS;

		if (!HubTabletRunPhaseHandle.IsValid())
		{
			HubTabletRunPhaseHandle = RunGS->OnRunPhaseChanged().AddUObject(this, &ThisClass::HandleHubTabletRunPhaseChanged);
		}

		if (!HubTabletPendingTravelHandle.IsValid())
		{
			HubTabletPendingTravelHandle = RunGS->OnPendingTravelChanged().AddUObject(this, &ThisClass::HandleHubTabletPendingTravelChanged);
		}

		if (!HubTabletSelectionCommittedHandle.IsValid())
		{
			HubTabletSelectionCommittedHandle = RunGS->OnNightShiftSelectionCommittedChanged().AddUObject(this, &ThisClass::HandleHubTabletSelectionCommittedChanged);
		}

        if (!HubTabletDecisionStageHandle.IsValid())
        {
            HubTabletDecisionStageHandle = RunGS->OnHubDecisionStageChanged().AddUObject(this, &ThisClass::HandleHubTabletDecisionStageChanged);
        }

		if (!WatchOvertimeHandle.IsValid())
		{
			WatchOvertimeHandle = RunGS->OnNightShiftOvertimeStarted().AddUObject(this, &ThisClass::HandleWatchOvertimeStarted);
		}

		if (!WatchSpecialEventPhaseHandle.IsValid())
		{
			WatchSpecialEventPhaseHandle = RunGS->OnSpecialEventPhaseChanged().AddUObject(this, &ThisClass::HandleWatchSpecialEventPhaseChanged);
		}

		if (RunGS->GetSpecialEventPhase() == EEMRSpecialEventPhase::Alert)
		{
			HandleWatchSpecialEventPhaseChanged(
				RunGS->GetSpecialEventPhase(),
				RunGS->GetActiveSpecialEventId(),
				RunGS->GetSpecialEventPhaseServerTimestamp());
		}

		UpdateHubTabletState();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s PlayerCharacter BindToHubTabletState retry scheduled (RunGS missing)."), PlayerCharacterUpgradesFlowLogPrefix);
	GetWorld()->GetTimerManager().ClearTimer(HubTabletBindRetryHandle);
	GetWorld()->GetTimerManager().SetTimer(HubTabletBindRetryHandle, this, &ThisClass::BindToHubTabletState, 0.2f, false);
}

void AEMRPlayerCharacter::UnbindFromHubTabletState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HubTabletBindRetryHandle);
	}

	if (AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get())
	{
		if (HubTabletRunPhaseHandle.IsValid())
		{
			RunGS->OnRunPhaseChanged().Remove(HubTabletRunPhaseHandle);
			HubTabletRunPhaseHandle.Reset();
		}

		if (HubTabletPendingTravelHandle.IsValid())
		{
			RunGS->OnPendingTravelChanged().Remove(HubTabletPendingTravelHandle);
			HubTabletPendingTravelHandle.Reset();
		}

		if (HubTabletSelectionCommittedHandle.IsValid())
		{
			RunGS->OnNightShiftSelectionCommittedChanged().Remove(HubTabletSelectionCommittedHandle);
			HubTabletSelectionCommittedHandle.Reset();
		}

        if (HubTabletDecisionStageHandle.IsValid())
        {
            RunGS->OnHubDecisionStageChanged().Remove(HubTabletDecisionStageHandle);
            HubTabletDecisionStageHandle.Reset();
        }

		if (WatchOvertimeHandle.IsValid())
		{
			RunGS->OnNightShiftOvertimeStarted().Remove(WatchOvertimeHandle);
			WatchOvertimeHandle.Reset();
		}

		if (WatchSpecialEventPhaseHandle.IsValid())
		{
			RunGS->OnSpecialEventPhaseChanged().Remove(WatchSpecialEventPhaseHandle);
			WatchSpecialEventPhaseHandle.Reset();
		}
	}

	CachedNightShiftGameState.Reset();
}

void AEMRPlayerCharacter::UpdateHubTabletState()
{
	const AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get();
	if (!RunGS && GetWorld())
	{
		RunGS = GetWorld()->GetGameState<AEMRNightShiftGameState>();
	}

	UpdateHubRotationBehavior(RunGS);

	const bool bInHubPhase = RunGS && RunGS->GetRunPhase() == EER_RunPhase::Hub;
	const bool bIsEndOfSession = RunGS
	&& (RunGS->GetRunPhase() == EER_RunPhase::MissionFinal || RunGS->GetRunPhase() == EER_RunPhase::RunFinished);
	const bool bHasPendingTravel = RunGS && !RunGS->GetPendingTravelURL().IsEmpty();
	const bool bSelectionCommitted = RunGS && RunGS->IsNightShiftSelectionCommitted();
    const EEMRHubDecisionStage HubDecisionStage = RunGS ? RunGS->GetHubDecisionStage() : EEMRHubDecisionStage::NightShiftSelection;
    const bool bIsUpgradeVoting = HubDecisionStage == EEMRHubDecisionStage::UpgradeVoting;
	const bool bShouldShowTabletActor = (bInHubPhase && !bHasPendingTravel) || (bIsEndOfSession && !bHasPendingTravel);
	const bool bShouldShowTabletWidget = bInHubPhase && !bHasPendingTravel && !bIsEndOfSession;
    const bool bCanInteractWithTabletWidget = bShouldShowTabletWidget && (bIsUpgradeVoting || !bSelectionCommitted);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("%s PlayerCharacter UpdateHubTabletState phase=%d stage=%d pendingTravel=%d selectionCommitted=%d showActor=%d showWidget=%d canInteract=%d"),
		PlayerCharacterUpgradesFlowLogPrefix,
		RunGS ? static_cast<int32>(RunGS->GetRunPhase()) : -1,
		static_cast<int32>(HubDecisionStage),
		bHasPendingTravel ? 1 : 0,
		bSelectionCommitted ? 1 : 0,
		bShouldShowTabletActor ? 1 : 0,
		bShouldShowTabletWidget ? 1 : 0,
		bCanInteractWithTabletWidget ? 1 : 0);

	if (AEMRHubTabletActor* TabletActor = ResolveOwnedHubTablet())
	{
		TabletActor->SetActorHiddenInGame(!bShouldShowTabletActor);
        TabletActor->SetTabletWidgetMode(
            bIsUpgradeVoting ? EEMRHubTabletWidgetMode::UpgradeVoting : EEMRHubTabletWidgetMode::NightShiftSelection);
		TabletActor->SetTabletInteractionEnabled(bCanInteractWithTabletWidget);

		if (UWidgetComponent* TabletWidgetComponent = TabletActor->GetTabletWidgetComponent())
		{
			TabletWidgetComponent->SetHiddenInGame(!bShouldShowTabletWidget);
			TabletWidgetComponent->SetVisibility(bShouldShowTabletWidget);
		}
	}

	UpdateWatchAvailability(RunGS);

    if (WorldWidgetInteraction)
        {
                const bool bEnableInteraction = IsLocallyControlled() && bCanInteractWithTabletWidget;
                WorldWidgetInteraction->SetActive(bEnableInteraction);
                WorldWidgetInteraction->bEnableHitTesting = false;
    }
}

void AEMRPlayerCharacter::UpdateHubRotationBehavior(const AEMRNightShiftGameState* RunGS)
{
	if (!RunGS)
	{
		return;
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		const bool bUseControllerRotation = RunGS->GetRunPhase() != EER_RunPhase::Hub;
		MovementComponent->bUseControllerDesiredRotation = bUseControllerRotation;
	}
}

bool AEMRPlayerCharacter::CanUseWatch() const
{
	const AEMRNightShiftGameState* RunGS = CachedNightShiftGameState.Get();
	if (!RunGS && GetWorld())
	{
		RunGS = GetWorld()->GetGameState<AEMRNightShiftGameState>();
	}

	return RunGS && RunGS->GetRunPhase() == EER_RunPhase::InNightShift;
}

void AEMRPlayerCharacter::UpdateWatchAvailability(const AEMRNightShiftGameState* RunGS)
{
	const bool bAllowed = RunGS && RunGS->GetRunPhase() == EER_RunPhase::InNightShift;
	if (!bAllowed && bLookingAtWatch && HasAuthority())
	{
		SetLookingAtWatch(false);
	}
	if (!bAllowed && bWatchSocketLookActive && HasAuthority())
	{
		SetWatchSocketLookActive(false);
	}
}

void AEMRPlayerCharacter::HandleWatchOvertimeStarted()
{
	// Overtime start alert is emitted from NightShift authoritative flow as a global 2D sound.
	// Keep this callback as a no-op to avoid watch-look-state-dependent duplicates.
}

void AEMRPlayerCharacter::HandleWatchSpecialEventPhaseChanged(
	EEMRSpecialEventPhase NewPhase,
	FName EventId,
	float PhaseServerTimestamp)
{
	(void)NewPhase;
	(void)EventId;
	(void)PhaseServerTimestamp;
	// Special-event alert audio is emitted from NightShift authoritative flow as global 2D sound.
	// Keep this callback as a no-op to avoid watch-based alert playback.
}

void AEMRPlayerCharacter::PlayWatchOvertimeCue()
{
    if (!WatchOvertimeSound)
    {
        return;
    }

    const FVector SoundLocation = (WatchWidgetComponent
        ? WatchWidgetComponent->GetComponentLocation()
        : (ThirdPersonWatchMesh ? ThirdPersonWatchMesh->GetComponentLocation() : GetActorLocation()));

    if (!HasAuthority())
    {
        if (IsLocallyControlled())
        {
            Server_PlayWatchSoundForAllPlayers(WatchOvertimeSound, SoundLocation);
        }
        return;
    }

    PlayWorldSoundForAllPlayers(WatchOvertimeSound, SoundLocation);
}

void AEMRPlayerCharacter::PlayWatchSpecialEventAlertCue()
{
    const FVector SoundLocation = (WatchWidgetComponent
        ? WatchWidgetComponent->GetComponentLocation()
        : (ThirdPersonWatchMesh ? ThirdPersonWatchMesh->GetComponentLocation() : GetActorLocation()));

    if (!WatchSpecialEventAlertSound)
    {
        UE_LOG(LogTemp, Warning, TEXT("[WatchSpecialEventCue] Missing WatchSpecialEventAlertSound on %s"), *GetNameSafe(this));
        return;
    }

    if (!HasAuthority())
    {
        if (IsLocallyControlled())
        {
            Server_PlayWatchSoundForAllPlayers(WatchSpecialEventAlertSound, SoundLocation);
        }
        return;
    }

    PlayWorldSoundForAllPlayers(WatchSpecialEventAlertSound, SoundLocation);
}

void AEMRPlayerCharacter::HandleHubTabletRunPhaseChanged(EER_RunPhase NewPhase, EER_RunPhase PreviousPhase)
{
	UpdateHubTabletState();
}

void AEMRPlayerCharacter::HandleHubTabletPendingTravelChanged()
{
	UpdateHubTabletState();
}

void AEMRPlayerCharacter::HandleHubTabletSelectionCommittedChanged()
{
    UpdateHubTabletState();
}

void AEMRPlayerCharacter::HandleHubTabletDecisionStageChanged(EEMRHubDecisionStage NewStage, EEMRHubDecisionStage PreviousStage)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s PlayerCharacter HandleHubTabletDecisionStageChanged %d -> %d"),
        PlayerCharacterUpgradesFlowLogPrefix,
        static_cast<int32>(PreviousStage),
        static_cast<int32>(NewStage));
    UpdateHubTabletState();
}

bool AEMRPlayerCharacter::TryHandleUltrasoundSliderSelect(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Confirm)
    {
        return false;
    }

    if (!GetWorld() || !Camera)
    {
        return false;
    }

    AEMRUltrasoundMachine* Machine = ActiveUltrasoundMachine.Get();
    if (!Machine)
    {
        return false;
    }

    const FVector TraceStart = Camera->GetComponentLocation();
    const FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * WorldWidgetInteractionDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UltrasoundSliderSelectTrace), false, this);
    QueryParams.AddIgnoredActor(this);

    TArray<FHitResult> HitResults;
    const bool bHit = GetWorld()->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, WorldWidgetInteractionTraceChannel, QueryParams);
    if (!bHit || HitResults.IsEmpty())
    {
        return false;
    }

    for (const FHitResult& Hit : HitResults)
    {
        const UPrimitiveComponent* HitComponent = Hit.GetComponent();
        const int32 SliderIndex = Machine->GetSliderKnobIndexForComponent(HitComponent);
        UE_LOG(LogTemp, Log, TEXT("[UltrasoundSliderSelect] Hit component=%s actor=%s sliderIndex=%d dist=%.1f"),
            *GetNameSafe(HitComponent),
            *GetNameSafe(Hit.GetActor()),
            SliderIndex,
            Hit.Distance);
        if (SliderIndex != INDEX_NONE)
        {
            UE_LOG(LogTemp, Log, TEXT("[UltrasoundSliderSelect] Sending slider select to server player=%s machine=%s sliderIndex=%d"),
                *GetNameSafe(this),
                *GetNameSafe(Machine),
                SliderIndex);
            Server_SetUltrasoundActiveSlider(Machine, SliderIndex);
            return true;
        }
    }

    return false;
}

bool AEMRPlayerCharacter::TryHandleCTScanButtonSelect(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Confirm)
    {
        return false;
    }

    if (!GetWorld() || !Camera)
    {
        return false;
    }

    AEMRCTScanMachine* Machine = ActiveCTScanMachine.Get();
    if (!Machine)
    {
        return false;
    }

    const FVector TraceStart = Camera->GetComponentLocation();
    const FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * WorldWidgetInteractionDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CTScanButtonSelectTrace), false, this);
    QueryParams.AddIgnoredActor(this);

    TArray<FHitResult> HitResults;
    const bool bHit = GetWorld()->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, WorldWidgetInteractionTraceChannel, QueryParams);
    if (!bHit || HitResults.IsEmpty())
    {
        return false;
    }

    for (const FHitResult& Hit : HitResults)
    {
        const UPrimitiveComponent* HitComponent = Hit.GetComponent();
        const int32 ButtonIndex = Machine->GetButtonIndexForComponent(HitComponent);
        if (ButtonIndex != INDEX_NONE)
        {
            Server_HandleCTScanButtonPressed(Machine, ButtonIndex);
            return true;
        }
    }

    return false;
}

bool AEMRPlayerCharacter::TryHandleIVStandConfirm(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Confirm)
    {
        return false;
    }

    if (!ActiveIVStand.IsValid())
    {
        return false;
    }

    Server_HandleIVStandConfirm(ActiveIVStand.Get());
    return true;
}

bool AEMRPlayerCharacter::TryHandleOxygenMaskCancelInput(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Cancel)
    {
        return false;
    }

    AActor* CarriedActor = FindCarriedActor();
    AEMROxygenMask* Mask = CarriedActor ? Cast<AEMROxygenMask>(CarriedActor) : nullptr;
    if (!Mask || Mask->IsMaskAttached())
    {
        return false;
    }

    Server_RequestOxygenMaskReturn(Mask);
    return true;
}

bool AEMRPlayerCharacter::TryHandleItemDispenserButtonSelect(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Confirm)
    {
        if (AbilityInputID == EAbilityInputID::Confirm)
        {
            EMR_CONFIRM_LOG(Warning, "TryHandleItemDispenserButtonSelect ignored player=%s local=%d",
                *GetNameSafe(this), IsLocallyControlled() ? 1 : 0);
        }
        return false;
    }

    if (!GetWorld() || !Camera)
    {
        EMR_CONFIRM_LOG(Warning, "TryHandleItemDispenserButtonSelect missing world/camera player=%s world=%d camera=%d",
            *GetNameSafe(this), GetWorld() ? 1 : 0, Camera ? 1 : 0);
        return false;
    }

    const FVector TraceStart = Camera->GetComponentLocation();
    const FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * WorldWidgetInteractionDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ItemDispenserButtonSelectTrace), false, this);
    QueryParams.AddIgnoredActor(this);

    TArray<FHitResult> HitResults;
    const bool bHit = GetWorld()->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, WorldWidgetInteractionTraceChannel, QueryParams);
    if (!bHit || HitResults.IsEmpty())
    {
        EMR_CONFIRM_LOG(Warning, "Dispenser trace miss player=%s start=%s end=%s channel=%d",
            *GetNameSafe(this), *TraceStart.ToCompactString(), *TraceEnd.ToCompactString(), static_cast<int32>(WorldWidgetInteractionTraceChannel.GetValue()));
        return false;
    }

    EMR_CONFIRM_LOG(Warning, "Dispenser trace hit count=%d player=%s", HitResults.Num(), *GetNameSafe(this));

    float ClosestButtonDistance = TNumericLimits<float>::Max();
    TWeakObjectPtr<AEMRItemDispenser> ClosestButtonDispenser = nullptr;
    EEMRItemDispenserButtonType ClosestButtonType = EEMRItemDispenserButtonType::Confirm;
    int32 ClosestButtonDigit = 0;
    bool bHasButtonCandidate = false;

    for (const FHitResult& Hit : HitResults)
    {
        const UPrimitiveComponent* HitComponent = Hit.GetComponent();
        if (!HitComponent)
        {
            continue;
        }

        AEMRItemDispenser* Dispenser = Cast<AEMRItemDispenser>(HitComponent->GetOwner());
        if (!Dispenser)
        {
            continue;
        }

        EMR_CONFIRM_LOG(Log, "Dispenser trace candidate player=%s dispenser=%s component=%s dist=%.2f",
            *GetNameSafe(this), *GetNameSafe(Dispenser), *GetNameSafe(HitComponent), Hit.Distance);

        int32 Digit = 0;
        if (Dispenser->GetDigitForComponent(HitComponent, Digit))
        {
            if (Hit.Distance < ClosestButtonDistance)
            {
                ClosestButtonDistance = Hit.Distance;
                ClosestButtonDispenser = Dispenser;
                ClosestButtonType = EEMRItemDispenserButtonType::Digit;
                ClosestButtonDigit = Digit;
                bHasButtonCandidate = true;
            }
            EMR_CONFIRM_LOG(Log, "Dispenser candidate type=Digit digit=%d dist=%.2f dispenser=%s",
                Digit, Hit.Distance, *GetNameSafe(Dispenser));
            continue;
        }

        if (Dispenser->IsConfirmButtonComponent(HitComponent))
        {
            if (Hit.Distance < ClosestButtonDistance)
            {
                ClosestButtonDistance = Hit.Distance;
                ClosestButtonDispenser = Dispenser;
                ClosestButtonType = EEMRItemDispenserButtonType::Confirm;
                ClosestButtonDigit = 0;
                bHasButtonCandidate = true;
            }
            EMR_CONFIRM_LOG(Log, "Dispenser candidate type=Confirm dist=%.2f dispenser=%s",
                Hit.Distance, *GetNameSafe(Dispenser));
            continue;
        }

        if (Dispenser->IsResetButtonComponent(HitComponent))
        {
            if (Hit.Distance < ClosestButtonDistance)
            {
                ClosestButtonDistance = Hit.Distance;
                ClosestButtonDispenser = Dispenser;
                ClosestButtonType = EEMRItemDispenserButtonType::Reset;
                ClosestButtonDigit = 0;
                bHasButtonCandidate = true;
            }
            EMR_CONFIRM_LOG(Log, "Dispenser candidate type=Reset dist=%.2f dispenser=%s",
                Hit.Distance, *GetNameSafe(Dispenser));
            continue;
        }

        if (Dispenser->IsIncreaseButtonComponent(HitComponent))
        {
            if (Hit.Distance < ClosestButtonDistance)
            {
                ClosestButtonDistance = Hit.Distance;
                ClosestButtonDispenser = Dispenser;
                ClosestButtonType = EEMRItemDispenserButtonType::Increase;
                ClosestButtonDigit = 0;
                bHasButtonCandidate = true;
            }
            EMR_CONFIRM_LOG(Log, "Dispenser candidate type=Increase dist=%.2f dispenser=%s",
                Hit.Distance, *GetNameSafe(Dispenser));
            continue;
        }

        if (Dispenser->IsDecreaseButtonComponent(HitComponent))
        {
            if (Hit.Distance < ClosestButtonDistance)
            {
                ClosestButtonDistance = Hit.Distance;
                ClosestButtonDispenser = Dispenser;
                ClosestButtonType = EEMRItemDispenserButtonType::Decrease;
                ClosestButtonDigit = 0;
                bHasButtonCandidate = true;
            }
            EMR_CONFIRM_LOG(Log, "Dispenser candidate type=Decrease dist=%.2f dispenser=%s",
                Hit.Distance, *GetNameSafe(Dispenser));
            continue;
        }
    }

    if (!bHasButtonCandidate || !ClosestButtonDispenser.IsValid())
    {
        EMR_CONFIRM_LOG(Warning, "Dispenser confirm found no actionable button candidate player=%s",
            *GetNameSafe(this));
        return false;
    }

    FHitResult ItemHit;
    if (FindBestItemHit(TraceStart, Camera->GetForwardVector(), ItemHit) && ItemHit.Distance <= ClosestButtonDistance)
    {
        EMR_CONFIRM_LOG(Warning,
            "Dispenser button deferred because item is closer player=%s item=%s itemDist=%.2f buttonType=%s buttonDist=%.2f dispenser=%s",
            *GetNameSafe(this),
            *GetNameSafe(ItemHit.GetActor()),
            ItemHit.Distance,
            GetDispenserButtonTypeLabel(ClosestButtonType),
            ClosestButtonDistance,
            *GetNameSafe(ClosestButtonDispenser.Get()));
        return false;
    }

    EMR_CONFIRM_LOG(Warning, "Dispenser button RPC player=%s dispenser=%s type=%s digit=%d dist=%.2f",
        *GetNameSafe(this),
        *GetNameSafe(ClosestButtonDispenser.Get()),
        GetDispenserButtonTypeLabel(ClosestButtonType),
        ClosestButtonDigit,
        ClosestButtonDistance);
    Server_HandleItemDispenserButtonPressed(ClosestButtonDispenser.Get(), ClosestButtonType, ClosestButtonDigit);
    return true;
}

bool AEMRPlayerCharacter::TryHandleOvertimeStopTerminalButtonSelect(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Confirm)
    {
        return false;
    }

    if (!GetWorld() || !Camera)
    {
        return false;
    }

    const FVector TraceStart = Camera->GetComponentLocation();
    const FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * WorldWidgetInteractionDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OvertimeTerminalButtonSelectTrace), false, this);
    QueryParams.AddIgnoredActor(this);

    TArray<FHitResult> HitResults;
    const bool bHit = GetWorld()->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, WorldWidgetInteractionTraceChannel, QueryParams);
    if (!bHit || HitResults.IsEmpty())
    {
        return false;
    }

    float ClosestButtonDistance = TNumericLimits<float>::Max();
    TWeakObjectPtr<AEMROvertimeStopTerminal> ClosestTerminal = nullptr;
    for (const FHitResult& Hit : HitResults)
    {
        const UPrimitiveComponent* HitComponent = Hit.GetComponent();
        AEMROvertimeStopTerminal* Terminal = HitComponent ? Cast<AEMROvertimeStopTerminal>(HitComponent->GetOwner()) : nullptr;
        if (!Terminal || !Terminal->IsButtonComponent(HitComponent) || !Terminal->IsConfirmButtonInteractionEnabled())
        {
            continue;
        }

        if (Hit.Distance < ClosestButtonDistance)
        {
            ClosestButtonDistance = Hit.Distance;
            ClosestTerminal = Terminal;
        }
    }

    if (!ClosestTerminal.IsValid())
    {
        return false;
    }

    FHitResult ItemHit;
    if (FindBestItemHit(TraceStart, Camera->GetForwardVector(), ItemHit) && ItemHit.Distance <= ClosestButtonDistance)
    {
        return false;
    }

    Server_HandleOvertimeStopTerminalButtonPressed(ClosestTerminal.Get());
    return true;
}

bool AEMRPlayerCharacter::TryHandleLabAnalyzerButtonSelect(EAbilityInputID AbilityInputID)
{
    if (!IsLocallyControlled() || AbilityInputID != EAbilityInputID::Confirm)
    {
        return false;
    }

    if (!ActiveLabAnalyzerMachine.IsValid())
    {
        return false;
    }

    if (!GetWorld() || !Camera)
    {
        return false;
    }

    EMR_LAB_LOG(Log, "LabAnalyzer confirm input trace");
    const FVector TraceStart = Camera->GetComponentLocation();
    const FVector TraceEnd = TraceStart + (Camera->GetForwardVector() * WorldWidgetInteractionDistance);

    // Lid selection intentionally uses visibility so it follows regular world-hit behavior.
    FCollisionQueryParams LidTraceQueryParams(SCENE_QUERY_STAT(LabAnalyzerLidSelectTrace), false, this);
    LidTraceQueryParams.AddIgnoredActor(this);

    TArray<FHitResult> LidHitResults;
    const bool bLidHit = GetWorld()->LineTraceMultiByChannel(LidHitResults, TraceStart, TraceEnd, ECC_Visibility, LidTraceQueryParams);
    if (bLidHit)
    {
        for (const FHitResult& Hit : LidHitResults)
        {
            const UPrimitiveComponent* HitComponent = Hit.GetComponent();
            if (!HitComponent)
            {
                continue;
            }

            AEMRLabAnalyzerMachine* Machine = ResolveLabAnalyzerFromHit(HitComponent);
            if (!Machine || Machine != ActiveLabAnalyzerMachine.Get())
            {
                continue;
            }

            if (Machine->IsLidComponent(HitComponent))
            {
                Server_HandleLabAnalyzerLidPressed(Machine);
                return true;
            }
        }
    }

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LabAnalyzerButtonSelectTrace), false, this);
    QueryParams.AddIgnoredActor(this);

    TArray<FHitResult> HitResults;
    const bool bHit = GetWorld()->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, WorldWidgetInteractionTraceChannel, QueryParams);
    if (!bHit || HitResults.IsEmpty())
    {
        EMR_LAB_LOG(Log, "LabAnalyzer trace miss");
        return false;
    }

    for (const FHitResult& Hit : HitResults)
    {
        const UPrimitiveComponent* HitComponent = Hit.GetComponent();
        if (!HitComponent)
        {
            continue;
        }

        AEMRLabAnalyzerMachine* Machine = ResolveLabAnalyzerFromHit(HitComponent);
        if (!Machine)
        {
            continue;
        }

        if (Machine != ActiveLabAnalyzerMachine.Get())
        {
            continue;
        }

        EMR_LAB_LOG(Log, "LabAnalyzer trace hit %s", *GetNameSafe(HitComponent));

        if (Machine->IsStartButtonComponent(HitComponent))
        {
            Server_HandleLabAnalyzerStartPressed(Machine);
            return true;
        }
    }

    return false;
}

bool AEMRPlayerCharacter::TryHandleWorldWidgetClick(EAbilityInputID AbilityInputID)
{
    const bool bHandled = PlayerWidgetInteractionComponent
    ? PlayerWidgetInteractionComponent->TryHandleWorldWidgetClick(
            AbilityInputID,
            Camera,
            WorldWidgetInteraction,
            WorldWidgetInteractionTraceChannel,
            WorldWidgetInteractionDistance)
    : false;

    if (AbilityInputID == EAbilityInputID::Confirm && IsLocallyControlled())
    {
        FHitResult WidgetHit;
        const bool bHasWidgetUnderCrosshair = HasWorldWidgetUnderCrosshair(WidgetHit);
        EMR_CONFIRM_LOG(Warning,
            "WorldWidgetClick result player=%s handled=%d hasWidget=%d widgetActor=%s component=%s dist=%.2f",
            *GetNameSafe(this),
            bHandled ? 1 : 0,
            bHasWidgetUnderCrosshair ? 1 : 0,
            *GetNameSafe(WidgetHit.GetActor()),
            *GetNameSafe(WidgetHit.GetComponent()),
            WidgetHit.Distance);
    }

    return bHandled;
}

bool AEMRPlayerCharacter::HasWorldWidgetUnderCrosshair(FHitResult& OutHit) const
{
    return PlayerWidgetInteractionComponent
    ? PlayerWidgetInteractionComponent->HasWorldWidgetUnderCrosshair(
            OutHit,
            Camera,
            WorldWidgetInteraction,
            WorldWidgetInteractionTraceChannel,
            WorldWidgetInteractionDistance)
    : false;
}

bool AEMRPlayerCharacter::FindBestItemHit(const FVector& ViewLocation, const FVector& ViewDirection, FHitResult& OutBestHit) const
{
    OutBestHit = FHitResult();

    UWorld* World = GetWorld();
    if (!World)
    {
        EMR_CONFIRM_LOG(Warning, "FindBestItemHit failed: no world player=%s", *GetNameSafe(this));
        return false;
    }

    float TraceRadius = 30.f;
    float TraceDistance = 900.f;
    float MaxInteractAngle = 25.f;
    if (InteractableDetectionComponent)
    {
        TraceRadius = InteractableDetectionComponent->TraceRadius;
        TraceDistance = InteractableDetectionComponent->TraceDistance;
        MaxInteractAngle = InteractableDetectionComponent->MaxInteractAngle;
    }

    const FVector SafeViewDirection = ViewDirection.GetSafeNormal();
    if (SafeViewDirection.IsNearlyZero())
    {
        EMR_CONFIRM_LOG(Warning, "FindBestItemHit failed: zero view direction player=%s rawDir=%s",
            *GetNameSafe(this), *ViewDirection.ToCompactString());
        return false;
    }

    const FVector TraceEnd = ViewLocation + (SafeViewDirection * TraceDistance);
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceRadius);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ConfirmItemTrace), false, this);
    QueryParams.AddIgnoredActor(this);

    TArray<FHitResult> HitResults;
    const bool bHit = World->SweepMultiByChannel(HitResults, ViewLocation, TraceEnd, FQuat::Identity, ECC_Visibility, SphereShape, QueryParams);
    if (!bHit)
    {
        EMR_CONFIRM_LOG(Warning, "FindBestItemHit sweep miss player=%s start=%s end=%s radius=%.1f",
            *GetNameSafe(this), *ViewLocation.ToCompactString(), *TraceEnd.ToCompactString(), TraceRadius);
        return false;
    }

    EMR_CONFIRM_LOG(Warning, "FindBestItemHit sweep hit count=%d player=%s", HitResults.Num(), *GetNameSafe(this));

    float BestScore = -1.f;
    AActor* ClosestHitActor = nullptr;
    float ClosestHitDistance = TNumericLimits<float>::Max();
    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || !IsItemActor(HitActor))
        {
            if (HitActor && Hit.Distance < ClosestHitDistance)
            {
                ClosestHitDistance = Hit.Distance;
                ClosestHitActor = HitActor;
            }
            if (HitActor)
            {
                EMR_CONFIRM_LOG(Log, "FindBestItemHit non-item hit actor=%s dist=%.2f player=%s",
                    *GetNameSafe(HitActor), Hit.Distance, *GetNameSafe(this));
            }
            continue;
        }

        UEMRCarryableComponent* Carryable = HitActor->FindComponentByClass<UEMRCarryableComponent>();
        if (!Carryable || Carryable->IsCarried())
        {
            EMR_CONFIRM_LOG(Log, "FindBestItemHit skip item=%s reason=%s player=%s",
                *GetNameSafe(HitActor),
                !Carryable ? TEXT("missing carryable") : TEXT("already carried"),
                *GetNameSafe(this));
            continue;
        }

          const bool bIsPatient = HitActor->IsA(AEMRPatient::StaticClass());
          if (!bIsPatient)
          {
              if (!HitActor->GetClass()->ImplementsInterface(UEMRInteractableInterface::StaticClass()))
              {
                  EMR_CONFIRM_LOG(Log, "FindBestItemHit skip item=%s reason=not interactable interface player=%s",
                      *GetNameSafe(HitActor), *GetNameSafe(this));
                  continue;
              }

              if (!IEMRInteractableInterface::Execute_IsInteractableEnabled(HitActor))
              {
                  EMR_CONFIRM_LOG(Log, "FindBestItemHit skip item=%s reason=interactable disabled player=%s",
                      *GetNameSafe(HitActor), *GetNameSafe(this));
                  continue;
              }
          }

        const FVector DirectionToHit = (Hit.ImpactPoint - ViewLocation).GetSafeNormal();
        const float Dot = FVector::DotProduct(DirectionToHit, SafeViewDirection);
        const float ClampedDot = FMath::Clamp(Dot, -1.f, 1.f);
        const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDot));

        if (AngleDegrees > MaxInteractAngle)
        {
            EMR_CONFIRM_LOG(Log, "FindBestItemHit skip item=%s reason=angle angle=%.2f max=%.2f player=%s",
                *GetNameSafe(HitActor), AngleDegrees, MaxInteractAngle, *GetNameSafe(this));
            continue;
        }

        const float Distance = Hit.Distance;
        const float Score = (1.f - (AngleDegrees / MaxInteractAngle)) + (TraceDistance - Distance);
        if (Score > BestScore)
        {
            BestScore = Score;
            OutBestHit = Hit;
            EMR_CONFIRM_LOG(Log, "FindBestItemHit best update item=%s dist=%.2f angle=%.2f score=%.2f player=%s",
                *GetNameSafe(HitActor), Distance, AngleDegrees, Score, *GetNameSafe(this));
        }
    }

    if (BestScore >= 0.f)
    {
        EMR_CONFIRM_LOG(Warning, "FindBestItemHit success item=%s component=%s dist=%.2f player=%s",
            *GetNameSafe(OutBestHit.GetActor()), *GetNameSafe(OutBestHit.GetComponent()), OutBestHit.Distance, *GetNameSafe(this));
        return true;
    }

    if (ClosestHitActor)
    {
        TArray<AActor*> AttachedActors;
        ClosestHitActor->GetAttachedActors(AttachedActors);

        float BestAttachedScore = -1.f;
        FHitResult BestAttachedHit;

        for (AActor* Attached : AttachedActors)
        {
            if (!Attached || !IsItemActor(Attached))
            {
                continue;
            }

            UEMRCarryableComponent* Carryable = Attached->FindComponentByClass<UEMRCarryableComponent>();
            if (!Carryable || Carryable->IsCarried())
            {
                continue;
            }

            if (!Attached->GetClass()->ImplementsInterface(UEMRInteractableInterface::StaticClass()))
            {
                continue;
            }

            if (!IEMRInteractableInterface::Execute_IsInteractableEnabled(Attached))
            {
                continue;
            }

            const FVector ItemLocation = Attached->GetActorLocation();
            const FVector DirectionToHit = (ItemLocation - ViewLocation).GetSafeNormal();
            const float Dot = FVector::DotProduct(DirectionToHit, SafeViewDirection);
            const float ClampedDot = FMath::Clamp(Dot, -1.f, 1.f);
            const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDot));
            if (AngleDegrees > MaxInteractAngle)
            {
                continue;
            }

            const float Distance = FVector::Dist(ViewLocation, ItemLocation);
            const float Score = (1.f - (AngleDegrees / MaxInteractAngle)) + (TraceDistance - Distance);
            if (Score > BestAttachedScore)
            {
                BestAttachedScore = Score;
                UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Attached->GetRootComponent());
                BestAttachedHit = FHitResult(Attached, RootPrimitive, ItemLocation, FVector::ZeroVector);
                BestAttachedHit.Distance = Distance;
                BestAttachedHit.Location = ItemLocation;
                BestAttachedHit.ImpactPoint = ItemLocation;
                BestAttachedHit.bBlockingHit = true;
            }
        }

        if (BestAttachedScore >= 0.f)
        {
            OutBestHit = BestAttachedHit;
            EMR_CONFIRM_LOG(Warning, "FindBestItemHit attached-item fallback success item=%s dist=%.2f player=%s",
                *GetNameSafe(OutBestHit.GetActor()), OutBestHit.Distance, *GetNameSafe(this));
            return true;
        }
    }

    EMR_CONFIRM_LOG(Warning, "FindBestItemHit failed: no eligible item player=%s closestBlockingActor=%s closestDist=%.2f",
        *GetNameSafe(this), *GetNameSafe(ClosestHitActor), ClosestHitDistance);
    return false;
}

bool AEMRPlayerCharacter::FindBestUsableTargetHit(const FVector& ViewLocation, const FVector& ViewDirection, const AActor* CarriedActor, FHitResult& OutBestHit) const
{
    OutBestHit = FHitResult();

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    float TraceRadius = 30.f;
    float TraceDistance = 900.f;
    float MaxInteractAngle = 25.f;
    if (InteractableDetectionComponent)
    {
        TraceRadius = InteractableDetectionComponent->TraceRadius;
        TraceDistance = InteractableDetectionComponent->TraceDistance;
        MaxInteractAngle = InteractableDetectionComponent->MaxInteractAngle;
    }

    const FVector SafeViewDirection = ViewDirection.GetSafeNormal();
    if (SafeViewDirection.IsNearlyZero())
    {
        return false;
    }

    FHitResult PatientHit;
    bool bHasPatientHit = false;
    {
        const FVector TraceStart = ViewLocation;
        const FVector TraceEnd = TraceStart + (SafeViewDirection * TraceDistance);
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ConfirmUseTargetTracePatient), false, this);
        QueryParams.AddIgnoredActor(this);
        if (CarriedActor)
        {
            QueryParams.AddIgnoredActor(CarriedActor);
        }

        if (World->LineTraceSingleByChannel(PatientHit, TraceStart, TraceEnd, ECC_Patient, QueryParams))
        {
            if (AActor* HitActor = PatientHit.GetActor())
            {
                if (HitActor->IsA(AEMRPatient::StaticClass()))
                {
                    const FVector DirectionToHit = (PatientHit.ImpactPoint - ViewLocation).GetSafeNormal();
                    const float Dot = FVector::DotProduct(DirectionToHit, SafeViewDirection);
                    const float ClampedDot = FMath::Clamp(Dot, -1.f, 1.f);
                    const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDot));
                    if (AngleDegrees <= MaxInteractAngle)
                    {
                        bHasPatientHit = true;
                    }
                }
            }
        }
    }

    const FVector TraceEnd = ViewLocation + (SafeViewDirection * TraceDistance);
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceRadius);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ConfirmUseTargetTrace), false, this);
    QueryParams.AddIgnoredActor(this);
    if (CarriedActor)
    {
        QueryParams.AddIgnoredActor(CarriedActor);
    }
    if (Cast<AEMRLabAnalyzerTube>(CarriedActor))
    {
        // When carrying a lab tube, ignore other tubes so we can hit the machine behind them.
        for (TActorIterator<AEMRLabAnalyzerTube> It(World); It; ++It)
        {
            QueryParams.AddIgnoredActor(*It);
        }
    }

    TArray<FHitResult> HitResults;
    const bool bHit = World->SweepMultiByChannel(HitResults, ViewLocation, TraceEnd, FQuat::Identity, ECC_Visibility, SphereShape, QueryParams);
    if (!bHit)
    {
        return false;
    }

    float BestScore = -1.f;
    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor || IsItemActor(HitActor))
        {
            continue;
        }

        if (!HitActor->GetClass()->ImplementsInterface(UEMRInteractableInterface::StaticClass()))
        {
            continue;
        }

        const bool bInteractableEnabled = IEMRInteractableInterface::Execute_IsInteractableEnabled(HitActor);
        const bool bAllowDisabledIVStand = CanUseDisabledIVStandTarget(CarriedActor, HitActor);
        if (!bInteractableEnabled && !bAllowDisabledIVStand)
        {
            continue;
        }

        const FVector DirectionToHit = (Hit.ImpactPoint - ViewLocation).GetSafeNormal();
        const float Dot = FVector::DotProduct(DirectionToHit, SafeViewDirection);
        const float ClampedDot = FMath::Clamp(Dot, -1.f, 1.f);
        const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDot));

        if (AngleDegrees > MaxInteractAngle)
        {
            continue;
        }

        const float Distance = Hit.Distance;
        const float Score = (1.f - (AngleDegrees / MaxInteractAngle)) + (TraceDistance - Distance);
      if (Score > BestScore)
      {
          BestScore = Score;
          OutBestHit = Hit;
      }
    }

    if (bHasPatientHit)
    {
        if (BestScore < 0.f || PatientHit.Distance <= OutBestHit.Distance)
        {
            OutBestHit = PatientHit;
            return true;
        }
    }

    return BestScore >= 0.f;
}

bool AEMRPlayerCharacter::IsItemActor(const AActor* Actor) const
{
    return Actor && Actor->FindComponentByClass<UEMRCarryableComponent>() != nullptr;
}

AEMRHubTabletActor* AEMRPlayerCharacter::ResolveOwnedHubTablet() const
{
	return HubTabletComponent ? Cast<AEMRHubTabletActor>(HubTabletComponent->GetChildActor()) : nullptr;
}


void AEMRPlayerCharacter::GrantStartupAbilities()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (StartupAbilities.IsEmpty() || !ASC || !HasAuthority())
    {
        EMR_CONFIRM_LOG(Warning, "GrantStartupAbilities skipped player=%s hasAuthority=%d startupCount=%d asc=%d",
            *GetNameSafe(this), HasAuthority() ? 1 : 0, StartupAbilities.Num(), ASC ? 1 : 0);
		return;
	}
	

	for (const TPair<EAbilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair : StartupAbilities)
	{
		if (AbilityPair.Value)
		{
			const int32 InputID = static_cast<int32>(AbilityPair.Key);
            
			FGameplayAbilitySpec AbilitySpec(
				AbilityPair.Value,  
				1,                  
				InputID,     
				this 
			);

			ASC->GiveAbility(AbilitySpec);
            EMR_CONFIRM_LOG(Warning, "GrantStartupAbilities gave startup ability player=%s input=%d ability=%s",
                *GetNameSafe(this), InputID, *GetNameSafe(AbilityPair.Value));
		}
	}

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : PassiveAbilities)
    {
        if (AbilityClass)
        {
            FGameplayAbilitySpec AbilitySpec(
                    AbilityClass,
                    1,
                    INDEX_NONE,
                    this
            );
            ASC->GiveAbility(AbilitySpec);
            EMR_CONFIRM_LOG(Warning, "GrantStartupAbilities gave passive ability player=%s ability=%s",
                *GetNameSafe(this), *GetNameSafe(AbilityClass));
        }
    }

    const TArray<FGameplayAbilitySpec>& GrantedAbilities = ASC->GetActivatableAbilities();
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] AbilityList ServerGrant player=%s count=%d netmode=%s"),
        *GetNameSafe(this), GrantedAbilities.Num(), GetNetModeLabel(GetNetMode()));
    for (int32 i = 0; i < GrantedAbilities.Num(); ++i)
    {
        const FGameplayAbilitySpec& Spec = GrantedAbilities[i];
        if (Spec.Ability)
        {
            UE_LOG(LogTemp, Log, TEXT("[XRayFlow] AbilityList ServerGrant [%d] %s (Handle=%ls Active=%s)"),
                i,
                *Spec.Ability->GetName(),
                *Spec.Handle.ToString(),
                Spec.IsActive() ? TEXT("YES") : TEXT("NO"));

            if (const UEMRGameplayAbility_PerformXRay* XRayAbility = Cast<UEMRGameplayAbility_PerformXRay>(Spec.Ability))
            {
                XRayAbility->LogAbilityTriggers(TEXT("ServerGrant"));
            }
        }
    }

    int32 ItemPickAbilityCount = 0;
    for (const FGameplayAbilitySpec& Spec : GrantedAbilities)
    {
        if (!Spec.Ability)
        {
            continue;
        }

        if (Spec.Ability->GetClass()->IsChildOf(UEMRGameplayAbility_PickItem::StaticClass()))
        {
            ++ItemPickAbilityCount;
        }
    }

    EMR_CONFIRM_LOG(Warning, "GrantStartupAbilities complete player=%s totalGranted=%d itemPickAbilities=%d",
        *GetNameSafe(this), GrantedAbilities.Num(), ItemPickAbilityCount);
}
