#include "Interaction/EMRLabAnalyzerMachine.h"

#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "EMRLabAnalyzerLog.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Interaction/EMRLabAnalyzerLid.h"
#include "Interaction/EMRLabAnalyzerTube.h"
#include "Interaction/EMRLabAnalyzerTubeSpawnPoint.h"
#include "Interfaces/EMRSeatedAnimationInterface.h"
#include "LOD/EMRCollisionChannels.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/EMRExamQueueSubsystem.h"
#include "UI/Machines/LabAnalyzer/EMRLabAnalyzerValidationErrorWidget.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

AEMRLabAnalyzerMachine::AEMRLabAnalyzerMachine()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(false);
    bReplicates = true;

    MachineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MachineMesh"));
    SetRootComponent(MachineMesh);
    MachineMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MachineMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    MachineMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    CentrifugeRotorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CentrifugeRotorMesh"));
    CentrifugeRotorMesh->SetupAttachment(GetRootComponent());
    CentrifugeRotorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CentrifugeRotorMesh->SetMobility(EComponentMobility::Movable);

    LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
    LidMesh->SetupAttachment(GetRootComponent());
    LidMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    LidMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    LidMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    LidMesh->SetMobility(EComponentMobility::Movable);

    StartButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartButtonMesh"));
    StartButtonMesh->SetupAttachment(GetRootComponent());
    StartButtonMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    StartButtonMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    StartButtonMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    StartButtonMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
    StartButtonMesh->SetMobility(EComponentMobility::Movable);

    SeatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SeatMesh"));
    SeatMesh->SetupAttachment(GetRootComponent());
    SeatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SeatMesh->SetMobility(EComponentMobility::Movable);

    OperatorLockPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("OperatorLockPoint"));
    OperatorLockPoint->SetupAttachment(GetRootComponent());
    OperatorLockPoint->SetArrowColor(FLinearColor::Blue);
    OperatorLockPoint->bIsScreenSizeScaled = true;

    OperatorExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("OperatorExitPoint"));
    OperatorExitPoint->SetupAttachment(GetRootComponent());
    OperatorExitPoint->SetArrowColor(FLinearColor::Yellow);
    OperatorExitPoint->bIsScreenSizeScaled = true;

    SeatPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SeatPoint"));
    SeatPoint->SetupAttachment(SeatMesh);
    SeatPoint->SetArrowColor(FLinearColor::Blue);
    SeatPoint->bIsScreenSizeScaled = true;

    SeatExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SeatExitPoint"));
    SeatExitPoint->SetupAttachment(GetRootComponent());
    SeatExitPoint->SetArrowColor(FLinearColor::Yellow);
    SeatExitPoint->bIsScreenSizeScaled = true;

    BalanceTubeSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BalanceTubeSpawnPoint"));
    BalanceTubeSpawnPoint->SetupAttachment(GetRootComponent());

    StartValidationErrorWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StartValidationErrorWidget"));
    StartValidationErrorWidgetComponent->SetupAttachment(GetRootComponent());
    StartValidationErrorWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    StartValidationErrorWidgetComponent->SetDrawAtDesiredSize(true);
    StartValidationErrorWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StartValidationErrorWidgetComponent->SetGenerateOverlapEvents(false);
    StartValidationErrorWidgetComponent->SetWindowFocusable(false);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
    if (InteractableComponent)
    {
        InteractableComponent->SetDisplayName(FText::FromString(TEXT("Lab Analyzer")));
        InteractableComponent->SetInteractionEventTag(EMRTags::Machine::LabAnalyzer);
    }
}

void AEMRLabAnalyzerMachine::BeginPlay()
{
    Super::BeginPlay();

    EMR_LAB_LOG(Log, "Machine BeginPlay %s", *GetNameSafe(this));
    ApplyLidRotation();
    ApplyTestButtonColors();
    UpdateTickEnabled();

    if (InteractableComponent)
    {
        InteractableComponent->SetInteractionEventTag(EMRTags::Machine::LabAnalyzer);
    }

    if (!LidActor)
    {
        TArray<AActor*> AttachedActors;
        GetAttachedActors(AttachedActors);
        for (AActor* AttachedActor : AttachedActors)
        {
            if (AEMRLabAnalyzerLid* AttachedLidActor = Cast<AEMRLabAnalyzerLid>(AttachedActor))
            {
                LidActor = AttachedLidActor;
                break;
            }
        }
    }

    if (LidActor)
    {
        LidActor->SetOwningMachine(this);

        if (LidMesh)
        {
            LidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    if (SeatMesh)
    {
        SeatMesh->SetMobility(EComponentMobility::Movable);
        SeatMesh->SetUsingAbsoluteLocation(false);
        SeatMesh->SetUsingAbsoluteRotation(false);
        SeatMesh->SetUsingAbsoluteScale(false);
    }

    ApplyTestButtonColors();

    CacheTubeSlotAnchors();
    if (TubeSlots.IsEmpty() && CachedTubeSlotAnchors.Num() > 0)
    {
        const int32 SlotCount = FMath::Min(CachedTubeSlotAnchors.Num(), 14);
        TubeSlots.SetNum(SlotCount);
    }

    EMR_LAB_LOG(Log, "Machine slot anchors cached=%d slots=%d", CachedTubeSlotAnchors.Num(), TubeSlots.Num());

    CacheTubeSpawnAnchors();

    if (UEMRExamQueueSubsystem* QueueSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>())
    {
        QueueSubsystem->OnPatientQueuedForExam.AddDynamic(this, &ThisClass::HandlePatientQueuedForExam);
    }

    if (GetNetMode() != NM_DedicatedServer)
    {
        RefreshStartValidationErrorWidget();
    }

    CacheSpawnPoint();
    SpawnBalanceTube();
}

void AEMRLabAnalyzerMachine::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyTestButtonColors();
}

void AEMRLabAnalyzerMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRLabAnalyzerMachine, bLidClosed);
    DOREPLIFETIME(AEMRLabAnalyzerMachine, bIsSpinning);
    DOREPLIFETIME(AEMRLabAnalyzerMachine, SelectedTests);
    DOREPLIFETIME(AEMRLabAnalyzerMachine, CurrentOperator);
    DOREPLIFETIME(AEMRLabAnalyzerMachine, SeatMeshYawOffset);
    DOREPLIFETIME(AEMRLabAnalyzerMachine, StartValidationError);
    DOREPLIFETIME(AEMRLabAnalyzerMachine, StartValidationFeedbackRevision);
}

FGameplayEventData AEMRLabAnalyzerMachine::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    FGameplayEventData EventData;
    EventData.EventTag = EMRTags::Machine::LabAnalyzer;
    EventData.Instigator = InterActor;
    EventData.Target = const_cast<AEMRLabAnalyzerMachine*>(this);
    EventData.OptionalObject = const_cast<AEMRLabAnalyzerMachine*>(this);
    return EventData;
}

FText AEMRLabAnalyzerMachine::GetInteractableDisplayName_Implementation() const
{
    if (InteractableComponent)
    {
        return InteractableComponent->GetDisplayName();
    }

    return FText::FromString(GetName());
}

bool AEMRLabAnalyzerMachine::IsInteractableEnabled_Implementation() const
{
    return InteractableComponent ? InteractableComponent->IsEnabled() : true;
}

void AEMRLabAnalyzerMachine::TryStartExam(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    if (CurrentOperator && CurrentOperator != Player)
    {
        EMR_LAB_LOG(Log, "StartExam rejected: operator already %s", *GetNameSafe(CurrentOperator));
        return;
    }

    CurrentOperator = Player;
    EMR_LAB_LOG(Log, "StartExam operator=%s machine=%s", *GetNameSafe(Player), *GetNameSafe(this));
    SeatOperator(Player);
    UpdateTubeInteractionLocks();
    Player->Client_BeginLabAnalyzerExam(this, ExamInputMappingContext);
}

void AEMRLabAnalyzerMachine::StopExam(AEMRPlayerCharacter* Player, bool bFromCancel)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    if (CurrentOperator != Player)
    {
        return;
    }

    EMR_LAB_LOG(Log, "StopExam operator=%s cancel=%s", *GetNameSafe(Player), bFromCancel ? TEXT("true") : TEXT("false"));
    Player->Client_EndLabAnalyzerExam(this);
    UnseatOperator(Player);
    CurrentOperator = nullptr;
    UpdateTubeInteractionLocks();
}

void AEMRLabAnalyzerMachine::HandleStartButtonPressed(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    if (CurrentOperator != Player)
    {
        return;
    }

    // Requirement: clear any previous Start validation message on each new click.
    SetStartValidationError(EEMRLabAnalyzerStartValidationError::None);

    if (bIsSpinning || !bLidClosed)
    {
        EMR_LAB_LOG(Log, "StartButton ignored: spinning=%s lidClosed=%s", bIsSpinning ? TEXT("true") : TEXT("false"), bLidClosed ? TEXT("true") : TEXT("false"));
        if (!bLidClosed)
        {
            SetStartValidationError(EEMRLabAnalyzerStartValidationError::LidOpen);
        }
        PlayCueForNearbyPlayers(StartRejectedSound);
        return;
    }

    bool bHasPatientTube = false;
    const bool bBalanced = IsCentrifugeBalanced(bHasPatientTube);
    if (!bBalanced || !bHasPatientTube)
    {
        EMR_LAB_LOG(Log, "StartButton unbalanced: balanced=%s hasPatientTube=%s totalInserted=%d",
            bBalanced ? TEXT("true") : TEXT("false"),
            bHasPatientTube ? TEXT("true") : TEXT("false"),
            InsertedTubes.Num());
        SetStartValidationError(EEMRLabAnalyzerStartValidationError::Unbalanced);
        PlayCueForNearbyPlayers(StartRejectedSound);
        return;
    }

    EMR_LAB_LOG(Log, "StartButton accepted");
    CurrentOperator = Player;
    bIsSpinning = true;
    OnRep_SpinState();
    UpdateTubeInteractionLocks();
    ForceNetUpdate();
    PlayCueForNearbyPlayers(StartAcceptedSound);
    PlayCueForNearbyPlayers(RunningSound);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SpinTimerHandle, this, &ThisClass::HandleSpinCompleted, SpinDuration, false);
    }
}

void AEMRLabAnalyzerMachine::HandleLidTogglePressed(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    if (CurrentOperator != Player)
    {
        return;
    }

    if (bIsSpinning)
    {
        EMR_LAB_LOG(Log, "LidToggle ignored: spinning");
        return;
    }

    bLidClosed = !bLidClosed;
    EMR_LAB_LOG(Log, "LidToggle nowClosed=%s", bLidClosed ? TEXT("true") : TEXT("false"));
    OnRep_LidState();
    PlayCueForNearbyPlayers(bLidClosed ? LidCloseSound : LidOpenSound);
    UpdateTubeInteractionLocks();
    ForceNetUpdate();
}

bool AEMRLabAnalyzerMachine::TryInsertTube(AEMRLabAnalyzerTube* Tube)
{
    if (!HasAuthority() || !Tube)
    {
        return false;
    }

    if (bIsSpinning || bLidClosed)
    {
        EMR_LAB_LOG(Log, "InsertTube rejected: spinning=%s lidClosed=%s tube=%s", bIsSpinning ? TEXT("true") : TEXT("false"),
            bLidClosed ? TEXT("true") : TEXT("false"), *GetNameSafe(Tube));
        return false;
    }

    if (InsertedTubes.Contains(Tube))
    {
        EMR_LAB_LOG(Log, "InsertTube rejected: already inserted tube=%s", *GetNameSafe(Tube));
        return false;
    }

    if (CachedTubeSlotAnchors.IsEmpty() && TubeSlotAnchors.Num() > 0)
    {
        CacheTubeSlotAnchors();
    }

    if (TubeSlots.IsEmpty() && CachedTubeSlotAnchors.Num() > 0)
    {
        const int32 SlotCount = FMath::Min(CachedTubeSlotAnchors.Num(), 14);
        TubeSlots.SetNum(SlotCount);
    }

    const int32 SlotIndex = FindAvailableSlotIndex();
    if (SlotIndex == INDEX_NONE)
    {
        EMR_LAB_LOG(Log, "InsertTube rejected: no free slots tube=%s", *GetNameSafe(Tube));
        return false;
    }

    AssignTubeToSlot(Tube, SlotIndex);
    EMR_LAB_LOG(Log, "InsertTube ok: tube=%s slot=%d", *GetNameSafe(Tube), SlotIndex);
    return true;
}

bool AEMRLabAnalyzerMachine::TryTrashTube(AEMRLabAnalyzerTube* Tube, AEMRPlayerCharacter* Player)
{
    (void)Player;
    if (!HasAuthority() || !Tube)
    {
        return false;
    }

    if (!Tube->CanBeTrashed())
    {
        EMR_LAB_LOG(Log, "TrashTube rejected: tube=%s", *GetNameSafe(Tube));
        return false;
    }

    RemoveTubeFromSlot(Tube);
    EMR_LAB_LOG(Log, "TrashTube ok: tube=%s", *GetNameSafe(Tube));
    Tube->Destroy();
    return true;
}

void AEMRLabAnalyzerMachine::PlayTrashAttemptCue(AEMRPlayerCharacter* Player, bool bBalanceTubeAttempt)
{
    USoundBase* Sound = bBalanceTubeAttempt ? TrashBalanceTubeRejectedSound : TrashPatientTubeSound;
    EMR_LAB_LOG(Log, "PlayTrashAttemptCue player=%s balanceAttempt=%s sound=%s",
        *GetNameSafe(Player),
        bBalanceTubeAttempt ? TEXT("true") : TEXT("false"),
        *GetNameSafe(Sound));
    PlayCue(Player, Sound);
}

bool AEMRLabAnalyzerMachine::IsStartButtonComponent(const UPrimitiveComponent* Component) const
{
    return Component && StartButtonMesh && Component == StartButtonMesh;
}

bool AEMRLabAnalyzerMachine::IsLidComponent(const UPrimitiveComponent* Component) const
{
    if (Component && LidMesh && Component == LidMesh)
    {
        return true;
    }

    return LidActor && LidActor->IsLidComponent(Component);
}

void AEMRLabAnalyzerMachine::HandleTubePickedUp(AEMRLabAnalyzerTube* Tube)
{
    if (!HasAuthority() || !Tube)
    {
        return;
    }

    EMR_LAB_LOG(Log, "TubePickedUp: tube=%s", *GetNameSafe(Tube));
    RemoveTubeFromSlot(Tube);
}

void AEMRLabAnalyzerMachine::OnRep_LidState()
{
    HandleLidStateChanged(bLidClosed);
    ApplyLidRotation();
}

void AEMRLabAnalyzerMachine::OnRep_SpinState()
{
    HandleSpinStateChanged(bIsSpinning);
    UpdateTickEnabled();

    EMR_LAB_LOG(Log, "[LabTubeRotate] SpinState changed spinning=%s rotor=%s anchors=%d",
        bIsSpinning ? TEXT("true") : TEXT("false"),
        *GetNameSafe(CentrifugeRotorMesh),
        CachedTubeSlotAnchors.Num());
}

void AEMRLabAnalyzerMachine::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateSeatMeshYawFromPlayer(DeltaSeconds);

    if (bIsLidAnimating)
    {
        UpdateLidRotation(DeltaSeconds);
    }

    if (bIsSpinning)
    {
        RotateRotor(DeltaSeconds);
    }
}

void AEMRLabAnalyzerMachine::OnRep_SelectedTests()
{
    HandleSelectedTestsChanged();
}

void AEMRLabAnalyzerMachine::OnRep_CurrentOperator()
{
    if (CurrentOperator)
    {
        AttachSeatMeshToPlayer(CurrentOperator);
    }
    else
    {
        DetachSeatMeshToOriginal();
    }
}

void AEMRLabAnalyzerMachine::OnRep_SeatMeshYawOffset()
{
    if (!bSeatMeshYawInitialized)
    {
        SeatMeshYawOffsetSmoothed = SeatMeshYawOffset;
        bSeatMeshYawInitialized = true;
    }
    ApplySeatMeshYawOffset();
}

void AEMRLabAnalyzerMachine::OnRep_StartValidationFeedback()
{
    RefreshStartValidationErrorWidget();
}

void AEMRLabAnalyzerMachine::HandlePatientQueuedForExam(AEMRPatient* Patient, FGameplayTag ExamTag)
{
    if (!HasAuthority() || !Patient)
    {
        return;
    }

    if (!ExamTag.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root))
    {
        return;
    }

    EMR_LAB_LOG(Log, "PatientQueued: patient=%s exam=%s", *GetNameSafe(Patient), *ExamTag.ToString());
    if (const TObjectPtr<AEMRLabAnalyzerTube>* ExistingTube = SpawnedTubesByPatient.Find(Patient))
    {
        if (IsValid(*ExistingTube))
        {
            return;
        }

        SpawnedTubesByPatient.Remove(Patient);
    }

    if (!PatientTubeClass)
    {
        EMR_LAB_LOG(Warning, "PatientTubeClass missing on %s", *GetNameSafe(this));
        return;
    }

    const FGameplayTagContainer RequiredTests = GetLabTestsForPatient(Patient, ExamTag);
    if (RequiredTests.IsEmpty())
    {
        EMR_LAB_LOG(Log, "PatientQueued: no required tests for patient=%s", *GetNameSafe(Patient));
        return;
    }

    USceneComponent* SpawnAnchor = nullptr;
    const FTransform SpawnTransform = GetSpawnPointTransform(SpawnAnchor);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEMRLabAnalyzerTube* Tube = GetWorld()->SpawnActor<AEMRLabAnalyzerTube>(PatientTubeClass, SpawnTransform, SpawnParams);
    if (!Tube)
    {
        EMR_LAB_LOG(Warning, "Tube spawn failed for patient=%s", *GetNameSafe(Patient));
        return;
    }

    Tube->InitializeTube(Patient, RequiredTests);
    Tube->SetInteractionLocked(bLidClosed || bIsSpinning || !CurrentOperator);
    if (SpawnAnchor)
    {
        Tube->SetStoredInRack(true, SpawnAnchor);
    }
    EMR_LAB_LOG(Log, "Tube spawned=%s patient=%s tests=%s", *GetNameSafe(Tube), *GetNameSafe(Patient), *RequiredTests.ToStringSimple());
    Tube->OnDestroyed.AddDynamic(this, &ThisClass::HandleSpawnedTubeDestroyed);
    SpawnedTubesByPatient.Add(Patient, Tube);
}

void AEMRLabAnalyzerMachine::HandleSpawnedTubeDestroyed(AActor* DestroyedActor)
{
    AEMRLabAnalyzerTube* Tube = Cast<AEMRLabAnalyzerTube>(DestroyedActor);
    if (!Tube)
    {
        return;
    }

    EMR_LAB_LOG(Log, "Tube destroyed: %s", *GetNameSafe(Tube));
    for (auto It = SpawnedTubesByPatient.CreateIterator(); It; ++It)
    {
        if (It.Value() == Tube)
        {
            It.RemoveCurrent();
        }
    }

    RemoveTubeFromSlot(Tube);
}

void AEMRLabAnalyzerMachine::CacheSpawnPoint()
{
    if (CachedSpawnPoint.IsValid())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    float BestDistanceSq = MAX_flt;
    AEMRLabAnalyzerTubeSpawnPoint* BestPoint = nullptr;

    for (TActorIterator<AEMRLabAnalyzerTubeSpawnPoint> It(World); It; ++It)
    {
        AEMRLabAnalyzerTubeSpawnPoint* SpawnPoint = *It;
        if (!SpawnPoint)
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(SpawnPoint->GetActorLocation(), GetActorLocation());
        if (DistSq < BestDistanceSq)
        {
            BestDistanceSq = DistSq;
            BestPoint = SpawnPoint;
        }
    }

    if (BestPoint)
    {
        CachedSpawnPoint = BestPoint;
        EMR_LAB_LOG(Log, "SpawnPoint cached: %s", *GetNameSafe(BestPoint));
    }
    else
    {
        EMR_LAB_LOG(Log, "SpawnPoint not found, using machine transform");
    }
}

void AEMRLabAnalyzerMachine::CacheTubeSpawnAnchors()
{
    CachedTubeSpawnAnchors.Reset();

    for (const FComponentReference& AnchorRef : TubeSpawnAnchors)
    {
        UActorComponent* Component = AnchorRef.GetComponent(this);
        USceneComponent* SceneComponent = Cast<USceneComponent>(Component);
        if (SceneComponent)
        {
            CachedTubeSpawnAnchors.Add(SceneComponent);
        }
    }

    EMR_LAB_LOG(Log, "Cached %d tube spawn anchors", CachedTubeSpawnAnchors.Num());
    for (int32 Index = 0; Index < CachedTubeSpawnAnchors.Num(); ++Index)
    {
        if (USceneComponent* Anchor = CachedTubeSpawnAnchors[Index].Get())
        {
            EMR_LAB_LOG(Log, "SpawnAnchor[%d]=%s loc=%s", Index, *GetNameSafe(Anchor), *Anchor->GetComponentLocation().ToString());
        }
    }
}

void AEMRLabAnalyzerMachine::CacheTubeSlotAnchors()
{
    CachedTubeSlotAnchors.Reset();

    for (const FComponentReference& AnchorRef : TubeSlotAnchors)
    {
        UActorComponent* Component = AnchorRef.GetComponent(this);
        USceneComponent* SceneComponent = Cast<USceneComponent>(Component);
        if (SceneComponent)
        {
            if (CentrifugeRotorMesh && SceneComponent->GetAttachParent() != CentrifugeRotorMesh)
            {
                const bool bAttached = SceneComponent->AttachToComponent(CentrifugeRotorMesh, FAttachmentTransformRules::KeepWorldTransform);
                EMR_LAB_LOG(Log, "[LabTubeRotate] Anchor attach request anchor=%s rotor=%s result=%s",
                    *GetNameSafe(SceneComponent),
                    *GetNameSafe(CentrifugeRotorMesh),
                    bAttached ? TEXT("true") : TEXT("false"));
            }
            CachedTubeSlotAnchors.Add(SceneComponent);
        }
    }

    EMR_LAB_LOG(Log, "Cached %d tube slot anchors", CachedTubeSlotAnchors.Num());
    for (int32 Index = 0; Index < CachedTubeSlotAnchors.Num(); ++Index)
    {
        if (USceneComponent* Anchor = CachedTubeSlotAnchors[Index].Get())
        {
            EMR_LAB_LOG(Log, "[LabTubeRotate] SlotAnchor[%d]=%s parent=%s loc=%s",
                Index,
                *GetNameSafe(Anchor),
                *GetNameSafe(Anchor->GetAttachParent()),
                *Anchor->GetComponentLocation().ToString());
            EMR_LAB_LOG(Log, "[LabTubeRotate] SlotAnchor[%d] absLoc=%d absRot=%d absScale=%d",
                Index,
                Anchor->IsUsingAbsoluteLocation() ? 1 : 0,
                Anchor->IsUsingAbsoluteRotation() ? 1 : 0,
                Anchor->IsUsingAbsoluteScale() ? 1 : 0);
        }
    }
}

USceneComponent* AEMRLabAnalyzerMachine::GetTubeSlotAnchor(int32 Index) const
{
    if (CachedTubeSlotAnchors.IsValidIndex(Index))
    {
        return CachedTubeSlotAnchors[Index].Get();
    }

    return nullptr;
}

FTransform AEMRLabAnalyzerMachine::GetSpawnPointTransform(USceneComponent*& OutAnchor) const
{
    OutAnchor = nullptr;

    if (CachedTubeSpawnAnchors.IsEmpty() && TubeSpawnAnchors.Num() > 0)
    {
        const_cast<AEMRLabAnalyzerMachine*>(this)->CacheTubeSpawnAnchors();
    }

    if (!CachedTubeSpawnAnchors.IsEmpty())
    {
        UWorld* World = GetWorld();
        const float RadiusSq = FMath::Square(FMath::Max(0.0f, TubeSpawnOccupancyRadius));
        for (const TWeakObjectPtr<USceneComponent>& AnchorPtr : CachedTubeSpawnAnchors)
        {
            USceneComponent* Anchor = AnchorPtr.Get();
            if (!Anchor)
            {
                continue;
            }

            bool bOccupied = false;
            if (World && RadiusSq > 0.0f)
            {
                for (TActorIterator<AEMRLabAnalyzerTube> It(World); It; ++It)
                {
                    const AEMRLabAnalyzerTube* Tube = *It;
                    if (!Tube)
                    {
                        continue;
                    }

                    if (Tube->IsStoredInRack())
                    {
                        const USceneComponent* TubeRoot = Tube->GetRootComponent();
                        if (TubeRoot && TubeRoot->GetAttachParent() == Anchor)
                        {
                            bOccupied = true;
                            break;
                        }
                    }
                    else if (FVector::DistSquared(Tube->GetActorLocation(), Anchor->GetComponentLocation()) <= RadiusSq)
                    {
                        bOccupied = true;
                        break;
                    }
                }
            }

            if (!bOccupied)
            {
                OutAnchor = Anchor;
                return Anchor->GetComponentTransform();
            }
        }

        const FTransform MachineTransform = GetActorTransform();
        const FVector Offset = (MachineTransform.GetRotation().GetForwardVector() * TubeSpawnFallbackOffset.X)
            + (MachineTransform.GetRotation().GetRightVector() * TubeSpawnFallbackOffset.Y)
            + (MachineTransform.GetRotation().GetUpVector() * TubeSpawnFallbackOffset.Z);
        FTransform Fallback = MachineTransform;
        Fallback.AddToTranslation(Offset);
        return Fallback;
    }

    if (CachedSpawnPoint.IsValid())
    {
        return CachedSpawnPoint->GetActorTransform();
    }

    return GetActorTransform();
}

void AEMRLabAnalyzerMachine::SpawnBalanceTube()
{
    if (!HasAuthority() || BalanceTubeClass == nullptr)
    {
        return;
    }

    if (BalanceTube.IsValid())
    {
        return;
    }

    const USceneComponent* SpawnRoot = BalanceTubeSpawnPoint;
    const FTransform SpawnTransform = SpawnRoot ? SpawnRoot->GetComponentTransform() : GetActorTransform();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEMRLabAnalyzerTube* Tube = GetWorld()->SpawnActor<AEMRLabAnalyzerTube>(BalanceTubeClass, SpawnTransform, SpawnParams);
    if (!Tube)
    {
        EMR_LAB_LOG(Warning, "Balance tube spawn failed");
        return;
    }

    Tube->SetBalanceTube(true);
    Tube->SetInteractionLocked(bLidClosed || bIsSpinning || !CurrentOperator);
    BalanceTube = Tube;
    EMR_LAB_LOG(Log, "Balance tube spawned: %s", *GetNameSafe(Tube));
}

void AEMRLabAnalyzerMachine::HandleSpinCompleted()
{
    if (!HasAuthority())
    {
        return;
    }

    bIsSpinning = false;
    OnRep_SpinState();

    UEMRExamQueueSubsystem* QueueSubsystem = nullptr;
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            QueueSubsystem = GameInstance->GetSubsystem<UEMRExamQueueSubsystem>();
        }
    }

    EMR_LAB_LOG(Log, "SpinCompleted tubes=%d", InsertedTubes.Num());
    for (AEMRLabAnalyzerTube* Tube : InsertedTubes)
    {
        if (!Tube || Tube->IsBalanceTube())
        {
            continue;
        }

        FGameplayTagContainer CompletionExamTags = Tube->GetRequiredTests();
        if (CompletionExamTags.IsEmpty())
        {
            CompletionExamTags.AddTag(EMRTags::Abilities::Exam::LabAnalyzer::Root);
        }

        Tube->MarkTestsCompleted(CompletionExamTags);

        if (!QueueSubsystem || Tube->HasTriggeredExamCompletion() || Tube->IsAbandoned())
        {
            continue;
        }

        AEMRPatient* Patient = Tube->GetOwningPatient();
        if (!Patient)
        {
            continue;
        }

        for (const FGameplayTag& TestTag : CompletionExamTags)
        {
            if (!TestTag.IsValid())
            {
                continue;
            }

            QueueSubsystem->CompleteExamForPatient(Patient, TestTag, true);
        }

        Tube->MarkExamCompletionTriggered();
    }

    SelectedTests.Reset();
    OnRep_SelectedTests();
    PlayCueForNearbyPlayers(CompletedSound);
    UpdateTubeInteractionLocks();
    ForceNetUpdate();
}

void AEMRLabAnalyzerMachine::UpdateTubeInteractionLocks()
{
    const bool bShouldLock = bLidClosed || bIsSpinning || !CurrentOperator;

    auto SetLockIfValid = [bShouldLock](AEMRLabAnalyzerTube* Tube)
    {
        if (Tube)
        {
            Tube->SetInteractionLocked(bShouldLock);
        }
    };

    for (AEMRLabAnalyzerTube* Tube : InsertedTubes)
    {
        SetLockIfValid(Tube);
    }

    for (const auto& Pair : SpawnedTubesByPatient)
    {
        SetLockIfValid(Pair.Value);
    }

    SetLockIfValid(BalanceTube.Get());
}

bool AEMRLabAnalyzerMachine::IsCentrifugeBalanced(bool& bHasPatientTube) const
{
    int32 TotalTubes = 0;
    bHasPatientTube = false;

    for (const AEMRLabAnalyzerTube* Tube : InsertedTubes)
    {
        if (!Tube)
        {
            continue;
        }

        ++TotalTubes;
        if (!Tube->IsBalanceTube())
        {
            bHasPatientTube = true;
        }
    }

    return TotalTubes > 0 && (TotalTubes % 2 == 0);
}

int32 AEMRLabAnalyzerMachine::FindAvailableSlotIndex() const
{
    if (TubeSlots.IsEmpty())
    {
        return INDEX_NONE;
    }

    for (int32 Index = 0; Index < TubeSlots.Num(); ++Index)
    {
        if (!TubeSlots[Index])
        {
            return Index;
        }
    }

    return INDEX_NONE;
}

void AEMRLabAnalyzerMachine::AssignTubeToSlot(AEMRLabAnalyzerTube* Tube, int32 SlotIndex)
{
    if (!Tube || SlotIndex == INDEX_NONE)
    {
        return;
    }

    USceneComponent* Anchor = GetTubeSlotAnchor(SlotIndex);
    if (!Anchor)
    {
        EMR_LAB_LOG(Log, "AssignTubeToSlot failed: no anchor slot=%d cached=%d", SlotIndex, CachedTubeSlotAnchors.Num());
        return;
    }

    if (UEMRCarryableComponent* Carryable = Tube->GetCarryableComponent())
    {
        Carryable->DropAtLocation(Anchor->GetComponentLocation());
        Carryable->SetLockedInPlace(true);
    }

    USceneComponent* TubeRoot = Tube->GetRootComponent();
    if (!TubeRoot)
    {
        EMR_LAB_LOG(Log, "[LabTubeRotate] Tube attach failed: no root tube=%s", *GetNameSafe(Tube));
        return;
    }

    if (UPrimitiveComponent* TubePrimitive = Cast<UPrimitiveComponent>(TubeRoot))
    {
        TubePrimitive->SetSimulatePhysics(false);
    }

    TubeRoot->SetMobility(EComponentMobility::Movable);
    TubeRoot->SetUsingAbsoluteLocation(false);
    TubeRoot->SetUsingAbsoluteRotation(false);
    TubeRoot->SetUsingAbsoluteScale(false);
    EMR_LAB_LOG(Log, "[LabTubeRotate] Tube root flags tube=%s absLoc=%d absRot=%d absScale=%d mobility=%d",
        *GetNameSafe(Tube),
        TubeRoot->IsUsingAbsoluteLocation() ? 1 : 0,
        TubeRoot->IsUsingAbsoluteRotation() ? 1 : 0,
        TubeRoot->IsUsingAbsoluteScale() ? 1 : 0,
        static_cast<int32>(TubeRoot->Mobility));

    if (!TubeRoot->IsRegistered())
    {
        TubeRoot->RegisterComponent();
    }
    if (!Anchor->IsRegistered())
    {
        Anchor->RegisterComponent();
    }

    const bool bAttached = TubeRoot->AttachToComponent(Anchor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    Tube->SetActorTransform(Anchor->GetComponentTransform());
    Tube->SetReplicateMovement(false);
    EMR_LAB_LOG(Log, "[LabTubeRotate] Tube attached tube=%s anchor=%s anchorParent=%s",
        *GetNameSafe(Tube),
        *GetNameSafe(Anchor),
        *GetNameSafe(Anchor->GetAttachParent()));
    EMR_LAB_LOG(Log, "[LabTubeRotate] Tube attach result=%s parentActor=%s parentComp=%s",
        bAttached ? TEXT("true") : TEXT("false"),
        *GetNameSafe(Tube->GetAttachParentActor()),
        *GetNameSafe(TubeRoot->GetAttachParent()));
    EMR_LAB_LOG(Log, "AssignTubeToSlot attached tube=%s slot=%d anchor=%s loc=%s",
        *GetNameSafe(Tube),
        SlotIndex,
        *GetNameSafe(Anchor),
        *Anchor->GetComponentLocation().ToString());
    Tube->SetInsertedInMachine(this);
    Tube->SetInteractionLocked(bLidClosed || bIsSpinning);

    if (TubeSlots.IsValidIndex(SlotIndex))
    {
        TubeSlots[SlotIndex] = Tube;
    }

    InsertedTubes.AddUnique(Tube);
}

void AEMRLabAnalyzerMachine::RemoveTubeFromSlot(AEMRLabAnalyzerTube* Tube)
{
    if (!Tube)
    {
        return;
    }

    InsertedTubes.Remove(Tube);

    for (int32 Index = 0; Index < TubeSlots.Num(); ++Index)
    {
        if (TubeSlots[Index] == Tube)
        {
            TubeSlots[Index] = nullptr;
        }
    }

    Tube->SetInsertedInMachine(nullptr);
    Tube->SetInteractionLocked(false);
    if (UEMRCarryableComponent* Carryable = Tube->GetCarryableComponent())
    {
        if (Carryable->IsCarried())
        {
            return;
        }
    }

    Tube->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Tube->SetReplicateMovement(true);
}

void AEMRLabAnalyzerMachine::PlayCue(AEMRPlayerCharacter* Player, USoundBase* Sound)
{
    if (!HasAuthority())
    {
        EMR_LAB_LOG(Warning, "PlayCue ignored: no authority instigator=%s", *GetNameSafe(Player));
        return;
    }

    if (!Sound)
    {
        EMR_LAB_LOG(Warning, "PlayCue skipped: missing sound instigator=%s", *GetNameSafe(Player));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        EMR_LAB_LOG(Warning, "PlayCue skipped: missing world sound=%s", *GetNameSafe(Sound));
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

    EMR_LAB_LOG(Log, "PlayCue executed for all players instigator=%s sound=%s",
        *GetNameSafe(Player),
        *GetNameSafe(Sound));
}

void AEMRLabAnalyzerMachine::ApplyTestButtonColors()
{
    for (const FEMRLabAnalyzerTestButtonDef& ButtonDef : TestButtons)
    {
        UPrimitiveComponent* ButtonComponent = Cast<UPrimitiveComponent>(ButtonDef.ButtonMesh.GetComponent(this));
        if (!ButtonComponent)
        {
            continue;
        }

        ButtonComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ButtonComponent->SetVisibility(false, true);
        ButtonComponent->SetHiddenInGame(true, true);
    }
}

UStaticMeshComponent* AEMRLabAnalyzerMachine::ResolveLidMesh() const
{
    if (LidActor)
    {
        if (UStaticMeshComponent* ExternalLidMesh = LidActor->GetLidMeshComponent())
        {
            return ExternalLidMesh;
        }
    }

    return LidMesh;
}

void AEMRLabAnalyzerMachine::ApplyLidRotation()
{
    UStaticMeshComponent* ActiveLidMesh = ResolveLidMesh();
    if (!ActiveLidMesh)
    {
        return;
    }

    TargetLidRotation = bLidClosed ? LidClosedRotation : LidOpenRotation;
    if (LidInterpSpeed <= 0.0f || ActiveLidMesh->GetRelativeRotation().Equals(TargetLidRotation, 0.1f))
    {
        ActiveLidMesh->SetRelativeRotation(TargetLidRotation, false, nullptr, ETeleportType::None);
        bIsLidAnimating = false;
        UpdateTickEnabled();
        return;
    }

    bIsLidAnimating = true;
    UpdateTickEnabled();
}

void AEMRLabAnalyzerMachine::UpdateLidRotation(float DeltaSeconds)
{
    UStaticMeshComponent* ActiveLidMesh = ResolveLidMesh();
    if (!ActiveLidMesh)
    {
        bIsLidAnimating = false;
        UpdateTickEnabled();
        return;
    }

    const FRotator CurrentRotation = ActiveLidMesh->GetRelativeRotation();
    const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetLidRotation, DeltaSeconds, LidInterpSpeed);
    ActiveLidMesh->SetRelativeRotation(NewRotation, false, nullptr, ETeleportType::None);

    if (NewRotation.Equals(TargetLidRotation, 0.1f))
    {
        ActiveLidMesh->SetRelativeRotation(TargetLidRotation, false, nullptr, ETeleportType::None);
        bIsLidAnimating = false;
        UpdateTickEnabled();
    }
}

void AEMRLabAnalyzerMachine::RotateRotor(float DeltaSeconds)
{
    if (!CentrifugeRotorMesh || RotorDegreesPerSecond <= 0.0f)
    {
        return;
    }

    const float DeltaYaw = RotorDegreesPerSecond * DeltaSeconds;
    const FRotator DeltaRotation(0.0f, DeltaYaw, 0.0f);
    CentrifugeRotorMesh->AddRelativeRotation(DeltaRotation, false, nullptr, ETeleportType::None);

    RotorDebugAccumulator += DeltaSeconds;
    if (RotorDebugAccumulator >= 1.0f)
    {
        RotorDebugAccumulator = 0.0f;
        const FString NetMode = HasAuthority() ? TEXT("Server") : TEXT("Client");
        const FRotator RotorWorld = CentrifugeRotorMesh->GetComponentRotation();
        const bool bAbsRot = CentrifugeRotorMesh->IsUsingAbsoluteRotation();
        EMR_LAB_LOG(Log, "[LabTubeRotate] %s rotor world=%s absRot=%d mobility=%d",
            *NetMode,
            *RotorWorld.ToCompactString(),
            bAbsRot ? 1 : 0,
            static_cast<int32>(CentrifugeRotorMesh->Mobility));

        if (CachedTubeSlotAnchors.Num() > 0)
        {
            if (USceneComponent* Anchor = CachedTubeSlotAnchors[0].Get())
            {
                const FRotator AnchorWorld = Anchor->GetComponentRotation();
                const bool bAnchorAbsRot = Anchor->IsUsingAbsoluteRotation();
                EMR_LAB_LOG(Log, "[LabTubeRotate] %s anchor0 world=%s absRot=%d parent=%s",
                    *NetMode,
                    *AnchorWorld.ToCompactString(),
                    bAnchorAbsRot ? 1 : 0,
                    *GetNameSafe(Anchor->GetAttachParent()));
            }
        }

        if (TubeSlots.Num() > 0 && TubeSlots[0])
        {
            const FRotator TubeWorld = TubeSlots[0]->GetActorRotation();
            EMR_LAB_LOG(Log, "[LabTubeRotate] %s tube0 world=%s parent=%s",
                *NetMode,
                *TubeWorld.ToCompactString(),
                *GetNameSafe(TubeSlots[0]->GetAttachParentActor()));
        }
    }
}

void AEMRLabAnalyzerMachine::UpdateTickEnabled()
{
    SetActorTickEnabled(bIsSpinning || bIsLidAnimating || bSeatMeshFollowYaw);
}

void AEMRLabAnalyzerMachine::SeatOperator(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    USceneComponent* AttachComponent = SeatAttachComponent;
    if (!AttachComponent && SeatMesh)
    {
        AttachComponent = SeatMesh;
    }
    if (!AttachComponent && OperatorLockPoint)
    {
        AttachComponent = OperatorLockPoint;
    }
    if (!AttachComponent)
    {
        AttachComponent = GetRootComponent();
    }

    USceneComponent* SeatTransformComponent = SeatPoint ? SeatPoint : (OperatorLockPoint ? OperatorLockPoint : AttachComponent);
    if (!AttachComponent || !SeatTransformComponent)
    {
        return;
    }

    const FTransform SeatTransform = SeatTransformComponent->GetComponentTransform();
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

    AttachSeatMeshToPlayer(Player);

    if (SeatEnterSound)
    {
        Player->PlayWorldSoundForAllPlayers(SeatEnterSound, SeatTransform.GetLocation());
    }
}

void AEMRLabAnalyzerMachine::UnseatOperator(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    Player->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    UArrowComponent* ExitComponent = SeatExitPoint ? SeatExitPoint : OperatorExitPoint;
    if (ExitComponent)
    {
        const FTransform ExitTransform = ExitComponent->GetComponentTransform();
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

    DetachSeatMeshToOriginal();

    if (SeatExitSound)
    {
        const FVector SoundLocation = ExitComponent ? ExitComponent->GetComponentLocation() : Player->GetActorLocation();
        Player->PlayWorldSoundForAllPlayers(SeatExitSound, SoundLocation);
    }
}

void AEMRLabAnalyzerMachine::AttachSeatMeshToPlayer(AEMRPlayerCharacter* Player)
{
    if (!SeatMesh || !Player)
    {
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
    UpdateTickEnabled();
}

void AEMRLabAnalyzerMachine::DetachSeatMeshToOriginal()
{
    if (!SeatMesh || !bSeatMeshAttachCached)
    {
        return;
    }

    USceneComponent* RestoreParent = SeatMeshOriginalAttachParent.IsValid()
    ? SeatMeshOriginalAttachParent.Get()
    : GetRootComponent();
    if (RestoreParent)
    {
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
    bSeatMeshAttachCached = false;
    SeatMeshOriginalAttachParent = nullptr;
    UpdateTickEnabled();
}

void AEMRLabAnalyzerMachine::UpdateSeatMeshYawFromPlayer(float DeltaSeconds)
{
    if (!bSeatMeshFollowYaw || !SeatMesh || !CurrentOperator)
    {
        return;
    }

    float CurrentControlYaw = SeatMeshInitialControlYaw;
    if (AController* Controller = CurrentOperator->GetController())
    {
        CurrentControlYaw = Controller->GetControlRotation().Yaw;
    }
    else
    {
        CurrentControlYaw = CurrentOperator->GetActorRotation().Yaw;
    }

    const float TargetOffset = FMath::FindDeltaAngleDegrees(SeatMeshInitialControlYaw, CurrentControlYaw);

    if (HasAuthority())
    {
        SeatMeshYawOffset = TargetOffset;
    }

    const bool bLocallyControlled = CurrentOperator->IsLocallyControlled();
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

void AEMRLabAnalyzerMachine::ApplySeatMeshYawOffset()
{
    if (!SeatMesh || !bSeatMeshAttachCached)
    {
        return;
    }

    FRotator NewRotation = SeatMeshBaseWorldRotation;
    NewRotation.Yaw = FMath::UnwindDegrees(SeatMeshBaseWorldRotation.Yaw + SeatMeshYawOffsetSmoothed);
    SeatMesh->SetWorldRotation(NewRotation, false, nullptr, ETeleportType::None);
}

void AEMRLabAnalyzerMachine::SetStartValidationError(EEMRLabAnalyzerStartValidationError NewError)
{
    if (!HasAuthority())
    {
        return;
    }

    StartValidationError = NewError;
    ++StartValidationFeedbackRevision;
    OnRep_StartValidationFeedback();
    ForceNetUpdate();
}

void AEMRLabAnalyzerMachine::RefreshStartValidationErrorWidget()
{
    if (GetNetMode() == NM_DedicatedServer || !StartValidationErrorWidgetComponent)
    {
        return;
    }

    if (!StartValidationErrorWidgetComponent->GetUserWidgetObject())
    {
        StartValidationErrorWidgetComponent->InitWidget();
    }

    UEMRLabAnalyzerValidationErrorWidget* ValidationWidget =
    Cast<UEMRLabAnalyzerValidationErrorWidget>(StartValidationErrorWidgetComponent->GetUserWidgetObject());
    if (!ValidationWidget)
    {
        return;
    }

    FText Message;
    switch (StartValidationError)
    {
    case EEMRLabAnalyzerStartValidationError::LidOpen:
        Message = LidOpenValidationErrorText;
        break;
    case EEMRLabAnalyzerStartValidationError::Unbalanced:
        Message = UnbalancedValidationErrorText;
        break;
    default:
        break;
    }

    if (Message.IsEmpty())
    {
        ValidationWidget->ClearMessage();
        return;
    }

    ValidationWidget->ShowMessage(Message);
}

void AEMRLabAnalyzerMachine::PlayCueForNearbyPlayers(USoundBase* Sound)
{
    PlayCue(nullptr, Sound);
}

FGameplayTagContainer AEMRLabAnalyzerMachine::GetLabTestsForPatient(AEMRPatient* Patient, const FGameplayTag& ExamTag) const
{
    FGameplayTagContainer Result;

    if (!Patient)
    {
        return Result;
    }

    const UEMRExamQueueSubsystem* QueueSubsystem = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>() : nullptr;
    if (!QueueSubsystem)
    {
        return Result;
    }

    const FGameplayTagContainer MissingTests = QueueSubsystem->GetMissingRequiredExamsForPatient(Patient);
    for (const FGameplayTag& MissingTag : MissingTests)
    {
        if (MissingTag.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root))
        {
            Result.AddTag(MissingTag);
        }
    }

    if (Result.IsEmpty() && ExamTag.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root))
    {
        Result.AddTag(
            ExamTag.MatchesTagExact(EMRTags::Abilities::Exam::LabAnalyzer::Root)
                ? EMRTags::Abilities::Exam::LabAnalyzer::Root
                : ExamTag);
    }

    return Result;
}
