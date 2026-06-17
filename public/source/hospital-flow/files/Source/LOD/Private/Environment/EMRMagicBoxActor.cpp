#include "Environment/EMRMagicBoxActor.h"

#include "Characters/Patient/EMRPatient.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/EMRTags.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/EMRMagicBoxSubsystem.h"

namespace
{
    constexpr TCHAR MagicBoxFlowLogPrefix[] = TEXT("[MagicBoxFlow]");
    constexpr float MagicBoxLoopAudioRestartRetrySeconds = 0.35f;
}

AEMRMagicBoxActor::AEMRMagicBoxActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoorMesh"));
    LeftDoorMesh->SetupAttachment(SceneRoot);
    LeftDoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoorMesh"));
    RightDoorMesh->SetupAttachment(SceneRoot);
    RightDoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    EntryTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryTrigger"));
    EntryTrigger->SetupAttachment(SceneRoot);
    EntryTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EntryTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    EntryTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    EntryTrigger->SetGenerateOverlapEvents(false);
    EntryTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleEntryTriggerOverlap);

    TreatmentLoopAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("TreatmentLoopAudioComponent"));
    TreatmentLoopAudioComponent->SetupAttachment(SceneRoot);
    TreatmentLoopAudioComponent->SetAutoActivate(false);
    TreatmentLoopAudioComponent->bAutoActivate = false;

    RequiredUpgradeTag = EMRTags::RunUpgrade::MagicBox;
}

void AEMRMagicBoxActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshDoorPose(DeltaSeconds);

    const bool bShouldRetryLoopAudio =
    DeltaSeconds > 0.0f
    && GetNetMode() != NM_DedicatedServer
    && bMagicBoxEnabledByUpgrade
    && bTreatmentLoopAudioPlaying
    && TreatmentLoopAudioComponent
    && TreatmentLoopSound
    && !TreatmentLoopAudioComponent->IsPlaying();

    if (!bShouldRetryLoopAudio)
    {
        LoopAudioRestartRetryElapsedSeconds = 0.0f;
        return;
    }

    LoopAudioRestartRetryElapsedSeconds += DeltaSeconds;
    if (LoopAudioRestartRetryElapsedSeconds >= MagicBoxLoopAudioRestartRetrySeconds)
    {
        LoopAudioRestartRetryElapsedSeconds = 0.0f;
        UpdateLoopAudioState();
    }
}

void AEMRMagicBoxActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("%s Actor BeginPlay box=%s authority=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(this), HasAuthority() ? 1 : 0);

    if (HasAuthority())
    {
        if (UEMRMagicBoxSubsystem* MagicBoxSubsystem = GetWorld()->GetSubsystem<UEMRMagicBoxSubsystem>())
        {
            UE_LOG(LogTemp, Warning, TEXT("%s Actor BeginPlay registering to subsystem box=%s subsystem=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this), *GetNameSafe(MagicBoxSubsystem));
            MagicBoxSubsystem->RegisterMagicBox(this);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s Actor BeginPlay no subsystem found box=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this));
        }

        const AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
        const int32 UpgradeStackCount = RunGS ? RunGS->GetActiveRunUpgradeStackCount(RequiredUpgradeTag) : 0;
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s Actor BeginPlay upgrade check box=%s requiredTag=%s stack=%d requiredStack=%d"),
            MagicBoxFlowLogPrefix,
            *GetNameSafe(this),
            *RequiredUpgradeTag.ToString(),
            UpgradeStackCount,
            GetRequiredUpgradeStackCount());
        SetMagicBoxEnabledByUpgrade(UpgradeStackCount >= GetRequiredUpgradeStackCount());
    }
    else
    {
        ApplyRuntimeState();
    }

    DoorOpenAlpha = bDoorsOpen ? 1.0f : 0.0f;
    DoorTargetAlpha = DoorOpenAlpha;
    bDoorMotionActive = false;
    ApplyDoorPose();
    UpdateLoopAudioState();
}

void AEMRMagicBoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Actor EndPlay box=%s reason=%d authority=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(this), static_cast<int32>(EndPlayReason), HasAuthority() ? 1 : 0);
    if (HasAuthority())
    {
        if (UEMRMagicBoxSubsystem* MagicBoxSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEMRMagicBoxSubsystem>() : nullptr)
        {
            MagicBoxSubsystem->UnregisterMagicBox(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void AEMRMagicBoxActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, bMagicBoxEnabledByUpgrade);
    DOREPLIFETIME(ThisClass, bIsOccupied);
    DOREPLIFETIME(ThisClass, bDoorsOpen);
    DOREPLIFETIME(ThisClass, bTreatmentLoopAudioPlaying);
}

bool AEMRMagicBoxActor::IsAvailable() const
{
    return bMagicBoxEnabledByUpgrade && !bIsOccupied && !AssignedPatient.IsValid();
}

void AEMRMagicBoxActor::AssignToPatient(AEMRPatient* Patient)
{
    AssignedPatient = Patient;
    UE_LOG(LogTemp, Warning, TEXT("%s Actor AssignToPatient box=%s patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this), *GetNameSafe(Patient));
}

void AEMRMagicBoxActor::ClearAssignment()
{
    UE_LOG(LogTemp, Warning, TEXT("%s Actor ClearAssignment box=%s patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this), *GetNameSafe(AssignedPatient.Get()));
    AssignedPatient.Reset();
}

void AEMRMagicBoxActor::SetOccupied(const bool bNewOccupied)
{
    if (bIsOccupied == bNewOccupied)
    {
        return;
    }

    bIsOccupied = bNewOccupied;
    UE_LOG(LogTemp, Warning, TEXT("%s Actor SetOccupied box=%s occupied=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(this), bIsOccupied ? 1 : 0);
    OnRep_Occupied();
    ForceNetUpdate();
}

void AEMRMagicBoxActor::SetMagicBoxEnabledByUpgrade(const bool bEnabled)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Actor SetEnabled ignored no authority box=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this));
        return;
    }

    if (bMagicBoxEnabledByUpgrade == bEnabled)
    {
        ApplyRuntimeState();
        UpdateLoopAudioState();
        return;
    }

    bMagicBoxEnabledByUpgrade = bEnabled;
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s Actor SetEnabled box=%s enabled=%d occupied=%d assigned=%s"),
        MagicBoxFlowLogPrefix,
        *GetNameSafe(this),
        bMagicBoxEnabledByUpgrade ? 1 : 0,
        bIsOccupied ? 1 : 0,
        *GetNameSafe(AssignedPatient.Get()));
    if (!bMagicBoxEnabledByUpgrade)
    {
        bIsOccupied = false;
        bDoorsOpen = false;
        AssignedPatient.Reset();
        SetTreatmentLoopAudioPlaying(false);
    }

    OnRep_MagicBoxEnabled();
    ForceNetUpdate();
}

FTransform AEMRMagicBoxActor::GetApproachTransform() const
{
    return ApproachTargetPoint ? ApproachTargetPoint->GetActorTransform() : GetActorTransform();
}

FTransform AEMRMagicBoxActor::GetInsideTransform() const
{
    return InsideTargetPoint ? InsideTargetPoint->GetActorTransform() : GetActorTransform();
}

AActor* AEMRMagicBoxActor::GetApproachNavigationTargetActor() const
{
    return ApproachTargetPoint;
}

AActor* AEMRMagicBoxActor::GetInsideNavigationTargetActor() const
{
    return InsideTargetPoint;
}

float AEMRMagicBoxActor::GetDoorMoveDurationSeconds() const
{
    return FMath::Max(DoorMoveDurationSeconds, 0.01f);
}

void AEMRMagicBoxActor::SetDoorsOpen(const bool bOpen)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Actor SetDoorsOpen ignored no authority box=%s targetOpen=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(this), bOpen ? 1 : 0);
        return;
    }

    if (bDoorsOpen == bOpen)
    {
        ApplyDoorTarget(bOpen, false);
        return;
    }

    bDoorsOpen = bOpen;
    UE_LOG(LogTemp, Warning, TEXT("%s Actor SetDoorsOpen box=%s targetOpen=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(this), bDoorsOpen ? 1 : 0);
    OnRep_DoorsOpen();
    ForceNetUpdate();
}

void AEMRMagicBoxActor::NotifyTreatmentStarted(AEMRPatient* Patient, float DurationSeconds)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Actor NotifyTreatmentStarted ignored no authority box=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this));
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s Actor NotifyTreatmentStarted box=%s patient=%s duration=%.2f"),
        MagicBoxFlowLogPrefix,
        *GetNameSafe(this),
        *GetNameSafe(Patient),
        DurationSeconds);
    SetTreatmentLoopAudioPlaying(true);
    Multicast_OnMagicBoxTreatmentStarted(Patient, DurationSeconds);
}

void AEMRMagicBoxActor::NotifyTreatmentFinished(AEMRPatient* Patient)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Actor NotifyTreatmentFinished ignored no authority box=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s Actor NotifyTreatmentFinished box=%s patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this), *GetNameSafe(Patient));
    SetDoorsOpen(true);
    SetTreatmentLoopAudioPlaying(false);
    Multicast_OnMagicBoxTreatmentFinished(Patient);
}

void AEMRMagicBoxActor::NotifyTreatmentAborted(AEMRPatient* Patient)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Actor NotifyTreatmentAborted ignored no authority box=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s Actor NotifyTreatmentAborted box=%s patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this), *GetNameSafe(Patient));
    SetDoorsOpen(false);
    SetTreatmentLoopAudioPlaying(false);
    Multicast_OnMagicBoxTreatmentAborted(Patient);
}

void AEMRMagicBoxActor::HandleEntryTriggerOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s Actor EntryOverlap box=%s otherActor=%s assignedPatient=%s enabled=%d occupied=%d authority=%d"),
        MagicBoxFlowLogPrefix,
        *GetNameSafe(this),
        *GetNameSafe(OtherActor),
        *GetNameSafe(AssignedPatient.Get()),
        bMagicBoxEnabledByUpgrade ? 1 : 0,
        bIsOccupied ? 1 : 0,
        HasAuthority() ? 1 : 0);

    if (!HasAuthority() || !bMagicBoxEnabledByUpgrade || bIsOccupied || !AssignedPatient.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Actor EntryOverlap ignored precondition failed box=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this));
        return;
    }

    if (OtherActor != AssignedPatient.Get())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Actor EntryOverlap ignored actor mismatch box=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this));
        return;
    }

    if (UEMRMagicBoxSubsystem* MagicBoxSubsystem = GetWorld()->GetSubsystem<UEMRMagicBoxSubsystem>())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Actor EntryOverlap forwarding arrival box=%s patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this), *GetNameSafe(AssignedPatient.Get()));
        MagicBoxSubsystem->NotifyMagicBoxArrival(this, AssignedPatient.Get());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s Actor EntryOverlap no subsystem box=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this));
    }
}

void AEMRMagicBoxActor::OnRep_MagicBoxEnabled()
{
    ApplyRuntimeState();
    UpdateLoopAudioState();
}

void AEMRMagicBoxActor::OnRep_Occupied()
{
    ApplyRuntimeState();
}

void AEMRMagicBoxActor::OnRep_DoorsOpen()
{
    UE_LOG(LogTemp, Warning, TEXT("%s Actor OnRep_DoorsOpen box=%s doorsOpen=%d"), MagicBoxFlowLogPrefix, *GetNameSafe(this), bDoorsOpen ? 1 : 0);
    ApplyDoorTarget(bDoorsOpen, true);
}

void AEMRMagicBoxActor::OnRep_TreatmentLoopAudioPlaying()
{
    LoopAudioRestartRetryElapsedSeconds = 0.0f;
    UpdateLoopAudioState();
}

void AEMRMagicBoxActor::Multicast_OnMagicBoxTreatmentStarted_Implementation(AEMRPatient* Patient, float DurationSeconds)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Actor Multicast TreatmentStarted box=%s patient=%s duration=%.2f"), MagicBoxFlowLogPrefix, *GetNameSafe(this), *GetNameSafe(Patient), DurationSeconds);
    SetTreatmentLoopAudioPlaying(true);
    PlayLocalSound(TreatmentStartedSound);
}

void AEMRMagicBoxActor::Multicast_OnMagicBoxTreatmentFinished_Implementation(AEMRPatient* Patient)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Actor Multicast TreatmentFinished box=%s patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this), *GetNameSafe(Patient));
    SetTreatmentLoopAudioPlaying(false);
    PlayLocalSound(TreatmentFinishedSound);
}

void AEMRMagicBoxActor::Multicast_OnMagicBoxTreatmentAborted_Implementation(AEMRPatient* Patient)
{
    UE_LOG(LogTemp, Warning, TEXT("%s Actor Multicast TreatmentAborted box=%s patient=%s"), MagicBoxFlowLogPrefix, *GetNameSafe(this), *GetNameSafe(Patient));
    SetTreatmentLoopAudioPlaying(false);
}

void AEMRMagicBoxActor::ApplyRuntimeState()
{
    SetActorHiddenInGame(!bMagicBoxEnabledByUpgrade);

    if (EntryTrigger)
    {
        EntryTrigger->SetCollisionEnabled(bMagicBoxEnabledByUpgrade ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        EntryTrigger->SetGenerateOverlapEvents(bMagicBoxEnabledByUpgrade);
    }

    if (!bMagicBoxEnabledByUpgrade)
    {
        DoorOpenAlpha = 0.0f;
        DoorTargetAlpha = 0.0f;
        bDoorsOpen = false;
        bDoorMotionActive = false;
        ApplyDoorPose();
    }
}

void AEMRMagicBoxActor::SetTreatmentLoopAudioPlaying(const bool bNewPlaying)
{
    if (bTreatmentLoopAudioPlaying == bNewPlaying)
    {
        return;
    }

    bTreatmentLoopAudioPlaying = bNewPlaying;
    LoopAudioRestartRetryElapsedSeconds = 0.0f;
    UpdateLoopAudioState();

    if (HasAuthority())
    {
        ForceNetUpdate();
    }
}

void AEMRMagicBoxActor::UpdateLoopAudioState()
{
    if (GetNetMode() == NM_DedicatedServer || !TreatmentLoopAudioComponent)
    {
        return;
    }

    const bool bShouldPlayLoop = bMagicBoxEnabledByUpgrade && bTreatmentLoopAudioPlaying;
    if (bShouldPlayLoop)
    {
        if (TreatmentLoopSound && TreatmentLoopAudioComponent->GetSound() != TreatmentLoopSound)
        {
            TreatmentLoopAudioComponent->SetSound(TreatmentLoopSound);
        }

        if (TreatmentLoopAudioComponent->GetSound() && !TreatmentLoopAudioComponent->IsPlaying())
        {
            TreatmentLoopAudioComponent->FadeIn(TreatmentLoopFadeInSeconds, 1.0f, 0.0f);
        }
        return;
    }

    if (TreatmentLoopAudioComponent->IsPlaying())
    {
        TreatmentLoopAudioComponent->FadeOut(TreatmentLoopFadeOutSeconds, 0.0f);
    }
}

void AEMRMagicBoxActor::PlayLocalSound(USoundBase* Sound) const
{
    if (GetNetMode() == NM_DedicatedServer || !Sound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
}

void AEMRMagicBoxActor::RefreshDoorPose(const float DeltaSeconds)
{
    if (!bDoorMotionActive)
    {
        return;
    }

    const float InterpRate = 1.0f / GetDoorMoveDurationSeconds();
    DoorOpenAlpha = FMath::FInterpConstantTo(DoorOpenAlpha, DoorTargetAlpha, DeltaSeconds, InterpRate);
    if (FMath::IsNearlyEqual(DoorOpenAlpha, DoorTargetAlpha, 0.001f))
    {
        DoorOpenAlpha = DoorTargetAlpha;
        bDoorMotionActive = false;
    }

    ApplyDoorPose();
}

void AEMRMagicBoxActor::ApplyDoorPose() const
{
    if (LeftDoorMesh)
    {
        const FQuat LeftQuat = FQuat::Slerp(LeftDoorClosedRotation.Quaternion(), LeftDoorOpenRotation.Quaternion(), DoorOpenAlpha);
        LeftDoorMesh->SetRelativeRotation(LeftQuat);
    }

    if (RightDoorMesh)
    {
        const FQuat RightQuat = FQuat::Slerp(RightDoorClosedRotation.Quaternion(), RightDoorOpenRotation.Quaternion(), DoorOpenAlpha);
        RightDoorMesh->SetRelativeRotation(RightQuat);
    }
}

void AEMRMagicBoxActor::ApplyDoorTarget(const bool bOpen, const bool bPlaySound)
{
    const float NewTargetAlpha = bOpen ? 1.0f : 0.0f;
    if (FMath::IsNearlyEqual(DoorTargetAlpha, NewTargetAlpha, 0.001f)
        && FMath::IsNearlyEqual(DoorOpenAlpha, NewTargetAlpha, 0.001f))
    {
        return;
    }

    DoorTargetAlpha = NewTargetAlpha;
    bDoorMotionActive = true;
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s Actor ApplyDoorTarget box=%s targetOpen=%d alpha=%.2f targetAlpha=%.2f playSound=%d"),
        MagicBoxFlowLogPrefix,
        *GetNameSafe(this),
        bOpen ? 1 : 0,
        DoorOpenAlpha,
        DoorTargetAlpha,
        bPlaySound ? 1 : 0);
    if (bPlaySound)
    {
        PlayLocalSound(bOpen ? DoorOpenSound : DoorCloseSound);
    }
}
