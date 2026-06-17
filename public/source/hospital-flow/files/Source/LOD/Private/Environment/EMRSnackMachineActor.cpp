#include "Environment/EMRSnackMachineActor.h"

#include "Characters/Patient/EMRPatient.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/EMRTags.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/EMRSnackMachineSubsystem.h"

AEMRSnackMachineActor::AEMRSnackMachineActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    ApproachArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ApproachArrow"));
    ApproachArrow->SetupAttachment(SceneRoot);

    UseTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("UseTrigger"));
    UseTrigger->SetupAttachment(SceneRoot);
    UseTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UseTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    UseTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    UseTrigger->SetGenerateOverlapEvents(false);
    UseTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleUseTriggerOverlap);

    RequiredUpgradeTag = EMRTags::RunUpgrade::SnackMachine;
}

void AEMRSnackMachineActor::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        if (UEMRSnackMachineSubsystem* SnackSubsystem = GetWorld()->GetSubsystem<UEMRSnackMachineSubsystem>())
        {
            SnackSubsystem->RegisterMachine(this);
        }

        const AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
        const int32 UpgradeStackCount = RunGS ? RunGS->GetActiveRunUpgradeStackCount(RequiredUpgradeTag) : 0;
        SetSnackMachineEnabledByUpgrade(UpgradeStackCount >= GetRequiredUpgradeStackCount());
    }
    else
    {
        ApplyMachineRuntimeState();
    }
}

void AEMRSnackMachineActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
    {
        if (UEMRSnackMachineSubsystem* SnackSubsystem = GetWorld()->GetSubsystem<UEMRSnackMachineSubsystem>())
        {
            SnackSubsystem->UnregisterMachine(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void AEMRSnackMachineActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, bSnackMachineEnabledByUpgrade);
    DOREPLIFETIME(ThisClass, bIsOccupied);
}

bool AEMRSnackMachineActor::IsAvailable() const
{
    return bSnackMachineEnabledByUpgrade && !bIsOccupied && !AssignedPatient.IsValid();
}

void AEMRSnackMachineActor::AssignToPatient(AEMRPatient* Patient)
{
    AssignedPatient = Patient;
}

void AEMRSnackMachineActor::ClearAssignment()
{
    AssignedPatient.Reset();
}

void AEMRSnackMachineActor::SetOccupied(const bool bNewOccupied)
{
    if (bIsOccupied == bNewOccupied)
    {
        return;
    }

    bIsOccupied = bNewOccupied;
    OnRep_Occupied();
    ForceNetUpdate();
}

void AEMRSnackMachineActor::SetSnackMachineEnabledByUpgrade(const bool bEnabled)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bSnackMachineEnabledByUpgrade == bEnabled)
    {
        if (UEMRSnackMachineSubsystem* SnackSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEMRSnackMachineSubsystem>() : nullptr)
        {
            if (bSnackMachineEnabledByUpgrade)
            {
                SnackSubsystem->RegisterMachine(this);
            }
            else
            {
                SnackSubsystem->UnregisterMachine(this);
            }
        }

        ApplyMachineRuntimeState();
        return;
    }

    bSnackMachineEnabledByUpgrade = bEnabled;
    if (!bSnackMachineEnabledByUpgrade)
    {
        bIsOccupied = false;
        AssignedPatient.Reset();
    }

    if (UEMRSnackMachineSubsystem* SnackSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UEMRSnackMachineSubsystem>() : nullptr)
    {
        if (bSnackMachineEnabledByUpgrade)
        {
            SnackSubsystem->RegisterMachine(this);
        }
        else
        {
            SnackSubsystem->UnregisterMachine(this);
        }
    }

    OnRep_SnackMachineEnabled();
    ForceNetUpdate();
}

FTransform AEMRSnackMachineActor::GetApproachTransform() const
{
    if (ApproachArrow)
    {
        return ApproachArrow->GetComponentTransform();
    }

    return GetActorTransform();
}

void AEMRSnackMachineActor::HandleUseTriggerOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority() || !bSnackMachineEnabledByUpgrade || bIsOccupied || !AssignedPatient.IsValid())
    {
        return;
    }

    if (OtherActor != AssignedPatient.Get())
    {
        return;
    }

    if (UEMRSnackMachineSubsystem* SnackSubsystem = GetWorld()->GetSubsystem<UEMRSnackMachineSubsystem>())
    {
        SnackSubsystem->NotifyMachineArrival(this, AssignedPatient.Get());
    }
}

void AEMRSnackMachineActor::OnRep_SnackMachineEnabled()
{
    ApplyMachineRuntimeState();
}

void AEMRSnackMachineActor::OnRep_Occupied()
{
    ApplyMachineRuntimeState();
}

void AEMRSnackMachineActor::PlayUseStartSound()
{
    if (!HasAuthority())
    {
        return;
    }

    Multicast_PlaySnackUseStartSound();
}

void AEMRSnackMachineActor::Multicast_PlaySnackUseStartSound_Implementation()
{
    if (GetNetMode() == NM_DedicatedServer || !SnackUseStartGlobalSound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, SnackUseStartGlobalSound, GetActorLocation());
}

void AEMRSnackMachineActor::ApplyMachineRuntimeState()
{
    const bool bShouldEnable = bSnackMachineEnabledByUpgrade;
    SetActorHiddenInGame(!bShouldEnable);

    if (UseTrigger)
    {
        UseTrigger->SetCollisionEnabled(bShouldEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        UseTrigger->SetGenerateOverlapEvents(bShouldEnable);
    }
}
