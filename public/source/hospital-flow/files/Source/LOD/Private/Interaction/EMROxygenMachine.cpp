#include "Interaction/EMROxygenMachine.h"

#include "AbilitySystemComponent.h"
#include "CableComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Data/EMRNightShiftDefinition.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMROxygenMask.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/EMRTreatmentSubsystem.h"
#include "TimerManager.h"

#define OXY_LOG(Format, ...) UE_LOG(LogTemp, Warning, TEXT("[OxygenMachine] " Format), ##__VA_ARGS__)

AEMROxygenMachine::AEMROxygenMachine()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(60.0f);
    SetMinNetUpdateFrequency(30.0f);

    FRepMovement& ReplicatedMovementConfig = GetReplicatedMovement_Mutable();
    ReplicatedMovementConfig.LocationQuantizationLevel = EVectorQuantization::RoundTwoDecimals;
    ReplicatedMovementConfig.VelocityQuantizationLevel = EVectorQuantization::RoundTwoDecimals;

    MachineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MachineMesh"));
    SetRootComponent(MachineMesh);
    MachineMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    MaskHomeAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("MaskHomeAnchor"));
    MaskHomeAnchor->SetupAttachment(GetRootComponent());

    MaskCable = CreateDefaultSubobject<UCableComponent>(TEXT("MaskCable"));
    MaskCable->SetupAttachment(MaskHomeAnchor);
    MaskCable->bAttachStart = true;
    MaskCable->bAttachEnd = true;
    MaskCable->CableLength = 120.0f;
    MaskCable->NumSegments = 12;
    MaskCable->SolverIterations = 6;
    MaskCable->CableWidth = 1.25f;
    MaskCable->CableGravityScale = 1.0f;
    MaskCable->bEnableCollision = true;
    MaskCable->CollisionFriction = 0.2f;
    MaskCable->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MaskCable->SetCollisionResponseToAllChannels(ECR_Ignore);
    MaskCable->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    MaskCable->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    MaskCable->SetVisibility(true, true);
    MaskCable->SetHiddenInGame(false);

    MaskReturnTraceSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MaskReturnTraceSphere"));
    MaskReturnTraceSphere->SetupAttachment(MaskHomeAnchor);
    MaskReturnTraceSphere->SetSphereRadius(20.0f);
    MaskReturnTraceSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MaskReturnTraceSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    MaskReturnTraceSphere->SetGenerateOverlapEvents(false);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));

    TreatmentLoopAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("TreatmentLoopAudioComponent"));
    TreatmentLoopAudioComponent->SetupAttachment(GetRootComponent());
    TreatmentLoopAudioComponent->bAutoActivate = false;
}

AEMROxygenMask* AEMROxygenMachine::GetOwnedMaskActor() const
{
    AEMROxygenMask* CandidateMask = MaskActor.Get();
    if (!CandidateMask)
    {
        return nullptr;
    }

    if (!CandidateMask->OwningMachine || CandidateMask->OwningMachine == this)
    {
        return CandidateMask;
    }

    return nullptr;
}

void AEMROxygenMachine::ResolveMaskActorBinding()
{
    AEMROxygenMask* CandidateMask = MaskActor.Get();
    if (!CandidateMask)
    {
        return;
    }

    if (CandidateMask->OwningMachine && CandidateMask->OwningMachine != this)
    {
        OXY_LOG(
            "ResolveMaskActorBinding: Machine '%s' references mask '%s' already owned by '%s'. Clearing binding to avoid cross-machine ownership.",
            *GetNameSafe(this),
            *GetNameSafe(CandidateMask),
            *GetNameSafe(CandidateMask->OwningMachine));
        MaskActor = nullptr;
        return;
    }

    CandidateMask->SetOwningMachine(this);
}

void AEMROxygenMachine::BeginPlay()
{
    Super::BeginPlay();

    if (MachineMesh && !MoveCollisionProfileOverride.IsNone())
    {
        MachineMesh->SetCollisionProfileName(MoveCollisionProfileOverride);
        if (MachineMesh->GetCollisionProfileName() != MoveCollisionProfileOverride)
        {
            UE_LOG(LogTemp, Warning, TEXT("[OxygenMachine] Invalid MoveCollisionProfileOverride '%s' on %s. Using existing mesh collision profile."),
                *MoveCollisionProfileOverride.ToString(),
                *GetNameSafe(this));
        }
    }

    if (InteractableComponent)
    {
        InteractableComponent->SetInteractionEventTag(EMRTags::Machine::Oxygen);
        if (InteractableComponent->GetDisplayName().IsEmpty())
        {
            InteractableComponent->SetDisplayName(FText::FromString(TEXT("Oxygen Machine")));
        }
    }

    ResolveMaskActorBinding();
    if (!MaskActor && HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[OxygenMachine] Missing MaskActor on %s"), *GetNameSafe(this));
    }

    if (TreatmentLoopAudioComponent)
    {
        TreatmentLoopAudioComponent->SetSound(TreatmentLoopSound);
    }

    if (HasAuthority())
    {
        const bool bUsesUpgradeGate = RequiredUpgradeTag.IsValid();
        bool bShouldEnableByUpgrade = true;
        if (bUsesUpgradeGate)
        {
            const AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
            const int32 UpgradeStackCount = RunGS ? RunGS->GetActiveRunUpgradeStackCount(RequiredUpgradeTag) : 0;
            bShouldEnableByUpgrade = UpgradeStackCount >= GetRequiredUpgradeStackCount();
        }

        SetOxygenMachineEnabledByUpgrade(bShouldEnableByUpgrade);
    }
    else
    {
        // Keep baseline machines (without upgrade tag or with stack-0 requirement) visible before replication arrives.
        bOxygenMachineEnabledByUpgrade = !RequiredUpgradeTag.IsValid() || GetRequiredUpgradeStackCount() <= 0;
        ApplyMachineEnabledState();
    }

    UpdateMaskCable();
    RefreshInteractableState();
    RefreshTreatmentLoopAudioPlayback();
}

void AEMROxygenMachine::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateMaskCable();
    RefreshInteractableState();

    if (MaskReturnTraceSphere)
    {
        const AEMROxygenMask* EffectiveMask = GetOwnedMaskActor();
        const bool bEnableReturnTrace =
            bOxygenMachineEnabledByUpgrade
            && HasAuthority()
            && EffectiveMask
            && EffectiveMask->IsMaskCarried()
            && !EffectiveMask->IsMaskAttached();

        MaskReturnTraceSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
        MaskReturnTraceSphere->SetGenerateOverlapEvents(false);
        if (bEnableReturnTrace)
        {
            MaskReturnTraceSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            MaskReturnTraceSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        }
        else
        {
            MaskReturnTraceSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    if (!HasAuthority())
    {
        if (LocalMoveDriver.IsValid())
        {
            TickMoverFollow(DeltaSeconds);
        }
        return;
    }

    if (CurrentMover && !IsValid(CurrentMover))
    {
        StopMove(nullptr);
    }
    else if (CurrentMover)
    {
        TickMoverFollow(DeltaSeconds);
    }

    if (bTreatmentActive && (!CurrentPatient || IsPatientLeaving(CurrentPatient)))
    {
        CancelTreatment();
    }
}

void AEMROxygenMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMROxygenMachine, CurrentMover);
    DOREPLIFETIME(AEMROxygenMachine, CurrentPatient);
    DOREPLIFETIME(AEMROxygenMachine, bTreatmentActive);
    DOREPLIFETIME(AEMROxygenMachine, bOxygenMachineEnabledByUpgrade);
}

FGameplayEventData AEMROxygenMachine::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    FGameplayEventData EventData;

    if (!bOxygenMachineEnabledByUpgrade)
    {
        return EventData;
    }

    if (CurrentMover && InterActor && CurrentMover != InterActor)
    {
        return EventData;
    }

    EventData.EventTag = EMRTags::Machine::Oxygen;
    EventData.Instigator = InterActor;
    EventData.Target = nullptr;
    EventData.OptionalObject = this;
    return EventData;
}

FText AEMROxygenMachine::GetInteractableDisplayName_Implementation() const
{
    if (InteractableComponent)
    {
        return InteractableComponent->GetDisplayName();
    }

    return FText::FromString(GetName());
}

bool AEMROxygenMachine::IsInteractableEnabled_Implementation() const
{
    if (!bOxygenMachineEnabledByUpgrade)
    {
        return false;
    }

    if (InteractableComponent && !InteractableComponent->IsEnabled())
    {
        return false;
    }

    if (!GetOwnedMaskActor())
    {
        return false;
    }

    if (IsMaskInUse())
    {
        return false;
    }

    return true;
}

void AEMROxygenMachine::ToggleMove(AEMRPlayerCharacter* Player)
{
    if (!Player || !bOxygenMachineEnabledByUpgrade)
    {
        return;
    }

    if (CurrentMover == Player)
    {
        StopMove(Player);
        return;
    }

    TryStartMove(Player);
}

void AEMROxygenMachine::TryStartMove(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player || !bOxygenMachineEnabledByUpgrade)
    {
        return;
    }

    if (!GetOwnedMaskActor())
    {
        return;
    }

    if (CurrentMover && CurrentMover != Player)
    {
        return;
    }

    if (IsMaskInUse() || bTreatmentActive)
    {
        return;
    }

    CurrentMover = Player;
    CachedMoverRelativeOffset = Player->GetTransform().InverseTransformPositionNoScale(GetActorLocation());
    bHasCachedMoverRelativeOffset = true;
    AttachToMover(Player);

    if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
    {
        CachedMoverMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
        MovementComponent->MaxWalkSpeed = MovementComponent->MaxWalkSpeed * MoveSpeedMultiplier;
    }

    Player->Client_BeginOxygenMove(this);
    ForceNetUpdate();
}

void AEMROxygenMachine::StopMove(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority())
    {
        return;
    }

    if (Player && CurrentMover && CurrentMover != Player)
    {
        return;
    }

    AEMRPlayerCharacter* Mover = CurrentMover;
    CurrentMover = nullptr;
    DetachFromMover();

    if (Mover && IsValid(Mover))
    {
        if (UCharacterMovementComponent* MovementComponent = Mover->GetCharacterMovement())
        {
            MovementComponent->MaxWalkSpeed = CachedMoverMaxWalkSpeed > 0.0f
                ? CachedMoverMaxWalkSpeed
                : MovementComponent->MaxWalkSpeed;
        }

        Mover->Client_EndOxygenMove(this);
    }

    CachedMoverMaxWalkSpeed = 0.0f;
    bHasCachedMoverRelativeOffset = false;
    CachedMoverRelativeOffset = FVector::ZeroVector;
    ForceNetUpdate();
}

void AEMROxygenMachine::BeginLocalMoveDriver(AEMRPlayerCharacter* Player)
{
    if (!Player || !Player->IsLocallyControlled())
    {
        return;
    }

    LocalMoveDriver = Player;
    CachedMoverRelativeOffset = Player->GetTransform().InverseTransformPositionNoScale(GetActorLocation());
    bHasCachedMoverRelativeOffset = true;
    AttachToMover(Player);
}

void AEMROxygenMachine::EndLocalMoveDriver(AEMRPlayerCharacter* Player)
{
    if (Player && LocalMoveDriver.Get() != Player)
    {
        return;
    }

    LocalMoveDriver.Reset();
    bHasCachedMoverRelativeOffset = false;
    CachedMoverRelativeOffset = FVector::ZeroVector;
    DetachFromMover();
}

void AEMROxygenMachine::OnRep_CurrentMover()
{
    if (CurrentMover)
    {
        AttachToMover(CurrentMover.Get());
    }
    else
    {
        bHasCachedMoverRelativeOffset = false;
        CachedMoverRelativeOffset = FVector::ZeroVector;
        DetachFromMover();
    }
}

void AEMROxygenMachine::OnRep_ReplicatedMovement()
{
    const bool bIsLocalMoveDriverActive = LocalMoveDriver.IsValid() && LocalMoveDriver->IsLocallyControlled();
    if (bIsLocalMoveDriverActive)
    {
        return;
    }

    Super::OnRep_ReplicatedMovement();
}

void AEMROxygenMachine::OnRep_TreatmentActive()
{
    RefreshTreatmentLoopAudioPlayback();
}

void AEMROxygenMachine::OnRep_OxygenMachineEnabledByUpgrade()
{
    ApplyMachineEnabledState();
    RefreshInteractableState();
}

void AEMROxygenMachine::AttachToMover(AEMRPlayerCharacter* Player)
{
    if (!Player)
    {
        return;
    }

    SetMoverCollisionIgnore(Player, true);
}

void AEMROxygenMachine::DetachFromMover()
{
    SetMoverCollisionIgnore(nullptr, false);

    if (GetAttachParentActor())
    {
        DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    }
}

void AEMROxygenMachine::TickMoverFollow(float DeltaSeconds)
{
    AEMRPlayerCharacter* MoveDriver = HasAuthority()
    ? CurrentMover.Get()
    : LocalMoveDriver.Get();
    if (!MoveDriver || DeltaSeconds <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    SetMoverCollisionIgnore(MoveDriver, true);

    const FVector TargetLocation = ComputeMoverFollowTarget(MoveDriver);
    FVector DesiredDelta = TargetLocation - GetActorLocation();
    if (DesiredDelta.IsNearlyZero(1.0f))
    {
        return;
    }

    if (MoveFollowCatchupSpeed > KINDA_SMALL_NUMBER)
    {
        const float CatchupAlpha = FMath::Clamp(MoveFollowCatchupSpeed * DeltaSeconds, 0.0f, 1.0f);
        DesiredDelta *= CatchupAlpha;
    }

    if (MoveMaxStepPerSecond > KINDA_SMALL_NUMBER)
    {
        const float MaxStepDistance = MoveMaxStepPerSecond * DeltaSeconds;
        const float DesiredDistance = DesiredDelta.Size();
        if (DesiredDistance > MaxStepDistance)
        {
            DesiredDelta = DesiredDelta.GetSafeNormal() * MaxStepDistance;
        }
    }

    ApplySweepMoveWithSlide(DesiredDelta);
}

FVector AEMROxygenMachine::ComputeMoverFollowTarget(const AEMRPlayerCharacter* Player) const
{
    if (!Player)
    {
        return GetActorLocation();
    }

    const FVector RelativeOffset = bHasCachedMoverRelativeOffset
    ? CachedMoverRelativeOffset
    : MoveFollowOffset;
    return Player->GetTransform().TransformPositionNoScale(RelativeOffset);
}

void AEMROxygenMachine::ApplySweepMoveWithSlide(const FVector& DesiredDelta)
{
    UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(GetRootComponent());
    if (!RootPrimitive || DesiredDelta.IsNearlyZero())
    {
        return;
    }

    FHitResult BlockingHit;
    RootPrimitive->MoveComponent(DesiredDelta, GetActorQuat(), true, &BlockingHit, MOVECOMP_NoFlags, ETeleportType::None);
    if (!BlockingHit.IsValidBlockingHit())
    {
        return;
    }

    FVector SlideDelta = FVector::VectorPlaneProject(DesiredDelta * (1.0f - BlockingHit.Time), BlockingHit.Normal);
    if (MoveSlideFriction > 0.0f)
    {
        const float FrictionAlpha = FMath::Clamp(1.0f - MoveSlideFriction, 0.0f, 1.0f);
        SlideDelta *= FrictionAlpha;
    }

    if (SlideDelta.IsNearlyZero())
    {
        return;
    }

    FHitResult SlideHit;
    RootPrimitive->MoveComponent(SlideDelta, GetActorQuat(), true, &SlideHit, MOVECOMP_NoFlags, ETeleportType::None);
}

void AEMROxygenMachine::SetMoverCollisionIgnore(AEMRPlayerCharacter* Player, bool bIgnore)
{
    if (!MachineMesh)
    {
        return;
    }

    if (bIgnore)
    {
        if (LastIgnoredMover.IsValid() && LastIgnoredMover.Get() != Player)
        {
            MachineMesh->IgnoreActorWhenMoving(LastIgnoredMover.Get(), false);
        }

        if (Player)
        {
            MachineMesh->IgnoreActorWhenMoving(Player, true);
            LastIgnoredMover = Player;
        }
        return;
    }

    if (Player)
    {
        MachineMesh->IgnoreActorWhenMoving(Player, false);
    }

    if (LastIgnoredMover.IsValid())
    {
        MachineMesh->IgnoreActorWhenMoving(LastIgnoredMover.Get(), false);
        LastIgnoredMover.Reset();
    }
}

bool AEMROxygenMachine::TryAttachMaskToPatient(AEMRPlayerCharacter* Player, AEMRPatient* Patient, const FHitResult& HitResult)
{
    if (!bOxygenMachineEnabledByUpgrade)
    {
        return false;
    }

    if (!HasAuthority() || !Player || !Patient)
    {
        OXY_LOG("TryAttachMaskToPatient: Invalid authority or actors. HasAuthority=%d Player=%s Patient=%s.",
            HasAuthority(),
            *GetNameSafe(Player),
            *GetNameSafe(Patient));
        return false;
    }

    if (!HitResult.bBlockingHit)
    {
        OXY_LOG("TryAttachMaskToPatient: Missing blocking hit. Player=%s Patient=%s.",
            *GetNameSafe(Player),
            *GetNameSafe(Patient));
        return false;
    }

    AEMROxygenMask* EffectiveMask = GetOwnedMaskActor();
    if (!EffectiveMask || EffectiveMask->IsMaskAttached())
    {
        OXY_LOG("TryAttachMaskToPatient: Mask missing or already attached. Mask=%s Attached=%d.",
            *GetNameSafe(EffectiveMask),
            EffectiveMask ? EffectiveMask->IsMaskAttached() : 0);
        return false;
    }

    if (!EffectiveMask->IsCarriedBy(Player))
    {
        OXY_LOG("TryAttachMaskToPatient: Mask not carried by player. Player=%s Mask=%s AttachParent=%s.",
            *GetNameSafe(Player),
            *GetNameSafe(EffectiveMask),
            EffectiveMask ? *GetNameSafe(EffectiveMask->GetAttachParentActor()) : TEXT("None"));
        return false;
    }

    if (!IsMaskWithinDistanceLimit(EffectiveMask->GetActorLocation()))
    {
        OXY_LOG("TryAttachMaskToPatient: Mask is too far from machine. Machine=%s Mask=%s Limit=%.2f Distance=%.2f.",
            *GetNameSafe(this),
            *GetNameSafe(EffectiveMask),
            MaxMaskDistanceFromMachine,
            FVector::Dist(GetActorLocation(), EffectiveMask->GetActorLocation()));
        return false;
    }

    if (CurrentMover)
    {
        OXY_LOG("TryAttachMaskToPatient: Machine is moving. Mover=%s Player=%s.",
            *GetNameSafe(CurrentMover),
            *GetNameSafe(Player));
        return false;
    }

    if (bTreatmentActive || IsPatientLeaving(Patient))
    {
        OXY_LOG("TryAttachMaskToPatient: Treatment active or patient leaving. Active=%d Leaving=%d Patient=%s.",
            bTreatmentActive,
            IsPatientLeaving(Patient),
            *GetNameSafe(Patient));
        return false;
    }

    if (!DoesPatientNeedOxygen(Patient))
    {
        OXY_LOG("TryAttachMaskToPatient: Patient does not need oxygen. Patient=%s.", *GetNameSafe(Patient));
        PlayBlockedMaskPlacementFeedback(Player);
        return false;
    }

    if (!Patient->CanReceiveTreatment())
    {
        OXY_LOG("TryAttachMaskToPatient: Patient cannot receive treatment. Patient=%s.", *GetNameSafe(Patient));
        return false;
    }

    USkeletalMeshComponent* PatientMesh = ResolvePatientMeshFor(Patient);
    if (!PatientMesh || !PatientMesh->DoesSocketExist(PatientMaskSocketName))
    {
        OXY_LOG("TryAttachMaskToPatient: Patient mesh/socket missing. Patient=%s Mesh=%s Socket=%s.",
            *GetNameSafe(Patient),
            *GetNameSafe(PatientMesh),
            *PatientMaskSocketName.ToString());
        return false;
    }

    if (!IsMaskPlacementValid(PatientMesh, HitResult))
    {
        const FName InteractionSocketName = PatientMaskInteractSocketName.IsNone()
            ? PatientMaskSocketName
            : PatientMaskInteractSocketName;
        const FVector SocketLocation = PatientMesh->GetSocketLocation(InteractionSocketName);
        OXY_LOG("TryAttachMaskToPatient: Placement invalid. Patient=%s Socket=%s SocketLoc=%s HitLoc=%s Tol=%.2f.",
            *GetNameSafe(Patient),
            *InteractionSocketName.ToString(),
            *SocketLocation.ToString(),
            *HitResult.ImpactPoint.ToString(),
            MaskPlacementToleranceRadius);
        return false;
    }

    EffectiveMask->AttachToPatient(PatientMesh, PatientMaskSocketName, MaskAttachOffset);
    CurrentPatient = Patient;
    SetCurrentPatientPatienceDrainSuppressed(true);
    SetTreatmentActive(true);
    StartTreatmentTimer();
    OXY_LOG("TryAttachMaskToPatient: Success. Player=%s Patient=%s Mask=%s.",
        *GetNameSafe(Player),
        *GetNameSafe(Patient),
        *GetNameSafe(EffectiveMask));
    ForceNetUpdate();
    return true;
}

bool AEMROxygenMachine::TryReturnMaskFromTrace(AEMRPlayerCharacter* Player, const FVector& ViewLocation, const FVector& ViewDirection, float TraceDistance)
{
    AEMROxygenMask* EffectiveMask = GetOwnedMaskActor();
    if (!bOxygenMachineEnabledByUpgrade || !HasAuthority() || !Player || !EffectiveMask || !MaskReturnTraceSphere)
    {
        return false;
    }

    if (EffectiveMask->IsMaskAttached() || !EffectiveMask->IsCarriedBy(Player))
    {
        return false;
    }

    const FVector SafeViewDirection = ViewDirection.GetSafeNormal();
    if (SafeViewDirection.IsNearlyZero() || TraceDistance <= 0.0f)
    {
        return false;
    }

    if (MaskReturnTraceSphere->GetScaledSphereRadius() <= 0.0f)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const FVector TraceEnd = ViewLocation + (SafeViewDirection * TraceDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OxygenMaskReturnTrace), false, Player);
    QueryParams.AddIgnoredActor(Player);
    QueryParams.AddIgnoredActor(EffectiveMask);

    TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
    for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
    {
        if (!PrimitiveComponent || PrimitiveComponent == MaskReturnTraceSphere)
        {
            continue;
        }

        QueryParams.AddIgnoredComponent(PrimitiveComponent);
    }

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
    if (!bHit || Hit.GetComponent() != MaskReturnTraceSphere)
    {
        return false;
    }

    EffectiveMask->ReturnToMachine();
    return true;
}

bool AEMROxygenMachine::IsMaskInUse() const
{
    const AEMROxygenMask* EffectiveMask = GetOwnedMaskActor();
    if (!EffectiveMask)
    {
        return false;
    }

    if (EffectiveMask->IsMaskAttached())
    {
        return true;
    }

    if (UEMRCarryableComponent* Carryable = EffectiveMask->GetCarryableComponent())
    {
        if (Carryable->IsCarried())
        {
            return true;
        }
    }

    return EffectiveMask->IsMaskCarried();
}

void AEMROxygenMachine::UpdateMaskCable()
{
    AEMROxygenMask* EffectiveMask = GetOwnedMaskActor();
    if (!bOxygenMachineEnabledByUpgrade || !MaskCable || !EffectiveMask)
    {
        return;
    }

    UStaticMeshComponent* MaskMesh = EffectiveMask->FindComponentByClass<UStaticMeshComponent>();
    if (!MaskMesh)
    {
        return;
    }

    MaskCable->bAttachStart = true;
    MaskCable->bAttachEnd = true;
    if (MaskCable->GetAttachedComponent() != MaskMesh || MaskCable->AttachEndToSocketName != MaskCableSocketName)
    {
        MaskCable->SetAttachEndToComponent(MaskMesh, MaskCableSocketName);
    }
    MaskCable->EndLocation = FVector::ZeroVector;
}

void AEMROxygenMachine::ApplyMachineEnabledState()
{
    SetActorHiddenInGame(!bOxygenMachineEnabledByUpgrade);
    SetActorEnableCollision(bOxygenMachineEnabledByUpgrade);

    if (MachineMesh)
    {
        MachineMesh->SetCollisionEnabled(
            bOxygenMachineEnabledByUpgrade
            ? ECollisionEnabled::QueryAndPhysics
            : ECollisionEnabled::NoCollision);
    }

    if (MaskReturnTraceSphere)
    {
        MaskReturnTraceSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        MaskReturnTraceSphere->SetGenerateOverlapEvents(false);
    }

    AEMROxygenMask* EffectiveMask = GetOwnedMaskActor();
    if (EffectiveMask)
    {
        EffectiveMask->SetActorHiddenInGame(!bOxygenMachineEnabledByUpgrade);
        EffectiveMask->SetActorEnableCollision(bOxygenMachineEnabledByUpgrade);

        if (HasAuthority()
            && !bOxygenMachineEnabledByUpgrade
            && !IsMaskInUse())
        {
            EffectiveMask->AttachToMachineHome();
        }
    }
}

void AEMROxygenMachine::RefreshInteractableState()
{
    const AEMROxygenMask* EffectiveMask = GetOwnedMaskActor();
    if (InteractableComponent)
    {
        const bool bEnabled = bOxygenMachineEnabledByUpgrade && EffectiveMask && !IsMaskInUse();
        InteractableComponent->SetEnabled(bEnabled);
    }
}

void AEMROxygenMachine::SetOxygenMachineEnabledByUpgrade(const bool bEnabled)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bOxygenMachineEnabledByUpgrade == bEnabled)
    {
        AEMROxygenMask* EffectiveMask = GetOwnedMaskActor();
        if (bOxygenMachineEnabledByUpgrade
            && EffectiveMask
            && !IsMaskInUse())
        {
            EffectiveMask->AttachToMachineHome();
        }

        ApplyMachineEnabledState();
        RefreshInteractableState();
        return;
    }

    bOxygenMachineEnabledByUpgrade = bEnabled;
    if (bOxygenMachineEnabledByUpgrade)
    {
        if (AEMROxygenMask* EffectiveMask = GetOwnedMaskActor(); EffectiveMask && !IsMaskInUse())
        {
            EffectiveMask->AttachToMachineHome();
        }
    }
    else
    {
        StopMove(nullptr);
        CancelTreatment();
    }

    OnRep_OxygenMachineEnabledByUpgrade();
    ForceNetUpdate();
}

void AEMROxygenMachine::StartTreatmentTimer()
{
    if (!HasAuthority() || !CurrentPatient)
    {
        return;
    }

    float DelaySeconds = 0.0f;
    if (const AEMRNightShiftGameState* NightShiftState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr)
    {
        if (const UEMRDifficultyTuningData* Tuning = NightShiftState->GetDifficultyTuning())
        {
            const UEMRNightShiftDefinition* Definition = NightShiftState->GetCurrentNightShiftDefinition();
            const EEMRNightShiftDifficultyTier Difficulty = Definition ? Definition->DifficultyTier : EEMRNightShiftDifficultyTier::Calm;
            DelaySeconds = Tuning->GetOxygenTreatmentCompletionDelay(Difficulty);
        }
    }

    ClearTreatmentTimer();

    if (DelaySeconds <= 0.0f)
    {
        HandleTreatmentComplete();
        return;
    }

    GetWorldTimerManager().SetTimer(
        TreatmentTimerHandle,
        this,
        &ThisClass::HandleTreatmentComplete,
        DelaySeconds,
        false);
}

void AEMROxygenMachine::ClearTreatmentTimer()
{
    if (!HasAuthority())
    {
        return;
    }

    if (TreatmentTimerHandle.IsValid())
    {
        GetWorldTimerManager().ClearTimer(TreatmentTimerHandle);
    }
}

void AEMROxygenMachine::HandleTreatmentComplete()
{
    if (!HasAuthority() || !bTreatmentActive || !CurrentPatient)
    {
        return;
    }

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

    TreatmentSubsystem->CompleteTreatmentForPatient(CurrentPatient, EMRTags::Abilities::Treatment::Oxygen, true);

    Multicast_PlayTreatmentCompletedSound();
    SetCurrentPatientPatienceDrainSuppressed(false);
    SetTreatmentActive(false);
    CurrentPatient = nullptr;
    ClearTreatmentTimer();

    if (AEMROxygenMask* EffectiveMask = GetOwnedMaskActor())
    {
        EffectiveMask->ReturnToMachine();
    }

    ForceNetUpdate();
}

void AEMROxygenMachine::PlayBlockedMaskPlacementFeedback(AEMRPlayerCharacter* Player) const
{
    if (!Player || !MaskPlacementBlockedSound)
    {
        return;
    }

    Player->PlayWorldSoundForAllPlayers(MaskPlacementBlockedSound, GetActorLocation());
}

void AEMROxygenMachine::CancelTreatment()
{
    if (!HasAuthority())
    {
        return;
    }

    ClearPatienceDrainSuppression();

    SetTreatmentActive(false);
    CurrentPatient = nullptr;
    ClearTreatmentTimer();

    if (AEMROxygenMask* EffectiveMask = GetOwnedMaskActor())
    {
        EffectiveMask->ReturnToMachine();
    }

    ForceNetUpdate();
}

void AEMROxygenMachine::SetCurrentPatientPatienceDrainSuppressed(const bool bSuppressed)
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
        PatienceDrainSuppressedPatient->SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::OxygenTreatment, false);
        PatienceDrainSuppressedPatient.Reset();
    }

    Patient->SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::OxygenTreatment, true);
    PatienceDrainSuppressedPatient = Patient;
}

void AEMROxygenMachine::ClearPatienceDrainSuppression()
{
    if (!HasAuthority())
    {
        return;
    }

    if (PatienceDrainSuppressedPatient.IsValid())
    {
        PatienceDrainSuppressedPatient->SetPatienceDrainSuppressedBySource(EEMRPatienceDrainSuppressor::OxygenTreatment, false);
    }

    PatienceDrainSuppressedPatient.Reset();
}

void AEMROxygenMachine::SetTreatmentActive(bool bNewActive)
{
    if (bTreatmentActive == bNewActive)
    {
        return;
    }

    bTreatmentActive = bNewActive;
    RefreshTreatmentLoopAudioPlayback();

    if (HasAuthority())
    {
        ForceNetUpdate();
    }
}

void AEMROxygenMachine::RefreshTreatmentLoopAudioPlayback()
{
    if (GetNetMode() == NM_DedicatedServer || !TreatmentLoopAudioComponent)
    {
        return;
    }

    TreatmentLoopAudioComponent->SetSound(TreatmentLoopSound);

    if (bTreatmentActive && TreatmentLoopSound)
    {
        TreatmentLoopAudioComponent->FadeIn(TreatmentLoopFadeInSeconds, 1.0f, 0.0f);
    }
    else if (TreatmentLoopAudioComponent->IsPlaying())
    {
        TreatmentLoopAudioComponent->FadeOut(TreatmentLoopFadeOutSeconds, 0.0f);
    }
}

void AEMROxygenMachine::Multicast_PlayTreatmentCompletedSound_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer || !TreatmentCompletedSound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, TreatmentCompletedSound, GetActorLocation());
}

bool AEMROxygenMachine::IsMaskPlacementValid(const USkeletalMeshComponent* PatientMesh, const FHitResult& HitResult) const
{
    if (!PatientMesh || PatientMaskSocketName.IsNone())
    {
        return false;
    }

    const FName InteractionSocketName = PatientMaskInteractSocketName.IsNone()
    ? PatientMaskSocketName
    : PatientMaskInteractSocketName;

    if (!PatientMesh->DoesSocketExist(InteractionSocketName))
    {
        return false;
    }

    const FVector SocketLocation = PatientMesh->GetSocketLocation(InteractionSocketName);
    const FVector HitLocation = HitResult.ImpactPoint;
    const float Distance = FVector::Dist(SocketLocation, HitLocation);
    return Distance <= MaskPlacementToleranceRadius;
}

bool AEMROxygenMachine::IsMaskWithinDistanceLimit(const FVector& MaskWorldLocation) const
{
    if (MaxMaskDistanceFromMachine <= 0.0f)
    {
        return true;
    }

    const float DistanceSq = FVector::DistSquared(GetActorLocation(), MaskWorldLocation);
    return DistanceSq <= FMath::Square(MaxMaskDistanceFromMachine);
}

bool AEMROxygenMachine::IsMaskPlacementSocketAimed(const AEMRPatient* Patient, const FHitResult& HitResult) const
{
    if (!Patient || !HitResult.bBlockingHit)
    {
        return false;
    }

    USkeletalMeshComponent* PatientMesh = ResolvePatientMeshFor(Patient);
    if (!PatientMesh || !PatientMesh->DoesSocketExist(PatientMaskSocketName))
    {
        return false;
    }

    return IsMaskPlacementValid(PatientMesh, HitResult);
}

bool AEMROxygenMachine::DoesPatientNeedOxygen(const AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return false;
    }

    if (const UAbilitySystemComponent* ASC = Patient->GetAbilitySystemComponent())
    {
        return ASC->HasMatchingGameplayTag(EMRTags::Patient::TreatmentQueue::WaitForOxygenTreatment);
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
    return NeededTreatments.HasTagExact(EMRTags::Abilities::Treatment::Oxygen);
}

bool AEMROxygenMachine::IsPatientLeaving(const AEMRPatient* Patient) const
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

USkeletalMeshComponent* AEMROxygenMachine::ResolvePatientMeshFor(const AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return nullptr;
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
                return MeshComponent;
            }
        }
    }

    return Patient->GetMesh();
}

#undef OXY_LOG
