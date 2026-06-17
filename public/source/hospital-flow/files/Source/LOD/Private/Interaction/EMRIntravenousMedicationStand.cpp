#include "Interaction/EMRIntravenousMedicationStand.h"

#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CableComponent.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GAS/EMRTags.h"
#include "GameplayEffectTypes.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Interaction/EMRTreatmentBed.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NavigationSystem.h"
#include "Shop/EMRItemActor.h"
#include "Subsystems/EMRTreatmentSubsystem.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"

AEMRIntravenousMedicationStand::AEMRIntravenousMedicationStand()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    StandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StandMesh"));
    SetRootComponent(StandMesh);
    StandMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    StandMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    StandMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    CannulaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CannulaMesh"));
    CannulaMesh->SetupAttachment(GetRootComponent());
    CannulaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CannulaMesh->SetMobility(EComponentMobility::Movable);
    CannulaMesh->SetVisibility(bCannulaActive, true);
    CannulaMesh->SetHiddenInGame(!bCannulaActive);

    BagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BagMesh"));
    BagMesh->SetupAttachment(GetRootComponent());
    BagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BagMesh->SetVisibility(true, true);
    BagMesh->SetHiddenInGame(false);

    BagInteriorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BagInteriorMesh"));
    BagInteriorMesh->SetupAttachment(BagMesh);
    BagInteriorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BagInteriorMesh->SetVisibility(false, true);
    BagInteriorMesh->SetHiddenInGame(true);

    TubeStartAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("TubeStartAnchor"));
    TubeStartAnchor->SetupAttachment(GetRootComponent());

    TubeCable = CreateDefaultSubobject<UCableComponent>(TEXT("TubeCable"));
    TubeCable->SetupAttachment(TubeStartAnchor);
    TubeCable->bAttachStart = true;
    TubeCable->bAttachEnd = true;
    TubeCable->CableLength = 120.0f;
    TubeCable->NumSegments = 12;
    TubeCable->SolverIterations = 6;
    TubeCable->CableWidth = 1.25f;
    TubeCable->CableGravityScale = 1.0f;
    TubeCable->bEnableCollision = true;
    TubeCable->CollisionFriction = 0.2f;
    TubeCable->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TubeCable->SetCollisionResponseToAllChannels(ECR_Ignore);
    TubeCable->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    TubeCable->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    TubeCable->SetVisibility(true, true);
    TubeCable->SetHiddenInGame(false);

    OperatorLockPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("OperatorLockPoint"));
    OperatorLockPoint->SetupAttachment(GetRootComponent());
    OperatorLockPoint->SetArrowColor(FLinearColor::Blue);
    OperatorLockPoint->bIsScreenSizeScaled = true;

    OperatorExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("OperatorExitPoint"));
    OperatorExitPoint->SetupAttachment(GetRootComponent());
    OperatorExitPoint->SetArrowColor(FLinearColor::Yellow);
    OperatorExitPoint->bIsScreenSizeScaled = true;

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
}

void AEMRIntravenousMedicationStand::BeginPlay()
{
    Super::BeginPlay();

    if (InteractableComponent)
    {
        InteractableComponent->SetInteractionEventTag(EMRTags::Machine::IntravenousMedication);
        if (InteractableComponent->GetDisplayName().IsEmpty())
        {
            InteractableComponent->SetDisplayName(FText::FromString(TEXT("Intravenous Medication")));
        }
    }

    UpdateBagVisuals();
    UpdateBagFillMaterial();
    if (!bBagInstalled)
    {
        CableFillNormalized = 0.0f;
    }
    UpdateCableFillMaterial();
    if (TubeCable && CannulaMesh)
    {
        TubeCable->SetAttachEndToComponent(CannulaMesh, TubeEndSocketName);
        TubeCable->EndLocation = FVector::ZeroVector;
    }
    UpdateTubeCable();
}

void AEMRIntravenousMedicationStand::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateTubeCable();

    if (bDebugCannulaTargets)
    {
        DebugDrawCannulaPoints();
    }

    if (!HasAuthority())
    {
        UpdateCannulaMovementOnClient(DeltaSeconds);
        return;
    }

    UpdateCurrentPatient();

    if (CurrentOperator && !IsValid(CurrentOperator))
    {
        StopTreatment(nullptr, true);
    }
    else if (CurrentOperator && (!CurrentPatient || IsPatientLeaving(CurrentPatient)))
    {
        StopTreatment(CurrentOperator, true);
    }

    if (bCompletionPending && (!CurrentPatient || IsPatientLeaving(CurrentPatient)))
    {
        ClearCompletionTimer();
        StopBagDepletion();
        StopCableDepletion();
        bBagInstalled = false;
        BagFillNormalized = 0.0f;
        BagFillNetUpdateAccumulator = 0.0f;
        CableFillNormalized = 0.0f;
        CableFillNetUpdateAccumulator = 0.0f;
        PlannedCableDuration = 0.0f;
        bCannulaMovementPaused = false;
        GetWorldTimerManager().ClearTimer(MissPauseTimer);
        DetachCannulaFromPatient();
        bCannulaAttached = false;
        bCannulaActive = false;
        ServerCannulaOffset = FVector::ZeroVector;
        ServerCannulaOffset.Z = 0.0f;
        CannulaOffset = ServerCannulaOffset;
        CannulaTargetOffset = FVector::ZeroVector;
        CannulaNetUpdateAccumulator = 0.0f;
        bHasCannulaTarget = false;
        UpdateCannulaVisibility();
        UpdateCannulaOpacity(false);
        UpdateBagVisuals();
        UpdateBagFillMaterial();
        UpdateCableFillMaterial();
        ForceNetUpdate();
    }

    if (bCannulaActive && CurrentPatient && !bCompletionPending && !bCannulaMovementPaused)
    {
        UpdateCannulaMovement(DeltaSeconds);
    }

    if (bBagDepleting && BagDepleteDuration > 0.0f)
    {
        const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        const float Elapsed = FMath::Max(TimeSeconds - BagDepleteStartTime, 0.0f);
        const float Alpha = FMath::Clamp(Elapsed / BagDepleteDuration, 0.0f, 1.0f);
        const float NewFill = FMath::Clamp(BagDepleteStartFill * (1.0f - Alpha), 0.0f, BagDepleteStartFill);

        if (!FMath::IsNearlyEqual(NewFill, BagFillNormalized, KINDA_SMALL_NUMBER))
        {
            BagFillNormalized = NewFill;
            UpdateBagFillMaterial();
            UpdateBagVisuals();
        }

        BagFillNetUpdateAccumulator += DeltaSeconds;
        if (BagFillNetUpdateAccumulator >= BagFillNetUpdateInterval)
        {
            BagFillNetUpdateAccumulator = 0.0f;
            ForceNetUpdate();
        }

        if (Alpha >= 1.0f)
        {
            CompleteBagDepletion();
            if (!bCableDepleting && PlannedCableDuration > 0.0f)
            {
                StartCableDepletion(PlannedCableDuration);
            }
        }
    }

    if (bCableDepleting && CableDepleteDuration > 0.0f)
    {
        const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        const float Elapsed = FMath::Max(TimeSeconds - CableDepleteStartTime, 0.0f);
        const float Alpha = FMath::Clamp(Elapsed / CableDepleteDuration, 0.0f, 1.0f);
        const float NewFill = FMath::Clamp(CableDepleteStartFill * (1.0f - Alpha), 0.0f, CableDepleteStartFill);

        if (!FMath::IsNearlyEqual(NewFill, CableFillNormalized, KINDA_SMALL_NUMBER))
        {
            CableFillNormalized = NewFill;
            UpdateCableFillMaterial();
        }

        CableFillNetUpdateAccumulator += DeltaSeconds;
        if (CableFillNetUpdateAccumulator >= BagFillNetUpdateInterval)
        {
            CableFillNetUpdateAccumulator = 0.0f;
            ForceNetUpdate();
        }

        if (Alpha >= 1.0f)
        {
            CompleteCableDepletion();
        }
    }
}

void AEMRIntravenousMedicationStand::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRIntravenousMedicationStand, CurrentPatient);
    DOREPLIFETIME(AEMRIntravenousMedicationStand, CurrentOperator);
    DOREPLIFETIME(AEMRIntravenousMedicationStand, CannulaOffset);
    DOREPLIFETIME(AEMRIntravenousMedicationStand, bCannulaActive);
    DOREPLIFETIME(AEMRIntravenousMedicationStand, bCannulaAttached);
    DOREPLIFETIME(AEMRIntravenousMedicationStand, bCompletionPending);
    DOREPLIFETIME(AEMRIntravenousMedicationStand, bBagInstalled);
    DOREPLIFETIME(AEMRIntravenousMedicationStand, BagFillNormalized);
    DOREPLIFETIME(AEMRIntravenousMedicationStand, CableFillNormalized);
}

FGameplayEventData AEMRIntravenousMedicationStand::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    FGameplayEventData EventData;
    EventData.EventTag = EMRTags::Machine::IntravenousMedication;
    EventData.Instigator = InterActor;
    EventData.Target = CurrentPatient;
    EventData.OptionalObject = this;
    return EventData;
}

FText AEMRIntravenousMedicationStand::GetInteractableDisplayName_Implementation() const
{
    if (InteractableComponent)
    {
        return InteractableComponent->GetDisplayName();
    }

    return FText::FromString(GetName());
}

bool AEMRIntravenousMedicationStand::IsInteractableEnabled_Implementation() const
{
    if (InteractableComponent && !InteractableComponent->IsEnabled())
    {
        return false;
    }

    return false;
}

bool AEMRIntravenousMedicationStand::CanAutoStartTreatmentAfterBagInstall() const
{
    if (!HasAuthority())
    {
        return false;
    }

    if (CurrentOperator || bCompletionPending)
    {
        return false;
    }

    if (!CurrentPatient || IsPatientLeaving(CurrentPatient))
    {
        return false;
    }

    if (!DoesPatientNeedTreatment(CurrentPatient))
    {
        return false;
    }

    return HasValidSocket(CurrentPatient);
}

void AEMRIntravenousMedicationStand::PlaceCannulaAutomatically()
{
    if (!HasAuthority())
    {
        return;
    }

    bCannulaActive = false;
    bCannulaAttached = true;
    bCannulaMovementPaused = false;
    bHasCannulaTarget = false;
    GetWorldTimerManager().ClearTimer(MissPauseTimer);

    ServerCannulaOffset = FVector::ZeroVector;
    ServerCannulaOffset.Z = 0.0f;
    CannulaOffset = ServerCannulaOffset;
    CannulaTargetOffset = FVector::ZeroVector;
    CannulaNetUpdateAccumulator = 0.0f;

    DetachCannulaFromPatient();
    ApplyCannulaOffset(ServerCannulaOffset);
    AttachCannulaToPatient();
    UpdateCannulaVisibility();
    UpdateCannulaOpacity(true);
}

void AEMRIntravenousMedicationStand::BeginAutomaticTreatmentAfterBagInstall(AEMRPlayerCharacter* InstallingPlayer)
{
    if (!InstallingPlayer || !CanAutoStartTreatmentAfterBagInstall())
    {
        return;
    }

    PlaceCannulaAutomatically();
    TriggerResultCue(true);
    bCompletionPending = true;
    StartCompletionTimer();
}

void AEMRIntravenousMedicationStand::TryStartTreatment(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    if (CurrentOperator && CurrentOperator != Player)
    {
        return;
    }

    if (bCompletionPending || !CurrentPatient || IsPatientLeaving(CurrentPatient))
    {
        return;
    }

    if (!DoesPatientNeedTreatment(CurrentPatient))
    {
        return;
    }

    if (!HasValidSocket(CurrentPatient))
    {
        return;
    }

    if (!HasFullBag())
    {
        PlayBagCue(MissingBagSound, Player);
        return;
    }

    CurrentOperator = Player;
    LockOperator(Player);
    Player->Client_BeginIVTreatment(this, TreatmentInputMappingContext);

    bCannulaAttached = false;
    DetachCannulaFromPatient();
    bCannulaMovementPaused = false;
    GetWorldTimerManager().ClearTimer(MissPauseTimer);
    ServerCannulaOffset = CannulaOffset;
    ServerCannulaOffset.Z = 0.0f;
    CannulaOffset = ServerCannulaOffset;
    CannulaNetUpdateAccumulator = 0.0f;
    bCannulaActive = true;
    bHasCannulaTarget = false;
    UpdateCannulaVisibility();
    UpdateCannulaOpacity(IsCannulaWithinTolerance());
    ForceNetUpdate();
}

void AEMRIntravenousMedicationStand::StopTreatment(AEMRPlayerCharacter* Player, bool bFromCancel)
{
    if (!HasAuthority())
    {
        return;
    }

    if (Player && CurrentOperator && CurrentOperator != Player)
    {
        return;
    }

    if (Player)
    {
        UnlockOperator(Player);
        Player->Client_EndIVTreatment(this);
    }

    CurrentOperator = nullptr;
    bCannulaActive = false;
    bCannulaMovementPaused = false;
    GetWorldTimerManager().ClearTimer(MissPauseTimer);
    if (bFromCancel)
    {
        StopBagDepletion();
        StopCableDepletion();
    }

    const bool bShouldDetach = bFromCancel || !bCannulaAttached;
    if (bShouldDetach)
    {
        DetachCannulaFromPatient();
        bCannulaAttached = false;
        ServerCannulaOffset = FVector::ZeroVector;
        ServerCannulaOffset.Z = 0.0f;
        CannulaOffset = ServerCannulaOffset;
        CannulaTargetOffset = FVector::ZeroVector;
        CannulaNetUpdateAccumulator = 0.0f;
        bHasCannulaTarget = false;
    }

    UpdateCannulaVisibility();
    UpdateCannulaOpacity(bCannulaAttached ? true : IsOffsetWithinTolerance(CannulaOffset));
    ForceNetUpdate();
}

void AEMRIntravenousMedicationStand::HandleConfirmPressed(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player || CurrentOperator != Player)
    {
        return;
    }

    if (!bCannulaActive || bCompletionPending || !CurrentPatient || bCannulaMovementPaused)
    {
        return;
    }

    const bool bSuccess = IsCannulaWithinTolerance();
    TriggerResultCue(bSuccess);

    if (bSuccess)
    {
        bCannulaAttached = true;
        bCannulaActive = false;
        bCannulaMovementPaused = false;
        GetWorldTimerManager().ClearTimer(MissPauseTimer);
        AttachCannulaToPatient();
        UpdateCannulaVisibility();
        UpdateCannulaOpacity(true);
        bCompletionPending = true;
        StartCompletionTimer();
        StopTreatment(Player, false);
        return;
    }

    bHasCannulaTarget = false;
    StartMissPause();
}

bool AEMRIntravenousMedicationStand::TryInstallBag(AEMRPlayerCharacter* Player, AEMRItemActor* BagActor)
{
    if (!HasAuthority() || !Player || !BagActor)
    {
        return false;
    }

    if (CurrentOperator || bCompletionPending)
    {
        return false;
    }

    if (UEMRCarryableComponent* Carryable = BagActor->FindComponentByClass<UEMRCarryableComponent>())
    {
        if (!Carryable->IsCarried() || BagActor->GetAttachParentActor() != Player)
        {
            return false;
        }
    }

    if (!CanAutoStartTreatmentAfterBagInstall())
    {
        return false;
    }

    StopBagDepletion();
    StopCableDepletion();
    bBagInstalled = true;
    BagFillNormalized = 1.0f;
    BagFillNetUpdateAccumulator = 0.0f;
    CableFillNormalized = 1.0f;
    CableFillNetUpdateAccumulator = 0.0f;
    UpdateBagFillMaterial();
    UpdateCableFillMaterial();
    UpdateBagVisuals();
    PlayBagCue(BagInstalledSound, Player);
    BeginAutomaticTreatmentAfterBagInstall(Player);
    ForceNetUpdate();

    BagActor->Destroy();
    return true;
}

void AEMRIntravenousMedicationStand::OnRep_CurrentPatient()
{
    bClientCannulaOffsetInitialized = false;
    ApplyCannulaOffset(CannulaOffset);
    UpdateCannulaVisibility();
    UpdateCannulaOpacity(IsOffsetWithinTolerance(CannulaOffset));
    if (bCannulaAttached && IsCurrentPatientAttachedToBed())
    {
        AttachCannulaToPatient();
    }
}

void AEMRIntravenousMedicationStand::OnRep_CurrentOperator()
{
}

void AEMRIntravenousMedicationStand::OnRep_CannulaOffset()
{
    if (!HasAuthority() && bCannulaActive && !bCannulaAttached && CurrentPatient)
    {
        if (!bClientCannulaOffsetInitialized)
        {
            ServerCannulaOffset = CannulaOffset;
            ServerCannulaOffset.Z = 0.0f;
            bClientCannulaOffsetInitialized = true;
            ApplyCannulaOffset(ServerCannulaOffset);
        }

        UpdateCannulaOpacity(IsOffsetWithinTolerance(ServerCannulaOffset));
        return;
    }

    ApplyCannulaOffset(CannulaOffset);
    UpdateCannulaOpacity(IsOffsetWithinTolerance(CannulaOffset));
}

void AEMRIntravenousMedicationStand::OnRep_CannulaActive()
{
    bClientCannulaOffsetInitialized = false;
    UpdateCannulaVisibility();
    UpdateCannulaOpacity(IsOffsetWithinTolerance(CannulaOffset));
}

void AEMRIntravenousMedicationStand::OnRep_CannulaAttached()
{
    bClientCannulaOffsetInitialized = false;
    if (bCannulaAttached && IsCurrentPatientAttachedToBed())
    {
        AttachCannulaToPatient();
    }
    else
    {
        DetachCannulaFromPatient();
    }
    UpdateCannulaVisibility();
    UpdateCannulaOpacity(bCannulaAttached ? true : IsOffsetWithinTolerance(CannulaOffset));
}

void AEMRIntravenousMedicationStand::OnRep_BagInstalled()
{
    UpdateBagVisuals();
    UpdateBagFillMaterial();
    if (!bBagInstalled)
    {
        CableFillNormalized = 0.0f;
    }
    UpdateCableFillMaterial();
}

void AEMRIntravenousMedicationStand::OnRep_BagFill()
{
    UpdateBagFillMaterial();
    UpdateBagVisuals();
}

void AEMRIntravenousMedicationStand::OnRep_CableFill()
{
    UpdateCableFillMaterial();
}

void AEMRIntravenousMedicationStand::UpdateCurrentPatient()
{
    AEMRPatient* BedPatient = AssignedBed ? AssignedBed->GetCurrentPatient() : nullptr;
    if (BedPatient != CurrentPatient)
    {
        if (CurrentOperator)
        {
            StopTreatment(CurrentOperator, true);
        }

        CurrentPatient = BedPatient;
        ClearCompletionTimer();
        StopBagDepletion();
        StopCableDepletion();
        bBagInstalled = false;
        BagFillNormalized = 0.0f;
        BagFillNetUpdateAccumulator = 0.0f;
        CableFillNormalized = 0.0f;
        CableFillNetUpdateAccumulator = 0.0f;
        PlannedCableDuration = 0.0f;
        bCannulaMovementPaused = false;
        GetWorldTimerManager().ClearTimer(MissPauseTimer);
        DetachCannulaFromPatient();
        bCannulaAttached = false;
        ServerCannulaOffset = FVector::ZeroVector;
        ServerCannulaOffset.Z = 0.0f;
        CannulaOffset = ServerCannulaOffset;
        CannulaTargetOffset = FVector::ZeroVector;
        CannulaNetUpdateAccumulator = 0.0f;
        bHasCannulaTarget = false;
        bCannulaActive = false;
        bLoggedMissingSocket = false;
        UpdateCannulaVisibility();
        UpdateCannulaOpacity(false);
        UpdateBagVisuals();
        UpdateBagFillMaterial();
        UpdateCableFillMaterial();
        ForceNetUpdate();
    }
}

void AEMRIntravenousMedicationStand::UpdateCannulaMovement(float DeltaSeconds)
{
    if (!CurrentPatient)
    {
        return;
    }

    if (!HasValidSocket(CurrentPatient))
    {
        return;
    }

    if (!bHasCannulaTarget)
    {
        PickNewCannulaTarget();
    }

    const FVector Direction = (CannulaTargetOffset - ServerCannulaOffset);
    const FVector Direction2D(Direction.X, Direction.Y, 0.0f);
    const float Distance = Direction.Size();
    const float Step = CannulaMoveSpeed * DeltaSeconds;

    if (Distance <= Step || Distance <= KINDA_SMALL_NUMBER)
    {
        ServerCannulaOffset = CannulaTargetOffset;
        bHasCannulaTarget = false;
    }
    else
    {
        ServerCannulaOffset += Direction2D.GetSafeNormal() * Step;
        ServerCannulaOffset.Z = 0.0f;
    }

    ApplyCannulaOffset(ServerCannulaOffset);
    UpdateCannulaOpacity(IsCannulaWithinTolerance());

    CannulaNetUpdateAccumulator += DeltaSeconds;
    if (CannulaNetUpdateAccumulator >= CannulaNetUpdateInterval)
    {
        CannulaOffset = ServerCannulaOffset;
        CannulaNetUpdateAccumulator = 0.0f;
        ForceNetUpdate();
    }
}

void AEMRIntravenousMedicationStand::UpdateCannulaMovementOnClient(float DeltaSeconds)
{
    if (HasAuthority())
    {
        return;
    }

    if (!bCannulaActive || bCannulaAttached || !CurrentPatient || bCompletionPending || bCannulaMovementPaused)
    {
        bClientCannulaOffsetInitialized = false;
        return;
    }

    const FVector TargetOffset(CannulaOffset.X, CannulaOffset.Y, 0.0f);
    if (!bClientCannulaOffsetInitialized)
    {
        ServerCannulaOffset = TargetOffset;
        bClientCannulaOffsetInitialized = true;
    }
    else
    {
        const float InterpSpeed = CannulaMoveSpeed > 0.0f ? CannulaMoveSpeed : 1.0f;
        ServerCannulaOffset = FMath::VInterpConstantTo(ServerCannulaOffset, TargetOffset, DeltaSeconds, InterpSpeed);
        ServerCannulaOffset.Z = 0.0f;
    }

    ApplyCannulaOffset(ServerCannulaOffset);
    UpdateCannulaOpacity(IsOffsetWithinTolerance(ServerCannulaOffset));
}

void AEMRIntravenousMedicationStand::ApplyCannulaOffset(const FVector& Offset)
{
    if (!CannulaMesh)
    {
        return;
    }

    if (!CurrentPatient || !IsCurrentPatientAttachedToBed())
    {
        CannulaMesh->SetVisibility(false, true);
        CannulaMesh->SetHiddenInGame(true);
        return;
    }

    USkeletalMeshComponent* PatientMesh = nullptr;
    FName TargetSocketName = NAME_None;
    if (!ResolveTargetSocket(CurrentPatient, PatientMesh, TargetSocketName))
    {
        CannulaMesh->SetVisibility(false, true);
        CannulaMesh->SetHiddenInGame(true);
        return;
    }

    const FTransform SocketWorld = PatientMesh->GetSocketTransform(TargetSocketName);
    const FQuat CombinedRotation = SocketWorld.GetRotation() * CannulaRotationOffset.Quaternion();
    const FVector LocalOffset(Offset.X, Offset.Y, 0.0f);
    const FVector WorldOffset = CombinedRotation.RotateVector(LocalOffset);
    const FVector WorldLocation = SocketWorld.GetLocation()
    + CombinedRotation.RotateVector(CannulaLocationOffset)
    + WorldOffset;
    CannulaMesh->SetWorldLocationAndRotation(WorldLocation, CombinedRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AEMRIntravenousMedicationStand::UpdateCannulaVisibility()
{
    if (!CannulaMesh)
    {
        return;
    }

    const bool bVisible = (bCannulaActive || bCannulaAttached)
    && CurrentPatient != nullptr
    && IsCurrentPatientAttachedToBed();
    CannulaMesh->SetVisibility(bVisible, true);
    CannulaMesh->SetHiddenInGame(!bVisible);
}

void AEMRIntravenousMedicationStand::UpdateCannulaOpacity(bool bInRange)
{
    if (!CannulaMesh)
    {
        return;
    }

    if (!CannulaMID)
    {
        CannulaMID = CannulaMesh->CreateDynamicMaterialInstance(0);
    }

    if (!CannulaMID || CannulaOpacityParameterName == NAME_None)
    {
        return;
    }

    const float TargetOpacity = bInRange ? CannulaReadyOpacity : CannulaDimOpacity;
    CannulaMID->SetScalarParameterValue(CannulaOpacityParameterName, TargetOpacity);
}

void AEMRIntravenousMedicationStand::PickNewCannulaTarget()
{
    CannulaTargetOffset = FVector(
        FMath::FRandRange(-CannulaBounds.X, CannulaBounds.X),
        FMath::FRandRange(-CannulaBounds.Y, CannulaBounds.Y),
        0.0f);
    bHasCannulaTarget = true;
}

bool AEMRIntravenousMedicationStand::IsCannulaWithinTolerance() const
{
    return IsOffsetWithinTolerance(ServerCannulaOffset);
}

bool AEMRIntravenousMedicationStand::IsOffsetWithinTolerance(const FVector& Offset) const
{
    const FVector2D Offset2D(Offset.X, Offset.Y);
    return Offset2D.SizeSquared() <= FMath::Square(TargetToleranceRadius);
}

bool AEMRIntravenousMedicationStand::IsCurrentPatientAttachedToBed() const
{
    return AssignedBed
    && CurrentPatient
    && CurrentPatient->GetAttachParentActor() == AssignedBed;
}

void AEMRIntravenousMedicationStand::StartMissPause()
{
    if (!HasAuthority())
    {
        return;
    }

    if (MissPauseDuration <= 0.0f)
    {
        bCannulaMovementPaused = false;
        return;
    }

    bCannulaMovementPaused = true;
    GetWorldTimerManager().ClearTimer(MissPauseTimer);
    GetWorldTimerManager().SetTimer(
        MissPauseTimer,
        this,
        &ThisClass::HandleMissPauseTimer,
        MissPauseDuration,
        false);
}

void AEMRIntravenousMedicationStand::HandleMissPauseTimer()
{
    if (!HasAuthority())
    {
        return;
    }

    bCannulaMovementPaused = false;
    bHasCannulaTarget = false;
}


void AEMRIntravenousMedicationStand::AttachCannulaToPatient()
{
    if (!CannulaMesh || !CurrentPatient || !IsCurrentPatientAttachedToBed())
    {
        return;
    }

    USkeletalMeshComponent* PatientMesh = ResolvePatientMeshFor(CurrentPatient);
    if (!PatientMesh)
    {
        return;
    }

    CannulaMesh->AttachToComponent(PatientMesh, FAttachmentTransformRules::KeepWorldTransform);
}

void AEMRIntravenousMedicationStand::DetachCannulaFromPatient()
{
    if (!CannulaMesh)
    {
        return;
    }

    USceneComponent* AttachParent = CannulaMesh->GetAttachParent();
    if (!AttachParent)
    {
        return;
    }

    if (const AActor* ParentOwner = AttachParent->GetOwner())
    {
        if (ParentOwner->IsA<AEMRPatient>())
        {
            CannulaMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        }
    }
}

void AEMRIntravenousMedicationStand::DebugDrawCannulaPoints() const
{
    if (!CurrentPatient)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    USkeletalMeshComponent* PatientMesh = nullptr;
    FName TargetSocketName = NAME_None;
    FVector SocketLocation = FVector::ZeroVector;
    FQuat SocketRotation = FQuat::Identity;
    bool bHasSocket = false;
    if (ResolveTargetSocket(CurrentPatient, PatientMesh, TargetSocketName))
    {
        const FTransform SocketWorld = PatientMesh->GetSocketTransform(TargetSocketName);
        SocketLocation = SocketWorld.GetLocation();
        SocketRotation = SocketWorld.GetRotation();
        bHasSocket = true;
        DrawDebugSphere(World, SocketLocation, TargetToleranceRadius, 16, FColor::Green, false, 0.0f);
    }

    if (CannulaMesh)
    {
        const FVector OffsetForCheck = HasAuthority() ? ServerCannulaOffset : CannulaOffset;
        const bool bWithinTolerance = IsOffsetWithinTolerance(OffsetForCheck);
        const FColor CannulaColor = bWithinTolerance ? FColor::Green : FColor::Red;
        FVector CannulaPoint = CannulaMesh->GetComponentLocation();
        if (bHasSocket)
        {
            const FQuat CombinedRotation = SocketRotation * CannulaRotationOffset.Quaternion();
            CannulaPoint = CannulaMesh->GetComponentLocation() - CombinedRotation.RotateVector(CannulaLocationOffset);
        }
        DrawDebugSphere(World, CannulaPoint, DebugPointSize, 12, CannulaColor, false, 0.0f);
    }
}

bool AEMRIntravenousMedicationStand::DoesPatientNeedTreatment(const AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World || !World->GetGameInstance())
    {
        return false;
    }

    UEMRTreatmentSubsystem* TreatmentSubsystem = World->GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>();
    if (!TreatmentSubsystem)
    {
        return false;
    }

    const FGameplayTagContainer NeededTreatments = TreatmentSubsystem->GetTreatmentsFromPatient(const_cast<AEMRPatient*>(Patient));
    return NeededTreatments.HasTagExact(EMRTags::Abilities::Treatment::IntravenousMedication);
}

bool AEMRIntravenousMedicationStand::IsPatientLeaving(const AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return true;
    }

    if (const UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent())
    {
        return ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving);
    }

    return false;
}

bool AEMRIntravenousMedicationStand::HasValidSocket(const AEMRPatient* Patient) const
{
    USkeletalMeshComponent* PatientMesh = nullptr;
    FName TargetSocketName = NAME_None;
    return ResolveTargetSocket(Patient, PatientMesh, TargetSocketName);
}

FName AEMRIntravenousMedicationStand::GetConfiguredTargetSocketName() const
{
    return CannulaTargetSide == EEMRIVStandCannulaSide::Right
    ? RightTargetSocketName
    : LeftTargetSocketName;
}

bool AEMRIntravenousMedicationStand::ResolveTargetSocket(
    const AEMRPatient* Patient,
    USkeletalMeshComponent*& OutPatientMesh,
    FName& OutSocketName) const
{
    OutPatientMesh = ResolvePatientMeshFor(Patient);
    OutSocketName = NAME_None;

    if (!OutPatientMesh)
    {
        if (HasAuthority() && !bLoggedMissingSocket)
        {
            UE_LOG(LogTemp, Warning, TEXT("[IVStand] Patient %s missing mesh"), *GetNameSafe(Patient));
            bLoggedMissingSocket = true;
        }
        return false;
    }

    if (LeftTargetSocketName.IsNone() || RightTargetSocketName.IsNone())
    {
        if (HasAuthority() && !bLoggedMissingSocket)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[IVStand] Stand %s has invalid socket config. Left=%s Right=%s"),
                *GetNameSafe(this),
                *LeftTargetSocketName.ToString(),
                *RightTargetSocketName.ToString());
            bLoggedMissingSocket = true;
        }
        return false;
    }

    const bool bHasLeftSocket = OutPatientMesh->DoesSocketExist(LeftTargetSocketName);
    const bool bHasRightSocket = OutPatientMesh->DoesSocketExist(RightTargetSocketName);
    if (!bHasLeftSocket || !bHasRightSocket)
    {
        if (HasAuthority() && !bLoggedMissingSocket)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[IVStand] Patient %s missing required IV sockets. Left=%s (%d) Right=%s (%d)"),
                *GetNameSafe(Patient),
                *LeftTargetSocketName.ToString(),
                bHasLeftSocket ? 1 : 0,
                *RightTargetSocketName.ToString(),
                bHasRightSocket ? 1 : 0);
            bLoggedMissingSocket = true;
        }
        return false;
    }

    OutSocketName = GetConfiguredTargetSocketName();
    if (!OutPatientMesh->DoesSocketExist(OutSocketName))
    {
        if (HasAuthority() && !bLoggedMissingSocket)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[IVStand] Patient %s missing selected IV socket %s"),
                *GetNameSafe(Patient),
                *OutSocketName.ToString());
            bLoggedMissingSocket = true;
        }
        return false;
    }

    return true;
}

bool AEMRIntravenousMedicationStand::HasFullBag() const
{
    return bBagInstalled && BagFillNormalized > KINDA_SMALL_NUMBER;
}

USkeletalMeshComponent* AEMRIntravenousMedicationStand::ResolvePatientMeshFor(const AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return nullptr;
    }

    if (CachedPatientMesh.IsValid() && CachedPatientMeshOwner.Get() == Patient)
    {
        return CachedPatientMesh.Get();
    }

    if (PatientMeshComponentTag != NAME_None)
    {
        const TArray<UActorComponent*> TaggedComponents = Patient->GetComponentsByTag(
            USkeletalMeshComponent::StaticClass(),
            PatientMeshComponentTag);
        if (TaggedComponents.Num() > 0)
        {
            if (USkeletalMeshComponent* TaggedMesh = Cast<USkeletalMeshComponent>(TaggedComponents[0]))
            {
                CachedPatientMesh = TaggedMesh;
                CachedPatientMeshOwner = Patient;
                return TaggedMesh;
            }
        }
    }

    if (PatientMeshComponentName != NAME_None)
    {
        TArray<USkeletalMeshComponent*> MeshComponents;
        Patient->GetComponents<USkeletalMeshComponent>(MeshComponents);
        for (USkeletalMeshComponent* MeshComponent : MeshComponents)
        {
            if (MeshComponent && MeshComponent->GetFName() == PatientMeshComponentName)
            {
                CachedPatientMesh = MeshComponent;
                CachedPatientMeshOwner = Patient;
                return MeshComponent;
            }
        }
    }

    if (USkeletalMeshComponent* DefaultMesh = Patient->GetMesh())
    {
        CachedPatientMesh = DefaultMesh;
        CachedPatientMeshOwner = Patient;
        return DefaultMesh;
    }

    return nullptr;
}

void AEMRIntravenousMedicationStand::LockOperator(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    USceneComponent* LockTransformComponent = OperatorLockPoint ? OperatorLockPoint : GetRootComponent();
    if (!LockTransformComponent)
    {
        return;
    }

    const FTransform LockTransform = LockTransformComponent->GetComponentTransform();
    FVector LockLocation = LockTransform.GetLocation();

    if (UWorld* World = GetWorld())
    {
        const float CapsuleHalfHeight = Player->GetCapsuleComponent()
            ? Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
            : 0.0f;

        bool bSnapped = false;
        const FVector TraceStart = LockLocation + FVector(0.0f, 0.0f, 50.0f);
        const FVector TraceEnd = LockLocation - FVector(0.0f, 0.0f, 2000.0f);
        FHitResult Hit;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(IVStandOperatorSnapTrace), false, this);
        QueryParams.AddIgnoredActor(Player);
        QueryParams.AddIgnoredActor(this);

        if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
        {
            LockLocation.Z = Hit.Location.Z + CapsuleHalfHeight;
            bSnapped = true;
        }

        if (!bSnapped)
        {
            if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
            {
                FNavLocation NavLocation;
                if (NavSys->ProjectPointToNavigation(LockLocation, NavLocation, FVector(50.0f, 50.0f, 500.0f)))
                {
                    LockLocation.Z = NavLocation.Location.Z + CapsuleHalfHeight;
                }
            }
        }
    }

    Player->SetActorLocationAndRotation(
        LockLocation,
        LockTransform.GetRotation().Rotator(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

    if (AController* Controller = Player->GetController())
    {
        const FRotator LockRotation = LockTransform.GetRotation().Rotator();
        Controller->SetControlRotation(LockRotation);

        if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
        {
            PlayerController->ClientSetRotation(LockRotation, true);
        }
    }

    if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
    {
        MovementComponent->DisableMovement();
    }
}

void AEMRIntravenousMedicationStand::UnlockOperator(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    UArrowComponent* ExitComponent = OperatorExitPoint;
    if (ExitComponent)
    {
        const FTransform ExitTransform = ExitComponent->GetComponentTransform();
        FVector ExitLocation = ExitTransform.GetLocation();

        if (UWorld* World = GetWorld())
        {
            const float CapsuleHalfHeight = Player->GetCapsuleComponent()
                ? Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
                : 0.0f;

            bool bSnapped = false;
            const FVector TraceStart = ExitLocation + FVector(0.0f, 0.0f, 50.0f);
            const FVector TraceEnd = ExitLocation - FVector(0.0f, 0.0f, 2000.0f);
            FHitResult Hit;
            FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(IVStandOperatorExitTrace), false, this);
            QueryParams.AddIgnoredActor(Player);
            QueryParams.AddIgnoredActor(this);

            if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
            {
                ExitLocation.Z = Hit.Location.Z + CapsuleHalfHeight;
                bSnapped = true;
            }

            if (!bSnapped)
            {
                if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
                {
                    FNavLocation NavLocation;
                    if (NavSys->ProjectPointToNavigation(ExitLocation, NavLocation, FVector(50.0f, 50.0f, 500.0f)))
                    {
                        ExitLocation.Z = NavLocation.Location.Z + CapsuleHalfHeight;
                    }
                }
            }
        }

        Player->SetActorLocationAndRotation(
            ExitLocation,
            ExitTransform.GetRotation().Rotator(),
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
    }

    if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
    {
        MovementComponent->SetMovementMode(MOVE_Walking);
    }
}

void AEMRIntravenousMedicationStand::StartCompletionTimer()
{
    if (!HasAuthority() || !CurrentPatient)
    {
        return;
    }

    SetCurrentPatientPatienceDrainSuppressed(true);

    float DelaySeconds = 0.0f;
    if (const AEMRNightShiftGameState* NightShiftState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr)
    {
        if (const UEMRDifficultyTuningData* Tuning = NightShiftState->GetDifficultyTuning())
        {
            const UEMRNightShiftDefinition* Definition = NightShiftState->GetCurrentNightShiftDefinition();
            const EEMRNightShiftDifficultyTier Difficulty = Definition ? Definition->DifficultyTier : EEMRNightShiftDifficultyTier::Calm;
            DelaySeconds = Tuning->GetIntravenousMedicationCompletionDelay(Difficulty);
        }
    }

    StopBagDepletion();
    StopCableDepletion();

    PlannedCableDuration = FMath::Clamp(CableDepleteDurationSeconds, 0.0f, DelaySeconds);
    const float BagDuration = FMath::Max(DelaySeconds - PlannedCableDuration, 0.0f);

    if (DelaySeconds <= 0.0f)
    {
        FinishTreatment();
        return;
    }

    if (BagDuration > 0.0f && BagFillNormalized > 0.0f)
    {
        StartBagDepletion(BagDuration);
    }
    else
    {
        CompleteBagDepletion();
        if (PlannedCableDuration > 0.0f)
        {
            StartCableDepletion(PlannedCableDuration);
        }
    }

    GetWorldTimerManager().SetTimer(
        CompletionTimer,
        this,
        &ThisClass::HandleCompletionTimer,
        DelaySeconds,
        false);
}

void AEMRIntravenousMedicationStand::HandleCompletionTimer()
{
    FinishTreatment();
}

void AEMRIntravenousMedicationStand::ClearCompletionTimer()
{
    if (!HasAuthority())
    {
        return;
    }

    ClearPatienceDrainSuppression();

    if (CompletionTimer.IsValid())
    {
        GetWorldTimerManager().ClearTimer(CompletionTimer);
    }

    bCompletionPending = false;
}

void AEMRIntravenousMedicationStand::SetCurrentPatientPatienceDrainSuppressed(const bool bSuppressed)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!bSuppressed)
    {
        ClearPatienceDrainSuppression();
        return;
    }

    AEMRPatient* Patient = CurrentPatient;
    if (!Patient)
    {
        return;
    }

    if (PatienceDrainSuppressedPatient.IsValid() && PatienceDrainSuppressedPatient.Get() != Patient)
    {
        PatienceDrainSuppressedPatient->SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::IntravenousTreatment, false);
        PatienceDrainSuppressedPatient.Reset();
    }

    Patient->SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::IntravenousTreatment, true);
    PatienceDrainSuppressedPatient = Patient;
}

void AEMRIntravenousMedicationStand::ClearPatienceDrainSuppression()
{
    if (!HasAuthority())
    {
        return;
    }

    if (PatienceDrainSuppressedPatient.IsValid())
    {
        PatienceDrainSuppressedPatient->SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::IntravenousTreatment, false);
    }

    PatienceDrainSuppressedPatient.Reset();
}

void AEMRIntravenousMedicationStand::TriggerResultCue(bool bSuccess)
{
    USoundBase* ResultSound = bSuccess ? SuccessResultSound : FailureResultSound;
    if (!ResultSound)
    {
        return;
    }

    PlayBagCue(ResultSound, CurrentOperator.Get());
}

void AEMRIntravenousMedicationStand::PlayBagCue(USoundBase* Sound, AEMRPlayerCharacter* Player) const
{
    if (!HasAuthority() || !Sound)
    {
        return;
    }

    (void)Player;

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector SoundLocation = GetActorLocation();

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

void AEMRIntravenousMedicationStand::StartBagDepletion(float DurationSeconds)
{
    if (!HasAuthority() || !bBagInstalled)
    {
        return;
    }

    BagFillNormalized = FMath::Clamp(BagFillNormalized, 0.0f, 1.0f);
    BagDepleteStartFill = BagFillNormalized;
    BagDepleteDuration = FMath::Max(DurationSeconds, 0.0f);
    BagDepleteStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    BagFillNetUpdateAccumulator = 0.0f;

    if (BagDepleteDuration <= 0.0f || BagDepleteStartFill <= 0.0f)
    {
        CompleteBagDepletion();
        return;
    }

    bBagDepleting = true;
}

void AEMRIntravenousMedicationStand::StopBagDepletion()
{
    bBagDepleting = false;
    BagDepleteDuration = 0.0f;
    BagDepleteStartTime = 0.0f;
    BagDepleteStartFill = BagFillNormalized;
    BagFillNetUpdateAccumulator = 0.0f;
}

void AEMRIntravenousMedicationStand::CompleteBagDepletion()
{
    bBagDepleting = false;
    BagDepleteDuration = 0.0f;
    BagDepleteStartTime = 0.0f;
    BagDepleteStartFill = 0.0f;
    BagFillNetUpdateAccumulator = 0.0f;
    BagFillNormalized = 0.0f;
    UpdateBagFillMaterial();
    UpdateBagVisuals();
    ForceNetUpdate();
}

void AEMRIntravenousMedicationStand::StartCableDepletion(float DurationSeconds)
{
    if (!HasAuthority())
    {
        return;
    }

    CableFillNormalized = FMath::Clamp(CableFillNormalized, 0.0f, 1.0f);
    CableDepleteStartFill = CableFillNormalized;
    CableDepleteDuration = FMath::Max(DurationSeconds, 0.0f);
    CableDepleteStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    CableFillNetUpdateAccumulator = 0.0f;

    if (CableDepleteDuration <= 0.0f || CableDepleteStartFill <= 0.0f)
    {
        CompleteCableDepletion();
        return;
    }

    bCableDepleting = true;
}

void AEMRIntravenousMedicationStand::StopCableDepletion()
{
    bCableDepleting = false;
    CableDepleteDuration = 0.0f;
    CableDepleteStartTime = 0.0f;
    CableDepleteStartFill = CableFillNormalized;
    CableFillNetUpdateAccumulator = 0.0f;
}

void AEMRIntravenousMedicationStand::CompleteCableDepletion()
{
    bCableDepleting = false;
    CableDepleteDuration = 0.0f;
    CableDepleteStartTime = 0.0f;
    CableDepleteStartFill = 0.0f;
    CableFillNetUpdateAccumulator = 0.0f;
    CableFillNormalized = 0.0f;
    UpdateCableFillMaterial();
    ForceNetUpdate();
}

void AEMRIntravenousMedicationStand::UpdateCableFillMaterial()
{
    if (!TubeCable)
    {
        return;
    }

    UMaterialInterface* CurrentMaterial = TubeCable->GetMaterial(0);
    UMaterialInstanceDynamic* CurrentMID = Cast<UMaterialInstanceDynamic>(CurrentMaterial);
    if (CurrentMID && CurrentMID != CableMID)
    {
        CableMID = CurrentMID;
    }

    if (!CableMID)
    {
        CableMID = TubeCable->CreateDynamicMaterialInstance(0, CurrentMaterial);
    }

    if (!CableMID || BagFillParameterName == NAME_None)
    {
        return;
    }

    CableMID->SetScalarParameterValue(BagFillParameterName, FMath::Clamp(CableFillNormalized, 0.0f, 1.0f));
}

void AEMRIntravenousMedicationStand::FinishTreatment()
{
    if (!HasAuthority() || !bCompletionPending || !CurrentPatient)
    {
        return;
    }

    CompleteCableDepletion();
    bCompletionPending = false;
    ClearCompletionTimer();

    UWorld* World = GetWorld();
    if (!World || !World->GetGameInstance())
    {
        return;
    }

    UEMRTreatmentSubsystem* TreatmentSubsystem = World->GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>();
    if (!TreatmentSubsystem)
    {
        return;
    }

    TreatmentSubsystem->CompleteTreatmentForPatient(CurrentPatient, EMRTags::Abilities::Treatment::IntravenousMedication, true);
    ForceNetUpdate();
}

void AEMRIntravenousMedicationStand::UpdateBagVisuals()
{
    if (BagMesh)
    {
        BagMesh->SetVisibility(true, true);
        BagMesh->SetHiddenInGame(false);
    }

    const bool bInteriorVisible = bBagInstalled && BagFillNormalized > KINDA_SMALL_NUMBER;
    if (BagInteriorMesh)
    {
        BagInteriorMesh->SetVisibility(bInteriorVisible, true);
        BagInteriorMesh->SetHiddenInGame(!bInteriorVisible);
    }

    if (!bBagInstalled && !bCableDepleting && !FMath::IsNearlyZero(CableFillNormalized))
    {
        CableFillNormalized = 0.0f;
        UpdateCableFillMaterial();
    }

    UpdateTubeCable();
}

void AEMRIntravenousMedicationStand::UpdateBagFillMaterial()
{
    if (!BagInteriorMesh)
    {
        return;
    }

    if (!BagInteriorSourceMaterial)
    {
        BagInteriorSourceMaterial = BagInteriorMesh->GetMaterial(0);
    }

    if (!BagInteriorMID)
    {
        BagInteriorMID = BagInteriorMesh->CreateDynamicMaterialInstance(0, BagInteriorSourceMaterial);
    }

    if (!BagInteriorMID || BagFillParameterName == NAME_None)
    {
        return;
    }

    BagInteriorMID->SetScalarParameterValue(BagFillParameterName, FMath::Clamp(BagFillNormalized, 0.0f, 1.0f));
}

bool AEMRIntravenousMedicationStand::ShouldShowTube() const
{
    if (bTubeAlwaysVisible)
    {
        return true;
    }

    if (bTubeRequiresCannula)
    {
        return bCannulaActive || bCannulaAttached;
    }

    return true;
}

void AEMRIntravenousMedicationStand::UpdateTubeCable()
{
    if (!TubeCable || !CannulaMesh)
    {
        return;
    }

    const bool bShowTube = ShouldShowTube();
    TubeCable->SetVisibility(bShowTube, true);
    TubeCable->SetHiddenInGame(!bShowTube);
    if (!bShowTube)
    {
        return;
    }

    TubeCable->bAttachStart = true;
    TubeCable->bAttachEnd = true;
    if (TubeCable->GetAttachedComponent() != CannulaMesh || TubeCable->AttachEndToSocketName != TubeEndSocketName)
    {
        TubeCable->SetAttachEndToComponent(CannulaMesh, TubeEndSocketName);
    }
    TubeCable->EndLocation = FVector::ZeroVector;
}
