#include "Interaction/EMRXRayMachine.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRAIController.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Characters/Player/EMRPlayerController.h"
#include "Components/AudioComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/EMRXRayTargetData.h"
#include "Framework/EMRAssetManager.h"
#include "GAS/EMRTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/EMRSeatedAnimationInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "TimerManager.h"
#include "Components/SphereComponent.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UI/Machines/Triage/EMRTriagePatientCard.h"
#include "UI/Patient/EMRPatientUIController.h"

namespace
{
    bool AppendFallbackXRaySocketsFromRow(const TArray<FEMRXRayTargetData*>& Rows, TSet<FName>& OutSockets)
    {
        if (Rows.IsEmpty())
        {
            return false;
        }

        const int32 StartIndex = FMath::RandRange(0, Rows.Num() - 1);
        for (int32 Offset = 0; Offset < Rows.Num(); ++Offset)
        {
            const int32 RowIndex = (StartIndex + Offset) % Rows.Num();
            const FEMRXRayTargetData* Row = Rows.IsValidIndex(RowIndex) ? Rows[RowIndex] : nullptr;
            if (!Row)
            {
                continue;
            }

            bool bAdded = false;
            for (const FName& SocketName : Row->SocketNames)
            {
                if (SocketName != NAME_None)
                {
                    OutSockets.Add(SocketName);
                    bAdded = true;
                }
            }

            if (bAdded)
            {
                return true;
            }
        }

        return false;
    }
}

AEMRXRayMachine::AEMRXRayMachine()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    XAxisMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("XAxisMesh"));
    XAxisMesh->SetupAttachment(GetRootComponent());
    XAxisMesh->SetMobility(EComponentMobility::Movable);

    YAxisMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("YAxisMesh"));
    YAxisMesh->SetupAttachment(XAxisMesh);
    YAxisMesh->SetMobility(EComponentMobility::Movable);

    MovementLoopAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MovementLoopAudioComponent"));
    MovementLoopAudioComponent->SetupAttachment(YAxisMesh);
    MovementLoopAudioComponent->bAutoActivate = false;
    MovementLoopAudioComponent->bAllowSpatialization = true;

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

    PatientAttachComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PatientAttachPoint"));
    PatientAttachComponent->SetupAttachment(GetRootComponent());

    MonitorCaptureRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MonitorCaptureRoot"));
    MonitorCaptureRoot->SetupAttachment(YAxisMesh);

    MonitorCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MonitorCapture"));
    MonitorCapture->SetupAttachment(MonitorCaptureRoot);
    MonitorCapture->bCaptureEveryFrame = false;
    MonitorCapture->bCaptureOnMovement = false;

    MonitorScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MonitorScreenMesh"));
    MonitorScreenMesh->SetupAttachment(GetRootComponent());

    PatientCardWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PatientCardWidget"));
    PatientCardWidgetComponent->SetupAttachment(GetRootComponent());
    PatientCardWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    PatientCardWidgetComponent->SetDrawAtDesiredSize(true);
    PatientCardWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PatientCardWidgetComponent->SetVisibility(false);
    PatientCardWidgetComponent->SetHiddenInGame(true);

    SeatAnimationTag = EMRTags::SeatAnimation::XRay;
}

void AEMRXRayMachine::BeginPlay()
{
    Super::BeginPlay();

    if (MovementLoopAudioComponent)
    {
        MovementLoopAudioComponent->SetSound(MovementLoopSound);
    }

    CacheAxisBaseLocations();
    ApplyAxisOffsets();

    if (DetectionRadius)
    {
        DetectionRadius->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnDetectionRadiusBeginOverlap);
        DetectionRadius->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnDetectionRadiusEndOverlap);
    }

    if (SeatMesh)
    {
        SeatMesh->SetMobility(EComponentMobility::Movable);
        SeatMesh->SetUsingAbsoluteLocation(false);
        SeatMesh->SetUsingAbsoluteRotation(false);
        SeatMesh->SetUsingAbsoluteScale(false);
    }

    if (GetNetMode() != NM_DedicatedServer)
    {
        InitializeMonitorCapture();
        RefreshMonitorCaptureTargets();
        RefreshPatientCardWidget();
    }

    RefreshMovementLoopAudioPlayback();
}

void AEMRXRayMachine::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (GetNetMode() != NM_DedicatedServer)
    {
        RefreshMonitorCaptureActivity();
        MonitorOverlayRefreshAccumulatorSeconds += DeltaSeconds;
        UpdateMonitorOverlay();
    }

    UpdateSeatMeshYawFromPlayer(DeltaSeconds);

    if (!HasAuthority())
    {
        if (CachedAssignedPatient.Get() != CurrentPatient)
        {
            CachedAssignedPatient = CurrentPatient;
            RefreshMonitorCaptureTargets();
            RefreshPatientCardWidget();
        }
        return;
    }

    if (CachedAssignedPatient.Get() != CurrentPatient)
    {
        const bool bKeepOperator = (CurrentOperator != nullptr) || bKeepOperatorAfterCompletion;
        ResetExamState(bKeepOperator);
        bKeepOperatorAfterCompletion = false;
        CachedAssignedPatient = CurrentPatient;
        RefreshMonitorCaptureTargets();
        RefreshPatientCardWidget();

        if (bKeepOperator && CurrentPatient)
        {
            RefreshRequiredSockets();
        }
    }

    if (CurrentPatient && !bPatientOnTable)
    {
        TryAttachPatient();
    }

    if (CurrentOperator)
    {
        UpdateHeadOffsets(DeltaSeconds);
        ValidateTargetSockets();
    }
    else
    {
        MovementLoopIdleElapsedSeconds = 0.0f;
        SetMovementLoopActive(false);
    }

    if (bCompletionPending && PendingCueCount == 0 && RequiredSocketNames.Num() > 0
        && FoundSocketNames.Num() == RequiredSocketNames.Num())
    {
        TryCompleteExam();
    }
}

void AEMRXRayMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRXRayMachine, CurrentOperator);
    DOREPLIFETIME(AEMRXRayMachine, SeatMeshYawOffset);
    DOREPLIFETIME(AEMRXRayMachine, XAxisOffset);
    DOREPLIFETIME(AEMRXRayMachine, YAxisOffset);
    DOREPLIFETIME(AEMRXRayMachine, FoundSocketNamesReplicated);
    DOREPLIFETIME(AEMRXRayMachine, AttachedPatient);
    DOREPLIFETIME(AEMRXRayMachine, bMovementLoopActive);
}

bool AEMRXRayMachine::ShouldReleasePatientOnExamCompleted(FGameplayTag ExamTag) const
{
    return !ExamTag.MatchesTagExact(EMRTags::Abilities::Exam::XRay);
}

void AEMRXRayMachine::TryStartExam(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] TryStartExam request machine=%s operator=%s patient=%s"),
        *GetNameSafe(this), *GetNameSafe(Player), *GetNameSafe(CurrentPatient));

    if (CurrentOperator && CurrentOperator != Player)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] TryStartExam rejected: already in use by %s"), *GetNameSafe(CurrentOperator));
        UE_LOG(LogTemp, Log, TEXT("[XRayFlow] TryStartExam rejected busy operator=%s current=%s"),
            *GetNameSafe(Player), *GetNameSafe(CurrentOperator.Get()));
        return;
    }

    if (!CurrentPatient)
    {
        UE_LOG(LogTemp, Log, TEXT("[XRayMachine] TryStartExam: no assigned patient, entering standby"));
        UE_LOG(LogTemp, Log, TEXT("[XRayFlow] TryStartExam standby operator=%s"),
            *GetNameSafe(Player));

        CurrentOperator = Player;
        CachedOperatorController = Cast<AEMRPlayerController>(Player->GetController());

        SeatOperator(Player);
        Player->Client_BeginXRayExam(this, ExamInputMappingContext);

        RefreshMonitorCaptureTargets();
        return;
    }

    if (RequiredSocketNames.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("[XRayMachine] TryStartExam: refreshing required sockets for patient %s"), *GetNameSafe(CurrentPatient));
        RefreshRequiredSockets();
    }

    if (RequiredSocketNames.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] TryStartExam rejected: no target sockets configured"));
        UE_LOG(LogTemp, Log, TEXT("[XRayFlow] TryStartExam rejected no sockets operator=%s patient=%s"),
            *GetNameSafe(Player), *GetNameSafe(CurrentPatient));
        return;
    }

    CurrentOperator = Player;
    CachedOperatorController = Cast<AEMRPlayerController>(Player->GetController());
    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] TryStartExam accepted: operator %s, patient %s, targets %d"),
        *GetNameSafe(Player), *GetNameSafe(CurrentPatient), RequiredSocketNames.Num());
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] TryStartExam accepted operator=%s patient=%s targets=%d"),
        *GetNameSafe(Player), *GetNameSafe(CurrentPatient), RequiredSocketNames.Num());

    SeatOperator(Player);
    Player->Client_BeginXRayExam(this, ExamInputMappingContext);

    RefreshMonitorCaptureTargets();
    TryAttachPatient();

    if (bCompletionPending && PendingCueCount == 0 && FoundSocketNames.Num() == RequiredSocketNames.Num())
    {
        TryCompleteExam();
    }
}

void AEMRXRayMachine::StopExam(AEMRPlayerCharacter* Player, bool bFromCancel)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    if (CurrentOperator != Player)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] StopExam: operator %s, patient %s, fromCancel=%s"),
        *GetNameSafe(Player), *GetNameSafe(CurrentPatient), bFromCancel ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] StopExam operator=%s patient=%s cancel=%s"),
        *GetNameSafe(Player), *GetNameSafe(CurrentPatient), bFromCancel ? TEXT("true") : TEXT("false"));

    UnseatOperator(Player);
    Player->Client_EndXRayExam(this);

    CurrentOperator = nullptr;
    MoveInputAxis = FVector2D::ZeroVector;
    MovementLoopIdleElapsedSeconds = 0.0f;
    SetMovementLoopActive(false);

    if (!bFromCancel)
    {
        CachedOperatorController.Reset();
    }
}

void AEMRXRayMachine::SetMoveInput(AEMRPlayerCharacter* Player, float InputX, float InputY)
{
    if (!HasAuthority() || !Player || CurrentOperator != Player)
    {
        return;
    }

    MoveInputAxis.X = FMath::Clamp(InputX, -1.0f, 1.0f);
    MoveInputAxis.Y = FMath::Clamp(InputY, -1.0f, 1.0f);
}

void AEMRXRayMachine::UpdateHeadOffsets(float DeltaSeconds)
{
    bool bHeadMovedThisTick = false;

    if (MoveInputAxis.IsNearlyZero())
    {
        UpdateMovementLoopState(false, DeltaSeconds);
        return;
    }

    const float NewX = FMath::Clamp(XAxisOffset + MoveInputAxis.Y * XMoveSpeed * DeltaSeconds, XMinOffset, XMaxOffset);
    const float NewY = FMath::Clamp(YAxisOffset + MoveInputAxis.X * YMoveSpeed * DeltaSeconds, YMinOffset, YMaxOffset);

    if (!FMath::IsNearlyEqual(NewX, XAxisOffset) || !FMath::IsNearlyEqual(NewY, YAxisOffset))
    {
        XAxisOffset = NewX;
        YAxisOffset = NewY;
        ApplyAxisOffsets();
        bHeadMovedThisTick = true;
    }

    UpdateMovementLoopState(bHeadMovedThisTick, DeltaSeconds);
}

void AEMRXRayMachine::UpdateMovementLoopState(bool bHeadMovedThisTick, float DeltaSeconds)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bHeadMovedThisTick)
    {
        MovementLoopIdleElapsedSeconds = 0.0f;
        SetMovementLoopActive(true);
        return;
    }

    MovementLoopIdleElapsedSeconds += DeltaSeconds;
    if (MovementLoopIdleElapsedSeconds >= MovementLoopStopGraceSeconds)
    {
        SetMovementLoopActive(false);
    }
}

void AEMRXRayMachine::SetMovementLoopActive(bool bNewActive)
{
    if (!HasAuthority() || bMovementLoopActive == bNewActive)
    {
        return;
    }

    bMovementLoopActive = bNewActive;
    ForceNetUpdate();
    RefreshMovementLoopAudioPlayback();
}

void AEMRXRayMachine::RefreshMovementLoopAudioPlayback()
{
    if (GetNetMode() == NM_DedicatedServer || !MovementLoopAudioComponent)
    {
        return;
    }

    MovementLoopAudioComponent->SetSound(MovementLoopSound);

    if (bMovementLoopActive && MovementLoopSound)
    {
        MovementLoopAudioComponent->FadeIn(MovementLoopFadeInSeconds, 1.0f, 0.0f);
    }
    else if (MovementLoopAudioComponent->IsPlaying())
    {
        MovementLoopAudioComponent->FadeOut(MovementLoopFadeOutSeconds, 0.0f);
    }
}

void AEMRXRayMachine::ApplyAxisOffsets()
{
    CacheAxisBaseLocations();

    if (XAxisMesh)
    {
        XAxisMesh->SetWorldLocation(XAxisBaseWorldLocation + FVector(XAxisOffset, 0.0f, 0.0f));
    }

    if (YAxisMesh)
    {
        YAxisMesh->SetWorldLocation(YAxisBaseWorldLocation + FVector(XAxisOffset, YAxisOffset, 0.0f));
    }

}

void AEMRXRayMachine::TryAttachPatient()
{
    if (!CurrentPatient || bPatientOnTable)
    {
        return;
    }

    USceneComponent* AttachComponent = PatientAttachComponent;
    if (!AttachComponent)
    {
        return;
    }

    if (!DetectionRadius || !DetectionRadius->IsOverlappingActor(CurrentPatient))
    {
        return;
    }

    if (!bHasCachedPatientTransform || CachedPatientTransformOwner.Get() != CurrentPatient)
    {
        CachedPatientTransformOwner = CurrentPatient;
        CachedPatientTransform = CurrentPatient->GetActorTransform();
        bHasCachedPatientTransform = true;
        UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Cached patient transform for %s"), *GetNameSafe(CurrentPatient));
    }

    const bool bHasSocket = (PatientAttachSocketName != NAME_None) && AttachComponent->DoesSocketExist(PatientAttachSocketName);
    const FTransform AttachTransform = bHasSocket
    ? AttachComponent->GetSocketTransform(PatientAttachSocketName)
    : AttachComponent->GetComponentTransform();

    CurrentPatient->SetActorLocationAndRotation(
        AttachTransform.GetLocation(),
        AttachTransform.GetRotation().Rotator(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
    CurrentPatient->AttachToComponent(AttachComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, PatientAttachSocketName);

    if (AController* Controller = CurrentPatient->GetController())
    {
        if (AAIController* AIController = Cast<AAIController>(Controller))
        {
            AIController->StopMovement();
        }

        if (AEMRAIController* EMRAIController = Cast<AEMRAIController>(Controller))
        {
            EMRAIController->ResetBlackboardState();
        }
    }

    if (UCharacterMovementComponent* MovementComponent = CurrentPatient->GetCharacterMovement())
    {
        MovementComponent->DisableMovement();
    }

    if (HasAuthority())
    {
        Multicast_PlayPatientEnterMontage(CurrentPatient);
    }

    if (HasAuthority())
    {
        AttachedPatient = CurrentPatient;
        ForceNetUpdate();
    }

    bPatientOnTable = true;
    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Patient attached to table: %s"), *GetNameSafe(CurrentPatient));
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Patient attached to table patient=%s machine=%s"),
        *GetNameSafe(CurrentPatient), *GetNameSafe(this));
    RefreshMonitorCaptureTargets();
}

void AEMRXRayMachine::DetachPatient(AEMRPatient* Patient)
{
    if (!Patient)
    {
        return;
    }

    Patient->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    if (bHasCachedPatientTransform && CachedPatientTransformOwner.Get() == Patient)
    {
        Patient->SetActorLocationAndRotation(
            CachedPatientTransform.GetLocation(),
            CachedPatientTransform.GetRotation().Rotator(),
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
        UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Restored patient transform for %s"), *GetNameSafe(Patient));
    }

    CachedPatientTransformOwner.Reset();
    bHasCachedPatientTransform = false;

    if (HasAuthority())
    {
        Multicast_StopPatientEnterMontage(Patient);
    }

    if (HasAuthority() && AttachedPatient == Patient)
    {
        AttachedPatient = nullptr;
        ForceNetUpdate();
    }

    if (UCharacterMovementComponent* MovementComponent = Patient->GetCharacterMovement())
    {
        MovementComponent->SetMovementMode(MOVE_Walking);
    }

    bPatientOnTable = false;
    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Patient detached from table: %s"), *GetNameSafe(Patient));
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Patient detached from table patient=%s machine=%s"),
        *GetNameSafe(Patient), *GetNameSafe(this));
}

void AEMRXRayMachine::PlayPatientEnterMontageFor(AEMRPatient* Patient)
{
    if (!Patient || !PatientEnterTableMontage || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    USkeletalMeshComponent* TargetMesh = ResolvePatientMeshFor(Patient);
    if (!TargetMesh)
    {
        TargetMesh = Patient->GetMesh();
    }

    if (TargetMesh)
    {
        if (UAnimInstance* AnimInstance = TargetMesh->GetAnimInstance())
        {
            AnimInstance->Montage_Play(PatientEnterTableMontage);
            UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Playing patient enter montage %s on mesh %s"),
                *GetNameSafe(PatientEnterTableMontage), *GetNameSafe(TargetMesh));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] Patient mesh %s has no anim instance for montage %s"),
                *GetNameSafe(TargetMesh), *GetNameSafe(PatientEnterTableMontage));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] No patient mesh available to play montage %s"),
            *GetNameSafe(PatientEnterTableMontage));
    }
}

void AEMRXRayMachine::StopPatientEnterMontageFor(AEMRPatient* Patient)
{
    if (!Patient || !PatientEnterTableMontage || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    USkeletalMeshComponent* TargetMesh = ResolvePatientMeshFor(Patient);
    if (!TargetMesh)
    {
        TargetMesh = Patient->GetMesh();
    }

    if (TargetMesh)
    {
        if (UAnimInstance* AnimInstance = TargetMesh->GetAnimInstance())
        {
            constexpr float MontageBlendOutTime = 0.2f;
            AnimInstance->Montage_Stop(MontageBlendOutTime, PatientEnterTableMontage);
            UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Stopping patient montage %s on mesh %s"),
                *GetNameSafe(PatientEnterTableMontage), *GetNameSafe(TargetMesh));
        }
    }
}

void AEMRXRayMachine::RefreshRequiredSockets()
{
    RequiredSocketNames.Reset();
    FoundSocketNames.Reset();
    PendingCueCount = 0;
    CachedTargetMesh.Reset();

    if (HasAuthority())
    {
        FoundSocketNamesReplicated.Reset();
        if (AttachedPatient && AttachedPatient != CurrentPatient)
        {
            AttachedPatient = nullptr;
        }
        ForceNetUpdate();
    }

    for (FTimerHandle& TimerHandle : ActiveCueTimers)
    {
        GetWorldTimerManager().ClearTimer(TimerHandle);
    }
    ActiveCueTimers.Reset();

    if (!CurrentPatient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] RefreshRequiredSockets: no patient"));
        return;
    }

    const UEMRPatientData* PatientData = CurrentPatient->GetPatientData();
    if (!PatientData)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] RefreshRequiredSockets: patient %s has no data"), *GetNameSafe(CurrentPatient));
        return;
    }

    const FGameplayTagContainer Pathologies = PatientData->GetPathologies();
    if (Pathologies.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] RefreshRequiredSockets: patient %s has no pathologies"), *GetNameSafe(CurrentPatient));
        return;
    }

    TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
    UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);
    if (LoadedSubsystemConfig.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] RefreshRequiredSockets: no subsystem config loaded"));
        return;
    }

    const UDataTable* XRayTargetsTable = LoadedSubsystemConfig[0]->GetXRayTargetsFromPathologyTable();
    if (!XRayTargetsTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] RefreshRequiredSockets: XRay target table not set on subsystem config"));
        return;
    }

    TSet<FName> UniqueSockets;
    TArray<FEMRXRayTargetData*> Rows;
    XRayTargetsTable->GetAllRows(TEXT("XRayTargets"), Rows);
    for (const FEMRXRayTargetData* Row : Rows)
    {
        if (!Row || !Row->PathologyTag.IsValid())
        {
            continue;
        }

        if (!Pathologies.HasTagExact(Row->PathologyTag))
        {
            continue;
        }

        for (const FName& SocketName : Row->SocketNames)
        {
            if (SocketName != NAME_None)
            {
                UniqueSockets.Add(SocketName);
            }
        }
    }

    if (UniqueSockets.IsEmpty())
    {
        if (AppendFallbackXRaySocketsFromRow(Rows, UniqueSockets))
        {
            UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] RefreshRequiredSockets: using fallback targets for optional exam (patient=%s)"),
                *GetNameSafe(CurrentPatient));
        }
    }

    USkeletalMeshComponent* PatientMesh = ResolvePatientMesh();
    if (!PatientMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] RefreshRequiredSockets: patient %s has no target mesh (NameOverride=%s TagOverride=%s)"),
            *GetNameSafe(CurrentPatient),
            PatientMeshComponentName != NAME_None ? *PatientMeshComponentName.ToString() : TEXT("None"),
            PatientMeshComponentTag != NAME_None ? *PatientMeshComponentTag.ToString() : TEXT("None"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] RefreshRequiredSockets: using mesh %s for patient %s"),
        *GetNameSafe(PatientMesh), *GetNameSafe(CurrentPatient));

    for (const FName& SocketName : UniqueSockets)
    {
        if (PatientMesh->DoesSocketExist(SocketName))
        {
            RequiredSocketNames.Add(SocketName);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] RefreshRequiredSockets: patient %s targets %d (unique %d)"),
        *GetNameSafe(CurrentPatient), RequiredSocketNames.Num(), UniqueSockets.Num());
}

void AEMRXRayMachine::ValidateTargetSockets()
{
    if (!CurrentPatient || RequiredSocketNames.IsEmpty())
    {
        return;
    }

    USkeletalMeshComponent* PatientMesh = ResolvePatientMesh();
    if (!PatientMesh)
    {
        return;
    }

    const FVector HeadLocation = YAxisMesh ? YAxisMesh->GetComponentLocation() : GetActorLocation();
    const float ToleranceSq = FMath::Square(TargetSocketToleranceRadius);

    for (const FName& SocketName : RequiredSocketNames)
    {
        if (FoundSocketNames.Contains(SocketName))
        {
            continue;
        }

        if (!PatientMesh->DoesSocketExist(SocketName))
        {
            continue;
        }

        const FVector SocketLocation = PatientMesh->GetSocketLocation(SocketName);
        const float DistSq = FVector::DistSquared2D(HeadLocation, SocketLocation);
        
        if (DistSq <= ToleranceSq)
        {
            HandleTargetFound(SocketName, SocketLocation);
        }
    }

    if (PendingCueCount == 0 && FoundSocketNames.Num() == RequiredSocketNames.Num())
    {
        TryCompleteExam();
    }
}

void AEMRXRayMachine::HandleTargetFound(const FName& SocketName, const FVector& SocketLocation)
{
    if (FoundSocketNames.Contains(SocketName))
    {
        return;
    }

    FoundSocketNames.Add(SocketName);
    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Target found: %s (%d/%d)"),
        *SocketName.ToString(), FoundSocketNames.Num(), RequiredSocketNames.Num());
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Target found socket=%s progress=%d/%d patient=%s"),
        *SocketName.ToString(), FoundSocketNames.Num(), RequiredSocketNames.Num(), *GetNameSafe(CurrentPatient));

    if (HasAuthority())
    {
        FoundSocketNamesReplicated.Reset();
        for (const FName& FoundName : FoundSocketNames)
        {
            FoundSocketNamesReplicated.Add(FoundName);
        }
        ForceNetUpdate();
    }

    Multicast_PlayWorldSoundAtLocation(TargetFoundSound, SocketLocation);

    PendingCueCount++;

    if (TargetFoundCueDuration <= KINDA_SMALL_NUMBER)
    {
        HandleCueFinished();
        return;
    }

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::HandleCueFinished, TargetFoundCueDuration, false);
    ActiveCueTimers.Add(TimerHandle);
}

void AEMRXRayMachine::HandleCueFinished()
{
    PendingCueCount = FMath::Max(0, PendingCueCount - 1);
    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Target cue finished. Pending cues: %d"), PendingCueCount);

    if (PendingCueCount == 0 && FoundSocketNames.Num() == RequiredSocketNames.Num())
    {
        TryCompleteExam();
    }
}

bool AEMRXRayMachine::TrySendExamGameplayEvent(AEMRPlayerCharacter* Operator, const FGameplayEventData& EventData)
{
    if (!Operator)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayFlow] Dispatch failed: operator missing patient=%s"),
            *GetNameSafe(CurrentPatient));
        return false;
    }

    UAbilitySystemComponent* OperatorASC = Operator->GetAbilitySystemComponent();
    if (!OperatorASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayFlow] Dispatch failed: operator ASC missing operator=%s patient=%s"),
            *GetNameSafe(Operator), *GetNameSafe(CurrentPatient));
        return false;
    }

    FGameplayEventData EventPayload = EventData;
    if (!IsValid(EventPayload.Instigator))
    {
        EventPayload.Instigator = Operator;
    }

    const int32 TriggeredAbilities = OperatorASC->HandleGameplayEvent(EventPayload.EventTag, &EventPayload);
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Dispatch result event=%s operator=%s patient=%s triggered=%d"),
        *EventPayload.EventTag.ToString(), *GetNameSafe(Operator), *GetNameSafe(CurrentPatient), TriggeredAbilities);
    if (TriggeredAbilities <= 0)
    {
        const FGameplayAbilityActorInfo* ActorInfo = OperatorASC->AbilityActorInfo.Get();
        UE_LOG(LogTemp, Warning, TEXT("[XRayFlow] Dispatch debug owner=%s avatar=%s netmode=%s"),
            ActorInfo ? *GetNameSafe(ActorInfo->OwnerActor.Get()) : TEXT("None"),
            ActorInfo ? *GetNameSafe(ActorInfo->AvatarActor.Get()) : TEXT("None"),
            GetNetMode() == NM_Client ? TEXT("Client")
            : GetNetMode() == NM_ListenServer ? TEXT("ListenServer")
            : GetNetMode() == NM_DedicatedServer ? TEXT("DedicatedServer")
            : TEXT("Standalone"));
    }
    return TriggeredAbilities > 0;
}

void AEMRXRayMachine::TryCompleteExam()
{
    if (!HasAuthority() || !CurrentPatient)
    {
        return;
    }

    AEMRPatient* CompletedPatient = CurrentPatient;

    AEMRPlayerController* PlayerController = nullptr;
    if (CurrentOperator)
    {
        PlayerController = Cast<AEMRPlayerController>(CurrentOperator->GetController());
    }

    if (!PlayerController)
    {
        PlayerController = CachedOperatorController.Get();
    }

    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] TryCompleteExam pending: no player controller"));
        UE_LOG(LogTemp, Warning, TEXT("[XRayFlow] TryCompleteExam pending: controller missing operator=%s patient=%s"),
            *GetNameSafe(CurrentOperator.Get()), *GetNameSafe(CurrentPatient));
        bCompletionPending = true;
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] TryCompleteExam: sending gameplay event for patient %s"), *GetNameSafe(CurrentPatient));

    FGameplayEventData EventData;
    EventData.EventTag = EMRTags::Abilities::Exam::XRay;
    EventData.Target = CurrentPatient;
    EventData.OptionalObject = this;
    EventData.EventMagnitude = 1.0f;

    AEMRPlayerCharacter* DispatchOperator = CurrentOperator.Get();
    if (!DispatchOperator && PlayerController)
    {
        DispatchOperator = Cast<AEMRPlayerCharacter>(PlayerController->GetPawn());
    }

    if (!DispatchOperator)
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayFlow] TryCompleteExam pending: no operator actor for dispatch controller=%s patient=%s"),
            *GetNameSafe(PlayerController), *GetNameSafe(CurrentPatient));
        bCompletionPending = true;
        return;
    }

    if (!TrySendExamGameplayEvent(DispatchOperator, EventData))
    {
        UE_LOG(LogTemp, Warning, TEXT("[XRayFlow] TryCompleteExam pending: dispatch failed operator=%s patient=%s"),
            *GetNameSafe(DispatchOperator), *GetNameSafe(CurrentPatient));
        bCompletionPending = true;
        return;
    }

    bCompletionPending = false;

    bKeepOperatorAfterCompletion = true;

    if (bPatientOnTable && CompletedPatient)
    {
        DetachPatient(CompletedPatient);
    }

    ReleasePatient();
    UE_LOG(LogTemp, Log, TEXT("[XRayFlow] TryCompleteExam finished operator=%s patient=%s"),
        *GetNameSafe(CurrentOperator.Get()), *GetNameSafe(CompletedPatient));
}

void AEMRXRayMachine::ResetExamState(bool bKeepOperator)
{
    CachedTargetMesh.Reset();
    bPatientInDetectionRadius = false;
    UE_LOG(LogTemp, Log, TEXT("[XRayMachine] ResetExamState: machine %s"), *GetNameSafe(this));
    if (CurrentOperator && !bKeepOperator)
    {
        StopExam(CurrentOperator, false);
    }

    if (bPatientOnTable && CachedAssignedPatient.IsValid())
    {
        DetachPatient(CachedAssignedPatient.Get());
    }

    for (FTimerHandle TimerHandle : ActiveCueTimers)
    {
        GetWorldTimerManager().ClearTimer(TimerHandle);
    }
    ActiveCueTimers.Reset();

    RequiredSocketNames.Reset();
    FoundSocketNames.Reset();
    PendingCueCount = 0;
    bCompletionPending = false;
    bPatientOnTable = false;

    if (HasAuthority())
    {
        FoundSocketNamesReplicated.Reset();
        ForceNetUpdate();
    }

    if (!bKeepOperator)
    {
        XAxisOffset = 0.0f;
        YAxisOffset = 0.0f;
    }
    ApplyAxisOffsets();

    MoveInputAxis = FVector2D::ZeroVector;
    MovementLoopIdleElapsedSeconds = 0.0f;
    SetMovementLoopActive(false);
    CachedOperatorController.Reset();
}

void AEMRXRayMachine::OnDetectionRadiusBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || !OtherActor)
    {
        return;
    }

    if (OtherActor == CurrentPatient)
    {
        bPatientInDetectionRadius = true;
        UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Patient entered detection radius: %s"), *GetNameSafe(OtherActor));
        TryAttachPatient();
    }
}

void AEMRXRayMachine::OnDetectionRadiusEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!HasAuthority() || !OtherActor)
    {
        return;
    }

    if (OtherActor == CurrentPatient)
    {
        bPatientInDetectionRadius = false;
        UE_LOG(LogTemp, Log, TEXT("[XRayMachine] Patient exited detection radius: %s"), *GetNameSafe(OtherActor));
    }
}

USkeletalMeshComponent* AEMRXRayMachine::ResolvePatientMesh() const
{
    return ResolvePatientMeshFor(CurrentPatient);
}

USkeletalMeshComponent* AEMRXRayMachine::ResolvePatientMeshFor(const AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return nullptr;
    }

    const bool bCanCache = (Patient == CurrentPatient);
    if (bCanCache && CachedTargetMesh.IsValid())
    {
        if (CachedTargetMesh->GetOwner() == Patient)
        {
            return CachedTargetMesh.Get();
        }
        CachedTargetMesh.Reset();
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
                if (bCanCache)
                {
                    CachedTargetMesh = TaggedMesh;
                }
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
                if (bCanCache)
                {
                    CachedTargetMesh = MeshComponent;
                }
                return MeshComponent;
            }
        }
    }

    USkeletalMeshComponent* DefaultMesh = Patient->GetMesh();
    if (bCanCache && DefaultMesh)
    {
        CachedTargetMesh = DefaultMesh;
    }
    return DefaultMesh;
}

void AEMRXRayMachine::CacheAxisBaseLocations()
{
    if (bAxisBaseCached)
    {
        return;
    }

    if (XAxisMesh)
    {
        XAxisBaseWorldLocation = XAxisMesh->GetComponentLocation();
    }

    if (YAxisMesh)
    {
        YAxisBaseWorldLocation = YAxisMesh->GetComponentLocation();
    }

    bAxisBaseCached = true;
}

void AEMRXRayMachine::OnRep_CurrentOperator()
{
    if (CurrentOperator)
    {
        AttachSeatMeshToPlayer(CurrentOperator.Get());
    }
    else
    {
        DetachSeatMeshToOriginal();
    }

    RefreshMonitorCaptureActivity();
    UpdateMonitorOverlay(true);
}

void AEMRXRayMachine::OnRep_XAxisOffset()
{
    ApplyAxisOffsets();
}

void AEMRXRayMachine::OnRep_YAxisOffset()
{
    ApplyAxisOffsets();
}

void AEMRXRayMachine::OnRep_FoundSocketNames()
{
    UpdateMonitorOverlay();
}

void AEMRXRayMachine::OnRep_AttachedPatient()
{
    HandleAttachedPatientChanged();
}

void AEMRXRayMachine::OnRep_MovementLoopActive()
{
    RefreshMovementLoopAudioPlayback();
}

void AEMRXRayMachine::HandleAttachedPatientChanged()
{
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(AttachedPatientMontageRetryTimer);
    }

    if (LastAttachedPatient.IsValid() && LastAttachedPatient != AttachedPatient)
    {
        StopPatientEnterMontageFor(LastAttachedPatient.Get());
    }

    LastAttachedPatient = AttachedPatient;

    CachedTargetMesh.Reset();
    UpdateMonitorOverlay();

    if (!AttachedPatient)
    {
        return;
    }

    if (!TryPlayAttachedPatientMontage() && GetWorld())
    {
        GetWorldTimerManager().SetTimer(
            AttachedPatientMontageRetryTimer,
            this,
            &ThisClass::HandleAttachedPatientChanged,
            0.1f,
            false);
    }
}

bool AEMRXRayMachine::TryPlayAttachedPatientMontage()
{
    if (!AttachedPatient || !PatientEnterTableMontage || GetNetMode() == NM_DedicatedServer)
    {
        return true;
    }

    USkeletalMeshComponent* TargetMesh = ResolvePatientMeshFor(AttachedPatient);
    if (!TargetMesh)
    {
        TargetMesh = AttachedPatient->GetMesh();
    }

    if (!TargetMesh)
    {
        return false;
    }

    UAnimInstance* AnimInstance = TargetMesh->GetAnimInstance();
    if (!AnimInstance)
    {
        return false;
    }

    if (!AnimInstance->Montage_IsPlaying(PatientEnterTableMontage))
    {
        AnimInstance->Montage_Play(PatientEnterTableMontage);
    }

    return true;
}

void AEMRXRayMachine::Multicast_PlayPatientEnterMontage_Implementation(AEMRPatient* Patient)
{
    PlayPatientEnterMontageFor(Patient);
}

void AEMRXRayMachine::Multicast_StopPatientEnterMontage_Implementation(AEMRPatient* Patient)
{
    StopPatientEnterMontageFor(Patient);
}

void AEMRXRayMachine::Multicast_PlayWorldSoundAtLocation_Implementation(USoundBase* Sound, FVector_NetQuantize Location)
{
    if (GetNetMode() == NM_DedicatedServer || !Sound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
}

void AEMRXRayMachine::InitializeMonitorCapture()
{
    if (!MonitorCapture || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!MonitorRenderTarget && MonitorRenderTargetSize > 0)
    {
        MonitorRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
            this,
            MonitorRenderTargetSize,
            MonitorRenderTargetSize,
            RTF_RGBA16f);
        if (MonitorRenderTarget)
        {
            MonitorRenderTarget->ClearColor = FLinearColor::Black;
            MonitorRenderTarget->UpdateResourceImmediate(true);
        }
    }

    if (!MonitorOverlayRenderTarget && MonitorRenderTargetSize > 0)
    {
        MonitorOverlayRenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
            this,
            UCanvasRenderTarget2D::StaticClass(),
            MonitorRenderTargetSize,
            MonitorRenderTargetSize);
        if (MonitorOverlayRenderTarget)
        {
            MonitorOverlayRenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
            MonitorOverlayRenderTarget->UpdateResourceImmediate(true);
        }
    }

    if (MonitorOverlayRenderTarget)
    {
        MonitorOverlayRenderTarget->OnCanvasRenderTargetUpdate.RemoveAll(this);
        MonitorOverlayRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &ThisClass::HandleMonitorOverlayUpdate);
    }

    if (MonitorCapture)
    {
        MonitorCapture->ProjectionType = ECameraProjectionMode::Orthographic;
        MonitorCapture->OrthoWidth = MonitorOrthoWidth;
        MonitorCapture->bCaptureEveryFrame = false;
        MonitorCapture->bCaptureOnMovement = false;
        MonitorCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
        MonitorCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
        if (MonitorRenderTarget)
        {
            MonitorCapture->TextureTarget = MonitorRenderTarget;
        }
    }

    if (!MonitorScreenMaterial || !MonitorScreenMesh)
    {
        if (!MonitorScreenMesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] Monitor screen mesh not set on %s"), *GetNameSafe(this));
        }
        if (!MonitorScreenMaterial)
        {
            UE_LOG(LogTemp, Warning, TEXT("[XRayMachine] Monitor screen material not set on %s"), *GetNameSafe(this));
        }
        return;
    }

    if (!MonitorScreenMID)
    {
        MonitorScreenMID = UMaterialInstanceDynamic::Create(MonitorScreenMaterial, this);
    }

    if (MonitorScreenMID)
    {
        if (MonitorRenderTarget)
        {
            MonitorScreenMID->SetTextureParameterValue(MonitorSceneTextureParamName, MonitorRenderTarget);
        }
        if (MonitorOverlayRenderTarget)
        {
            MonitorScreenMID->SetTextureParameterValue(MonitorOverlayTextureParamName, MonitorOverlayRenderTarget);
        }
        MonitorScreenMesh->SetMaterial(0, MonitorScreenMID);
    }
}

void AEMRXRayMachine::RefreshMonitorCaptureTargets()
{
    if (!MonitorCapture || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (MonitorRenderTarget && MonitorCapture->TextureTarget != MonitorRenderTarget)
    {
        MonitorCapture->TextureTarget = MonitorRenderTarget;
    }

    RefreshMonitorCaptureActivity();
    if (!MonitorCapture->bCaptureEveryFrame && MonitorCapture->TextureTarget)
    {
        MonitorCapture->CaptureScene();
    }
}

bool AEMRXRayMachine::ShouldRefreshMonitorOverlayContinuously() const
{
    return CurrentPatient != nullptr && (CurrentOperator != nullptr || FoundSocketNamesReplicated.Num() > 0);
}

void AEMRXRayMachine::RefreshMonitorCaptureActivity()
{
    if (!MonitorCapture || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    const bool bShouldCaptureEveryFrame = (CurrentOperator != nullptr);
    if (MonitorCapture->bCaptureEveryFrame == bShouldCaptureEveryFrame)
    {
        return;
    }

    MonitorCapture->bCaptureEveryFrame = bShouldCaptureEveryFrame;

    if (MonitorCapture->TextureTarget)
    {
        MonitorCapture->CaptureScene();
    }
}

void AEMRXRayMachine::UpdateMonitorOverlay(bool bForce)
{
    if (GetNetMode() == NM_DedicatedServer || !MonitorOverlayRenderTarget)
    {
        return;
    }

    const bool bShouldRefreshContinuously = ShouldRefreshMonitorOverlayContinuously();
    if (!bShouldRefreshContinuously)
    {
        MonitorOverlayRefreshAccumulatorSeconds = 0.0f;
        if (!bForce && !bWasRefreshingMonitorOverlay)
        {
            return;
        }

        bWasRefreshingMonitorOverlay = false;
        MonitorOverlayRenderTarget->UpdateResource();
        return;
    }

    if (!bForce
        && MonitorOverlayRefreshIntervalSeconds > 0.0f
        && MonitorOverlayRefreshAccumulatorSeconds < MonitorOverlayRefreshIntervalSeconds)
    {
        return;
    }

    bWasRefreshingMonitorOverlay = true;
    MonitorOverlayRefreshAccumulatorSeconds = 0.0f;
    MonitorOverlayRenderTarget->UpdateResource();
}

void AEMRXRayMachine::RefreshPatientCardWidget()
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!PatientCardWidgetComponent)
    {
        return;
    }

    const bool bHasPatient = (CurrentPatient != nullptr);
    if (!bHasPatient)
    {
        PatientCardWidgetComponent->SetHiddenInGame(true);
        PatientCardWidgetComponent->SetVisibility(false);

        if (PatientCardUIController)
        {
            PatientCardUIController->StopMonitoring();
            PatientCardUIController = nullptr;
        }

        if (UEMRTriagePatientCard* CardWidget = ResolvePatientCardWidget())
        {
            CardWidget->ResetCard();
        }
        CachedPatientCardPatient.Reset();
        return;
    }

    UEMRTriagePatientCard* CardWidget = ResolvePatientCardWidget();
    if (!CardWidget)
    {
        PatientCardWidgetComponent->SetHiddenInGame(true);
        PatientCardWidgetComponent->SetVisibility(false);
        if (PatientCardUIController)
        {
            PatientCardUIController->StopMonitoring();
            PatientCardUIController = nullptr;
        }
        CachedPatientCardPatient.Reset();
        return;
    }

    PatientCardWidgetComponent->SetHiddenInGame(false);
    PatientCardWidgetComponent->SetVisibility(true);

    if (CachedPatientCardPatient.Get() == CurrentPatient && PatientCardUIController)
    {
        return;
    }

    if (PatientCardUIController)
    {
        PatientCardUIController->StopMonitoring();
    }

    PatientCardUIController = NewObject<UEMRPatientUIController>(this);
    if (!PatientCardUIController)
    {
        return;
    }

    PatientCardUIController->BindToPatient(CurrentPatient);
    CardWidget->InitializeCard(CurrentPatient, PatientCardUIController);
    CachedPatientCardPatient = CurrentPatient;
}

UEMRTriagePatientCard* AEMRXRayMachine::ResolvePatientCardWidget()
{
    if (CachedPatientCardWidget.IsValid())
    {
        return CachedPatientCardWidget.Get();
    }

    if (!PatientCardWidgetComponent)
    {
        return nullptr;
    }

    if (!PatientCardWidgetComponent->GetUserWidgetObject())
    {
        PatientCardWidgetComponent->InitWidget();
    }

    UUserWidget* UserWidget = PatientCardWidgetComponent->GetUserWidgetObject();
    if (!UserWidget)
    {
        return nullptr;
    }

    UEMRTriagePatientCard* CardWidget = Cast<UEMRTriagePatientCard>(UserWidget);
    if (!CardWidget)
    {
        return nullptr;
    }

    CachedPatientCardWidget = CardWidget;
    return CardWidget;
}

FVector2D AEMRXRayMachine::ComputeMonitorUV(const FVector& WorldLocation) const
{
    if (!MonitorCapture)
    {
        return FVector2D::ZeroVector;
    }

    const FTransform CaptureTransform = MonitorCapture->GetComponentTransform();
    const FVector CaptureLocation = CaptureTransform.GetLocation();
    const FVector Delta = WorldLocation - CaptureLocation;
    const FVector RightAxis = CaptureTransform.GetUnitAxis(EAxis::Y);
    const FVector UpAxis = CaptureTransform.GetUnitAxis(EAxis::Z);
    const float ProjectedX = FVector::DotProduct(Delta, RightAxis);
    const float ProjectedY = FVector::DotProduct(Delta, UpAxis);

    UTextureRenderTarget2D* Target = MonitorCapture->TextureTarget ? MonitorCapture->TextureTarget : MonitorRenderTarget;
    const float OrthoWidth = FMath::Max(1.0f, MonitorCapture->OrthoWidth);
    const float Aspect = Target
    ? (static_cast<float>(Target->SizeX) / FMath::Max(1.0f, static_cast<float>(Target->SizeY)))
    : 1.0f;
    const float OrthoHeight = OrthoWidth / FMath::Max(0.01f, Aspect);

    const float U = 0.5f + (ProjectedX / OrthoWidth);
    const float V = 0.5f - (ProjectedY / OrthoHeight);
    return FVector2D(U, V);
}

void AEMRXRayMachine::DrawMonitorDot(UCanvas* Canvas, const FVector2D& UV, float Size, const FLinearColor& Color, int32 Width, int32 Height) const
{
    if (!Canvas || Width <= 0 || Height <= 0)
    {
        return;
    }

    if (UV.X < 0.f || UV.X > 1.f || UV.Y < 0.f || UV.Y > 1.f)
    {
        return;
    }

    const FVector2D ScreenPos(UV.X * Width, UV.Y * Height);
    Canvas->K2_DrawTexture(
        MonitorDotTexture,
        ScreenPos,
        FVector2D(Size, Size),
        FVector2D::ZeroVector,
        FVector2D::UnitVector,
        Color,
        BLEND_Translucent,
        0.f,
        FVector2D(0.5f, 0.5f));
}

void AEMRXRayMachine::HandleMonitorOverlayUpdate(UCanvas* Canvas, int32 Width, int32 Height)
{
    if (!Canvas || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!MonitorCapture)
    {
        return;
    }

    USkeletalMeshComponent* PatientMesh = ResolvePatientMesh();
    if (!PatientMesh)
    {
        return;
    }

    if (CurrentOperator)
    {
        const FVector HeadLocation = YAxisMesh ? YAxisMesh->GetComponentLocation() : GetActorLocation();
        const FVector2D AimUV = ComputeMonitorUV(HeadLocation);
        DrawMonitorDot(Canvas, AimUV, MonitorAimDotSize, MonitorAimColor, Width, Height);
    }

    for (const FName& SocketName : FoundSocketNamesReplicated)
    {
        if (SocketName == NAME_None || !PatientMesh->DoesSocketExist(SocketName))
        {
            continue;
        }

        const FVector SocketLocation = PatientMesh->GetSocketLocation(SocketName);
        const FVector2D FoundUV = ComputeMonitorUV(SocketLocation);
        DrawMonitorDot(Canvas, FoundUV, MonitorFoundDotSize, MonitorFoundColor, Width, Height);
    }
}

void AEMRXRayMachine::SeatOperator(AEMRPlayerCharacter* Player)
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

void AEMRXRayMachine::UnseatOperator(AEMRPlayerCharacter* Player)
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

void AEMRXRayMachine::AttachSeatMeshToPlayer(AEMRPlayerCharacter* Player)
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
}

void AEMRXRayMachine::DetachSeatMeshToOriginal()
{
    if (!SeatMesh || !bSeatMeshAttachCached)
    {
        return;
    }

    USceneComponent* RestoreParent = SeatMeshOriginalAttachParent ? SeatMeshOriginalAttachParent.Get() : GetRootComponent();
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
}

void AEMRXRayMachine::UpdateSeatMeshYawFromPlayer(float DeltaSeconds)
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

void AEMRXRayMachine::ApplySeatMeshYawOffset()
{
    if (!SeatMesh || !bSeatMeshAttachCached)
    {
        return;
    }

    FRotator NewRotation = SeatMeshBaseWorldRotation;
    NewRotation.Yaw = FMath::UnwindDegrees(SeatMeshBaseWorldRotation.Yaw + SeatMeshYawOffsetSmoothed);
    SeatMesh->SetWorldRotation(NewRotation, false, nullptr, ETeleportType::None);
}

void AEMRXRayMachine::OnRep_SeatMeshYawOffset()
{
    if (!bSeatMeshYawInitialized)
    {
        SeatMeshYawOffsetSmoothed = SeatMeshYawOffset;
        bSeatMeshYawInitialized = true;
    }
    ApplySeatMeshYawOffset();
}
