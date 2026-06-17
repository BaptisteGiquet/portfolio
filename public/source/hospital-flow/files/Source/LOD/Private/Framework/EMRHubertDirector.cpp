#include "Framework/EMRHubertDirector.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/EMREconomySystemGenerics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Environment/EMRHospitalExit.h"
#include "Framework/EMRAssetManager.h"
#include "Framework/EMRNightShiftGameMode.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GAS/EMRTags.h"
#include "Navigation/PathFollowingComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Sound/SoundBase.h"
#include "Subsystems/EMRRunProgressionSubsystem.h"

namespace
{
const TCHAR* HubertStateToString_Director(const EEMRHubertState State)
{
    switch (State)
    {
    case EEMRHubertState::HubDriver:
        return TEXT("HubDriver");
    case EEMRHubertState::NightPatrol:
        return TEXT("NightPatrol");
    case EEMRHubertState::WindowWatch:
        return TEXT("WindowWatch");
    case EEMRHubertState::ChaseEscaper:
        return TEXT("ChaseEscaper");
    case EEMRHubertState::GrabEscaper:
        return TEXT("GrabEscaper");
    case EEMRHubertState::CarryEscaper:
        return TEXT("CarryEscaper");
    case EEMRHubertState::ThrowEscaper:
        return TEXT("ThrowEscaper");
    case EEMRHubertState::ReturnEscaper:
        return TEXT("ReturnEscaper");
    default:
        return TEXT("Unknown");
    }
}
}

AEMRHubertDirector::AEMRHubertDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(false);
}

void AEMRHubertDirector::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority())
    {
        return;
    }

    if (bHubRuntimeActive)
    {
        UpdateHubRuntime(DeltaSeconds);
    }

    if (bNightRuntimeActive)
    {
        UpdateNightRuntime(DeltaSeconds);
    }
}

void AEMRHubertDirector::StartHubRuntime()
{
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] StartHubRuntime director=%s authority=%d"),
        *GetNameSafe(this),
        HasAuthority() ? 1 : 0);

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] StartHubRuntime rejected on non-authority director=%s"), *GetNameSafe(this));
        return;
    }

    RestoreEscaperGrabCarryOverrides(ActiveEscaper.Get(), HubertCharacter.Get());

    bHubRuntimeActive = true;
    bNightRuntimeActive = false;
    bHubLoadTimeoutWarningLogged = false;
    HubRuntimeStartTimestampSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    NextOutcomeAnnouncementTimestampSeconds = HubRuntimeStartTimestampSeconds;

    PendingEscaperQueue.Reset();
    ActiveEscaper.Reset();
    ActivePatrolPoint.Reset();
    ActiveWindowPoint.Reset();
    EscaperActionEndTimestampSeconds = 0.0;
    ResetEscaperPhysicalThrowRuntime();
    ExitBindings.Reset();

    if (AEMRHubertCharacter* SpawnedHubert = EnsureHubertCharacter(true))
    {
        if (const AActor* HubAnchor = HubSpawnAnchor.Get())
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Hub runtime anchoring Hubert actor=%s anchor=%s location=%s rotation=%s"),
                *GetNameSafe(SpawnedHubert),
                *GetNameSafe(HubAnchor),
                *HubAnchor->GetActorLocation().ToCompactString(),
                *HubAnchor->GetActorRotation().ToCompactString());
            SpawnedHubert->SetActorLocationAndRotation(
                HubAnchor->GetActorLocation(),
                HubAnchor->GetActorRotation(),
                false,
                nullptr,
                ETeleportType::TeleportPhysics);

            SpawnedHubert->AttachToActor(const_cast<AActor*>(HubAnchor), FAttachmentTransformRules::KeepWorldTransform);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Hub runtime has no HubSpawnAnchor; Hubert will stay detached actor=%s"),
                *GetNameSafe(SpawnedHubert));
            SpawnedHubert->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        }

        if (UCharacterMovementComponent* MovementComponent = SpawnedHubert->GetCharacterMovement())
        {
            MovementComponent->DisableMovement();
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Hub runtime disabled movement actor=%s movementMode=%d"),
                *GetNameSafe(SpawnedHubert),
                static_cast<int32>(MovementComponent->MovementMode));
        }

        SpawnedHubert->SetHubertState(EEMRHubertState::HubDriver);
        StopHubertMovement();
    }

    CurrentState = EEMRHubertState::HubDriver;

    if (UEMRRunProgressionSubsystem* ProgressionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRRunProgressionSubsystem>() : nullptr)
    {
        TArray<EEMRHubertOutcomeAnnouncement> Announcements;
        ProgressionSubsystem->ConsumeHubertOutcomeAnnouncements(Announcements);
        QueueOutcomeAnnouncements(Announcements);
    }

    SetActorTickEnabled(true);
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Hub runtime started state=%s pendingAnnouncements=%d"),
        HubertStateToString_Director(CurrentState),
        PendingOutcomeAnnouncements.Num());
}

void AEMRHubertDirector::StartNightShiftRuntime()
{
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] StartNightShiftRuntime director=%s authority=%d"),
        *GetNameSafe(this),
        HasAuthority() ? 1 : 0);

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] StartNightShiftRuntime rejected on non-authority director=%s"), *GetNameSafe(this));
        return;
    }

    RestoreEscaperGrabCarryOverrides(ActiveEscaper.Get(), HubertCharacter.Get());

    bHubRuntimeActive = false;
    bNightRuntimeActive = true;
    bHubLoadTimeoutWarningLogged = false;

    PendingEscaperQueue.Reset();
    ActiveEscaper.Reset();
    ActivePatrolPoint.Reset();
    ActiveWindowPoint.Reset();
    PerPlayerPenaltyTimestampSeconds.Reset();
    LastGlobalPenaltyTimestampSeconds = -1.0;
    LastChaseRefreshTimestampSeconds = 0.0;
    EscaperActionEndTimestampSeconds = 0.0;
    ResetEscaperPhysicalThrowRuntime();

    BindHospitalExits();

    if (AEMRHubertCharacter* SpawnedHubert = EnsureHubertCharacter(false))
    {
        SpawnedHubert->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        if (const AActor* NightAnchor = NightShiftSpawnAnchor.Get())
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Night runtime placing Hubert actor=%s anchor=%s location=%s rotation=%s"),
                *GetNameSafe(SpawnedHubert),
                *GetNameSafe(NightAnchor),
                *NightAnchor->GetActorLocation().ToCompactString(),
                *NightAnchor->GetActorRotation().ToCompactString());
            SpawnedHubert->SetActorLocationAndRotation(
                NightAnchor->GetActorLocation(),
                NightAnchor->GetActorRotation(),
                false,
                nullptr,
                ETeleportType::TeleportPhysics);
        }

        if (UCharacterMovementComponent* MovementComponent = SpawnedHubert->GetCharacterMovement())
        {
            if (MovementComponent->MovementMode == MOVE_None)
            {
                MovementComponent->SetMovementMode(MOVE_Walking);
                UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Night runtime restored movement actor=%s movementMode=%d"),
                    *GetNameSafe(SpawnedHubert),
                    static_cast<int32>(MovementComponent->MovementMode));
            }
        }

        SpawnedHubert->SetHubertState(EEMRHubertState::NightPatrol);
        ApplyHubertMoveSpeed(PatrolMoveSpeed);
    }

    EnterState(EEMRHubertState::NightPatrol);
    SetActorTickEnabled(true);
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] NightShift runtime started state=%s patrolPoints=%d windowPoints=%d"),
        HubertStateToString_Director(CurrentState),
        PatrolRoutePoints.Num(),
        WindowWatchPoints.Num());
}

void AEMRHubertDirector::StopHubertRuntime()
{
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] StopHubertRuntime director=%s authority=%d"),
        *GetNameSafe(this),
        HasAuthority() ? 1 : 0);

    if (!HasAuthority())
    {
        return;
    }

    const bool bWasNightRuntime = bNightRuntimeActive;
    AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    RestoreEscaperGrabCarryOverrides(ActiveEscaper.Get(), SpawnedHubert);

    bHubRuntimeActive = false;
    bNightRuntimeActive = false;
    SetActorTickEnabled(false);

    UnbindHospitalExits();

    PendingEscaperQueue.Reset();
    ActiveEscaper.Reset();
    ActivePatrolPoint.Reset();
    ActiveWindowPoint.Reset();
    EscaperActionEndTimestampSeconds = 0.0;
    ResetEscaperPhysicalThrowRuntime();

    if (bWasNightRuntime)
    {
        if (SpawnedHubert)
        {
            SpawnedHubert->Destroy();
        }
        HubertCharacter.Reset();
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Runtime stopped wasNightRuntime=%d"), bWasNightRuntime ? 1 : 0);
}

void AEMRHubertDirector::QueueOutcomeAnnouncements(const TArray<EEMRHubertOutcomeAnnouncement>& Announcements)
{
    if (Announcements.Num() <= 0)
    {
        return;
    }

    PendingOutcomeAnnouncements.Append(Announcements);
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Queued outcome announcements added=%d pending=%d"),
        Announcements.Num(),
        PendingOutcomeAnnouncements.Num());
}

void AEMRHubertDirector::NotifyPlayerExitedHospital(AEMRPlayerCharacter* Escaper)
{
    if (!HasAuthority() || !bNightRuntimeActive || !Escaper)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] NotifyPlayerExitedHospital ignored authority=%d nightRuntime=%d escaper=%s"),
            HasAuthority() ? 1 : 0,
            bNightRuntimeActive ? 1 : 0,
            *GetNameSafe(Escaper));
        return;
    }

    if (!CanAcceptEscaperEvent(Escaper))
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] NotifyPlayerExitedHospital rejected by CanAcceptEscaperEvent escaper=%s"),
            *GetNameSafe(Escaper));
        return;
    }

    if (ActiveEscaper.Get() == Escaper)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] NotifyPlayerExitedHospital ignored because escaper is already active escaper=%s"),
            *GetNameSafe(Escaper));
        return;
    }

    for (const TWeakObjectPtr<AEMRPlayerCharacter>& PendingEscaper : PendingEscaperQueue)
    {
        if (PendingEscaper.Get() == Escaper)
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] NotifyPlayerExitedHospital ignored because escaper already queued escaper=%s"),
                *GetNameSafe(Escaper));
            return;
        }
    }

    const double CurrentServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    LastGlobalPenaltyTimestampSeconds = CurrentServerTime;
    PerPlayerPenaltyTimestampSeconds.FindOrAdd(Escaper) = CurrentServerTime;

    PendingEscaperQueue.Add(Escaper);

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Escaper queued player=%s pendingCount=%d"),
        *GetNameSafe(Escaper),
        PendingEscaperQueue.Num());
    TryStartNextEscaperChase();
}

void AEMRHubertDirector::EnterState(const EEMRHubertState NewState)
{
    const EEMRHubertState PreviousState = CurrentState;
    CurrentState = NewState;
    EscaperActionEndTimestampSeconds = 0.0;
    if (NewState != EEMRHubertState::ReturnEscaper)
    {
        ResetEscaperPhysicalThrowRuntime();
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] EnterState director=%s from=%s to=%s activeEscaper=%s"),
        *GetNameSafe(this),
        HubertStateToString_Director(PreviousState),
        HubertStateToString_Director(NewState),
        *GetNameSafe(ActiveEscaper.Get()));

    AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    if (SpawnedHubert)
    {
        SpawnedHubert->SetHubertState(NewState);
    }

    switch (NewState)
    {
    case EEMRHubertState::HubDriver:
        StopHubertMovement();
        break;
    case EEMRHubertState::NightPatrol:
        RestoreEscaperGrabCarryOverrides(ActiveEscaper.Get(), SpawnedHubert);
        ApplyHubertMoveSpeed(PatrolMoveSpeed);
        ActivePatrolPoint = SelectRandomPatrolPoint();
        if (const AActor* PatrolPoint = ActivePatrolPoint.Get())
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] NightPatrol target point=%s location=%s"),
                *GetNameSafe(PatrolPoint),
                *PatrolPoint->GetActorLocation().ToCompactString());
            MoveHubertToLocation(PatrolPoint->GetActorLocation(), PatrolArrivalDistance);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] No patrol points configured."));
            StopHubertMovement();
        }
        ScheduleNextWindowWatch();
        break;
    case EEMRHubertState::WindowWatch:
        ApplyHubertMoveSpeed(PatrolMoveSpeed);
        ActiveWindowPoint = SelectRandomWindowWatchPoint();
        if (const AActor* WindowPoint = ActiveWindowPoint.Get())
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] WindowWatch target point=%s location=%s"),
                *GetNameSafe(WindowPoint),
                *WindowPoint->GetActorLocation().ToCompactString());
            MoveHubertToLocation(WindowPoint->GetActorLocation(), PatrolArrivalDistance);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] No window-watch points configured, returning to patrol."));
            EnterState(EEMRHubertState::NightPatrol);
            return;
        }
        break;
    case EEMRHubertState::ChaseEscaper:
        ApplyHubertMoveSpeed(ChaseMoveSpeed);
        LastChaseRefreshTimestampSeconds = 0.0;
        if (const AEMRPlayerCharacter* Escaper = ActiveEscaper.Get())
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ChaseEscaper target=%s location=%s"),
                *GetNameSafe(Escaper),
                *Escaper->GetActorLocation().ToCompactString());
            MoveHubertToLocation(Escaper->GetActorLocation(), EscapeGrabDistance);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ChaseEscaper entered without a valid target."));
            EnterState(EEMRHubertState::NightPatrol);
            return;
        }
        break;
    case EEMRHubertState::GrabEscaper:
    {
        AEMRPlayerCharacter* Escaper = ActiveEscaper.Get();
        if (!SpawnedHubert || !Escaper)
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] GrabEscaper entered without valid Hubert or target."));
            ActiveEscaper.Reset();
            EnterState(EEMRHubertState::NightPatrol);
            return;
        }

        StopHubertMovement();
        EscaperActionEndTimestampSeconds = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0) + FMath::Max(GrabAnimationDurationSeconds, 0.0f);
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] GrabEscaper start escaper=%s endTime=%.3f socket=%s"),
            *GetNameSafe(Escaper),
            EscaperActionEndTimestampSeconds,
            *GrabAttachSocketName.ToString());

        if (UCharacterMovementComponent* EscaperMovement = Escaper->GetCharacterMovement())
        {
            EscaperMovement->DisableMovement();
        }

        ApplyEscaperGrabCarryOverrides(Escaper, SpawnedHubert);

        if (USkeletalMeshComponent* HubertMesh = SpawnedHubert->GetMesh())
        {
            Escaper->AttachToComponent(
                HubertMesh,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                GrabAttachSocketName.IsNone() ? NAME_None : GrabAttachSocketName);
        }
        else
        {
            Escaper->AttachToActor(SpawnedHubert, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        }

        break;
    }
    case EEMRHubertState::CarryEscaper:
        ApplyHubertMoveSpeed(CarryMoveSpeed);
        LastChaseRefreshTimestampSeconds = 0.0;
        if (const AActor* CarryAnchor = ThrowEscaperAnchor.Get())
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CarryEscaper target anchor=%s location=%s"),
                *GetNameSafe(CarryAnchor),
                *CarryAnchor->GetActorLocation().ToCompactString());
            MoveHubertToLocation(CarryAnchor->GetActorLocation(), CarryArrivalDistance);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Missing ThrowEscaperAnchor, skipping carry and enforcing immediately."));
            EnterState(EEMRHubertState::ThrowEscaper);
            return;
        }
        break;
    case EEMRHubertState::ThrowEscaper:
        StopHubertMovement();
        EscaperActionEndTimestampSeconds = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0) + FMath::Max(ThrowAnimationDurationSeconds, 0.0f);
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ThrowEscaper start escaper=%s endTime=%.3f"),
            *GetNameSafe(ActiveEscaper.Get()),
            EscaperActionEndTimestampSeconds);
        if (SpawnedHubert && ThrowEscaperAnchor)
        {
            SpawnedHubert->SetActorRotation(ThrowEscaperAnchor->GetActorRotation());
        }
        break;
    case EEMRHubertState::ReturnEscaper:
        StopHubertMovement();
        break;
    default:
        break;
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] EnterState complete current=%s"),
        HubertStateToString_Director(CurrentState));
}

void AEMRHubertDirector::UpdateHubRuntime(float DeltaSeconds)
{
    (void)DeltaSeconds;
    if (PendingOutcomeAnnouncements.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] UpdateHubRuntime pendingAnnouncements=%d nextAnnouncementTime=%.3f"),
            PendingOutcomeAnnouncements.Num(),
            NextOutcomeAnnouncementTimestampSeconds);
    }
    TryPlayPendingOutcomeAnnouncement();
}

void AEMRHubertDirector::UpdateNightRuntime(float DeltaSeconds)
{
    (void)DeltaSeconds;

    ClearInvalidEscaperQueue();

    if (!bNightRuntimeActive)
    {
        return;
    }

    const AEMRNightShiftGameState* RunGameState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
    if (!RunGameState || RunGameState->GetRunPhase() != EER_RunPhase::InNightShift)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] UpdateNightRuntime paused phase invalid runGameState=%s"),
            *GetNameSafe(RunGameState));
        return;
    }

    if (const AEMRNightShiftGameMode* NightShiftGameMode = GetWorld() ? Cast<AEMRNightShiftGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
    {
        if (NightShiftGameMode->IsNightShiftFinishPending())
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] UpdateNightRuntime paused because finish is pending"));
            return;
        }
    }

    if (!ActiveEscaper.IsValid() && PendingEscaperQueue.Num() > 0 && CurrentState != EEMRHubertState::ChaseEscaper && CurrentState != EEMRHubertState::ReturnEscaper)
    {
        TryStartNextEscaperChase();
    }

    switch (CurrentState)
    {
    case EEMRHubertState::NightPatrol:
        UpdateNightPatrolState();
        break;
    case EEMRHubertState::WindowWatch:
        UpdateWindowWatchState();
        break;
    case EEMRHubertState::ChaseEscaper:
        UpdateChaseEscaperState();
        break;
    case EEMRHubertState::GrabEscaper:
        UpdateGrabEscaperState();
        break;
    case EEMRHubertState::CarryEscaper:
        UpdateCarryEscaperState();
        break;
    case EEMRHubertState::ThrowEscaper:
        UpdateThrowEscaperState();
        break;
    case EEMRHubertState::ReturnEscaper:
        UpdateReturnEscaperState();
        break;
    default:
        break;
    }
}

void AEMRHubertDirector::UpdateNightPatrolState()
{
    AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    if (!SpawnedHubert)
    {
        return;
    }

    const double CurrentServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (WindowWatchPoints.Num() > 0 && CurrentServerTime >= NextWindowWatchServerTime)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] NightPatrol switching to WindowWatch currentTime=%.3f nextWatchTime=%.3f"),
            CurrentServerTime,
            NextWindowWatchServerTime);
        EnterState(EEMRHubertState::WindowWatch);
        return;
    }

    if (const AActor* PatrolPoint = ActivePatrolPoint.Get())
    {
        if (HasReachedLocation(PatrolPoint->GetActorLocation(), PatrolArrivalDistance))
        {
            ActivePatrolPoint = SelectRandomPatrolPoint();
            if (const AActor* NextPatrolPoint = ActivePatrolPoint.Get())
            {
                UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] NightPatrol reached point; moving to next=%s location=%s"),
                    *GetNameSafe(NextPatrolPoint),
                    *NextPatrolPoint->GetActorLocation().ToCompactString());
                MoveHubertToLocation(NextPatrolPoint->GetActorLocation(), PatrolArrivalDistance);
            }
        }
    }
    else
    {
        ActivePatrolPoint = SelectRandomPatrolPoint();
        if (const AActor* NextPatrolPoint = ActivePatrolPoint.Get())
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] NightPatrol selected new point=%s location=%s"),
                *GetNameSafe(NextPatrolPoint),
                *NextPatrolPoint->GetActorLocation().ToCompactString());
            MoveHubertToLocation(NextPatrolPoint->GetActorLocation(), PatrolArrivalDistance);
        }
    }
}

void AEMRHubertDirector::UpdateWindowWatchState()
{
    AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    if (!SpawnedHubert)
    {
        return;
    }

    if (const AActor* WindowPoint = ActiveWindowPoint.Get())
    {
        if (HasReachedLocation(WindowPoint->GetActorLocation(), PatrolArrivalDistance))
        {
            StopHubertMovement();
            SpawnedHubert->SetActorRotation(WindowPoint->GetActorRotation());
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] WindowWatch reached point=%s facing=%s"),
                *GetNameSafe(WindowPoint),
                *WindowPoint->GetActorRotation().ToCompactString());
        }
    }
    else
    {
        EnterState(EEMRHubertState::NightPatrol);
        return;
    }

    if (CanAnyPlayerSeeHubert())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Window watch interrupted: player sighted Hubert."));
        EnterState(EEMRHubertState::NightPatrol);
    }
}

void AEMRHubertDirector::UpdateChaseEscaperState()
{
    AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    AEMRPlayerCharacter* Escaper = ActiveEscaper.Get();
    if (!SpawnedHubert || !Escaper)
    {
        ActiveEscaper.Reset();
        EnterState(EEMRHubertState::NightPatrol);
        return;
    }

    const FVector HubertLocation = SpawnedHubert->GetActorLocation();
    const FVector EscaperLocation = Escaper->GetActorLocation();
    const float SafeGrabDistance = FMath::Max(EscapeGrabDistance, 1.0f);
    const float HubertRadius = FMath::Max(SpawnedHubert->GetSimpleCollisionRadius(), 0.0f);
    const float EscaperRadius = FMath::Max(Escaper->GetSimpleCollisionRadius(), 0.0f);
    const float Grab2DReachTolerance = SafeGrabDistance + HubertRadius + EscaperRadius;

    const float DistanceToEscaper3D = FVector::Dist(HubertLocation, EscaperLocation);
    const float DistanceToEscaper2D = FVector::Dist2D(HubertLocation, EscaperLocation);
    const bool bWithinGrabRange3D = DistanceToEscaper3D <= SafeGrabDistance;
    const bool bWithinGrabRange2D = DistanceToEscaper2D <= Grab2DReachTolerance;

    if (bWithinGrabRange3D || bWithinGrabRange2D)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ChaseEscaper within grab range escaper=%s dist3D=%.2f dist2D=%.2f threshold3D=%.2f threshold2D=%.2f"),
            *GetNameSafe(Escaper),
            DistanceToEscaper3D,
            DistanceToEscaper2D,
            SafeGrabDistance,
            Grab2DReachTolerance);
        EnterState(EEMRHubertState::GrabEscaper);
        return;
    }

    const double CurrentServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const bool bShouldRefreshMove = LastChaseRefreshTimestampSeconds <= 0.0
    || CurrentServerTime - LastChaseRefreshTimestampSeconds >= FMath::Max(ChasePathRefreshIntervalSeconds, 0.05f);

    if (bShouldRefreshMove)
    {
        LastChaseRefreshTimestampSeconds = CurrentServerTime;
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ChaseEscaper refresh move target=%s targetLocation=%s hubertLocation=%s dist2D=%.2f dist3D=%.2f threshold2D=%.2f"),
            *GetNameSafe(Escaper),
            *EscaperLocation.ToCompactString(),
            *HubertLocation.ToCompactString(),
            DistanceToEscaper2D,
            DistanceToEscaper3D,
            Grab2DReachTolerance);
        MoveHubertToLocation(EscaperLocation, EscapeGrabDistance);
    }
}

void AEMRHubertDirector::UpdateGrabEscaperState()
{
    AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    AEMRPlayerCharacter* Escaper = ActiveEscaper.Get();
    if (!SpawnedHubert || !Escaper)
    {
        ActiveEscaper.Reset();
        EnterState(EEMRHubertState::NightPatrol);
        return;
    }

    const double CurrentServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (CurrentServerTime >= EscaperActionEndTimestampSeconds)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] GrabEscaper completed escaper=%s currentTime=%.3f endTime=%.3f"),
            *GetNameSafe(Escaper),
            CurrentServerTime,
            EscaperActionEndTimestampSeconds);
        EnterState(EEMRHubertState::CarryEscaper);
    }
}

void AEMRHubertDirector::UpdateCarryEscaperState()
{
    AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    AEMRPlayerCharacter* Escaper = ActiveEscaper.Get();
    if (!SpawnedHubert || !Escaper)
    {
        ActiveEscaper.Reset();
        EnterState(EEMRHubertState::NightPatrol);
        return;
    }

    ApplyEscaperGrabCarryOverrides(Escaper, SpawnedHubert);

    const AActor* CarryAnchor = ThrowEscaperAnchor.Get();
    if (!CarryAnchor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CarryEscaper missing ThrowEscaperAnchor; forcing ThrowEscaper"));
        EnterState(EEMRHubertState::ThrowEscaper);
        return;
    }

    if (Escaper->GetAttachParentActor() != SpawnedHubert)
    {
        if (USkeletalMeshComponent* HubertMesh = SpawnedHubert->GetMesh())
        {
            Escaper->AttachToComponent(
                HubertMesh,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                GrabAttachSocketName.IsNone() ? NAME_None : GrabAttachSocketName);
        }
        else
        {
            Escaper->AttachToActor(SpawnedHubert, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        }
    }

    const FVector CarryAnchorLocation = CarryAnchor->GetActorLocation();
    const FVector HubertLocation = SpawnedHubert->GetActorLocation();
    const float SafeCarryTolerance = FMath::Max(CarryArrivalDistance, 1.0f);
    const float HubertSimpleRadius = FMath::Max(SpawnedHubert->GetSimpleCollisionRadius(), 0.0f);
    float HubertCapsuleRadius = 0.0f;
    if (const UCapsuleComponent* HubertCapsule = SpawnedHubert->GetCapsuleComponent())
    {
        HubertCapsuleRadius = FMath::Max(HubertCapsule->GetScaledCapsuleRadius(), 0.0f);
    }
    float HubertNavAgentRadius = 0.0f;
    if (const UCharacterMovementComponent* HubertMovement = SpawnedHubert->GetCharacterMovement())
    {
        HubertNavAgentRadius = FMath::Max(HubertMovement->GetNavAgentPropertiesRef().AgentRadius, 0.0f);
    }

    const float HubertEffectiveRadius = FMath::Max3(HubertSimpleRadius, HubertCapsuleRadius, HubertNavAgentRadius);
    constexpr float CarryReachEpsilon = 10.0f;
    const float Carry2DReachTolerance = SafeCarryTolerance + HubertEffectiveRadius + CarryReachEpsilon;

    const bool bReachedCarryAnchor3D =
    FVector::DistSquared(HubertLocation, CarryAnchorLocation) <= FMath::Square(SafeCarryTolerance);
    const bool bReachedCarryAnchor2D =
    FVector::DistSquared2D(HubertLocation, CarryAnchorLocation) <= FMath::Square(Carry2DReachTolerance);

    if (bReachedCarryAnchor3D || bReachedCarryAnchor2D)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CarryEscaper reached throw anchor=%s location=%s hubertLocation=%s tolerance3D=%.2f tolerance2D=%.2f"),
            *GetNameSafe(CarryAnchor),
            *CarryAnchorLocation.ToCompactString(),
            *HubertLocation.ToCompactString(),
            SafeCarryTolerance,
            Carry2DReachTolerance);
        EnterState(EEMRHubertState::ThrowEscaper);
        return;
    }

    const double CurrentServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const bool bShouldRefreshMove = LastChaseRefreshTimestampSeconds <= 0.0
    || CurrentServerTime - LastChaseRefreshTimestampSeconds >= FMath::Max(ChasePathRefreshIntervalSeconds, 0.05f);

    if (bShouldRefreshMove)
    {
        LastChaseRefreshTimestampSeconds = CurrentServerTime;
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CarryEscaper refresh move anchor=%s location=%s hubertLocation=%s dist2D=%.2f dist3D=%.2f tolerance2D=%.2f"),
            *GetNameSafe(CarryAnchor),
            *CarryAnchorLocation.ToCompactString(),
            *HubertLocation.ToCompactString(),
            FVector::Dist2D(HubertLocation, CarryAnchorLocation),
            FVector::Dist(HubertLocation, CarryAnchorLocation),
            Carry2DReachTolerance);
        MoveHubertToLocation(CarryAnchorLocation, CarryArrivalDistance);
    }
}

void AEMRHubertDirector::UpdateThrowEscaperState()
{
    AEMRPlayerCharacter* Escaper = ActiveEscaper.Get();
    if (!Escaper)
    {
        ActiveEscaper.Reset();
        EnterState(EEMRHubertState::NightPatrol);
        return;
    }

    const double CurrentServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (CurrentServerTime < EscaperActionEndTimestampSeconds)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ThrowEscaper completed escaper=%s currentTime=%.3f endTime=%.3f"),
        *GetNameSafe(Escaper),
        CurrentServerTime,
        EscaperActionEndTimestampSeconds);
    if (BeginEscaperPhysicalThrow(Escaper))
    {
        ApplyEscapeRevenuePenalty(Escaper);
        PerPlayerPenaltyTimestampSeconds.FindOrAdd(Escaper) = CurrentServerTime;
        EnterState(EEMRHubertState::ReturnEscaper);
        return;
    }

    ExecuteEscaperEnforcement(Escaper);
}

void AEMRHubertDirector::UpdateReturnEscaperState()
{
    AEMRPlayerCharacter* Escaper = ActiveEscaper.Get();
    if (!Escaper)
    {
        ActiveEscaper.Reset();
        ResetEscaperPhysicalThrowRuntime();
        TryStartNextEscaperChase();
        if (!ActiveEscaper.IsValid())
        {
            EnterState(EEMRHubertState::NightPatrol);
        }
        return;
    }

    const double CurrentServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (EscaperThrowStartTimestampSeconds <= 0.0)
    {
        EscaperThrowStartTimestampSeconds = CurrentServerTime;
    }

    if (CurrentServerTime - EscaperThrowStartTimestampSeconds >= FMath::Max(ThrowFallbackTimeoutSeconds, 0.05f))
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ReturnEscaper fallback timeout escaper=%s elapsed=%.3f"),
            *GetNameSafe(Escaper),
            CurrentServerTime - EscaperThrowStartTimestampSeconds);
        FinalizeEscaperPhysicalThrow(Escaper, true);
        return;
    }

    const bool bFlightTimeElapsed =
    CurrentServerTime - EscaperThrowStartTimestampSeconds >= FMath::Max(ThrowMinFlightTimeBeforeSettleSeconds, 0.0f);
    if (!bFlightTimeElapsed)
    {
        return;
    }

    if (IsEscaperVelocityBelowThreshold(Escaper))
    {
        if (EscaperLowVelocityWindowStartTimestampSeconds <= 0.0)
        {
            EscaperLowVelocityWindowStartTimestampSeconds = CurrentServerTime;
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ReturnEscaper low-velocity window started escaper=%s time=%.3f"),
                *GetNameSafe(Escaper),
                CurrentServerTime);
        }

        if (EscaperSettledTimestampSeconds <= 0.0
            && CurrentServerTime - EscaperLowVelocityWindowStartTimestampSeconds >= FMath::Max(ThrowLowVelocityWindowSeconds, 0.0f))
        {
            EscaperSettledTimestampSeconds = CurrentServerTime;
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ReturnEscaper settled escaper=%s windowDuration=%.3f"),
                *GetNameSafe(Escaper),
                CurrentServerTime - EscaperLowVelocityWindowStartTimestampSeconds);
        }
    }
    else
    {
        if (EscaperLowVelocityWindowStartTimestampSeconds > 0.0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ReturnEscaper low-velocity window reset escaper=%s elapsed=%.3f"),
                *GetNameSafe(Escaper),
                CurrentServerTime - EscaperLowVelocityWindowStartTimestampSeconds);
        }
        EscaperLowVelocityWindowStartTimestampSeconds = 0.0;
        EscaperSettledTimestampSeconds = 0.0;
    }

    if (EscaperSettledTimestampSeconds > 0.0
        && CurrentServerTime - EscaperSettledTimestampSeconds >= FMath::Max(ThrowRecoveryDelaySeconds, 0.0f))
    {
        bool bUseTeleportFallback = false;
        if (ReturnInsideAnchor && ThrowSuccessDistanceFromReturnAnchor > 0.0f)
        {
            FVector EscaperReferenceLocation = Escaper->GetActorLocation();
            FName PhysicsBoneForDistance = NAME_None;
            if (USkeletalMeshComponent* EscaperMesh = ResolveEscaperMeshForCarry(Escaper))
            {
                PhysicsBoneForDistance = ResolveEscaperThrowPhysicsBone(EscaperMesh);
                if (PhysicsBoneForDistance != NAME_None)
                {
                    EscaperReferenceLocation = EscaperMesh->GetBoneLocation(PhysicsBoneForDistance);
                }
            }

            const float DistanceToInsideAnchor = FVector::Dist(EscaperReferenceLocation, ReturnInsideAnchor->GetActorLocation());
            bUseTeleportFallback = DistanceToInsideAnchor > ThrowSuccessDistanceFromReturnAnchor;
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ReturnEscaper finalize distance check escaper=%s actorLocation=%s referenceLocation=%s referenceBone=%s distance=%.2f tolerance=%.2f fallback=%d"),
                *GetNameSafe(Escaper),
                *Escaper->GetActorLocation().ToCompactString(),
                *EscaperReferenceLocation.ToCompactString(),
                *PhysicsBoneForDistance.ToString(),
                DistanceToInsideAnchor,
                ThrowSuccessDistanceFromReturnAnchor,
                bUseTeleportFallback ? 1 : 0);
        }

        FinalizeEscaperPhysicalThrow(Escaper, bUseTeleportFallback);
    }

    if (CurrentServerTime >= EscaperNextThrowDebugLogTimestampSeconds)
    {
        if (USkeletalMeshComponent* EscaperMesh = ResolveEscaperMeshForCarry(Escaper))
        {
            const FName PhysicsBone = ResolveEscaperThrowPhysicsBone(EscaperMesh);

            const FVector Velocity = PhysicsBone == NAME_None
                ? FVector::ZeroVector
                : EscaperMesh->GetPhysicsLinearVelocity(PhysicsBone);
            UE_LOG(LogTemp, Verbose, TEXT("[HUBERT_FLOW] ReturnEscaper debug escaper=%s speed=%.2f bone=%s lowWindowStart=%.3f settled=%.3f"),
                *GetNameSafe(Escaper),
                Velocity.Size(),
                *PhysicsBone.ToString(),
                EscaperLowVelocityWindowStartTimestampSeconds,
                EscaperSettledTimestampSeconds);
        }

        EscaperNextThrowDebugLogTimestampSeconds = CurrentServerTime + 0.25;
    }
}

AEMRHubertCharacter* AEMRHubertDirector::EnsureHubertCharacter(const bool bHubContext)
{
    if (AEMRHubertCharacter* ExistingHubert = HubertCharacter.Get())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] EnsureHubertCharacter reused existing actor=%s context=%s"),
            *GetNameSafe(ExistingHubert),
            bHubContext ? TEXT("Hub") : TEXT("Night"));
        return ExistingHubert;
    }

    if (!HubertCharacterClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Missing HubertCharacterClass on director=%s"), *GetNameSafe(this));
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const AActor* SpawnAnchor = bHubContext ? HubSpawnAnchor.Get() : NightShiftSpawnAnchor.Get();
    const FTransform SpawnTransform = SpawnAnchor ? SpawnAnchor->GetActorTransform() : GetActorTransform();
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] EnsureHubertCharacter spawning context=%s class=%s anchor=%s location=%s rotation=%s"),
        bHubContext ? TEXT("Hub") : TEXT("Night"),
        *GetNameSafe(HubertCharacterClass),
        *GetNameSafe(SpawnAnchor),
        *SpawnTransform.GetLocation().ToCompactString(),
        *SpawnTransform.Rotator().ToCompactString());

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = nullptr;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEMRHubertCharacter* SpawnedHubert = World->SpawnActor<AEMRHubertCharacter>(HubertCharacterClass, SpawnTransform, SpawnParameters);
    if (!SpawnedHubert)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Failed to spawn HubertCharacter from class=%s"), *GetNameSafe(HubertCharacterClass));
        return nullptr;
    }

    SpawnedHubert->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

    HubertCharacter = SpawnedHubert;
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] EnsureHubertCharacter spawned actor=%s"),
        *GetNameSafe(SpawnedHubert));
    return SpawnedHubert;
}

bool AEMRHubertDirector::MoveHubertToLocation(const FVector& TargetLocation, const float AcceptanceRadius)
{
    AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    if (!SpawnedHubert)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] MoveHubertToLocation failed because HubertCharacter is null"));
        return false;
    }

    AAIController* AIController = Cast<AAIController>(SpawnedHubert->GetController());
    if (!AIController)
    {
        SpawnedHubert->SpawnDefaultController();
        AIController = Cast<AAIController>(SpawnedHubert->GetController());
    }

    if (!AIController)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Missing AIController for Hubert movement."));
        return false;
    }

    const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
        TargetLocation,
        FMath::Max(AcceptanceRadius, 1.0f),
        true,
        true,
        false,
        true,
        nullptr,
        true);

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] MoveHubertToLocation actor=%s target=%s acceptance=%.2f result=%d"),
        *GetNameSafe(SpawnedHubert),
        *TargetLocation.ToCompactString(),
        AcceptanceRadius,
        static_cast<int32>(MoveResult));

    return MoveResult != EPathFollowingRequestResult::Failed;
}

void AEMRHubertDirector::StopHubertMovement()
{
    AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    if (!SpawnedHubert)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] StopHubertMovement skipped because HubertCharacter is null"));
        return;
    }

    if (AAIController* AIController = Cast<AAIController>(SpawnedHubert->GetController()))
    {
        AIController->StopMovement();
    }

    if (UCharacterMovementComponent* MovementComponent = SpawnedHubert->GetCharacterMovement())
    {
        MovementComponent->StopMovementImmediately();
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] StopHubertMovement actor=%s location=%s"),
        *GetNameSafe(SpawnedHubert),
        *SpawnedHubert->GetActorLocation().ToCompactString());
}

void AEMRHubertDirector::ApplyHubertMoveSpeed(const float NewMoveSpeed) const
{
    if (AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get())
    {
        if (UCharacterMovementComponent* MovementComponent = SpawnedHubert->GetCharacterMovement())
        {
            MovementComponent->MaxWalkSpeed = FMath::Max(NewMoveSpeed, 0.0f);
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ApplyHubertMoveSpeed actor=%s speed=%.2f"),
                *GetNameSafe(SpawnedHubert),
                MovementComponent->MaxWalkSpeed);
        }
    }
}

bool AEMRHubertDirector::HasReachedLocation(const FVector& TargetLocation, const float Tolerance) const
{
    const AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    if (!SpawnedHubert)
    {
        return false;
    }

    return FVector::DistSquared(SpawnedHubert->GetActorLocation(), TargetLocation) <= FMath::Square(FMath::Max(Tolerance, 1.0f));
}

void AEMRHubertDirector::BindHospitalExits()
{
    UnbindHospitalExits();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AEMRHospitalExit> It(World); It; ++It)
    {
        AEMRHospitalExit* ExitActor = *It;
        if (!ExitActor)
        {
            continue;
        }

        FHubertExitBinding& Binding = ExitBindings.AddDefaulted_GetRef();
        Binding.ExitActor = ExitActor;
        Binding.DelegateHandle = ExitActor->OnPlayerExitOverlapNative().AddUObject(this, &ThisClass::HandleHospitalExitOverlap);
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Bound %d hospital exits for escape detection."), ExitBindings.Num());
}

void AEMRHubertDirector::UnbindHospitalExits()
{
    const int32 PreviousBindingCount = ExitBindings.Num();
    for (const FHubertExitBinding& Binding : ExitBindings)
    {
        if (AEMRHospitalExit* ExitActor = Binding.ExitActor.Get())
        {
            ExitActor->OnPlayerExitOverlapNative().Remove(Binding.DelegateHandle);
        }
    }

    ExitBindings.Reset();
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Unbound hospital exits count=%d"), PreviousBindingCount);
}

void AEMRHubertDirector::HandleHospitalExitOverlap(AEMRPlayerCharacter* Escaper)
{
    NotifyPlayerExitedHospital(Escaper);
}

void AEMRHubertDirector::ScheduleNextWindowWatch()
{
    const double CurrentServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const float SafeMinDelay = FMath::Max(WindowWatchMinDelaySeconds, 0.0f);
    float SafeMaxDelay = FMath::Max(WindowWatchMaxDelaySeconds, 0.0f);
    if (SafeMaxDelay < SafeMinDelay)
    {
        SafeMaxDelay = SafeMinDelay;
    }

    NextWindowWatchServerTime = CurrentServerTime + FMath::FRandRange(SafeMinDelay, SafeMaxDelay);
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Scheduled next window watch currentTime=%.3f nextTime=%.3f minDelay=%.2f maxDelay=%.2f"),
        CurrentServerTime,
        NextWindowWatchServerTime,
        SafeMinDelay,
        SafeMaxDelay);
}

AActor* AEMRHubertDirector::SelectRandomPatrolPoint() const
{
    TArray<AActor*> ValidPatrolPoints;
    ValidPatrolPoints.Reserve(PatrolRoutePoints.Num());
    for (AActor* PatrolPoint : PatrolRoutePoints)
    {
        if (PatrolPoint)
        {
            ValidPatrolPoints.Add(PatrolPoint);
        }
    }

    if (ValidPatrolPoints.Num() <= 0)
    {
        return nullptr;
    }

    const int32 RandomIndex = FMath::RandRange(0, ValidPatrolPoints.Num() - 1);
    AActor* ChosenPoint = ValidPatrolPoints[RandomIndex];
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] SelectRandomPatrolPoint selected=%s location=%s"),
        *GetNameSafe(ChosenPoint),
        *ChosenPoint->GetActorLocation().ToCompactString());
    return ChosenPoint;
}

AActor* AEMRHubertDirector::SelectRandomWindowWatchPoint() const
{
    TArray<AActor*> ValidWindowPoints;
    ValidWindowPoints.Reserve(WindowWatchPoints.Num());
    for (AActor* WindowPoint : WindowWatchPoints)
    {
        if (WindowPoint)
        {
            ValidWindowPoints.Add(WindowPoint);
        }
    }

    if (ValidWindowPoints.Num() <= 0)
    {
        return nullptr;
    }

    const int32 RandomIndex = FMath::RandRange(0, ValidWindowPoints.Num() - 1);
    AActor* ChosenPoint = ValidWindowPoints[RandomIndex];
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] SelectRandomWindowWatchPoint selected=%s location=%s"),
        *GetNameSafe(ChosenPoint),
        *ChosenPoint->GetActorLocation().ToCompactString());
    return ChosenPoint;
}

bool AEMRHubertDirector::CanAnyPlayerSeeHubert() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PlayerController = It->Get();
        if (IsPlayerViewingHubert(PlayerController))
        {
            return true;
        }
    }

    return false;
}

bool AEMRHubertDirector::IsPlayerViewingHubert(const APlayerController* PlayerController) const
{
    const AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get();
    if (!PlayerController || !SpawnedHubert)
    {
        return false;
    }

    const APawn* PlayerPawn = PlayerController->GetPawn();
    if (!PlayerPawn)
    {
        return false;
    }

    FVector ViewLocation = FVector::ZeroVector;
    FRotator ViewRotation = FRotator::ZeroRotator;
    PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

    const FVector HubertLocation = SpawnedHubert->GetActorLocation();
    const FVector ToHubert = HubertLocation - ViewLocation;
    const double DistanceSquared = ToHubert.SizeSquared();
    const double MaxDistance = FMath::Max(PlayerSeeMaxDistance, 0.0f);
    if (DistanceSquared > MaxDistance * MaxDistance)
    {
        return false;
    }

    if (ToHubert.IsNearlyZero())
    {
        return true;
    }

    const FVector ForwardVector = ViewRotation.Vector().GetSafeNormal();
    const FVector DirectionToHubert = ToHubert.GetSafeNormal();
    const double CosHalfFOV = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(PlayerSeeFOVDegrees, 1.0f, 179.0f) * 0.5f));
    if (FVector::DotProduct(ForwardVector, DirectionToHubert) < CosHalfFOV)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HubertPlayerVisibility), false, PlayerPawn);
    QueryParams.AddIgnoredActor(this);

    FHitResult HitResult;
    const bool bHitSomething = World->LineTraceSingleByChannel(HitResult, ViewLocation, HubertLocation, ECC_Visibility, QueryParams);
    if (!bHitSomething)
    {
        return true;
    }

    const AActor* HitActor = HitResult.GetActor();
    return HitActor == SpawnedHubert || (HitActor && HitActor->GetOwner() == SpawnedHubert);
}

bool AEMRHubertDirector::AreAllHubPlayersLoaded() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const AGameStateBase* GameState = World->GetGameState();
    if (!GameState)
    {
        return false;
    }

    int32 TrackedPlayers = 0;
    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        if (!PlayerState)
        {
            continue;
        }

        ++TrackedPlayers;

        const AController* OwningController = PlayerState->GetOwningController();
        const APlayerController* PlayerController = Cast<APlayerController>(OwningController);
        if (!PlayerController || !PlayerController->GetPawn())
        {
            return false;
        }
    }

    return TrackedPlayers > 0;
}

bool AEMRHubertDirector::CanAcceptEscaperEvent(const AEMRPlayerCharacter* Escaper) const
{
    if (!Escaper || !GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CanAcceptEscaperEvent=false reason=InvalidEscaperOrWorld escaper=%s"),
            *GetNameSafe(Escaper));
        return false;
    }

    const AEMRNightShiftGameMode* NightShiftGameMode = Cast<AEMRNightShiftGameMode>(GetWorld()->GetAuthGameMode());
    if (NightShiftGameMode && NightShiftGameMode->IsNightShiftFinishPending())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CanAcceptEscaperEvent=false reason=NightShiftFinishPending escaper=%s"),
            *GetNameSafe(Escaper));
        return false;
    }

    const AEMRNightShiftGameState* NightShiftGameState = GetWorld()->GetGameState<AEMRNightShiftGameState>();
    if (!NightShiftGameState || NightShiftGameState->GetRunPhase() != EER_RunPhase::InNightShift)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CanAcceptEscaperEvent=false reason=RunPhaseNotInNightShift escaper=%s"),
            *GetNameSafe(Escaper));
        return false;
    }

    const double CurrentServerTime = GetWorld()->GetTimeSeconds();
    const double SafeGlobalDebounce = FMath::Max(GlobalPenaltyDebounceSeconds, 0.0f);
    if (LastGlobalPenaltyTimestampSeconds >= 0.0 && CurrentServerTime - LastGlobalPenaltyTimestampSeconds < SafeGlobalDebounce)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CanAcceptEscaperEvent=false reason=GlobalDebounce escaper=%s elapsed=%.3f required=%.3f"),
            *GetNameSafe(Escaper),
            CurrentServerTime - LastGlobalPenaltyTimestampSeconds,
            SafeGlobalDebounce);
        return false;
    }

    if (const double* LastPenaltyTimestamp = PerPlayerPenaltyTimestampSeconds.Find(Escaper))
    {
        const double SafePlayerCooldown = FMath::Max(PerPlayerPenaltyCooldownSeconds, 0.0f);
        if (CurrentServerTime - *LastPenaltyTimestamp < SafePlayerCooldown)
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CanAcceptEscaperEvent=false reason=PerPlayerCooldown escaper=%s elapsed=%.3f required=%.3f"),
                *GetNameSafe(Escaper),
                CurrentServerTime - *LastPenaltyTimestamp,
                SafePlayerCooldown);
            return false;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] CanAcceptEscaperEvent=true escaper=%s"), *GetNameSafe(Escaper));
    return true;
}

void AEMRHubertDirector::TryStartNextEscaperChase()
{
    if (!bNightRuntimeActive || ActiveEscaper.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] TryStartNextEscaperChase skipped nightRuntime=%d activeEscaper=%s"),
            bNightRuntimeActive ? 1 : 0,
            *GetNameSafe(ActiveEscaper.Get()));
        return;
    }

    ClearInvalidEscaperQueue();
    if (PendingEscaperQueue.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] TryStartNextEscaperChase skipped because queue is empty"));
        return;
    }

    ActiveEscaper = PendingEscaperQueue[0];
    PendingEscaperQueue.RemoveAt(0);

    if (!ActiveEscaper.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] TryStartNextEscaperChase selected invalid escaper entry"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Starting chase for escaper=%s"), *GetNameSafe(ActiveEscaper.Get()));
    EnterState(EEMRHubertState::ChaseEscaper);
}

void AEMRHubertDirector::ClearInvalidEscaperQueue()
{
    const int32 PreviousCount = PendingEscaperQueue.Num();
    for (int32 Index = PendingEscaperQueue.Num() - 1; Index >= 0; --Index)
    {
        if (!PendingEscaperQueue[Index].IsValid())
        {
            PendingEscaperQueue.RemoveAt(Index);
        }
    }

    if (PendingEscaperQueue.Num() != PreviousCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ClearInvalidEscaperQueue removed=%d remaining=%d"),
            PreviousCount - PendingEscaperQueue.Num(),
            PendingEscaperQueue.Num());
    }
}

USkeletalMeshComponent* AEMRHubertDirector::ResolveEscaperMeshForCarry(const AEMRPlayerCharacter* Escaper) const
{
    if (!Escaper)
    {
        return nullptr;
    }

    AEMRPlayerCharacter* MutableEscaper = const_cast<AEMRPlayerCharacter*>(Escaper);

    if (EscaperCarryMeshComponentTag != NAME_None)
    {
        const TArray<UActorComponent*> TaggedComponents =
            MutableEscaper->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), EscaperCarryMeshComponentTag);
        for (UActorComponent* Component : TaggedComponents)
        {
            if (USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(Component))
            {
                return MeshComponent;
            }
        }
    }

    if (EscaperCarryMeshComponentName != NAME_None)
    {
        TArray<USkeletalMeshComponent*> MeshComponents;
        MutableEscaper->GetComponents<USkeletalMeshComponent>(MeshComponents);
        for (USkeletalMeshComponent* MeshComponent : MeshComponents)
        {
            if (MeshComponent && MeshComponent->GetFName() == EscaperCarryMeshComponentName)
            {
                return MeshComponent;
            }
        }
    }

    if (USkeletalMeshComponent* PlayerMesh = MutableEscaper->GetPlayerMeshComponentForSeating())
    {
        return PlayerMesh;
    }

    return MutableEscaper->FindComponentByClass<USkeletalMeshComponent>();
}

FName AEMRHubertDirector::ResolveEscaperThrowPhysicsBone(const USkeletalMeshComponent* EscaperMesh) const
{
    if (!EscaperMesh)
    {
        return NAME_None;
    }

    TArray<FName> CandidateBones;
    if (ThrowRagdollStartBone != NAME_None)
    {
        CandidateBones.Add(ThrowRagdollStartBone);
    }
    if (EscaperCarryResolvedRagdollBone != NAME_None && EscaperCarryResolvedRagdollBone != ThrowRagdollStartBone)
    {
        CandidateBones.Add(EscaperCarryResolvedRagdollBone);
    }
    if (EscaperCarryRagdollStartBone != NAME_None
        && EscaperCarryRagdollStartBone != ThrowRagdollStartBone
        && EscaperCarryRagdollStartBone != EscaperCarryResolvedRagdollBone)
    {
        CandidateBones.Add(EscaperCarryRagdollStartBone);
    }

    for (const FName CandidateBone : CandidateBones)
    {
        if (EscaperMesh->GetBoneIndex(CandidateBone) == INDEX_NONE)
        {
            continue;
        }

        if (EscaperMesh->GetBodyInstance(CandidateBone))
        {
            return CandidateBone;
        }
    }

    if (const UPhysicsAsset* PhysicsAsset = EscaperMesh->GetPhysicsAsset())
    {
        for (const USkeletalBodySetup* BodySetup : PhysicsAsset->SkeletalBodySetups)
        {
            if (!BodySetup || BodySetup->BoneName == NAME_None)
            {
                continue;
            }

            if (EscaperMesh->GetBoneIndex(BodySetup->BoneName) == INDEX_NONE)
            {
                continue;
            }

            if (EscaperMesh->GetBodyInstance(BodySetup->BoneName))
            {
                return BodySetup->BoneName;
            }
        }
    }

    return NAME_None;
}

void AEMRHubertDirector::ApplyEscaperGrabCarryOverrides(AEMRPlayerCharacter* Escaper, AEMRHubertCharacter* SpawnedHubert)
{
    if (!Escaper || !SpawnedHubert)
    {
        return;
    }

    if (EscaperWithCarryOverrides.IsValid() && EscaperWithCarryOverrides.Get() != Escaper)
    {
        RestoreEscaperGrabCarryOverrides(EscaperWithCarryOverrides.Get(), SpawnedHubert);
    }

    const bool bAlreadySavedForEscaper =
    bEscaperCarryOverridesSaved && EscaperWithCarryOverrides.IsValid() && EscaperWithCarryOverrides.Get() == Escaper;
    EscaperWithCarryOverrides = Escaper;

    if (!bAlreadySavedForEscaper)
    {
        bEscaperCarryActorCollisionEnabled = Escaper->GetActorEnableCollision();
        if (const UCapsuleComponent* EscaperCapsule = Escaper->GetCapsuleComponent())
        {
            EscaperCarryCapsuleCollisionMode = EscaperCapsule->GetCollisionEnabled();
        }
        if (USkeletalMeshComponent* EscaperMesh = ResolveEscaperMeshForCarry(Escaper))
        {
            EscaperCarryMeshCollisionMode = EscaperMesh->GetCollisionEnabled();
            bEscaperCarryMeshCollisionSaved = true;
            bEscaperCarryMeshBlendPhysicsSaved = EscaperMesh->bBlendPhysics;
        }
        bEscaperCarryOverridesSaved = true;
        if (UCapsuleComponent* EscaperCapsule = Escaper->GetCapsuleComponent())
        {
            EscaperCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    Escaper->MoveIgnoreActorAdd(SpawnedHubert);
    SpawnedHubert->MoveIgnoreActorAdd(Escaper);

    if (!bEscaperCarryRagdollApplied)
    {
        USkeletalMeshComponent* EscaperMesh = ResolveEscaperMeshForCarry(Escaper);
        if (EscaperMesh)
        {
            FName RagdollStartBone = EscaperCarryRagdollStartBone;
            if (EscaperMesh->GetBoneIndex(RagdollStartBone) == INDEX_NONE)
            {
                RagdollStartBone = EscaperMesh->GetNumBones() > 0 ? EscaperMesh->GetBoneName(0) : NAME_None;
                UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Carry ragdoll fallback bone escaper=%s requestedBone=%s fallbackBone=%s"),
                    *GetNameSafe(Escaper),
                    *EscaperCarryRagdollStartBone.ToString(),
                    *RagdollStartBone.ToString());
            }

            if (RagdollStartBone == NAME_None)
            {
                UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Carry ragdoll aborted due to invalid start bone escaper=%s requestedBone=%s"),
                    *GetNameSafe(Escaper),
                    *EscaperCarryRagdollStartBone.ToString());
            }
            else
            {
                EscaperCarryResolvedRagdollBone = RagdollStartBone;
                if (EscaperMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
                {
                    EscaperMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                }

                const float ClampedBlendWeight = FMath::Clamp(EscaperCarryRagdollBlendWeight, 0.0f, 1.0f);
                EscaperMesh->SetAllBodiesBelowSimulatePhysics(RagdollStartBone, true, true);
                EscaperMesh->SetAllBodiesBelowPhysicsBlendWeight(RagdollStartBone, ClampedBlendWeight, false, true);
                EscaperMesh->bBlendPhysics = ClampedBlendWeight > 0.0f;

                EscaperCarryRagdollMesh = EscaperMesh;
                bEscaperCarryRagdollApplied = true;

                UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Applied escaper carry ragdoll escaper=%s mesh=%s bone=%s blendWeight=%.2f meshCollision=%d"),
                    *GetNameSafe(Escaper),
                    *GetNameSafe(EscaperMesh),
                    *RagdollStartBone.ToString(),
                    ClampedBlendWeight,
                    static_cast<int32>(EscaperMesh->GetCollisionEnabled()));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Could not resolve escaper carry mesh for ragdoll escaper=%s meshTag=%s meshName=%s"),
                *GetNameSafe(Escaper),
                *EscaperCarryMeshComponentTag.ToString(),
                *EscaperCarryMeshComponentName.ToString());
        }
    }

    if (!bAlreadySavedForEscaper)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Applied escaper carry overrides escaper=%s hubert=%s actorCollision=%d capsuleCollision=NoCollision"),
            *GetNameSafe(Escaper),
            *GetNameSafe(SpawnedHubert),
            Escaper->GetActorEnableCollision() ? 1 : 0);
    }
}

void AEMRHubertDirector::RestoreEscaperGrabCarryOverrides(AEMRPlayerCharacter* Escaper, AEMRHubertCharacter* SpawnedHubert)
{
    AEMRPlayerCharacter* EscaperToRestore = Escaper ? Escaper : EscaperWithCarryOverrides.Get();
    if (!EscaperToRestore)
    {
        EscaperCarryRagdollMesh.Reset();
        EscaperWithCarryOverrides.Reset();
        EscaperCarryResolvedRagdollBone = NAME_None;
        bEscaperCarryOverridesSaved = false;
        bEscaperCarryRagdollApplied = false;
        bEscaperCarryMeshCollisionSaved = false;
        bEscaperCarryMeshBlendPhysicsSaved = false;
        return;
    }

    if (AEMRHubertCharacter* HubertToRestore = SpawnedHubert ? SpawnedHubert : HubertCharacter.Get())
    {
        EscaperToRestore->MoveIgnoreActorRemove(HubertToRestore);
        HubertToRestore->MoveIgnoreActorRemove(EscaperToRestore);
    }

    if (bEscaperCarryOverridesSaved)
    {
        EscaperToRestore->SetActorEnableCollision(bEscaperCarryActorCollisionEnabled);
        if (UCapsuleComponent* EscaperCapsule = EscaperToRestore->GetCapsuleComponent())
        {
            EscaperCapsule->SetCollisionEnabled(EscaperCarryCapsuleCollisionMode.GetValue());
        }
    }

    if (bEscaperCarryRagdollApplied)
    {
        USkeletalMeshComponent* EscaperMesh = EscaperCarryRagdollMesh.Get();
        if (!EscaperMesh)
        {
            EscaperMesh = ResolveEscaperMeshForCarry(EscaperToRestore);
        }

        if (EscaperMesh)
        {
            const FName BoneToRestore = EscaperCarryResolvedRagdollBone.IsNone() ? EscaperCarryRagdollStartBone : EscaperCarryResolvedRagdollBone;
            EscaperMesh->SetAllBodiesBelowPhysicsBlendWeight(BoneToRestore, 0.0f, false, true);
            EscaperMesh->SetAllBodiesBelowSimulatePhysics(BoneToRestore, false, true);
            EscaperMesh->bBlendPhysics = bEscaperCarryMeshBlendPhysicsSaved;
        }
    }

    if (bEscaperCarryMeshCollisionSaved)
    {
        USkeletalMeshComponent* EscaperMesh = EscaperCarryRagdollMesh.Get();
        if (!EscaperMesh)
        {
            EscaperMesh = ResolveEscaperMeshForCarry(EscaperToRestore);
        }

        if (EscaperMesh)
        {
            EscaperMesh->SetCollisionEnabled(EscaperCarryMeshCollisionMode.GetValue());
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Restored escaper carry overrides escaper=%s hubert=%s"),
        *GetNameSafe(EscaperToRestore),
        *GetNameSafe(SpawnedHubert ? SpawnedHubert : HubertCharacter.Get()));

    EscaperCarryRagdollMesh.Reset();
    EscaperWithCarryOverrides.Reset();
    EscaperCarryResolvedRagdollBone = NAME_None;
    bEscaperCarryOverridesSaved = false;
    bEscaperCarryRagdollApplied = false;
    bEscaperCarryMeshCollisionSaved = false;
    bEscaperCarryMeshBlendPhysicsSaved = false;
}

bool AEMRHubertDirector::BeginEscaperPhysicalThrow(AEMRPlayerCharacter* Escaper)
{
    if (!Escaper || !ReturnInsideAnchor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] BeginEscaperPhysicalThrow failed escaper=%s returnAnchor=%s"),
            *GetNameSafe(Escaper),
            *GetNameSafe(ReturnInsideAnchor));
        return false;
    }

    USkeletalMeshComponent* EscaperMesh = ResolveEscaperMeshForCarry(Escaper);
    if (!EscaperMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] BeginEscaperPhysicalThrow failed due to missing mesh escaper=%s"),
            *GetNameSafe(Escaper));
        return false;
    }

    Escaper->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    if (UCharacterMovementComponent* EscaperMovement = Escaper->GetCharacterMovement())
    {
        EscaperMovement->DisableMovement();
    }

    const FName PhysicsBone = ResolveEscaperThrowPhysicsBone(EscaperMesh);
    if (PhysicsBone == NAME_None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] BeginEscaperPhysicalThrow failed due to missing valid physics body escaper=%s requestedThrowBone=%s carryResolvedBone=%s carryRagdollBone=%s"),
            *GetNameSafe(Escaper),
            *ThrowRagdollStartBone.ToString(),
            *EscaperCarryResolvedRagdollBone.ToString(),
            *EscaperCarryRagdollStartBone.ToString());
        return false;
    }

    EscaperMesh->SetAllBodiesBelowSimulatePhysics(PhysicsBone, true, true);
    EscaperMesh->SetAllBodiesBelowPhysicsBlendWeight(PhysicsBone, 1.0f, false, true);
    EscaperMesh->bBlendPhysics = true;
    if (EscaperMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
    {
        EscaperMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    const FVector ThrowSourceLocation = HubertCharacter.IsValid()
    ? HubertCharacter->GetActorLocation()
    : Escaper->GetActorLocation();
    const FVector ThrowTargetLocation = ReturnInsideAnchor->GetActorLocation();
    FVector ThrowDirection = ThrowTargetLocation - ThrowSourceLocation;
    if (!ThrowDirection.Normalize())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] BeginEscaperPhysicalThrow failed due to zero throw direction escaper=%s source=%s target=%s"),
            *GetNameSafe(Escaper),
            *ThrowSourceLocation.ToCompactString(),
            *ThrowTargetLocation.ToCompactString());
        return false;
    }

    EscaperMesh->WakeAllRigidBodies();
    EscaperMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
    EscaperMesh->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    const FVector ThrowImpulse =
    ThrowDirection * FMath::Max(ThrowImpulseStrength, 0.0f)
    + FVector::UpVector * ThrowUpwardImpulse;
    EscaperMesh->AddImpulseToAllBodiesBelow(ThrowImpulse, PhysicsBone, true, true);
    EscaperMesh->WakeAllRigidBodies();

    const FVector VelocityAfterImpulse = EscaperMesh->GetPhysicsLinearVelocity(PhysicsBone);
    const float LaunchSpeed = VelocityAfterImpulse.Size();
    const float MinLaunchSpeed = FMath::Max(ThrowMinLaunchSpeed, 0.0f);
    if (LaunchSpeed < MinLaunchSpeed)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] BeginEscaperPhysicalThrow low launch speed escaper=%s speed=%.2f min=%.2f (continuing with physical throw; fallback handled in ReturnEscaper)"),
            *GetNameSafe(Escaper),
            LaunchSpeed,
            MinLaunchSpeed);
    }

    EscaperThrowStartTimestampSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    EscaperLowVelocityWindowStartTimestampSeconds = 0.0;
    EscaperSettledTimestampSeconds = 0.0;
    EscaperNextThrowDebugLogTimestampSeconds = EscaperThrowStartTimestampSeconds;

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] BeginEscaperPhysicalThrow started escaper=%s source=%s target=%s direction=%s impulse=%s launchVelocity=%s speed=%.2f bone=%s"),
        *GetNameSafe(Escaper),
        *ThrowSourceLocation.ToCompactString(),
        *ThrowTargetLocation.ToCompactString(),
        *ThrowDirection.ToCompactString(),
        *ThrowImpulse.ToCompactString(),
        *VelocityAfterImpulse.ToCompactString(),
        LaunchSpeed,
        *PhysicsBone.ToString());
    return true;
}

bool AEMRHubertDirector::IsEscaperVelocityBelowThreshold(const AEMRPlayerCharacter* Escaper) const
{
    if (!Escaper)
    {
        return false;
    }

    USkeletalMeshComponent* EscaperMesh = ResolveEscaperMeshForCarry(Escaper);
    if (!EscaperMesh)
    {
        return false;
    }

    const FName PhysicsBone = ResolveEscaperThrowPhysicsBone(EscaperMesh);

    if (PhysicsBone == NAME_None)
    {
        return false;
    }

    const FVector Velocity = EscaperMesh->GetPhysicsLinearVelocity(PhysicsBone);
    return Velocity.Size() <= FMath::Max(ThrowLowVelocityThreshold, 0.0f);
}

void AEMRHubertDirector::FinalizeEscaperPhysicalThrow(AEMRPlayerCharacter* Escaper, const bool bTeleportFallback)
{
    if (!Escaper)
    {
        ActiveEscaper.Reset();
        ResetEscaperPhysicalThrowRuntime();
        TryStartNextEscaperChase();
        if (!ActiveEscaper.IsValid())
        {
            EnterState(EEMRHubertState::NightPatrol);
        }
        return;
    }

    if (bTeleportFallback)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] FinalizeEscaperPhysicalThrow using teleport fallback escaper=%s"),
            *GetNameSafe(Escaper));
        ReturnEscaperInside(Escaper);
    }
    else if (AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get())
    {
        if (USkeletalMeshComponent* EscaperMesh = ResolveEscaperMeshForCarry(Escaper))
        {
            const FName PhysicsBone = ResolveEscaperThrowPhysicsBone(EscaperMesh);

            if (PhysicsBone != NAME_None)
            {
                const FVector LandingLocation = EscaperMesh->GetBoneLocation(PhysicsBone);
                Escaper->SetActorLocation(LandingLocation, false, nullptr, ETeleportType::TeleportPhysics);
                UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] FinalizeEscaperPhysicalThrow aligned actor to ragdoll landing escaper=%s bone=%s location=%s"),
                    *GetNameSafe(Escaper),
                    *PhysicsBone.ToString(),
                    *LandingLocation.ToCompactString());
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] FinalizeEscaperPhysicalThrow successful physical throw escaper=%s"),
            *GetNameSafe(Escaper));
        if (USoundBase* EscapeVoice = ResolveVoiceLineFromArray(EscapeVoiceLines, TEXT("EscapeEnforcement")))
        {
            SpawnedHubert->PlayVoiceLineForAllPlayers(EscapeVoice);
        }
    }

    RestoreEscaperGrabCarryOverrides(Escaper, HubertCharacter.Get());
    if (UCharacterMovementComponent* EscaperMovement = Escaper->GetCharacterMovement())
    {
        EscaperMovement->SetMovementMode(MOVE_Walking);
    }

    ActiveEscaper.Reset();
    ResetEscaperPhysicalThrowRuntime();
    TryStartNextEscaperChase();
    if (!ActiveEscaper.IsValid())
    {
        EnterState(EEMRHubertState::NightPatrol);
    }
}

void AEMRHubertDirector::ResetEscaperPhysicalThrowRuntime()
{
    EscaperThrowStartTimestampSeconds = 0.0;
    EscaperLowVelocityWindowStartTimestampSeconds = 0.0;
    EscaperSettledTimestampSeconds = 0.0;
    EscaperNextThrowDebugLogTimestampSeconds = 0.0;
}

void AEMRHubertDirector::ExecuteEscaperEnforcement(AEMRPlayerCharacter* Escaper)
{
    if (!Escaper)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ExecuteEscaperEnforcement skipped because escaper is null"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ExecuteEscaperEnforcement escaper=%s"),
        *GetNameSafe(Escaper));

    Escaper->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    if (UCharacterMovementComponent* EscaperMovement = Escaper->GetCharacterMovement())
    {
        EscaperMovement->SetMovementMode(MOVE_Walking);
    }

    ReturnEscaperInside(Escaper);
    RestoreEscaperGrabCarryOverrides(Escaper, HubertCharacter.Get());
    ApplyEscapeRevenuePenalty(Escaper);
    PerPlayerPenaltyTimestampSeconds.FindOrAdd(Escaper) = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    ActiveEscaper.Reset();
    ResetEscaperPhysicalThrowRuntime();
    TryStartNextEscaperChase();
    if (!ActiveEscaper.IsValid())
    {
        EnterState(EEMRHubertState::NightPatrol);
    }
}

void AEMRHubertDirector::ReturnEscaperInside(AEMRPlayerCharacter* Escaper)
{
    if (!Escaper)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ReturnEscaperInside skipped because escaper is null"));
        return;
    }

    if (!ReturnInsideAnchor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Missing ReturnInsideAnchor. Cannot return escaper=%s"), *GetNameSafe(Escaper));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ReturnEscaperInside teleport escaper=%s anchor=%s location=%s rotation=%s"),
            *GetNameSafe(Escaper),
            *GetNameSafe(ReturnInsideAnchor),
            *ReturnInsideAnchor->GetActorLocation().ToCompactString(),
            *ReturnInsideAnchor->GetActorRotation().ToCompactString());
        Escaper->SetActorLocationAndRotation(
            ReturnInsideAnchor->GetActorLocation(),
            ReturnInsideAnchor->GetActorRotation(),
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
    }

    if (AEMRHubertCharacter* SpawnedHubert = HubertCharacter.Get())
    {
        if (USoundBase* EscapeVoice = ResolveVoiceLineFromArray(EscapeVoiceLines, TEXT("EscapeEnforcement")))
        {
            SpawnedHubert->PlayVoiceLineForAllPlayers(EscapeVoice);
        }
    }
}

void AEMRHubertDirector::ApplyEscapeRevenuePenalty(AEMRPlayerCharacter* Escaper)
{
    if (!Escaper || EscapePenaltyRevenueAmount <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] ApplyEscapeRevenuePenalty skipped escaper=%s amount=%.2f"),
            *GetNameSafe(Escaper),
            EscapePenaltyRevenueAmount);
        return;
    }

    AEMRNightShiftGameState* NightShiftGameState = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
    UAbilitySystemComponent* TeamASC = NightShiftGameState ? NightShiftGameState->GetAbilitySystemComponent() : nullptr;
    if (!TeamASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Team ASC not available for escape penalty."));
        return;
    }

    TArray<const UEMREconomySystemGenerics*> LoadedEconomyConfigs;
    if (!UEMRAssetManager::Get().CollectLoadedEconomySystemGenerics(LoadedEconomyConfigs) || LoadedEconomyConfigs.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Economy config unavailable for escape penalty."));
        return;
    }

    const UEMREconomySystemGenerics* EconomyConfig = LoadedEconomyConfigs[0];
    if (!EconomyConfig || !EconomyConfig->GetSpendMoneyEffect())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] SpendMoneyEffect missing in economy config for escape penalty."));
        return;
    }

    FGameplayEffectContextHandle EffectContext = TeamASC->MakeEffectContext();
    EffectContext.AddSourceObject(Escaper);

    const FGameplayEffectSpecHandle EffectSpec = TeamASC->MakeOutgoingSpec(EconomyConfig->GetSpendMoneyEffect(), 1.0f, EffectContext);
    if (!EffectSpec.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Failed to create SpendMoney spec for escape penalty."));
        return;
    }

    EffectSpec.Data->SetSetByCallerMagnitude(EMRTags::SetByCaller::SpendMoney, -EscapePenaltyRevenueAmount);
    TeamASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Applied escape penalty amount=%.2f player=%s"), EscapePenaltyRevenueAmount, *GetNameSafe(Escaper));
}

void AEMRHubertDirector::TryPlayPendingOutcomeAnnouncement()
{
    if (!bHubRuntimeActive || PendingOutcomeAnnouncements.Num() <= 0)
    {
        return;
    }

    const double CurrentServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (CurrentServerTime < NextOutcomeAnnouncementTimestampSeconds)
    {
        return;
    }

    const bool bAllPlayersLoaded = AreAllHubPlayersLoaded();
    const double ElapsedHubRuntime = CurrentServerTime - HubRuntimeStartTimestampSeconds;
    const double LoadTimeout = FMath::Max(HubAllPlayersLoadedTimeoutSeconds, 0.0f);

    if (!bAllPlayersLoaded && ElapsedHubRuntime < LoadTimeout)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] TryPlayPendingOutcomeAnnouncement waiting for players elapsed=%.3f timeout=%.3f pending=%d"),
            ElapsedHubRuntime,
            LoadTimeout,
            PendingOutcomeAnnouncements.Num());
        return;
    }

    if (!bAllPlayersLoaded && !bHubLoadTimeoutWarningLogged)
    {
        bHubLoadTimeoutWarningLogged = true;
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Hub player-load timeout reached (%.2fs). Releasing queued announcements."), LoadTimeout);
    }

    AEMRHubertCharacter* SpawnedHubert = EnsureHubertCharacter(true);
    if (!SpawnedHubert)
    {
        return;
    }

    const EEMRHubertOutcomeAnnouncement AnnouncementType = PendingOutcomeAnnouncements[0];
    PendingOutcomeAnnouncements.RemoveAt(0);
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Playing outcome announcement type=%d pendingAfterPop=%d"),
        static_cast<int32>(AnnouncementType),
        PendingOutcomeAnnouncements.Num());

    if (USoundBase* VoiceLine = ResolveVoiceLineForOutcome(AnnouncementType))
    {
        SpawnedHubert->PlayVoiceLineForAllPlayers(VoiceLine);
    }

    NextOutcomeAnnouncementTimestampSeconds = CurrentServerTime + FMath::Max(HubOutcomeAnnouncementSpacingSeconds, 0.0f);
    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Scheduled next outcome announcement at %.3f"),
        NextOutcomeAnnouncementTimestampSeconds);
}

USoundBase* AEMRHubertDirector::ResolveVoiceLineFromArray(const TArray<TSoftObjectPtr<USoundBase>>& VoiceLines, const TCHAR* Context) const
{
    if (VoiceLines.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] No voice lines configured for %s"), Context ? Context : TEXT("<unknown>"));
        return nullptr;
    }

    TArray<int32> CandidateIndices;
    CandidateIndices.Reserve(VoiceLines.Num());
    for (int32 Index = 0; Index < VoiceLines.Num(); ++Index)
    {
        CandidateIndices.Add(Index);
    }

    while (CandidateIndices.Num() > 0)
    {
        const int32 PickIndex = FMath::RandRange(0, CandidateIndices.Num() - 1);
        const int32 VoiceIndex = CandidateIndices[PickIndex];
        CandidateIndices.RemoveAtSwap(PickIndex);

        const TSoftObjectPtr<USoundBase>& VoiceRef = VoiceLines[VoiceIndex];
        if (USoundBase* LoadedVoice = VoiceRef.LoadSynchronous())
        {
            return LoadedVoice;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Voice lines configured for %s but all failed to load."), Context ? Context : TEXT("<unknown>"));
    return nullptr;
}

USoundBase* AEMRHubertDirector::ResolveVoiceLineForOutcome(const EEMRHubertOutcomeAnnouncement AnnouncementType) const
{
    switch (AnnouncementType)
    {
    case EEMRHubertOutcomeAnnouncement::NightShiftWin:
        return ResolveVoiceLineFromArray(NightShiftWinVoiceLines, TEXT("NightShiftWin"));
    case EEMRHubertOutcomeAnnouncement::NightShiftLose:
        return ResolveVoiceLineFromArray(NightShiftLoseVoiceLines, TEXT("NightShiftLose"));
    case EEMRHubertOutcomeAnnouncement::CycleWin:
        return ResolveVoiceLineFromArray(CycleWinVoiceLines, TEXT("CycleWin"));
    case EEMRHubertOutcomeAnnouncement::CycleLose:
        return ResolveVoiceLineFromArray(CycleLoseVoiceLines, TEXT("CycleLose"));
    default:
        return nullptr;
    }
}
