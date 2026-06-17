#include "Interaction/EMRUltrasoundMachine.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Patient/EMRAIController.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Characters/Player/EMRPlayerController.h"
#include "Components/AudioComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/EMRUltrasoundTargetData.h"
#include "Engine/DataTable.h"
#include "Framework/EMRAssetManager.h"
#include "GAS/EMRTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Texture2D.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "UI/Machines/Triage/EMRTriagePatientCard.h"
#include "UI/Patient/EMRPatientUIController.h"

namespace
{
    bool AppendFallbackUltrasoundPathologyFromRow(const TArray<FEMRUltrasoundTargetData*>& Rows, TSet<FGameplayTag>& OutPathologies)
    {
        if (Rows.IsEmpty())
        {
            return false;
        }

        const int32 StartIndex = FMath::RandRange(0, Rows.Num() - 1);
        for (int32 Offset = 0; Offset < Rows.Num(); ++Offset)
        {
            const int32 RowIndex = (StartIndex + Offset) % Rows.Num();
            const FEMRUltrasoundTargetData* Row = Rows.IsValidIndex(RowIndex) ? Rows[RowIndex] : nullptr;
            if (!Row || !Row->PathologyTag.IsValid())
            {
                continue;
            }

            OutPathologies.Add(Row->PathologyTag);
            return true;
        }

        return false;
    }
}

AEMRUltrasoundMachine::AEMRUltrasoundMachine()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    OperatorLockPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("OperatorLockPoint"));
    OperatorLockPoint->SetupAttachment(GetRootComponent());
    OperatorLockPoint->SetArrowColor(FLinearColor::Blue);
    OperatorLockPoint->bIsScreenSizeScaled = true;

    OperatorExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("OperatorExitPoint"));
    OperatorExitPoint->SetupAttachment(GetRootComponent());
    OperatorExitPoint->SetArrowColor(FLinearColor::Yellow);
    OperatorExitPoint->bIsScreenSizeScaled = true;

    PatientAttachComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PatientAttachPoint"));
    PatientAttachComponent->SetupAttachment(GetRootComponent());

    ProbeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProbeMesh"));
    ProbeMesh->SetupAttachment(GetRootComponent());
    ProbeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProbeMesh->SetMobility(EComponentMobility::Movable);

    MovementLoopAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MovementLoopAudioComponent"));
    MovementLoopAudioComponent->SetupAttachment(ProbeMesh);
    MovementLoopAudioComponent->bAutoActivate = false;
    MovementLoopAudioComponent->bAllowSpatialization = true;

    MonitorScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MonitorScreenMesh"));
    MonitorScreenMesh->SetupAttachment(GetRootComponent());

    PatientCardWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PatientCardWidget"));
    PatientCardWidgetComponent->SetupAttachment(GetRootComponent());
    PatientCardWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    PatientCardWidgetComponent->SetDrawAtDesiredSize(true);
    PatientCardWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PatientCardWidgetComponent->SetVisibility(false);
    PatientCardWidgetComponent->SetHiddenInGame(true);

    SliderKnob1Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SliderKnob1Mesh"));
    SliderKnob1Mesh->SetupAttachment(GetRootComponent());
    SliderKnob1Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SliderKnob1Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    SliderKnob1Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
    SliderKnob1Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    SliderKnob1Mesh->SetMobility(EComponentMobility::Movable);

    SliderKnob2Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SliderKnob2Mesh"));
    SliderKnob2Mesh->SetupAttachment(GetRootComponent());
    SliderKnob2Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SliderKnob2Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    SliderKnob2Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
    SliderKnob2Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    SliderKnob2Mesh->SetMobility(EComponentMobility::Movable);

    SliderKnob3Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SliderKnob3Mesh"));
    SliderKnob3Mesh->SetupAttachment(GetRootComponent());
    SliderKnob3Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SliderKnob3Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    SliderKnob3Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
    SliderKnob3Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    SliderKnob3Mesh->SetMobility(EComponentMobility::Movable);

    SliderIndicator1Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SliderIndicator1Mesh"));
    SliderIndicator1Mesh->SetupAttachment(SliderKnob1Mesh);
    SliderIndicator1Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SliderIndicator1Mesh->SetMobility(EComponentMobility::Movable);

    SliderIndicator2Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SliderIndicator2Mesh"));
    SliderIndicator2Mesh->SetupAttachment(SliderKnob2Mesh);
    SliderIndicator2Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SliderIndicator2Mesh->SetMobility(EComponentMobility::Movable);

    SliderIndicator3Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SliderIndicator3Mesh"));
    SliderIndicator3Mesh->SetupAttachment(SliderKnob3Mesh);
    SliderIndicator3Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SliderIndicator3Mesh->SetMobility(EComponentMobility::Movable);

}

void AEMRUltrasoundMachine::BeginPlay()
{
    Super::BeginPlay();

    if (MovementLoopAudioComponent)
    {
        MovementLoopAudioComponent->SetSound(MovementLoopSound);
    }

    // Use the placed/editor knob transforms as runtime base rotations.
    if (SliderKnob1Mesh)
    {
        SliderKnob1BaseRotation = SliderKnob1Mesh->GetRelativeRotation();
    }
    if (SliderKnob2Mesh)
    {
        SliderKnob2BaseRotation = SliderKnob2Mesh->GetRelativeRotation();
    }
    if (SliderKnob3Mesh)
    {
        SliderKnob3BaseRotation = SliderKnob3Mesh->GetRelativeRotation();
    }

    InitializeScreenMaterial();
    InitializeSliderIndicatorMaterials();
    UpdateSliderIndicatorState();
    ResetSliderTargetReachedState();
    if (GetNetMode() != NM_DedicatedServer)
    {
        RefreshPatientCardWidget();
    }

    RefreshMovementLoopAudioPlayback();
}

void AEMRUltrasoundMachine::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority())
    {
        if (CachedAssignedPatient.Get() != CurrentPatient)
        {
            CachedAssignedPatient = CurrentPatient;
            RefreshPatientCardWidget();
        }
        return;
    }

    if (CachedAssignedPatient.Get() != CurrentPatient)
    {
        const bool bKeepOperator = (CurrentOperator != nullptr);
        ResetExamState(bKeepOperator);
        CachedAssignedPatient = CurrentPatient;

        ActivePathologyTag = FGameplayTag::EmptyTag;
        ActiveFlipbookTexture = nullptr;
        RemainingUltrasoundPathologies.Reset();
        BuildRemainingUltrasoundPathologies();
        if (CurrentOperator && CurrentPatient)
        {
            SelectActivePathology(false);
            ActiveFlipbookTexture = SelectRandomFlipbook();
        }
        RefreshTargetSocketForActivePathology();
        RandomizeSliderValues();
        UpdateScreenMaterial();
        UpdateSliderWidgetState();
        ActiveSliderIndex = 0;
        ForceNetUpdate();
        RefreshPatientCardWidget();
    }

    if (CurrentPatient && !bPatientOnTable)
    {
        TryAttachPatient();
    }

    if (CurrentOperator)
    {
        bool bProbeMovedThisTick = false;
        if (!bProbeLocked)
        {
            const FVector PreviousProbeWorldLocation = ProbeWorldLocation;
            const FRotator PreviousProbeWorldRotation = ProbeWorldRotation;
            UpdateProbePosition(DeltaSeconds);
            ValidateProbeTarget();
            bProbeMovedThisTick =
                !ProbeWorldLocation.Equals(PreviousProbeWorldLocation, KINDA_SMALL_NUMBER)
                || !ProbeWorldRotation.Equals(PreviousProbeWorldRotation, KINDA_SMALL_NUMBER);
        }

        UpdateMovementLoopState(bProbeMovedThisTick, DeltaSeconds);
        UpdateSliderPhase(DeltaSeconds);
    }
    else
    {
        MovementLoopIdleElapsedSeconds = 0.0f;
        SetMovementLoopActive(false);
    }

    if (bCompletionPending)
    {
        TryCompleteExam();
    }
}

void AEMRUltrasoundMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRUltrasoundMachine, CurrentOperator);
    DOREPLIFETIME(AEMRUltrasoundMachine, AttachedPatient);
    DOREPLIFETIME(AEMRUltrasoundMachine, TargetSocketName);
    DOREPLIFETIME(AEMRUltrasoundMachine, bProbeLocked);
    DOREPLIFETIME(AEMRUltrasoundMachine, ProbeWorldLocation);
    DOREPLIFETIME(AEMRUltrasoundMachine, ProbeWorldRotation);
    DOREPLIFETIME(AEMRUltrasoundMachine, SliderTargets);
    DOREPLIFETIME(AEMRUltrasoundMachine, SliderCurrents);
    DOREPLIFETIME(AEMRUltrasoundMachine, SliderNoiseAmount);
    DOREPLIFETIME(AEMRUltrasoundMachine, ActiveSliderIndex);
    DOREPLIFETIME(AEMRUltrasoundMachine, ActivePathologyTag);
    DOREPLIFETIME(AEMRUltrasoundMachine, ActiveFlipbookTexture);
    DOREPLIFETIME(AEMRUltrasoundMachine, bCompletionCueActive);
    DOREPLIFETIME(AEMRUltrasoundMachine, bMovementLoopActive);
}

bool AEMRUltrasoundMachine::ShouldReleasePatientOnExamCompleted(FGameplayTag ExamTag) const
{
    return !ExamTag.MatchesTagExact(EMRTags::Abilities::Exam::Ultrasound);
}

void AEMRUltrasoundMachine::TryStartExam(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    if (bUltrasoundTraceDebug)
    {
        UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] TryStartExam machine=%s operator=%s patient=%s"),
            *GetNameSafe(this), *GetNameSafe(Player), *GetNameSafe(CurrentPatient));
    }

    if (CurrentOperator && CurrentOperator != Player)
    {
        return;
    }

    if (CurrentPatient)
    {
        if (!ActivePathologyTag.IsValid())
        {
            if (!SelectActivePathology(false))
            {
                return;
            }
        }
        RefreshTargetSocketForActivePathology();
        if (TargetSocketName == NAME_None)
        {
            return;
        }
    }

    if (!ActiveFlipbookTexture)
    {
        ActiveFlipbookTexture = SelectRandomFlipbook();
        ForceNetUpdate();
    }

    CurrentOperator = Player;
    CachedOperatorController = Cast<AEMRPlayerController>(Player->GetController());
    LockOperator(Player);
    Player->Client_BeginUltrasoundExam(this, ExamInputMappingContext);

    if (CurrentPatient)
    {
        TryAttachPatient();
    }
}

void AEMRUltrasoundMachine::StopExam(AEMRPlayerCharacter* Player, bool bFromCancel)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    if (CurrentOperator != Player)
    {
        return;
    }

    UnlockOperator(Player);
    Player->Client_EndUltrasoundExam(this);

    DetachProbe();
    CurrentOperator = nullptr;
    ProbeInputAxis = FVector2D::ZeroVector;
    SliderAdjustInput = 0.0f;
    MovementLoopIdleElapsedSeconds = 0.0f;
    SetMovementLoopActive(false);
    if (!bFromCancel)
    {
        CachedOperatorController.Reset();
    }
}

void AEMRUltrasoundMachine::SetMoveInput(AEMRPlayerCharacter* Player, float InputX, float InputY)
{
    if (!HasAuthority() || !Player || CurrentOperator != Player)
    {
        return;
    }

    ProbeInputAxis.X = FMath::Clamp(InputX, -1.0f, 1.0f);
    ProbeInputAxis.Y = FMath::Clamp(InputY, -1.0f, 1.0f);
}

void AEMRUltrasoundMachine::SetActiveSlider(AEMRPlayerCharacter* Player, int32 SliderIndex)
{
    if (!HasAuthority() || !Player || CurrentOperator != Player || !bProbeLocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UltrasoundSliderSelect] Reject select machine=%s player=%s auth=%d currentOp=%s probeLocked=%d reqIndex=%d"),
            *GetNameSafe(this),
            *GetNameSafe(Player),
            HasAuthority() ? 1 : 0,
            *GetNameSafe(CurrentOperator),
            bProbeLocked ? 1 : 0,
            SliderIndex);
        return;
    }

    const int32 NewIndex = FMath::Clamp(SliderIndex, 0, 2);
    if (ActiveSliderIndex == NewIndex)
    {
        Multicast_PlaySliderSelectSound();
        UE_LOG(LogTemp, Log, TEXT("[UltrasoundSliderSelect] Slider already active machine=%s player=%s index=%d"),
            *GetNameSafe(this),
            *GetNameSafe(Player),
            NewIndex);
        return;
    }

    ActiveSliderIndex = NewIndex;
    Multicast_PlaySliderSelectSound();
    UE_LOG(LogTemp, Log, TEXT("[UltrasoundSliderSelect] Activated slider machine=%s player=%s index=%d"),
        *GetNameSafe(this),
        *GetNameSafe(Player),
        ActiveSliderIndex);
    ForceNetUpdate();
}

void AEMRUltrasoundMachine::SetSliderAdjustInput(AEMRPlayerCharacter* Player, float InputValue)
{
    if (!HasAuthority() || !Player || CurrentOperator != Player || !bProbeLocked)
    {
        return;
    }

    SliderAdjustInput = FMath::Clamp(InputValue, -1.0f, 1.0f);
}

void AEMRUltrasoundMachine::OnRep_CurrentOperator()
{
    if (CachedPreviousOperator.IsValid() && CachedPreviousOperator.Get() != CurrentOperator)
    {
    }

    if (CurrentOperator)
    {
        // Probe stays detached; no arm IK/physics linkage.
    }
    else
    {
        DetachProbe();
    }

    CachedPreviousOperator = CurrentOperator;
}

void AEMRUltrasoundMachine::OnRep_AttachedPatient()
{
    HandleAttachedPatientChanged();
}

void AEMRUltrasoundMachine::OnRep_TargetSocket()
{
}

void AEMRUltrasoundMachine::OnRep_ProbeLocked()
{
}

void AEMRUltrasoundMachine::OnRep_ProbeTransform()
{
    ApplyProbeTransform();
}

void AEMRUltrasoundMachine::OnRep_Sliders()
{
    UpdateScreenMaterial();
    UpdateSliderWidgetState();
    HandleLocalSliderTargetReachedSounds();
}

void AEMRUltrasoundMachine::OnRep_ActiveSlider()
{
    UpdateScreenMaterial();
    UpdateSliderWidgetState();
}

void AEMRUltrasoundMachine::OnRep_ActivePathology()
{
    UpdateSliderIndicatorState();
    ResetSliderTargetReachedState();
}

void AEMRUltrasoundMachine::OnRep_ActiveFlipbook()
{
    UpdateScreenMaterial();
}

void AEMRUltrasoundMachine::OnRep_MovementLoopActive()
{
    RefreshMovementLoopAudioPlayback();
}

void AEMRUltrasoundMachine::UpdateMovementLoopState(bool bProbeMovedThisTick, float DeltaSeconds)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bProbeMovedThisTick)
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

void AEMRUltrasoundMachine::SetMovementLoopActive(bool bNewActive)
{
    if (!HasAuthority() || bMovementLoopActive == bNewActive)
    {
        return;
    }

    bMovementLoopActive = bNewActive;
    ForceNetUpdate();
    RefreshMovementLoopAudioPlayback();
}

void AEMRUltrasoundMachine::RefreshMovementLoopAudioPlayback()
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

void AEMRUltrasoundMachine::LockOperator(AEMRPlayerCharacter* Player)
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
    Player->SetActorLocationAndRotation(
        LockTransform.GetLocation(),
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

void AEMRUltrasoundMachine::UnlockOperator(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    UArrowComponent* ExitComponent = OperatorExitPoint;
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

    if (UCharacterMovementComponent* MovementComponent = Player->GetCharacterMovement())
    {
        MovementComponent->SetMovementMode(MOVE_Walking);
    }
}

void AEMRUltrasoundMachine::AttachProbeToOperator(AEMRPlayerCharacter* Player)
{
    if (!ProbeMesh || !Player)
    {
        return;
    }

    USkeletalMeshComponent* PlayerMesh = Player->GetPlayerMeshComponentForSeating();
    if (!PlayerMesh)
    {
        PlayerMesh = Player->GetMesh();
    }

    if (!PlayerMesh)
    {
        return;
    }

    ProbeMesh->AttachToComponent(PlayerMesh, FAttachmentTransformRules::KeepRelativeTransform, ProbeAttachSocketName);
    ProbeMesh->SetRelativeLocationAndRotation(ProbeAttachOffset.GetLocation(), ProbeAttachOffset.Rotator());
    ApplyProbeTransform();
}

void AEMRUltrasoundMachine::DetachProbe()
{
    if (!ProbeMesh)
    {
        return;
    }

    USceneComponent* AttachParent = ProbeMesh->GetAttachParent();
    if (!AttachParent)
    {
        return;
    }

    const AActor* ParentOwner = AttachParent->GetOwner();
    if (ParentOwner && ParentOwner->IsA<AEMRPlayerCharacter>())
    {
        ProbeMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    }
}

void AEMRUltrasoundMachine::TryAttachPatient()
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
        AttachedPatient = CurrentPatient;
        ForceNetUpdate();
    }

    bPatientOnTable = true;
}

void AEMRUltrasoundMachine::DetachPatient(AEMRPatient* Patient)
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
    }

    CachedPatientTransformOwner.Reset();
    bHasCachedPatientTransform = false;

    if (HasAuthority() && AttachedPatient == Patient)
    {
        Multicast_StopPatientEnterMontage(Patient);
        AttachedPatient = nullptr;
        ForceNetUpdate();
    }

    if (UCharacterMovementComponent* MovementComponent = Patient->GetCharacterMovement())
    {
        MovementComponent->SetMovementMode(MOVE_Walking);
    }

    bPatientOnTable = false;
}

void AEMRUltrasoundMachine::PlayPatientEnterMontageFor(AEMRPatient* Patient)
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
            UE_LOG(LogTemp, Log, TEXT("[UltrasoundMachine] Playing patient enter montage %s on mesh %s"),
                *GetNameSafe(PatientEnterTableMontage), *GetNameSafe(TargetMesh));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[UltrasoundMachine] Patient mesh %s has no anim instance for montage %s"),
                *GetNameSafe(TargetMesh), *GetNameSafe(PatientEnterTableMontage));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[UltrasoundMachine] No patient mesh available to play montage %s"),
            *GetNameSafe(PatientEnterTableMontage));
    }
}

void AEMRUltrasoundMachine::StopPatientEnterMontageFor(AEMRPatient* Patient)
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
            UE_LOG(LogTemp, Log, TEXT("[UltrasoundMachine] Stopping patient montage %s on mesh %s"),
                *GetNameSafe(PatientEnterTableMontage), *GetNameSafe(TargetMesh));
        }
    }
}

void AEMRUltrasoundMachine::HandleAttachedPatientChanged()
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

bool AEMRUltrasoundMachine::TryPlayAttachedPatientMontage()
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

void AEMRUltrasoundMachine::Multicast_PlayPatientEnterMontage_Implementation(AEMRPatient* Patient)
{
    PlayPatientEnterMontageFor(Patient);
}

void AEMRUltrasoundMachine::Multicast_StopPatientEnterMontage_Implementation(AEMRPatient* Patient)
{
    StopPatientEnterMontageFor(Patient);
}

void AEMRUltrasoundMachine::RefreshTargetSocket()
{
    RefreshTargetSocketForActivePathology();
}

UStaticMeshComponent* AEMRUltrasoundMachine::ResolveSkinProxyComponent()
{
    if (!CurrentPatient)
    {
        return nullptr;
    }

    if (CachedSkinProxyComponent.IsValid())
    {
        if (CachedSkinProxyComponent->GetOwner() == CurrentPatient)
        {
            return CachedSkinProxyComponent.Get();
        }
        CachedSkinProxyComponent.Reset();
    }

    UStaticMeshComponent* Proxy = CurrentPatient->GetOrCreateUltrasoundSkinProxyComponent();
    if (Proxy)
    {
        if (bUltrasoundTraceDebug)
        {
            const FBoxSphereBounds Bounds = Proxy->Bounds;
            UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] Resolved proxy component %s for patient %s mesh=%s loc=%s boundsOrigin=%s boundsExtent=%s collision=%s"),
                *GetNameSafe(Proxy),
                *GetNameSafe(CurrentPatient),
                *GetNameSafe(Proxy->GetStaticMesh()),
                *Proxy->GetComponentLocation().ToCompactString(),
                *Bounds.Origin.ToCompactString(),
                *Bounds.BoxExtent.ToCompactString(),
                *UEnum::GetValueAsString(Proxy->GetCollisionEnabled()));
        }
        CachedSkinProxyComponent = Proxy;
        return Proxy;
    }

    if (bUltrasoundTraceDebug)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UltrasoundTrace] Failed to resolve proxy component for patient %s."),
            *GetNameSafe(CurrentPatient));
    }
    return nullptr;
}

bool AEMRUltrasoundMachine::TraceProbeSurface(const FTransform& AnchorTransform, const FVector& DesiredPlanePoint, FHitResult& OutHit) const
{
    UStaticMeshComponent* Proxy = CachedSkinProxyComponent.Get();
    if (!Proxy)
    {
        return false;
    }

    const FVector NormalAxis = AnchorTransform.GetUnitAxis(ProbeSurfaceNormalAxis);
    const FVector TraceStart = DesiredPlanePoint + NormalAxis * ProbeSurfaceTraceDistance;
    const FVector TraceEnd = DesiredPlanePoint - NormalAxis * ProbeSurfaceTraceDistance;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(UltrasoundProbeSurfaceTrace), true);
    Params.bTraceComplex = true;
    const bool bHit = Proxy->LineTraceComponent(OutHit, TraceStart, TraceEnd, Params);
    if (bUltrasoundTraceDebug)
    {
        UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] Line hit=%d start=%s end=%s impact=%s normal=%s"),
            bHit,
            *TraceStart.ToCompactString(),
            *TraceEnd.ToCompactString(),
            *OutHit.ImpactPoint.ToCompactString(),
            *OutHit.ImpactNormal.ToCompactString());
    }
    if (bHit)
    {
        return true;
    }

    if (ProbeSurfaceSweepRadius > KINDA_SMALL_NUMBER)
    {
        const bool bSweepHit = Proxy->SweepComponent(
            OutHit,
            TraceStart,
            TraceEnd,
            FQuat::Identity,
            FCollisionShape::MakeSphere(ProbeSurfaceSweepRadius),
            true);
        if (bUltrasoundTraceDebug)
        {
            UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] Sweep hit=%d start=%s end=%s impact=%s normal=%s"),
                bSweepHit,
                *TraceStart.ToCompactString(),
                *TraceEnd.ToCompactString(),
                *OutHit.ImpactPoint.ToCompactString(),
                *OutHit.ImpactNormal.ToCompactString());
        }
        if (bSweepHit)
        {
            return true;
        }
    }

    return false;
}

FRotator AEMRUltrasoundMachine::MakeProbeSurfaceRotation(const FTransform& AnchorTransform, const FVector& SurfaceNormal) const
{
    const FVector Normal = SurfaceNormal.GetSafeNormal();
    FVector Tangent = AnchorTransform.GetUnitAxis(ProbeSurfaceTangentAxis);
    Tangent -= FVector::DotProduct(Tangent, Normal) * Normal;

    if (Tangent.SizeSquared() < KINDA_SMALL_NUMBER)
    {
        const EAxis::Type AlternateAxis =
            (ProbeSurfaceTangentAxis == EAxis::X) ? EAxis::Y
            : (ProbeSurfaceTangentAxis == EAxis::Y) ? EAxis::Z
            : EAxis::X;
        Tangent = AnchorTransform.GetUnitAxis(AlternateAxis);
        Tangent -= FVector::DotProduct(Tangent, Normal) * Normal;
    }

    if (Tangent.SizeSquared() < KINDA_SMALL_NUMBER)
    {
        FVector FallbackTangent;
        FVector FallbackBinormal;
        Normal.FindBestAxisVectors(FallbackTangent, FallbackBinormal);
        Tangent = FallbackTangent;
    }

    Tangent.Normalize();
    const FQuat SurfaceQuat = FRotationMatrix::MakeFromXY(Normal, Tangent).ToQuat();
    const FQuat OffsetQuat = ProbeSurfaceRotationOffset.Quaternion();
    return (SurfaceQuat * OffsetQuat).Rotator();
}

void AEMRUltrasoundMachine::UpdateProbePosition(float DeltaSeconds)
{
    if (!CurrentPatient)
    {
        return;
    }

    if (!ProbeInputAxis.IsNearlyZero())
    {
        ProbeLocalOffset.X -= ProbeInputAxis.X * ProbeMoveSpeed * DeltaSeconds;
        ProbeLocalOffset.Y += ProbeInputAxis.Y * ProbeMoveSpeed * DeltaSeconds;

        if (bClampProbeToLocalBounds)
        {
            ProbeLocalOffset.X = FMath::Clamp(ProbeLocalOffset.X, -ProbeLocalBounds.X, ProbeLocalBounds.X);
            ProbeLocalOffset.Y = FMath::Clamp(ProbeLocalOffset.Y, -ProbeLocalBounds.Y, ProbeLocalBounds.Y);
        }
    }

    const FTransform AnchorTransform = PatientAttachComponent
    ? PatientAttachComponent->GetComponentTransform()
    : GetActorTransform();

    const FVector DesiredPlanePoint = AnchorTransform.GetLocation()
    + AnchorTransform.GetUnitAxis(EAxis::Y) * ProbeLocalOffset.X
    + AnchorTransform.GetUnitAxis(EAxis::Z) * ProbeLocalOffset.Y
    + AnchorTransform.GetUnitAxis(EAxis::X) * ProbeSurfaceTraceHeight;

    UStaticMeshComponent* Proxy = ResolveSkinProxyComponent();
    if (!Proxy && !bHasWarnedMissingProxy)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UltrasoundMachine] Missing ultrasound skin proxy mesh for patient %s."), *GetNameSafe(CurrentPatient));
        bHasWarnedMissingProxy = true;
    }

    if (bUltrasoundTraceDebug)
    {
        UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] UpdateProbePosition offset=%s desired=%s"),
            *ProbeLocalOffset.ToString(),
            *DesiredPlanePoint.ToCompactString());
    }

    FHitResult SurfaceHit;
    bool bHitSurface = false;
    if (Proxy)
    {
        bHitSurface = TraceProbeSurface(AnchorTransform, DesiredPlanePoint, SurfaceHit);
    }
    else if (USkeletalMeshComponent* PatientMesh = ResolvePatientMesh())
    {
        FVector ClosestPoint = FVector::ZeroVector;
        const float Distance = PatientMesh->GetClosestPointOnCollision(DesiredPlanePoint, ClosestPoint);
        if (Distance > KINDA_SMALL_NUMBER)
        {
            SurfaceHit.ImpactPoint = ClosestPoint;
            SurfaceHit.ImpactNormal = (DesiredPlanePoint - ClosestPoint).GetSafeNormal();
            bHitSurface = !SurfaceHit.ImpactNormal.IsNearlyZero();
        }
        if (bUltrasoundTraceDebug)
        {
            UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] Fallback closest-point hit=%d point=%s normal=%s dist=%.3f"),
                bHitSurface,
                *SurfaceHit.ImpactPoint.ToCompactString(),
                *SurfaceHit.ImpactNormal.ToCompactString(),
                Distance);
        }
    }

    if (!bHitSurface)
    {
        if (bUltrasoundTraceDebug)
        {
            UE_LOG(LogTemp, Warning, TEXT("[UltrasoundTrace] No surface hit for patient %s."), *GetNameSafe(CurrentPatient));
        }
        return;
    }

    FVector SurfaceNormal = SurfaceHit.ImpactNormal;
    if (SurfaceNormal.IsNearlyZero())
    {
        SurfaceNormal = SurfaceHit.Normal;
    }
    if (SurfaceNormal.IsNearlyZero())
    {
        SurfaceNormal = AnchorTransform.GetUnitAxis(ProbeSurfaceNormalAxis);
    }

    const FVector RawContactPoint = SurfaceHit.ImpactPoint;
    const FVector RawSurfaceNormal = SurfaceNormal.GetSafeNormal();

    LastProbeContactPoint = RawContactPoint;
    LastProbeSurfaceNormal = RawSurfaceNormal;
    if (!bHasProbeSurfaceHit)
    {
        SmoothedProbeContactPoint = RawContactPoint;
        SmoothedProbeSurfaceNormal = RawSurfaceNormal;
    }
    else
    {
        if (ProbeSurfacePositionInterpSpeed > 0.0f)
        {
            SmoothedProbeContactPoint = FMath::VInterpTo(
                SmoothedProbeContactPoint,
                RawContactPoint,
                DeltaSeconds,
                ProbeSurfacePositionInterpSpeed);
        }
        else
        {
            SmoothedProbeContactPoint = RawContactPoint;
        }

        if (ProbeSurfaceNormalInterpSpeed > 0.0f)
        {
            SmoothedProbeSurfaceNormal = FMath::VInterpTo(
                SmoothedProbeSurfaceNormal,
                RawSurfaceNormal,
                DeltaSeconds,
                ProbeSurfaceNormalInterpSpeed).GetSafeNormal();
        }
        else
        {
            SmoothedProbeSurfaceNormal = RawSurfaceNormal;
        }
    }
    bHasProbeSurfaceHit = true;

    const FVector UsedContactPoint = SmoothedProbeContactPoint;
    const FVector UsedNormal = SmoothedProbeSurfaceNormal.IsNearlyZero()
    ? RawSurfaceNormal
    : SmoothedProbeSurfaceNormal;
    const FVector NewLocation = UsedContactPoint + UsedNormal * ProbeSurfaceOffset;
    const FRotator NewRotation = MakeProbeSurfaceRotation(AnchorTransform, UsedNormal);
    if (!NewLocation.Equals(ProbeWorldLocation, KINDA_SMALL_NUMBER)
        || !NewRotation.Equals(ProbeWorldRotation, KINDA_SMALL_NUMBER))
    {
        ProbeWorldLocation = NewLocation;
        ProbeWorldRotation = NewRotation;
        ApplyProbeTransform();
    }

    if (bUltrasoundTraceDebug)
    {
        UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] Probe loc=%s rot=%s normal=%s raw=%s"),
            *ProbeWorldLocation.ToCompactString(),
            *ProbeWorldRotation.ToCompactString(),
            *UsedNormal.ToCompactString(),
            *RawSurfaceNormal.ToCompactString());
    }
}

void AEMRUltrasoundMachine::ApplyProbeTransform()
{
    if (!ProbeMesh)
    {
        return;
    }

    if (CurrentOperator && CurrentOperator->IsLocallyControlled())
    {
        USkeletalMeshComponent* PlayerMesh = CurrentOperator->GetPlayerMeshComponentForSeating();
        if (!PlayerMesh)
        {
            PlayerMesh = CurrentOperator->GetMesh();
        }

        if (PlayerMesh && ProbeMesh->GetAttachParent() == PlayerMesh)
        {
            return;
        }
    }

    ProbeMesh->SetWorldLocationAndRotation(
        ProbeWorldLocation,
        ProbeWorldRotation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
}

void AEMRUltrasoundMachine::ValidateProbeTarget()
{
    if (bProbeLocked || !CurrentPatient || TargetSocketName == NAME_None || !bHasProbeSurfaceHit)
    {
        return;
    }

    USkeletalMeshComponent* PatientMesh = ResolvePatientMesh();
    if (!PatientMesh || !PatientMesh->DoesSocketExist(TargetSocketName))
    {
        return;
    }

    const FVector SocketLocation = PatientMesh->GetSocketLocation(TargetSocketName);
    const float DistSq = FVector::DistSquared(SocketLocation, LastProbeContactPoint);
    if (bUltrasoundTraceDebug)
    {
        UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] Target check socket=%s dist=%.3f tol=%.3f"),
            *SocketLocation.ToCompactString(),
            FMath::Sqrt(DistSq),
            TargetSocketToleranceRadius);
    }
    if (DistSq <= FMath::Square(TargetSocketToleranceRadius))
    {
        const FTransform AnchorTransform = PatientAttachComponent
            ? PatientAttachComponent->GetComponentTransform()
            : GetActorTransform();

        FVector SurfaceNormal = LastProbeSurfaceNormal;
        if (SurfaceNormal.IsNearlyZero())
        {
            SurfaceNormal = AnchorTransform.GetUnitAxis(ProbeSurfaceNormalAxis);
        }

        bProbeLocked = true;
        ProbeWorldLocation = LastProbeContactPoint + SurfaceNormal.GetSafeNormal() * ProbeSurfaceOffset;
        ProbeWorldRotation = MakeProbeSurfaceRotation(AnchorTransform, SurfaceNormal);
        ApplyProbeTransform();
        ForceNetUpdate();
        Multicast_PlayWorldSoundAtLocation(TargetFoundSound, GetActorLocation());
        if (bUltrasoundTraceDebug)
        {
            UE_LOG(LogTemp, Log, TEXT("[UltrasoundTrace] Probe locked at %s"), *ProbeWorldLocation.ToCompactString());
        }
    }
}

void AEMRUltrasoundMachine::UpdateOperatorHandTarget()
{
    // No-op: arm IK/physics is disabled for ultrasound.
}

void AEMRUltrasoundMachine::UpdateSliderPhase(float DeltaSeconds)
{
    if (!bProbeLocked)
    {
        return;
    }

    bool bStateChanged = false;
    const bool bActiveLocked = IsSliderLocked(ActiveSliderIndex);

    if (!bActiveLocked && !FMath::IsNearlyZero(SliderAdjustInput))
    {
        const float Delta = SliderAdjustInput * SliderAdjustSpeed * DeltaSeconds;
        float CurrentValue = GetSliderValueByIndex(SliderCurrents, ActiveSliderIndex);
        const float NewValue = FMath::Clamp(CurrentValue + Delta, SliderMinValue, SliderMaxValue);
        if (!FMath::IsNearlyEqual(CurrentValue, NewValue))
        {
            SetSliderValueByIndex(SliderCurrents, ActiveSliderIndex, NewValue);
            bStateChanged = true;
        }
    }

    const float ActiveTarget = GetSliderValueByIndex(SliderTargets, ActiveSliderIndex);
    const float ActiveCurrent = GetSliderValueByIndex(SliderCurrents, ActiveSliderIndex);
    if (FMath::Abs(ActiveCurrent - ActiveTarget) <= SliderTolerance
        && !FMath::IsNearlyEqual(ActiveCurrent, ActiveTarget))
    {
        SetSliderValueByIndex(SliderCurrents, ActiveSliderIndex, ActiveTarget);
        bStateChanged = true;
    }

    const float TargetNoise = ComputeSliderNoise();
    if (!bSliderNoiseSmoothingActive)
    {
        bSliderNoiseSmoothingActive = bStateChanged || !FMath::IsNearlyZero(SliderAdjustInput);
    }

    if (bSliderNoiseSmoothingActive)
    {
        const float NewNoise = FMath::FInterpTo(SliderNoiseAmount, TargetNoise, DeltaSeconds, SliderNoiseInterpSpeed);
        if (!FMath::IsNearlyEqual(NewNoise, SliderNoiseAmount))
        {
            SliderNoiseAmount = NewNoise;
            bStateChanged = true;
        }

        if (FMath::IsNearlyEqual(SliderNoiseAmount, TargetNoise, KINDA_SMALL_NUMBER))
        {
            bSliderNoiseSmoothingActive = false;
        }
    }

    if (bStateChanged)
    {
        UpdateScreenMaterial();
        UpdateSliderWidgetState();
        HandleLocalSliderTargetReachedSounds();
        ForceNetUpdate();
    }

    if (HasAuthority())
    {
        TryCompleteExam();
    }
}

void AEMRUltrasoundMachine::UpdateScreenMaterial()
{
    if (!MonitorMaterialInstance)
    {
        InitializeScreenMaterial();
    }

    if (!MonitorMaterialInstance)
    {
        return;
    }

    if (ActiveFlipbookTexture)
    {
        MonitorMaterialInstance->SetTextureParameterValue(FlipbookTextureParamName, ActiveFlipbookTexture);
    }
    else if (DefaultUltrasoundScreenTexture)
    {
        MonitorMaterialInstance->SetTextureParameterValue(FlipbookTextureParamName, DefaultUltrasoundScreenTexture);
    }
    MonitorMaterialInstance->SetScalarParameterValue(NoiseParamName, SliderNoiseAmount);
    MonitorMaterialInstance->SetScalarParameterValue(Slider1ParamName, SliderCurrents.X);
    MonitorMaterialInstance->SetScalarParameterValue(Slider2ParamName, SliderCurrents.Y);
    MonitorMaterialInstance->SetScalarParameterValue(Slider3ParamName, SliderCurrents.Z);
}

void AEMRUltrasoundMachine::TryCompleteExam()
{
    if (!HasAuthority() || !CurrentPatient || !bProbeLocked)
    {
        return;
    }

    if (bCompletionCueActive)
    {
        return;
    }

    const bool bSolved =
    FMath::Abs(SliderCurrents.X - SliderTargets.X) <= SliderTolerance
    && FMath::Abs(SliderCurrents.Y - SliderTargets.Y) <= SliderTolerance
    && FMath::Abs(SliderCurrents.Z - SliderTargets.Z) <= SliderTolerance;

    if (!bSolved)
    {
        return;
    }

    const bool bHasMorePathologies = ActivePathologyTag.IsValid()
    ? RemainingUltrasoundPathologies.Num() > 1
    : false;

    if (bHasMorePathologies)
    {
        if (CompletionSound && CurrentOperator)
        {
            if (BeginCompletionCue(CurrentOperator))
            {
                return;
            }
        }

        if (!TryAdvanceToNextPathology())
        {
            return;
        }
        return;
    }

    FGameplayEventData EventData;
    EventData.EventTag = EMRTags::Abilities::Exam::Ultrasound;
    EventData.Target = CurrentPatient;
    EventData.OptionalObject = this;
    EventData.EventMagnitude = 1.0f;

    AEMRPlayerController* PlayerController = nullptr;
    if (CurrentOperator)
    {
        PlayerController = Cast<AEMRPlayerController>(CurrentOperator->GetController());
    }

    if (!PlayerController)
    {
        PlayerController = CachedOperatorController.Get();
    }

    AEMRPlayerCharacter* DispatchOperator = CurrentOperator.Get();
    if (!DispatchOperator && PlayerController)
    {
        DispatchOperator = Cast<AEMRPlayerCharacter>(PlayerController->GetPawn());
    }

    if (!TrySendExamGameplayEvent(DispatchOperator, EventData))
    {
        bCompletionPending = true;
        return;
    }

    bCompletionPending = false;

    if (CompletionSound && CurrentOperator)
    {
        if (BeginCompletionCue(CurrentOperator))
        {
            return;
        }
    }

    if (!TryAdvanceToNextPathology())
    {
        AEMRPatient* CompletedPatient = CurrentPatient;
        if (bPatientOnTable && CompletedPatient)
        {
            DetachPatient(CompletedPatient);
        }

        ReleasePatient();

        // Operator exits only via cancel input.
    }
}

bool AEMRUltrasoundMachine::BeginCompletionCue(AEMRPlayerCharacter* Operator)
{
    if (!HasAuthority() || !Operator || bCompletionCueActive)
    {
        return false;
    }

    if (!CompletionSound)
    {
        return false;
    }

    bCompletionCueActive = true;
    CompletionCueOperator = Operator;

    const FVector SoundLocation = CurrentPatient ? CurrentPatient->GetActorLocation() : GetActorLocation();
    Multicast_PlayWorldSoundAtLocation(CompletionSound, SoundLocation);

    if (CompletionCueDuration <= KINDA_SMALL_NUMBER)
    {
        HandleCompletionCueFinished(Operator, false);
        return true;
    }

    GetWorldTimerManager().ClearTimer(CompletionCueTimer);
    GetWorldTimerManager().SetTimer(
        CompletionCueTimer,
        FTimerDelegate::CreateUObject(this, &ThisClass::HandleCompletionCueFinished, Operator, false),
        CompletionCueDuration,
        false);
    return true;
}

void AEMRUltrasoundMachine::HandleCompletionCueFinished(AEMRPlayerCharacter* Operator, bool bCanceled)
{
    if (!HasAuthority() || !bCompletionCueActive || CompletionCueOperator.Get() != Operator)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(CompletionCueTimer);

    bCompletionCueActive = false;
    CompletionCueOperator.Reset();
    bCompletionPending = false;

    if (!TryAdvanceToNextPathology())
    {
        AEMRPatient* CompletedPatient = CurrentPatient;
        if (bPatientOnTable && CompletedPatient)
        {
            DetachPatient(CompletedPatient);
        }

        ReleasePatient();

        // Operator exits only via cancel input.
    }
}

void AEMRUltrasoundMachine::ResetExamState(bool bKeepOperator)
{
    CachedTargetMesh.Reset();
    CachedSkinProxyComponent.Reset();
    SmoothedProbeContactPoint = FVector::ZeroVector;
    SmoothedProbeSurfaceNormal = FVector::UpVector;
    LastProbeContactPoint = FVector::ZeroVector;
    LastProbeSurfaceNormal = FVector::UpVector;
    bHasProbeSurfaceHit = false;
    bHasWarnedMissingProxy = false;

    if (bCompletionCueActive)
    {
        GetWorldTimerManager().ClearTimer(CompletionCueTimer);
        bCompletionCueActive = false;
        CompletionCueOperator.Reset();
    }

    if (CurrentOperator && !bKeepOperator)
    {
        StopExam(CurrentOperator, false);
    }

    if (bPatientOnTable && CachedAssignedPatient.IsValid())
    {
        DetachPatient(CachedAssignedPatient.Get());
    }

    TargetSocketName = NAME_None;
    bProbeLocked = false;
    bCompletionPending = false;
    ActivePathologyTag = FGameplayTag::EmptyTag;
    ActiveFlipbookTexture = nullptr;
    ProbeInputAxis = FVector2D::ZeroVector;
    ProbeLocalOffset = FVector2D::ZeroVector;
    SliderAdjustInput = 0.0f;
    SliderTargets = FVector::ZeroVector;
    SliderCurrents = FVector(SliderMinValue, SliderMinValue, SliderMinValue);
    SliderStartValues = SliderCurrents;
    SliderInitialDeltas = FVector::ZeroVector;
    SliderNoiseAmount = ComputeSliderNoise();
    bSliderNoiseSmoothingActive = false;
    ActiveSliderIndex = 0;
    MovementLoopIdleElapsedSeconds = 0.0f;
    SetMovementLoopActive(false);
    ResetSliderTargetReachedState();

    if (HasAuthority())
    {
        ForceNetUpdate();
    }

    CachedOperatorController.Reset();
}

void AEMRUltrasoundMachine::BuildRemainingUltrasoundPathologies()
{
    RemainingUltrasoundPathologies.Reset();
    if (!CurrentPatient)
    {
        return;
    }

    const UEMRPatientData* PatientData = CurrentPatient->GetPatientData();
    if (!PatientData)
    {
        return;
    }

    const FGameplayTagContainer Pathologies = PatientData->GetPathologies();
    if (Pathologies.IsEmpty())
    {
        return;
    }

    TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
    UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);
    if (LoadedSubsystemConfig.IsEmpty())
    {
        return;
    }

    const UDataTable* UltrasoundTargetsTable = LoadedSubsystemConfig[0]->GetUltrasoundTargetsFromPathologyTable();
    if (!UltrasoundTargetsTable)
    {
        return;
    }

    TSet<FGameplayTag> UniquePathologies;
    TArray<FEMRUltrasoundTargetData*> Rows;
    UltrasoundTargetsTable->GetAllRows(TEXT("UltrasoundTargets"), Rows);
    for (const FEMRUltrasoundTargetData* Row : Rows)
    {
        if (!Row || !Row->PathologyTag.IsValid())
        {
            continue;
        }

        if (!Pathologies.HasTagExact(Row->PathologyTag))
        {
            continue;
        }

        UniquePathologies.Add(Row->PathologyTag);
    }

    if (UniquePathologies.IsEmpty())
    {
        if (AppendFallbackUltrasoundPathologyFromRow(Rows, UniquePathologies))
        {
            UE_LOG(LogTemp, Warning, TEXT("[UltrasoundMachine] BuildRemainingUltrasoundPathologies: using fallback pathology for optional exam (patient=%s)"),
                *GetNameSafe(CurrentPatient));
        }
    }

    for (const FGameplayTag& Tag : UniquePathologies)
    {
        RemainingUltrasoundPathologies.Add(Tag);
    }
}

bool AEMRUltrasoundMachine::SelectActivePathology(bool bForceNew)
{
    if (!HasAuthority())
    {
        return false;
    }

    if (!bForceNew && ActivePathologyTag.IsValid())
    {
        return true;
    }

    if (RemainingUltrasoundPathologies.IsEmpty())
    {
        ActivePathologyTag = FGameplayTag::EmptyTag;
        ResetSliderTargetReachedState();
        return false;
    }

    const int32 Index = FMath::RandRange(0, RemainingUltrasoundPathologies.Num() - 1);
    ActivePathologyTag = RemainingUltrasoundPathologies[Index];
    ResetSliderTargetReachedState();
    RefreshTargetSocketForActivePathology();
    ForceNetUpdate();
    return true;
}

void AEMRUltrasoundMachine::RefreshTargetSocketForActivePathology()
{
    if (!HasAuthority())
    {
        return;
    }

    TargetSocketName = NAME_None;
    bProbeLocked = false;
    CachedTargetMesh.Reset();

    if (!CurrentPatient || !ActivePathologyTag.IsValid())
    {
        return;
    }

    TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;
    UEMRAssetManager::Get().CollectLoadedSubsystemConfig(LoadedSubsystemConfig);
    if (LoadedSubsystemConfig.IsEmpty())
    {
        return;
    }

    const UDataTable* UltrasoundTargetsTable = LoadedSubsystemConfig[0]->GetUltrasoundTargetsFromPathologyTable();
    if (!UltrasoundTargetsTable)
    {
        return;
    }

    TSet<FName> UniqueSockets;
    TArray<FEMRUltrasoundTargetData*> Rows;
    UltrasoundTargetsTable->GetAllRows(TEXT("UltrasoundTargets"), Rows);
    for (const FEMRUltrasoundTargetData* Row : Rows)
    {
        if (!Row || !Row->PathologyTag.IsValid())
        {
            continue;
        }

        if (!ActivePathologyTag.MatchesTagExact(Row->PathologyTag))
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

    USkeletalMeshComponent* PatientMesh = ResolvePatientMesh();
    if (!PatientMesh)
    {
        return;
    }

    TArray<FName> ValidSockets;
    ValidSockets.Reserve(UniqueSockets.Num());
    for (const FName& SocketName : UniqueSockets)
    {
        if (PatientMesh->DoesSocketExist(SocketName))
        {
            ValidSockets.Add(SocketName);
        }
    }

    if (ValidSockets.IsEmpty())
    {
        return;
    }

    const int32 Index = FMath::RandRange(0, ValidSockets.Num() - 1);
    TargetSocketName = ValidSockets[Index];
    ForceNetUpdate();
}

UTexture2D* AEMRUltrasoundMachine::SelectRandomFlipbook() const
{
    TArray<UTexture2D*> Valid;
    for (UTexture2D* Texture : UltrasoundFlipbooks)
    {
        if (Texture)
        {
            Valid.Add(Texture);
        }
    }

    if (Valid.IsEmpty())
    {
        return DefaultUltrasoundScreenTexture;
    }

    const int32 Index = FMath::RandRange(0, Valid.Num() - 1);
    return Valid[Index];
}

void AEMRUltrasoundMachine::ResetExamForNextPathology()
{
    CachedTargetMesh.Reset();
    CachedSkinProxyComponent.Reset();
    SmoothedProbeContactPoint = FVector::ZeroVector;
    SmoothedProbeSurfaceNormal = FVector::UpVector;
    LastProbeContactPoint = FVector::ZeroVector;
    LastProbeSurfaceNormal = FVector::UpVector;
    bHasProbeSurfaceHit = false;
    bHasWarnedMissingProxy = false;

    TargetSocketName = NAME_None;
    bProbeLocked = false;
    bCompletionPending = false;
    ProbeInputAxis = FVector2D::ZeroVector;
    ProbeLocalOffset = FVector2D::ZeroVector;
    SliderAdjustInput = 0.0f;
    RandomizeSliderValues();
    ActiveSliderIndex = 0;
    ActiveFlipbookTexture = SelectRandomFlipbook();
    MovementLoopIdleElapsedSeconds = 0.0f;
    SetMovementLoopActive(false);

    RefreshTargetSocketForActivePathology();
    UpdateSliderWidgetState();
    UpdateScreenMaterial();
    ResetSliderTargetReachedState();
    ForceNetUpdate();
}

bool AEMRUltrasoundMachine::TryAdvanceToNextPathology()
{
    if (!HasAuthority() || !CurrentPatient)
    {
        return false;
    }

    if (ActivePathologyTag.IsValid())
    {
        RemainingUltrasoundPathologies.Remove(ActivePathologyTag);
    }

    ActivePathologyTag = FGameplayTag::EmptyTag;

    if (RemainingUltrasoundPathologies.IsEmpty())
    {
        return false;
    }

    SelectActivePathology(true);
    ResetExamForNextPathology();
    return true;
}

bool AEMRUltrasoundMachine::TrySendExamGameplayEvent(AEMRPlayerCharacter* Operator, const FGameplayEventData& EventData)
{
    if (!Operator)
    {
        return false;
    }

    UAbilitySystemComponent* OperatorASC = Operator->GetAbilitySystemComponent();
    if (!OperatorASC)
    {
        return false;
    }

    FGameplayEventData EventPayload = EventData;
    if (!IsValid(EventPayload.Instigator))
    {
        EventPayload.Instigator = Operator;
    }

    const int32 TriggeredAbilities = OperatorASC->HandleGameplayEvent(EventPayload.EventTag, &EventPayload);
    return TriggeredAbilities > 0;
}

USkeletalMeshComponent* AEMRUltrasoundMachine::ResolvePatientMesh() const
{
    return ResolvePatientMeshFor(CurrentPatient);
}

USkeletalMeshComponent* AEMRUltrasoundMachine::ResolvePatientMeshFor(const AEMRPatient* Patient) const
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

void AEMRUltrasoundMachine::InitializeScreenMaterial()
{
    if (!MonitorScreenMesh || !UltrasoundScreenMaterial)
    {
        return;
    }

    MonitorMaterialInstance = MonitorScreenMesh->CreateDynamicMaterialInstance(0, UltrasoundScreenMaterial);
    UpdateScreenMaterial();
}

void AEMRUltrasoundMachine::RandomizeSliderValues()
{
    ResetSliderTargetReachedState();

    if (!CurrentPatient)
    {
        SliderTargets = FVector::ZeroVector;
        SliderCurrents = FVector(SliderMinValue, SliderMinValue, SliderMinValue);
        SliderStartValues = SliderCurrents;
        SliderInitialDeltas = FVector::ZeroVector;
        SliderNoiseAmount = ComputeSliderNoise();
        bSliderNoiseSmoothingActive = false;
        return;
    }

    SliderStartValues = FVector(
        FMath::FRandRange(SliderMinValue, SliderMaxValue),
        FMath::FRandRange(SliderMinValue, SliderMaxValue),
        FMath::FRandRange(SliderMinValue, SliderMaxValue));
    SliderCurrents = SliderStartValues;
    SliderTargets = FVector(
        PickRandomSliderTarget(SliderStartValues.X),
        PickRandomSliderTarget(SliderStartValues.Y),
        PickRandomSliderTarget(SliderStartValues.Z));

    SliderInitialDeltas = FVector(
        FMath::Abs(SliderTargets.X - SliderStartValues.X),
        FMath::Abs(SliderTargets.Y - SliderStartValues.Y),
        FMath::Abs(SliderTargets.Z - SliderStartValues.Z));
    SliderInitialDeltas.X = FMath::Max(SliderInitialDeltas.X, KINDA_SMALL_NUMBER);
    SliderInitialDeltas.Y = FMath::Max(SliderInitialDeltas.Y, KINDA_SMALL_NUMBER);
    SliderInitialDeltas.Z = FMath::Max(SliderInitialDeltas.Z, KINDA_SMALL_NUMBER);

    SliderNoiseAmount = 1.0f;
    bSliderNoiseSmoothingActive = false;
}

float AEMRUltrasoundMachine::GetMinSliderTargetDistance() const
{
    const float Range = FMath::Max(0.0f, SliderMaxValue - SliderMinValue);
    const float Factor = FMath::Clamp(SliderTargetMinDistanceFactor, 0.0f, 1.0f);
    return Factor * Range;
}

float AEMRUltrasoundMachine::PickRandomSliderTarget(float StartValue) const
{
    const float MinDistance = GetMinSliderTargetDistance();
    const float MinValue = SliderMinValue;
    const float MaxValue = SliderMaxValue;
    const float LowerMax = StartValue - MinDistance;
    const float UpperMin = StartValue + MinDistance;
    const bool bHasLower = LowerMax >= MinValue;
    const bool bHasUpper = UpperMin <= MaxValue;

    if (bHasLower && bHasUpper)
    {
        const float LowerLen = LowerMax - MinValue;
        const float UpperLen = MaxValue - UpperMin;
        const float Total = LowerLen + UpperLen;
        if (Total <= KINDA_SMALL_NUMBER)
        {
            return FMath::Clamp(StartValue + MinDistance, MinValue, MaxValue);
        }

        const float Pick = FMath::FRandRange(0.0f, Total);
        if (Pick <= LowerLen)
        {
            return FMath::FRandRange(MinValue, LowerMax);
        }

        return FMath::FRandRange(UpperMin, MaxValue);
    }

    if (bHasLower)
    {
        return FMath::FRandRange(MinValue, LowerMax);
    }

    if (bHasUpper)
    {
        return FMath::FRandRange(UpperMin, MaxValue);
    }

    return FMath::Clamp(StartValue + MinDistance, MinValue, MaxValue);
}

float AEMRUltrasoundMachine::GetSliderValueByIndex(const FVector& Values, int32 Index) const
{
    switch (Index)
    {
    case 0:
        return Values.X;
    case 1:
        return Values.Y;
    case 2:
        return Values.Z;
    default:
        return Values.X;
    }
}

void AEMRUltrasoundMachine::SetSliderValueByIndex(FVector& Values, int32 Index, float Value) const
{
    switch (Index)
    {
    case 0:
        Values.X = Value;
        break;
    case 1:
        Values.Y = Value;
        break;
    case 2:
        Values.Z = Value;
        break;
    default:
        Values.X = Value;
        break;
    }
}

bool AEMRUltrasoundMachine::IsSliderLocked(int32 Index) const
{
    const float Current = GetSliderValueByIndex(SliderCurrents, Index);
    const float Target = GetSliderValueByIndex(SliderTargets, Index);
    return FMath::Abs(Current - Target) <= SliderTolerance;
}

void AEMRUltrasoundMachine::UpdateSliderWidgetState()
{
    if (CurrentOperator)
    {
        UpdateKnobRotations();
    }
    UpdateSliderIndicatorState();
}

void AEMRUltrasoundMachine::RefreshPatientCardWidget()
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

UEMRTriagePatientCard* AEMRUltrasoundMachine::ResolvePatientCardWidget()
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



int32 AEMRUltrasoundMachine::GetSliderKnobIndexForComponent(const UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return INDEX_NONE;
    }

    if (Component == SliderKnob1Mesh)
    {
        return 0;
    }

    if (Component == SliderKnob2Mesh)
    {
        return 1;
    }

    if (Component == SliderKnob3Mesh)
    {
        return 2;
    }

    return INDEX_NONE;
}

void AEMRUltrasoundMachine::InitializeSliderIndicatorMaterials()
{
    SliderIndicatorMIDs.Reset();
    SliderIndicatorMIDs.SetNum(3);

    for (int32 Index = 0; Index < 3; ++Index)
    {
        UStaticMeshComponent* Indicator = GetSliderIndicatorComponentByIndex(Index);
        if (!Indicator)
        {
            continue;
        }

        UMaterialInterface* BaseMaterial = SliderIndicatorMaterial;
        if (!BaseMaterial)
        {
            continue;
        }

        UMaterialInstanceDynamic* MID = Indicator->CreateDynamicMaterialInstance(0, BaseMaterial);
        SliderIndicatorMIDs[Index] = MID;
    }
}

void AEMRUltrasoundMachine::UpdateSliderIndicatorState()
{
    const bool bHasActivePathology = CurrentPatient && ActivePathologyTag.IsValid();
    for (int32 Index = 0; Index < 3; ++Index)
    {
        UMaterialInstanceDynamic* MID = SliderIndicatorMIDs.IsValidIndex(Index) ? SliderIndicatorMIDs[Index] : nullptr;
        if (!MID)
        {
            continue;
        }

        const bool bLocked = bHasActivePathology && IsSliderLocked(Index);
        const FLinearColor TargetColor = bLocked ? SliderIndicatorDoneColor : SliderIndicatorOffColor;
        MID->SetVectorParameterValue(SliderIndicatorEmissiveParamName, TargetColor);
    }
}

void AEMRUltrasoundMachine::ResetSliderTargetReachedState()
{
    SliderTargetReachedPlayed.Reset();
    SliderTargetReachedPlayed.SetNum(3);
    for (int32 Index = 0; Index < SliderTargetReachedPlayed.Num(); ++Index)
    {
        SliderTargetReachedPlayed[Index] = false;
    }
}

void AEMRUltrasoundMachine::HandleLocalSliderTargetReachedSounds()
{
    if (!bProbeLocked || !ActivePathologyTag.IsValid())
    {
        return;
    }

    if (SliderTargetReachedPlayed.Num() != 3)
    {
        ResetSliderTargetReachedState();
    }

    for (int32 Index = 0; Index < 3; ++Index)
    {
        if (SliderTargetReachedPlayed.IsValidIndex(Index) && SliderTargetReachedPlayed[Index])
        {
            continue;
        }

        if (IsSliderLocked(Index))
        {
            SliderTargetReachedPlayed[Index] = true;
            if (HasAuthority())
            {
                Multicast_PlaySliderTargetReachedSound();
            }
        }
    }
}

void AEMRUltrasoundMachine::Multicast_PlaySliderSelectSound_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    PlayLocalSliderSelectSound();
}

void AEMRUltrasoundMachine::Multicast_PlaySliderTargetReachedSound_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    PlayLocalSliderTargetReachedSound();
}

void AEMRUltrasoundMachine::Multicast_PlayWorldSoundAtLocation_Implementation(USoundBase* Sound, FVector_NetQuantize Location)
{
    if (GetNetMode() == NM_DedicatedServer || !Sound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
}

void AEMRUltrasoundMachine::PlayLocalSliderSelectSound() const
{
    if (!SliderSelectedSound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, SliderSelectedSound, GetActorLocation());
}

void AEMRUltrasoundMachine::PlayLocalSliderTargetReachedSound() const
{
    if (!SliderTargetReachedSound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, SliderTargetReachedSound, GetActorLocation());
}

void AEMRUltrasoundMachine::UpdateKnobRotations()
{
    const auto BuildKnobRelativeRotation = [](const FRotator& BaseRotation, float LocalYawDegrees) -> FQuat
    {
        const FQuat BaseQuat = BaseRotation.Quaternion();
        const FQuat LocalYawDelta(FVector::UpVector, FMath::DegreesToRadians(LocalYawDegrees));
        return (BaseQuat * LocalYawDelta).GetNormalized();
    };

    const float Range = SliderMaxValue - SliderMinValue;
    if (Range <= KINDA_SMALL_NUMBER)
    {
        for (int32 Index = 0; Index < 3; ++Index)
        {
            if (UStaticMeshComponent* Knob = GetKnobComponentByIndex(Index))
            {
                const FRotator BaseRotation = GetKnobBaseRotationByIndex(Index);
                Knob->SetRelativeRotation(BuildKnobRelativeRotation(BaseRotation, 0.0f));
            }
        }
        return;
    }

    for (int32 Index = 0; Index < 3; ++Index)
    {
        UStaticMeshComponent* Knob = GetKnobComponentByIndex(Index);
        if (!Knob)
        {
            continue;
        }

        const float Value = GetSliderValueByIndex(SliderCurrents, Index);
        const float Alpha = FMath::Clamp((Value - SliderMinValue) / Range, 0.0f, 1.0f);
        const float Degrees = Alpha * KnobRotationMaxDegrees;
        const FRotator BaseRotation = GetKnobBaseRotationByIndex(Index);
        Knob->SetRelativeRotation(BuildKnobRelativeRotation(BaseRotation, Degrees));
    }
}

UStaticMeshComponent* AEMRUltrasoundMachine::GetKnobComponentByIndex(int32 Index) const
{
    switch (Index)
    {
    case 0:
        return SliderKnob1Mesh;
    case 1:
        return SliderKnob2Mesh;
    case 2:
        return SliderKnob3Mesh;
    default:
        return SliderKnob1Mesh;
    }
}

UStaticMeshComponent* AEMRUltrasoundMachine::GetSliderIndicatorComponentByIndex(int32 Index) const
{
    switch (Index)
    {
    case 0:
        return SliderIndicator1Mesh;
    case 1:
        return SliderIndicator2Mesh;
    case 2:
        return SliderIndicator3Mesh;
    default:
        return SliderIndicator1Mesh;
    }
}

FRotator AEMRUltrasoundMachine::GetKnobBaseRotationByIndex(int32 Index) const
{
    switch (Index)
    {
    case 0:
        return SliderKnob1BaseRotation;
    case 1:
        return SliderKnob2BaseRotation;
    case 2:
        return SliderKnob3BaseRotation;
    default:
        return SliderKnob1BaseRotation;
    }
}

float AEMRUltrasoundMachine::ComputeSliderNoise() const
{
    const float DeltaX = FMath::Max(KINDA_SMALL_NUMBER, SliderInitialDeltas.X);
    const float DeltaY = FMath::Max(KINDA_SMALL_NUMBER, SliderInitialDeltas.Y);
    const float DeltaZ = FMath::Max(KINDA_SMALL_NUMBER, SliderInitialDeltas.Z);

    const float ErrorX = FMath::Clamp(FMath::Abs(SliderCurrents.X - SliderTargets.X) / DeltaX, 0.0f, 1.0f);
    const float ErrorY = FMath::Clamp(FMath::Abs(SliderCurrents.Y - SliderTargets.Y) / DeltaY, 0.0f, 1.0f);
    const float ErrorZ = FMath::Clamp(FMath::Abs(SliderCurrents.Z - SliderTargets.Z) / DeltaZ, 0.0f, 1.0f);

    return (ErrorX + ErrorY + ErrorZ) / 3.0f;
}
