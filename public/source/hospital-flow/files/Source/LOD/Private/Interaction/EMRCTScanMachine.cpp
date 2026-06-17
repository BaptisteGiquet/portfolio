#include "Interaction/EMRCTScanMachine.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Blueprint/UserWidget.h"
#include "Characters/Patient/EMRAIController.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Characters/Player/EMRPlayerController.h"
#include "Components/ArrowComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/EMRCTScanTargetData.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Framework/EMRAssetManager.h"
#include "GAS/EMRTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NavigationSystem.h"
#include "Sound/SoundBase.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "TimerManager.h"
#include "UI/Machines/Triage/EMRTriagePatientCard.h"
#include "UI/Patient/EMRPatientUIController.h"

namespace
{
    bool AppendFallbackCTScanSocketFromRow(const TArray<FEMRCTScanTargetData*>& Rows, USkeletalMeshComponent* PatientMesh, TArray<FName>& OutSockets)
    {
        if (Rows.IsEmpty() || !PatientMesh)
        {
            return false;
        }

        const int32 StartIndex = FMath::RandRange(0, Rows.Num() - 1);
        for (int32 Offset = 0; Offset < Rows.Num(); ++Offset)
        {
            const int32 RowIndex = (StartIndex + Offset) % Rows.Num();
            const FEMRCTScanTargetData* Row = Rows.IsValidIndex(RowIndex) ? Rows[RowIndex] : nullptr;
            if (!Row || Row->SocketName == NAME_None)
            {
                continue;
            }

            if (PatientMesh->DoesSocketExist(Row->SocketName))
            {
                OutSockets.Add(Row->SocketName);
                return true;
            }
        }

        return false;
    }
}

AEMRCTScanMachine::AEMRCTScanMachine()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    TableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableMesh"));
    TableMesh->SetupAttachment(GetRootComponent());
    TableMesh->SetMobility(EComponentMobility::Movable);
    TableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    TableMovementAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("TableMovementAudioComponent"));
    TableMovementAudioComponent->SetupAttachment(TableMesh ? TableMesh : GetRootComponent());
    TableMovementAudioComponent->bAutoActivate = false;
    TableMovementAudioComponent->bAllowSpatialization = true;

    LaserZoneDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("LaserZoneDecal"));
    LaserZoneDecal->SetupAttachment(GetRootComponent());
    LaserZoneDecal->SetVisibility(bLaserActive);
    LaserZoneDecal->SetHiddenInGame(!bLaserActive);

    StartButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartButtonMesh"));
    StartButtonMesh->SetupAttachment(GetRootComponent());
    StartButtonMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    StartButtonMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    StartButtonMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
    StartButtonMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    StartButtonMesh->SetMobility(EComponentMobility::Movable);

    ValidateButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ValidateButtonMesh"));
    ValidateButtonMesh->SetupAttachment(GetRootComponent());
    ValidateButtonMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ValidateButtonMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    ValidateButtonMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
    ValidateButtonMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    ValidateButtonMesh->SetMobility(EComponentMobility::Movable);

    PatientAttachComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PatientAttachPoint"));
    PatientAttachComponent->SetupAttachment(TableMesh ? TableMesh : GetRootComponent());

    OperatorLockPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("OperatorLockPoint"));
    OperatorLockPoint->SetupAttachment(GetRootComponent());
    OperatorLockPoint->SetArrowColor(FLinearColor::Blue);
    OperatorLockPoint->bIsScreenSizeScaled = true;

    OperatorExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("OperatorExitPoint"));
    OperatorExitPoint->SetupAttachment(GetRootComponent());
    OperatorExitPoint->SetArrowColor(FLinearColor::Yellow);
    OperatorExitPoint->bIsScreenSizeScaled = true;

    PatientCardWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PatientCardWidget"));
    PatientCardWidgetComponent->SetupAttachment(GetRootComponent());
    PatientCardWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    PatientCardWidgetComponent->SetDrawAtDesiredSize(true);
    PatientCardWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PatientCardWidgetComponent->SetVisibility(false);
    PatientCardWidgetComponent->SetHiddenInGame(true);

}

void AEMRCTScanMachine::BeginPlay()
{
    Super::BeginPlay();

    CacheTableBaseLocation();
    ApplyTableOffset();
    CacheButtonBaseTransforms();
    EnsureButtonMaterialInstances();
    ApplyButtonTransforms();
    ApplyButtonColors();
    if (TableMovementAudioComponent)
    {
        TableMovementAudioComponent->SetSound(TableMovementLoopSound);
        UE_LOG(
            LogTemp,
            Log,
            TEXT("[CTScanAudio] BeginPlay configured audio component machine=%s sound=%s"),
            *GetNameSafe(this),
            *GetNameSafe(TableMovementLoopSound));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CTScanAudio] Missing TableMovementAudioComponent on machine=%s"), *GetNameSafe(this));
    }

    if (LaserZoneDecal)
    {
        LaserZoneDecal->SetVisibility(bLaserActive, true);
        LaserZoneDecal->SetHiddenInGame(!bLaserActive);
    }

    if (GetNetMode() != NM_DedicatedServer)
    {
        RefreshPatientCardWidget();
    }

    RefreshTableMovementAudioPlayback();
}

void AEMRCTScanMachine::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority())
    {
        if (CachedAssignedPatient.Get() != CurrentPatient)
        {
            CachedAssignedPatient = CurrentPatient;
            RefreshPatientCardWidget();
        }
        RefreshTableMovementAudioPlayback();
        UpdateButtons(DeltaSeconds);
        return;
    }

    if (CachedAssignedPatient.Get() != CurrentPatient)
    {
        const bool bKeepOperator = (CurrentOperator != nullptr) || bKeepOperatorAfterCompletion;
        ResetExamState(bKeepOperator);
        bKeepOperatorAfterCompletion = false;
        CachedAssignedPatient = CurrentPatient;

        if (CurrentPatient)
        {
            RefreshTargetSocket();
        }

        RefreshPatientCardWidget();
    }

    if (CurrentPatient && !bPatientOnTable)
    {
        TryAttachPatient();
    }

    UpdateTableMotion(DeltaSeconds);

    if (bCompletionPending)
    {
        TryCompleteExam();
    }

    RefreshTableMovementAudioPlayback();
    UpdateButtons(DeltaSeconds);
}

void AEMRCTScanMachine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRCTScanMachine, CurrentOperator);
    DOREPLIFETIME(AEMRCTScanMachine, AttachedPatient);
    DOREPLIFETIME(AEMRCTScanMachine, TargetSocketName);
    DOREPLIFETIME(AEMRCTScanMachine, TableOffset);
    DOREPLIFETIME(AEMRCTScanMachine, bTableMoving);
    DOREPLIFETIME(AEMRCTScanMachine, bTableReturning);
    DOREPLIFETIME(AEMRCTScanMachine, bLaserActive);
    DOREPLIFETIME(AEMRCTScanMachine, bStartButtonEnabled);
    DOREPLIFETIME(AEMRCTScanMachine, bValidateButtonEnabled);
    DOREPLIFETIME(AEMRCTScanMachine, bStartButtonPressed);
    DOREPLIFETIME(AEMRCTScanMachine, bValidateButtonPressed);
    DOREPLIFETIME(AEMRCTScanMachine, ValidateFailurePulseCounter);
}

bool AEMRCTScanMachine::ShouldReleasePatientOnExamCompleted(FGameplayTag ExamTag) const
{
    return !ExamTag.MatchesTagExact(EMRTags::Abilities::Exam::CTScan);
}

void AEMRCTScanMachine::TryStartExam(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player)
    {
        return;
    }

    if (CurrentOperator && CurrentOperator != Player)
    {
        return;
    }

    if (CurrentPatient && TargetSocketName == NAME_None)
    {
        RefreshTargetSocket();
        if (TargetSocketName == NAME_None)
        {
            return;
        }
    }

    CurrentOperator = Player;
    CachedOperatorController = Cast<AEMRPlayerController>(Player->GetController());
    LockOperator(Player);
    // Keep CT progression machine/patient-owned across operator swaps.
    Player->Client_BeginCTScanExam(this, ExamInputMappingContext);

    if (CurrentPatient)
    {
        TryAttachPatient();
    }
}

void AEMRCTScanMachine::StopExam(AEMRPlayerCharacter* Player, bool bFromCancel)
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
    Player->Client_EndCTScanExam(this);

    CurrentOperator = nullptr;
    if (!bFromCancel)
    {
        CachedOperatorController.Reset();
    }
}

void AEMRCTScanMachine::HandleStartButtonPressed(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player || CurrentOperator != Player)
    {
        return;
    }

    if (!IsStartButtonInteractable())
    {
        return;
    }

    if (!CurrentPatient || bTableMoving || bTableReturning)
    {
        return;
    }

    if (TargetSocketName == NAME_None)
    {
        RefreshTargetSocket();
        if (TargetSocketName == NAME_None)
        {
            return;
        }
    }

    bTableMoving = true;
    bTableReturning = false;
    bLaserActive = true;
    if (LaserZoneDecal)
    {
        LaserZoneDecal->SetVisibility(bLaserActive, true);
        LaserZoneDecal->SetHiddenInGame(!bLaserActive);
    }
    if (TableDirection == 0)
    {
        TableDirection = 1;
    }
    bStartButtonEnabled = false;
    bValidateButtonEnabled = true;
    bStartButtonPressed = true;
    bValidateButtonPressed = false;
    ForceNetUpdate();
}

void AEMRCTScanMachine::HandleValidateButtonPressed(AEMRPlayerCharacter* Player)
{
    if (!HasAuthority() || !Player || CurrentOperator != Player)
    {
        return;
    }

    if (!IsValidateButtonInteractable())
    {
        return;
    }

    if (!bTableMoving || bTableReturning)
    {
        return;
    }

    bDeferredValidateSuccess = false;
    bValidateButtonPressed = true;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ValidateFailureResumeTimerHandle);
        World->GetTimerManager().ClearTimer(ValidatePressReleaseTimerHandle);
        World->GetTimerManager().SetTimer(
            ValidatePressReleaseTimerHandle,
            this,
            &ThisClass::HandleValidatePressRelease,
            ValidatePressHoldSeconds,
            false);
    }

    bTableMoving = false;

    const bool bSuccess = IsLaserOnTarget();
    TriggerResultCue(bSuccess);

    if (bSuccess)
    {
        bDeferredValidateSuccess = true;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(ValidateSuccessTimerHandle);
            World->GetTimerManager().SetTimer(
                ValidateSuccessTimerHandle,
                this,
                &ThisClass::HandleDeferredValidateSuccess,
                ValidateSuccessDelaySeconds,
                false);
        }
        else
        {
            HandleDeferredValidateSuccess();
        }
        ForceNetUpdate();
        return;
    }

    bLaserActive = false;
    if (LaserZoneDecal)
    {
        LaserZoneDecal->SetVisibility(bLaserActive, true);
        LaserZoneDecal->SetHiddenInGame(!bLaserActive);
    }
    bTableReturning = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ValidateFailureResumeTimerHandle,
            this,
            &ThisClass::HandleValidateFailureResume,
            ValidateFailurePauseSeconds,
            false);
    }
    else
    {
        HandleValidateFailureResume();
    }

    TriggerValidateFailurePulse();
    ForceNetUpdate();
}

int32 AEMRCTScanMachine::GetButtonIndexForComponent(const UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return INDEX_NONE;
    }

    if (Component == StartButtonMesh)
    {
        return 0;
    }

    if (Component == ValidateButtonMesh)
    {
        return 1;
    }

    return INDEX_NONE;
}

void AEMRCTScanMachine::OnRep_CurrentOperator()
{
}

void AEMRCTScanMachine::OnRep_AttachedPatient()
{
}

void AEMRCTScanMachine::OnRep_TargetSocket()
{
}

void AEMRCTScanMachine::OnRep_TableState()
{
    ApplyTableOffset();

    if (LaserZoneDecal)
    {
        LaserZoneDecal->SetVisibility(bLaserActive, true);
        LaserZoneDecal->SetHiddenInGame(!bLaserActive);
    }

    RefreshTableMovementAudioPlayback();
}

void AEMRCTScanMachine::OnRep_ButtonState()
{
    ApplyButtonColors();
}

void AEMRCTScanMachine::OnRep_ValidateFailurePulseCounter()
{
    StartValidateFailureFlicker();
}

void AEMRCTScanMachine::Multicast_PlayPatientEnterMontage_Implementation(AEMRPatient* Patient)
{
    PlayPatientEnterMontageFor(Patient);
}

void AEMRCTScanMachine::Multicast_StopPatientEnterMontage_Implementation(AEMRPatient* Patient)
{
    StopPatientEnterMontageFor(Patient);
}

void AEMRCTScanMachine::Multicast_PlayResultSound_Implementation(USoundBase* Sound, FVector Location)
{
    if (GetNetMode() == NM_DedicatedServer || !Sound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
}

void AEMRCTScanMachine::LockOperator(AEMRPlayerCharacter* Player)
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
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CTScanOperatorSnapTrace), false, this);
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

void AEMRCTScanMachine::UnlockOperator(AEMRPlayerCharacter* Player)
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
            FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CTScanOperatorExitTrace), false, this);
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

void AEMRCTScanMachine::TryAttachPatient()
{
    if (!CurrentPatient || bPatientOnTable)
    {
        return;
    }

    if (!PatientAttachComponent)
    {
        return;
    }

    if (DetectionRadius && !DetectionRadius->IsOverlappingActor(CurrentPatient))
    {
        return;
    }

    if (!bHasCachedPatientTransform || CachedPatientTransformOwner.Get() != CurrentPatient)
    {
        CachedPatientTransformOwner = CurrentPatient;
        CachedPatientTransform = CurrentPatient->GetActorTransform();
        bHasCachedPatientTransform = true;
    }

    const bool bHasSocket = (PatientAttachSocketName != NAME_None) && PatientAttachComponent->DoesSocketExist(PatientAttachSocketName);
    const FTransform AttachTransform = bHasSocket
    ? PatientAttachComponent->GetSocketTransform(PatientAttachSocketName)
    : PatientAttachComponent->GetComponentTransform();

    CurrentPatient->SetActorLocationAndRotation(
        AttachTransform.GetLocation(),
        AttachTransform.GetRotation().Rotator(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
    CurrentPatient->AttachToComponent(PatientAttachComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, PatientAttachSocketName);

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
        AttachedPatient = CurrentPatient;
        ForceNetUpdate();
        Multicast_PlayPatientEnterMontage(CurrentPatient);
    }

    bPatientOnTable = true;
}

void AEMRCTScanMachine::DetachPatient(AEMRPatient* Patient)
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
}

void AEMRCTScanMachine::RefreshTargetSocket()
{
    if (!HasAuthority())
    {
        return;
    }

    TargetSocketName = NAME_None;
    CachedTargetMesh.Reset();

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

    const UDataTable* CTScanTargetsTable = LoadedSubsystemConfig[0]->GetCTScanTargetsFromPathologyTable();
    if (!CTScanTargetsTable)
    {
        return;
    }

    USkeletalMeshComponent* PatientMesh = ResolvePatientMesh();
    if (!PatientMesh)
    {
        return;
    }

    TArray<FName> ValidSockets;
    TArray<FEMRCTScanTargetData*> Rows;
    CTScanTargetsTable->GetAllRows(TEXT("CTScanTargets"), Rows);
    for (const FEMRCTScanTargetData* Row : Rows)
    {
        if (!Row || !Row->PathologyTag.IsValid())
        {
            continue;
        }

        if (!Pathologies.HasTagExact(Row->PathologyTag))
        {
            continue;
        }

        if (Row->SocketName == NAME_None)
        {
            continue;
        }

        if (PatientMesh->DoesSocketExist(Row->SocketName))
        {
            ValidSockets.Add(Row->SocketName);
        }
    }

    if (ValidSockets.IsEmpty())
    {
        if (AppendFallbackCTScanSocketFromRow(Rows, PatientMesh, ValidSockets))
        {
            UE_LOG(LogTemp, Warning, TEXT("[CTScanMachine] RefreshTargetSocket: using fallback target for optional exam (patient=%s)"),
                *GetNameSafe(CurrentPatient));
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

USkeletalMeshComponent* AEMRCTScanMachine::ResolvePatientMesh() const
{
    return ResolvePatientMeshFor(CurrentPatient);
}

USkeletalMeshComponent* AEMRCTScanMachine::ResolvePatientMeshFor(const AEMRPatient* Patient) const
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

    if (USkeletalMeshComponent* DefaultMesh = Patient->GetMesh())
    {
        if (bCanCache)
        {
            CachedTargetMesh = DefaultMesh;
        }
        return DefaultMesh;
    }

    return nullptr;
}

void AEMRCTScanMachine::UpdateTableMotion(float DeltaSeconds)
{
    if (!TableMesh)
    {
        return;
    }

    if (!bTableMoving && !bTableReturning)
    {
        return;
    }

    if (bTableReturning)
    {
        const float Step = TableReturnSpeed * DeltaSeconds;
        if (FMath::Abs(TableOffset) <= Step)
        {
            TableOffset = 0.0f;
            bTableReturning = false;
            bTableMoving = false;
        }
        else
        {
            TableOffset = (TableOffset > 0.0f) ? (TableOffset - Step) : (TableOffset + Step);
        }

        ApplyTableOffset();
        return;
    }

    const float MoveStep = TableMoveSpeed * DeltaSeconds;
    const float NewOffset = TableOffset + (MoveStep * static_cast<float>(TableDirection));
    const float MinOffset = FMath::Min(TableMinOffset, TableMaxOffset);
    const float MaxOffset = FMath::Max(TableMinOffset, TableMaxOffset);

    TableOffset = FMath::Clamp(NewOffset, MinOffset, MaxOffset);

    if (TableOffset <= MinOffset + KINDA_SMALL_NUMBER || TableOffset >= MaxOffset - KINDA_SMALL_NUMBER)
    {
        TableDirection *= -1;
    }

    ApplyTableOffset();
}

void AEMRCTScanMachine::ApplyTableOffset()
{
    CacheTableBaseLocation();

    if (TableMesh && bHasCachedBaseLocation)
    {
        TableMesh->SetWorldLocation(TableBaseWorldLocation + FVector(TableOffset, 0.0f, 0.0f));
    }
}

void AEMRCTScanMachine::CacheTableBaseLocation()
{
    if (bHasCachedBaseLocation || !TableMesh)
    {
        return;
    }

    TableBaseWorldLocation = TableMesh->GetComponentLocation();
    bHasCachedBaseLocation = true;
}

void AEMRCTScanMachine::ResetExamState(bool bKeepOperator)
{
    if (CurrentOperator && !bKeepOperator)
    {
        StopExam(CurrentOperator, false);
    }

    if (bPatientOnTable && CachedAssignedPatient.IsValid())
    {
        DetachPatient(CachedAssignedPatient.Get());
    }

    bCompletionPending = false;
    bDeferredValidateSuccess = false;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ValidateSuccessTimerHandle);
        World->GetTimerManager().ClearTimer(ValidateFailureResumeTimerHandle);
    }
    TargetSocketName = NAME_None;
    bLaserActive = false;
    if (LaserZoneDecal)
    {
        LaserZoneDecal->SetVisibility(bLaserActive, true);
        LaserZoneDecal->SetHiddenInGame(!bLaserActive);
    }
    bTableMoving = false;
    bTableReturning = FMath::Abs(TableOffset) > KINDA_SMALL_NUMBER;
    TableDirection = 1;
    ResetButtonState();
    CachedOperatorController.Reset();
    ForceNetUpdate();
}

void AEMRCTScanMachine::ResetButtonState()
{
    bStartButtonEnabled = true;
    bValidateButtonEnabled = false;
    bStartButtonPressed = false;
    bValidateButtonPressed = false;
    bValidateFailureFlickerActive = false;
    ValidateFailureFlickerElapsed = 0.0f;
    bDeferredValidateSuccess = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ValidatePressReleaseTimerHandle);
        World->GetTimerManager().ClearTimer(ValidateSuccessTimerHandle);
        World->GetTimerManager().ClearTimer(ValidateFailureResumeTimerHandle);
    }
}

bool AEMRCTScanMachine::IsStartButtonInteractable() const
{
    return bStartButtonEnabled;
}

bool AEMRCTScanMachine::IsValidateButtonInteractable() const
{
    return bValidateButtonEnabled;
}

void AEMRCTScanMachine::TriggerValidateFailurePulse()
{
    ValidateFailurePulseCounter++;
    StartValidateFailureFlicker();
}

void AEMRCTScanMachine::HandleValidatePressRelease()
{
    if (!HasAuthority())
    {
        return;
    }

    bValidateButtonPressed = false;
    ForceNetUpdate();
}

void AEMRCTScanMachine::HandleDeferredValidateSuccess()
{
    if (!HasAuthority() || !bDeferredValidateSuccess)
    {
        return;
    }

    bDeferredValidateSuccess = false;
    TryCompleteExam();
    ForceNetUpdate();
}

void AEMRCTScanMachine::HandleValidateFailureResume()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!CurrentOperator || !CurrentPatient || !bValidateButtonEnabled || bDeferredValidateSuccess || bTableReturning)
    {
        return;
    }

    bTableMoving = true;
    bLaserActive = true;
    if (LaserZoneDecal)
    {
        LaserZoneDecal->SetVisibility(bLaserActive, true);
        LaserZoneDecal->SetHiddenInGame(!bLaserActive);
    }

    if (TableDirection == 0)
    {
        TableDirection = 1;
    }

    ForceNetUpdate();
}

void AEMRCTScanMachine::CacheButtonBaseTransforms()
{
    if (bButtonTransformsCached)
    {
        return;
    }

    if (StartButtonMesh)
    {
        StartButtonBaseRelativeTransform = StartButtonMesh->GetRelativeTransform();
    }

    if (ValidateButtonMesh)
    {
        ValidateButtonBaseRelativeTransform = ValidateButtonMesh->GetRelativeTransform();
    }

    bButtonTransformsCached = true;
}

void AEMRCTScanMachine::EnsureButtonMaterialInstances()
{
    if (!StartButtonMID && StartButtonMesh)
    {
        StartButtonMID = StartButtonMesh->CreateDynamicMaterialInstance(0);
    }

    if (!ValidateButtonMID && ValidateButtonMesh)
    {
        ValidateButtonMID = ValidateButtonMesh->CreateDynamicMaterialInstance(0);
    }
}

void AEMRCTScanMachine::UpdateButtons(float DeltaSeconds)
{
    CacheButtonBaseTransforms();
    EnsureButtonMaterialInstances();

    const float TargetStartAlpha = bStartButtonPressed ? 1.0f : 0.0f;
    const float TargetValidateAlpha = bValidateButtonPressed ? 1.0f : 0.0f;

    StartButtonPressAlpha = FMath::FInterpTo(StartButtonPressAlpha, TargetStartAlpha, DeltaSeconds, ButtonPressInterpSpeed);
    ValidateButtonPressAlpha = FMath::FInterpTo(ValidateButtonPressAlpha, TargetValidateAlpha, DeltaSeconds, ButtonPressInterpSpeed);
    ApplyButtonTransforms();

    if (bValidateFailureFlickerActive)
    {
        ValidateFailureFlickerElapsed += DeltaSeconds;
        if (ValidateFailureFlickerElapsed >= ValidateFailureFlickerDuration)
        {
            bValidateFailureFlickerActive = false;
            ValidateFailureFlickerElapsed = 0.0f;
        }
    }

    ApplyButtonColors();
}

void AEMRCTScanMachine::ApplyButtonTransforms()
{
    if (!bButtonTransformsCached)
    {
        return;
    }

    if (StartButtonMesh)
    {
        FTransform StartTransform = StartButtonBaseRelativeTransform;
        StartTransform.AddToTranslation(ButtonPressOffset * StartButtonPressAlpha);
        StartButtonMesh->SetRelativeTransform(StartTransform);
    }

    if (ValidateButtonMesh)
    {
        FTransform ValidateTransform = ValidateButtonBaseRelativeTransform;
        ValidateTransform.AddToTranslation(ButtonPressOffset * ValidateButtonPressAlpha);
        ValidateButtonMesh->SetRelativeTransform(ValidateTransform);
    }
}

void AEMRCTScanMachine::ApplyButtonColors()
{
    if (ButtonColorParameterName.IsNone())
    {
        return;
    }

    EnsureButtonMaterialInstances();

    if (StartButtonMID)
    {
        const FLinearColor StartColor = bStartButtonEnabled ? StartEnabledColor : StartDisabledColor;
        StartButtonMID->SetVectorParameterValue(ButtonColorParameterName, StartColor);
    }

    if (ValidateButtonMID)
    {
        const FLinearColor ValidateBaseColor = bValidateButtonEnabled ? ValidateEnabledColor : ValidateDisabledColor;
        const FLinearColor ValidateColor = IsValidateFailureFlickerVisible() ? ValidateFailureColor : ValidateBaseColor;
        ValidateButtonMID->SetVectorParameterValue(ButtonColorParameterName, ValidateColor);
    }
}

bool AEMRCTScanMachine::IsTableMovementAudioActive() const
{
    return bTableMoving || bTableReturning;
}

void AEMRCTScanMachine::RefreshTableMovementAudioPlayback()
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!TableMovementAudioComponent)
    {
        if (!bHasLoggedMissingTableMovementAudioComponent)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CTScanAudio] Refresh skipped: no audio component machine=%s"), *GetNameSafe(this));
            bHasLoggedMissingTableMovementAudioComponent = true;
        }
        return;
    }

    const bool bShouldPlay = IsTableMovementAudioActive() && (TableMovementLoopSound != nullptr);
    const int8 DesiredStateInt = bShouldPlay ? 1 : 0;
    if (LastLoggedTableMovementAudioDesiredState != DesiredStateInt)
    {
        LastLoggedTableMovementAudioDesiredState = DesiredStateInt;
        UE_LOG(
            LogTemp,
            Log,
            TEXT("[CTScanAudio] Desired playback changed machine=%s shouldPlay=%d moving=%d returning=%d sound=%s currentlyPlaying=%d"),
            *GetNameSafe(this),
            bShouldPlay ? 1 : 0,
            bTableMoving ? 1 : 0,
            bTableReturning ? 1 : 0,
            *GetNameSafe(TableMovementLoopSound),
            bIsTableMovementAudioPlaying ? 1 : 0);
    }

    if (!TableMovementLoopSound && IsTableMovementAudioActive() && !bHasLoggedMissingTableMovementLoopSound)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[CTScanAudio] Movement active but TableMovementLoopSound is null machine=%s"),
            *GetNameSafe(this));
        bHasLoggedMissingTableMovementLoopSound = true;
    }
    else if (TableMovementLoopSound)
    {
        bHasLoggedMissingTableMovementLoopSound = false;
    }

    if (bShouldPlay)
    {
        if (!bIsTableMovementAudioPlaying)
        {
            TableMovementAudioComponent->FadeIn(TableMovementLoopFadeInSeconds, 1.0f, 0.0f);
            bIsTableMovementAudioPlaying = true;
            UE_LOG(
                LogTemp,
                Log,
                TEXT("[CTScanAudio] FadeIn machine=%s fadeIn=%.2f"),
                *GetNameSafe(this),
                TableMovementLoopFadeInSeconds);
        }
    }
    else if (bIsTableMovementAudioPlaying)
    {
        TableMovementAudioComponent->FadeOut(TableMovementLoopFadeOutSeconds, 0.0f);
        bIsTableMovementAudioPlaying = false;
        UE_LOG(
            LogTemp,
            Log,
            TEXT("[CTScanAudio] FadeOut machine=%s fadeOut=%.2f"),
            *GetNameSafe(this),
            TableMovementLoopFadeOutSeconds);
    }
}

void AEMRCTScanMachine::StartValidateFailureFlicker()
{
    bValidateFailureFlickerActive = true;
    ValidateFailureFlickerElapsed = 0.0f;
}

bool AEMRCTScanMachine::IsValidateFailureFlickerVisible() const
{
    if (!bValidateFailureFlickerActive || ValidateFailureFlickerDuration <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const int32 FlickerToggles = FMath::Max(1, ValidateFailureFlickerToggleCount);
    const float FlickerSlice = ValidateFailureFlickerDuration / static_cast<float>(FlickerToggles);
    if (FlickerSlice <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const int32 SliceIndex = FMath::FloorToInt(ValidateFailureFlickerElapsed / FlickerSlice);
    return (SliceIndex % 2) == 0;
}

bool AEMRCTScanMachine::IsLaserOnTarget() const
{
    if (!CurrentPatient || TargetSocketName == NAME_None || !LaserZoneDecal)
    {
        return false;
    }

    USkeletalMeshComponent* PatientMesh = ResolvePatientMesh();
    if (!PatientMesh || !PatientMesh->DoesSocketExist(TargetSocketName))
    {
        return false;
    }

    const FVector LaserLocation = LaserZoneDecal->GetComponentLocation();
    const FVector SocketLocation = PatientMesh->GetSocketLocation(TargetSocketName);
    const float EffectiveTolerance = TargetSocketToleranceRadius;
    const float DistSq = FVector::DistSquared2D(LaserLocation, SocketLocation);
    return DistSq <= FMath::Square(EffectiveTolerance);
}

void AEMRCTScanMachine::TriggerResultCue(bool bSuccess)
{
    if (!HasAuthority())
    {
        return;
    }

    USoundBase* ResultSound = bSuccess ? SuccessSound : FailureSound;
    if (!ResultSound)
    {
        return;
    }

    const FVector SoundLocation = LaserZoneDecal ? LaserZoneDecal->GetComponentLocation() : GetActorLocation();
    Multicast_PlayResultSound(ResultSound, SoundLocation);
}

void AEMRCTScanMachine::TryCompleteExam()
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

    FGameplayEventData EventData;
    EventData.EventTag = EMRTags::Abilities::Exam::CTScan;
    EventData.Target = CompletedPatient;
    EventData.OptionalObject = this;
    EventData.EventMagnitude = 1.0f;

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
    bKeepOperatorAfterCompletion = true;
    bTableMoving = false;
    bLaserActive = false;
    if (LaserZoneDecal)
    {
        LaserZoneDecal->SetVisibility(bLaserActive, true);
        LaserZoneDecal->SetHiddenInGame(!bLaserActive);
    }
    bTableReturning = FMath::Abs(TableOffset) > KINDA_SMALL_NUMBER;
    ResetButtonState();

    if (bPatientOnTable && CompletedPatient)
    {
        DetachPatient(CompletedPatient);
    }

    if (CurrentPatient == CompletedPatient)
    {
        ReleasePatient();
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[CTScanFlow] TryCompleteExam ownership changed during completion. machine=%s completedPatient=%s currentPatient=%s"),
            *GetNameSafe(this),
            *GetNameSafe(CompletedPatient),
            *GetNameSafe(CurrentPatient));
    }
}

bool AEMRCTScanMachine::TrySendExamGameplayEvent(AEMRPlayerCharacter* Operator, const FGameplayEventData& EventData)
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

void AEMRCTScanMachine::PlayPatientEnterMontageFor(AEMRPatient* Patient)
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
        }
    }
}

void AEMRCTScanMachine::StopPatientEnterMontageFor(AEMRPatient* Patient)
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
            AnimInstance->Montage_Stop(0.1f, PatientEnterTableMontage);
        }
    }
}

void AEMRCTScanMachine::RefreshPatientCardWidget()
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

UEMRTriagePatientCard* AEMRCTScanMachine::ResolvePatientCardWidget()
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
