#include "Interaction/EMRReceptionMachine.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/EMRTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/EMRSeatedAnimationInterface.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"

AEMRReceptionMachine::AEMRReceptionMachine()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(false);
    bReplicates = true;

    SeatAnimationTag = EMRTags::SeatAnimation::Reception;

    TriageWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TriageWidget"));
    TriageWidgetComponent->SetupAttachment(GetRootComponent());
	TriageWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	TriageWidgetComponent->SetDrawAtDesiredSize(false);
	TriageWidgetComponent->SetComponentTickEnabled(false);
	TriageWidgetComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriageWidgetComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriageWidgetComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    TriageDetailPanelWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TriageDetailPanel"));
    TriageDetailPanelWidgetComponent->SetupAttachment(GetRootComponent());
	TriageDetailPanelWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    TriageDetailPanelWidgetComponent->SetDrawAtDesiredSize(false);
    TriageDetailPanelWidgetComponent->SetComponentTickEnabled(false);
	TriageDetailPanelWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TriageDetailPanelWidgetComponent->SetVisibility(false);
    TriageDetailPanelWidgetComponent->SetHiddenInGame(true);

    SeatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SeatMesh"));
    SeatMesh->SetupAttachment(GetRootComponent());
    SeatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SeatMesh->SetMobility(EComponentMobility::Movable);

    SeatPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SeatPoint"));
    SeatPoint->SetupAttachment(SeatMesh);
    SeatPoint->SetArrowColor(FLinearColor::Blue);
    SeatPoint->bIsScreenSizeScaled = true;

    ExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SeatExitPoint"));
    ExitPoint->SetupAttachment(GetRootComponent());
    ExitPoint->SetArrowColor(FLinearColor::Yellow);
    ExitPoint->bIsScreenSizeScaled = true;

}

void AEMRReceptionMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRReceptionMachine, SeatedPlayer);
    DOREPLIFETIME(AEMRReceptionMachine, SeatMeshYawOffset);
}

void AEMRReceptionMachine::BeginPlay()
{
    Super::BeginPlay();

	if (TriageDetailPanelWidgetComponent)
	{
		TriageDetailPanelWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TriageDetailPanelWidgetComponent->SetVisibility(false);
        TriageDetailPanelWidgetComponent->SetHiddenInGame(true);
    }

    if (SeatMesh)
    {
        SeatMesh->SetMobility(EComponentMobility::Movable);
        SeatMesh->SetUsingAbsoluteLocation(false);
        SeatMesh->SetUsingAbsoluteRotation(false);
        SeatMesh->SetUsingAbsoluteScale(false);
    }
}

void AEMRReceptionMachine::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateSeatMeshYawFromPlayer(DeltaSeconds);
}

void AEMRReceptionMachine::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority() && SeatedPlayer)
    {
        UnseatPlayer(SeatedPlayer.Get());
    }

    Super::EndPlay(EndPlayReason);
}

void AEMRReceptionMachine::SeatPlayer(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ReceptionSeat] SeatPlayer rejected (Authority=%d Player=%s)"), HasAuthority() ? 1 : 0, *GetNameSafe(Player));
        return;
    }

    if (SeatedPlayer && SeatedPlayer != Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ReceptionSeat] SeatPlayer rejected (AlreadySeated=%s Incoming=%s)"), *GetNameSafe(SeatedPlayer.Get()), *GetNameSafe(Player));
        return;
    }

    USceneComponent* AttachComponent = SeatAttachComponent;
    if (!AttachComponent && SeatMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ReceptionSeat] SeatPlayer missing root component; falling back to SeatMesh attachment."));
        AttachComponent = SeatMesh;
    }

    USceneComponent* SeatTransformComponent = SeatPoint ? SeatPoint : AttachComponent;
    if (!AttachComponent || !SeatTransformComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ReceptionSeat] SeatPlayer missing components (Attach=%s SeatTransform=%s)"), *GetNameSafe(AttachComponent), *GetNameSafe(SeatTransformComponent));
        return;
    }

    const FTransform SeatTransform = SeatTransformComponent->GetComponentTransform();
    UE_LOG(LogTemp, Log, TEXT("[ReceptionSeat] SeatPlayer %s at %s (Attach=%s SeatPoint=%s)"),
        *GetNameSafe(Player),
        *SeatTransform.GetLocation().ToString(),
        *GetNameSafe(AttachComponent),
        *GetNameSafe(SeatPoint));
    Player->SetActorLocationAndRotation(
        SeatTransform.GetLocation(),
        SeatTransform.GetRotation().Rotator(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
    Player->AttachToComponent(AttachComponent, FAttachmentTransformRules::KeepWorldTransform);

    if (AController* Controller = Player->GetController())
    {
        const FRotator SeatRotation = SeatTransform.GetRotation().Rotator();
        Controller->SetControlRotation(SeatRotation);

        if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
        {
            PlayerController->ClientSetRotation(SeatRotation, true);
        }
    }

    if (Player->GetClass()->ImplementsInterface(UEMRSeatedAnimationInterface::StaticClass()))
    {
        IEMRSeatedAnimationInterface::Execute_SetSeatedAnimationTag(Player, SeatAnimationTag);
    }

    Player->RequestInstantSeatedCameraTransition();
    Player->SetIsSeated(true);
    Player->SetSeatedRotateBodyWithCamera(true);

    if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
    {
        MovementComponent->DisableMovement();
    }

    SeatedPlayer = Player;
    UE_LOG(LogTemp, Log, TEXT("[ReceptionSeat] SeatedPlayer set to %s"), *GetNameSafe(SeatedPlayer.Get()));
    AttachSeatMeshToPlayer(Player);

    if (SeatEnterSound)
    {
        Player->PlayWorldSoundForAllPlayers(SeatEnterSound, SeatTransform.GetLocation());
    }
}

void AEMRReceptionMachine::UnseatPlayer(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ReceptionSeat] UnseatPlayer rejected (Authority=%d Player=%s)"), HasAuthority() ? 1 : 0, *GetNameSafe(Player));
        return;
    }

    if (SeatedPlayer != Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ReceptionSeat] UnseatPlayer rejected (Seated=%s Incoming=%s)"), *GetNameSafe(SeatedPlayer.Get()), *GetNameSafe(Player));
        return;
    }

    Player->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    if (ExitPoint)
    {
        UE_LOG(LogTemp, Log, TEXT("[ReceptionSeat] UnseatPlayer moving to ExitPoint %s"), *ExitPoint->GetComponentLocation().ToString());
        const FTransform ExitTransform = ExitPoint->GetComponentTransform();
        Player->SetActorLocationAndRotation(
            ExitTransform.GetLocation(),
            ExitTransform.GetRotation().Rotator(),
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
    }

    Player->RequestInstantSeatedCameraTransition();
    Player->SetIsSeated(false);

    if (Player->GetClass()->ImplementsInterface(UEMRSeatedAnimationInterface::StaticClass()))
    {
        IEMRSeatedAnimationInterface::Execute_SetSeatedAnimationTag(Player, FGameplayTag());
    }

    Player->SetSeatedRotateBodyWithCamera(false);

    if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
    {
        MovementComponent->SetMovementMode(MOVE_Walking);
    }

    SeatedPlayer = nullptr;
    UE_LOG(LogTemp, Log, TEXT("[ReceptionSeat] SeatedPlayer cleared"));
    DetachSeatMeshToOriginal();

    if (SeatExitSound)
    {
        const FVector SoundLocation = ExitPoint ? ExitPoint->GetComponentLocation() : Player->GetActorLocation();
        Player->PlayWorldSoundForAllPlayers(SeatExitSound, SoundLocation);
    }
}

void AEMRReceptionMachine::OnRep_SeatedPlayer()
{
    UE_LOG(LogTemp, Log, TEXT("[ReceptionSeat] OnRep_SeatedPlayer %s"), *GetNameSafe(SeatedPlayer.Get()));
    if (SeatedPlayer)
    {
        AttachSeatMeshToPlayer(SeatedPlayer.Get());
    }
    else
    {
        DetachSeatMeshToOriginal();
    }
}

void AEMRReceptionMachine::AttachSeatMeshToPlayer(AEMRPlayerCharacter* Player)
{
    if (!SeatMesh || !Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ReceptionSeat] AttachSeatMeshToPlayer missing SeatMesh=%s Player=%s"), *GetNameSafe(SeatMesh), *GetNameSafe(Player));
        return;
    }

    SeatMesh->SetMobility(EComponentMobility::Movable);
    SeatMesh->SetUsingAbsoluteLocation(false);
    SeatMesh->SetUsingAbsoluteRotation(false);
    SeatMesh->SetUsingAbsoluteScale(false);

    if (!bSeatMeshAttachCached)
    {
        SeatMeshOriginalAttachParent = SeatMesh->GetAttachParent();
        SeatMeshOriginalRelativeTransform = SeatMesh->GetRelativeTransform();
        bSeatMeshAttachCached = true;
    }

    SeatMeshBaseWorldRotation = SeatMesh->GetComponentRotation();
    SeatMeshInitialPlayerYaw = Player->GetActorRotation().Yaw;
    if (AController* Controller = Player->GetController())
    {
        SeatMeshInitialControlYaw = Controller->GetControlRotation().Yaw;
    }
    else
    {
        SeatMeshInitialControlYaw = SeatMeshInitialPlayerYaw;
    }
    bSeatMeshFollowYaw = true;
    if (HasAuthority())
    {
        SeatMeshYawOffset = 0.f;
    }
    SeatMeshYawOffsetSmoothed = 0.f;
    bSeatMeshYawInitialized = false;
    ApplySeatMeshYawOffset();
    SetActorTickEnabled(bSeatMeshFollowYaw);
}

void AEMRReceptionMachine::DetachSeatMeshToOriginal()
{
    if (!SeatMesh || !bSeatMeshAttachCached)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ReceptionSeat] DetachSeatMeshToOriginal skipped (SeatMesh=%s Cached=%d)"), *GetNameSafe(SeatMesh), bSeatMeshAttachCached ? 1 : 0);
        return;
    }

    USceneComponent* RestoreParent = SeatMeshOriginalAttachParent ? SeatMeshOriginalAttachParent.Get() : GetRootComponent();
    if (RestoreParent)
    {
        UE_LOG(LogTemp, Log, TEXT("[ReceptionSeat] Restoring SeatMesh parent=%s rel=%s"), *GetNameSafe(RestoreParent), *SeatMeshOriginalRelativeTransform.GetLocation().ToString());
        SeatMesh->AttachToComponent(RestoreParent, FAttachmentTransformRules::KeepWorldTransform);
        SeatMesh->SetRelativeTransform(SeatMeshOriginalRelativeTransform);
    }

    bSeatMeshFollowYaw = false;
    if (HasAuthority())
    {
        SeatMeshYawOffset = 0.f;
    }
    SeatMeshYawOffsetSmoothed = 0.f;
    bSeatMeshYawInitialized = false;
    SetActorTickEnabled(false);
    bSeatMeshAttachCached = false;
    SeatMeshOriginalAttachParent = nullptr;
}

void AEMRReceptionMachine::UpdateSeatMeshYawFromPlayer(float DeltaSeconds)
{
    if (!bSeatMeshFollowYaw || !SeatMesh || !SeatedPlayer)
    {
        return;
    }

    float CurrentControlYaw = SeatMeshInitialControlYaw;
    if (AController* Controller = SeatedPlayer->GetController())
    {
        CurrentControlYaw = Controller->GetControlRotation().Yaw;
    }
    else
    {
        CurrentControlYaw = SeatedPlayer->GetActorRotation().Yaw;
    }

    const float TargetOffset = FMath::FindDeltaAngleDegrees(SeatMeshInitialControlYaw, CurrentControlYaw);

    if (HasAuthority())
    {
        SeatMeshYawOffset = TargetOffset;
    }

    const bool bLocallyControlled = SeatedPlayer->IsLocallyControlled();
    const float DesiredOffset = bLocallyControlled ? TargetOffset : SeatMeshYawOffset;
    const bool bInstant = bLocallyControlled || SeatYawInterpSpeed <= 0.f;

    if (!bSeatMeshYawInitialized || bInstant)
    {
        SeatMeshYawOffsetSmoothed = DesiredOffset;
        bSeatMeshYawInitialized = true;
    }
    else
    {
        const float Delta = FMath::FindDeltaAngleDegrees(SeatMeshYawOffsetSmoothed, DesiredOffset);
        const float Step = Delta * FMath::Clamp(DeltaSeconds * SeatYawInterpSpeed, 0.f, 1.f);
        SeatMeshYawOffsetSmoothed = FMath::UnwindDegrees(SeatMeshYawOffsetSmoothed + Step);
    }

    ApplySeatMeshYawOffset();
}

void AEMRReceptionMachine::ApplySeatMeshYawOffset()
{
    if (!SeatMesh || !bSeatMeshAttachCached)
    {
        return;
    }

    FRotator NewRotation = SeatMeshBaseWorldRotation;
    NewRotation.Yaw = FMath::UnwindDegrees(SeatMeshBaseWorldRotation.Yaw + SeatMeshYawOffsetSmoothed);
    SeatMesh->SetWorldRotation(NewRotation, false, nullptr, ETeleportType::None);
}

void AEMRReceptionMachine::PlayValidationCueForNearbyPlayers(bool bSuccess, AActor* InstigatorActor)
{
    (void)InstigatorActor;

    if (!HasAuthority())
    {
        return;
    }

    USoundBase* ValidationSound = bSuccess ? ValidationSuccessSound : ValidationFailureSound;
    if (!ValidationSound)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector Origin = GetActorLocation();
    for (TActorIterator<AEMRPlayerCharacter> It(World); It; ++It)
    {
        AEMRPlayerCharacter* Player = *It;
        if (!Player)
        {
            continue;
        }

        Player->Client_PlayWorldSoundAtLocation(ValidationSound, Origin);
    }
}

void AEMRReceptionMachine::OnRep_SeatMeshYawOffset()
{
    if (!bSeatMeshYawInitialized)
    {
        SeatMeshYawOffsetSmoothed = SeatMeshYawOffset;
        bSeatMeshYawInitialized = true;
    }
    ApplySeatMeshYawOffset();
}
