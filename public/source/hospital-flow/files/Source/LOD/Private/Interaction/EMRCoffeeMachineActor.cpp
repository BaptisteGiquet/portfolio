#include "Interaction/EMRCoffeeMachineActor.h"

#include "Characters/Player/EMRPlayerCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/EMRDifficultyTuningData.h"
#include "Framework/EMRNightShiftGameState.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRCoffeePitcherItemActor.h"
#include "Interaction/EMRInteractableComponent.h"
#include "LOD/EMRCollisionChannels.h"
#include "Net/UnrealNetwork.h"

namespace
{
    constexpr TCHAR CoffeeMachineUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");
}

AEMRCoffeeMachineActor::AEMRCoffeeMachineActor()
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;

    MachineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MachineMesh"));
    SetRootComponent(MachineMesh);
    MachineMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MachineMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    MachineMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    PitcherAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PitcherAnchor"));
    PitcherAnchor->SetupAttachment(MachineMesh);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
    InteractableComponent->SetDisplayName(FText::FromString(TEXT("Coffee Machine")));
    InteractableComponent->SetInteractionEventTag(EMRTags::Machine::CoffeeMachine);

    BrewLoopAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BrewLoopAudioComponent"));
    BrewLoopAudioComponent->SetupAttachment(MachineMesh);
    BrewLoopAudioComponent->SetAutoActivate(false);
    BrewLoopAudioComponent->bAutoActivate = false;

    RequiredUpgradeTag = EMRTags::RunUpgrade::CoffeeMachine;
}

void AEMRCoffeeMachineActor::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        const AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
        const int32 UpgradeStackCount = RunGS ? RunGS->GetActiveRunUpgradeStackCount(RequiredUpgradeTag) : 0;
        const bool bShouldEnable = UpgradeStackCount >= GetRequiredUpgradeStackCount();
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CoffeeMachine BeginPlay authority. ShouldEnable=%d RequiredTag=%s Stack=%d RequiredStack=%d"),
            CoffeeMachineUpgradesFlowLogPrefix,
            bShouldEnable ? 1 : 0,
            *RequiredUpgradeTag.ToString(),
            UpgradeStackCount,
            GetRequiredUpgradeStackCount());
        SetCoffeeMachineEnabledByUpgrade(bShouldEnable);
    }
    else
    {
        ApplyMachineEnabledState();
    }

    UpdateMachineVisibilityTraceResponse();
    UpdateBrewLoopAudioState();
}

void AEMRCoffeeMachineActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateMachineVisibilityTraceResponse();
    UpdateBrewLoopAudioState();
}

void AEMRCoffeeMachineActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyMachineEnabledState();
}

void AEMRCoffeeMachineActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, bCoffeeMachineEnabledByUpgrade);
    DOREPLIFETIME(ThisClass, OwnedPitcher);
}

FGameplayEventData AEMRCoffeeMachineActor::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    return InteractableComponent ? InteractableComponent->BuildInteractionEventData(InterActor) : FGameplayEventData();
}

FText AEMRCoffeeMachineActor::GetInteractableDisplayName_Implementation() const
{
    return InteractableComponent ? InteractableComponent->GetDisplayName() : FText::FromString(GetName());
}

bool AEMRCoffeeMachineActor::IsInteractableEnabled_Implementation() const
{
    const bool bPitcherBrewing = OwnedPitcher && OwnedPitcher->IsBrewing();
    return bCoffeeMachineEnabledByUpgrade
    && InteractableComponent
    && InteractableComponent->IsEnabled()
    && !bPitcherBrewing
    && !ShouldHideMachineForPickup();
}

bool AEMRCoffeeMachineActor::TryDockPitcher(AEMRPlayerCharacter* Player, AEMRCoffeePitcherItemActor* PitcherActor)
{
    if (!HasAuthority() || !bCoffeeMachineEnabledByUpgrade || !Player || !PitcherActor || PitcherActor != OwnedPitcher)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CoffeeMachine TryDockPitcher rejected auth=%d enabled=%d player=%s pitcher=%s ownedPitcher=%s"),
            CoffeeMachineUpgradesFlowLogPrefix,
            HasAuthority() ? 1 : 0,
            bCoffeeMachineEnabledByUpgrade ? 1 : 0,
            *GetNameSafe(Player),
            *GetNameSafe(PitcherActor),
            *GetNameSafe(OwnedPitcher));
        return false;
    }

    if (!PitcherActor->IsCarriedBy(Player))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine TryDockPitcher rejected: pitcher is not carried by player."), CoffeeMachineUpgradesFlowLogPrefix);
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine TryDockPitcher accepted. Returning pitcher to anchor."), CoffeeMachineUpgradesFlowLogPrefix);
    PitcherActor->ReturnToAnchor();
    return true;
}

bool AEMRCoffeeMachineActor::TryStartBrew(AEMRPlayerCharacter* Player)
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CoffeeMachine TryStartBrew requested by player=%s"),
        CoffeeMachineUpgradesFlowLogPrefix,
        *GetNameSafe(Player));

    if (!HasAuthority() || !bCoffeeMachineEnabledByUpgrade)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CoffeeMachine TryStartBrew ignored auth=%d enabled=%d"),
            CoffeeMachineUpgradesFlowLogPrefix,
            HasAuthority() ? 1 : 0,
            bCoffeeMachineEnabledByUpgrade ? 1 : 0);
        return false;
    }

    if (!OwnedPitcher)
    {
        EnsurePitcherSpawned();
    }

    if (!OwnedPitcher)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine TryStartBrew aborted: no owned pitcher."), CoffeeMachineUpgradesFlowLogPrefix);
        return false;
    }

    if (OwnedPitcher->IsCurrentlyCarried())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine TryStartBrew rejected: pitcher is currently carried."), CoffeeMachineUpgradesFlowLogPrefix);
        return false;
    }

    if (OwnedPitcher->IsBrewing())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine TryStartBrew ignored: pitcher already brewing."), CoffeeMachineUpgradesFlowLogPrefix);
        return false;
    }

    OwnedPitcher->ConfigureCoffeePayload(MaxServingsPerFill, PatienceRestorePercentOfMax);
    if (!OwnedPitcher->IsFull())
    {
        if (OwnedPitcher->StartBrewFill(BrewFillDurationSeconds))
        {
            UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine brew started. Duration=%.2f"), CoffeeMachineUpgradesFlowLogPrefix, BrewFillDurationSeconds);
            UpdateMachineVisibilityTraceResponse();
            UpdateBrewLoopAudioState();
            return true;
        }

        UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine brew did not start (pitcher rejected)."), CoffeeMachineUpgradesFlowLogPrefix);
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine brew ignored: pitcher already full."), CoffeeMachineUpgradesFlowLogPrefix);
    return false;
}

void AEMRCoffeeMachineActor::ApplyUpgradeTuning(const FEMRCoffeeUpgradeTuning& InTuning)
{
    MaxServingsPerFill = FMath::Max(1, InTuning.MaxServingsPerFill);
    PatienceRestorePercentOfMax = FMath::Clamp(InTuning.PatienceRestorePercentOfMax, 0.0f, 1.0f);
    BrewFillDurationSeconds = FMath::Max(0.1f, InTuning.BrewFillDurationSeconds);

    if (OwnedPitcher)
    {
        OwnedPitcher->ConfigureCoffeePayload(MaxServingsPerFill, PatienceRestorePercentOfMax);
    }
}

void AEMRCoffeeMachineActor::SetCoffeeMachineEnabledByUpgrade(bool bEnabled)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine SetEnabled ignored: no authority."), CoffeeMachineUpgradesFlowLogPrefix);
        return;
    }

    if (bCoffeeMachineEnabledByUpgrade == bEnabled)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CoffeeMachine SetEnabled no-op enabled=%d pitcher=%s"),
            CoffeeMachineUpgradesFlowLogPrefix,
            bEnabled ? 1 : 0,
            *GetNameSafe(OwnedPitcher));
        if (bCoffeeMachineEnabledByUpgrade && !OwnedPitcher)
        {
            EnsurePitcherSpawned();
        }
        // Re-apply runtime state even on no-op transitions; otherwise machines can stay visible
        // when they start disabled and no replicated value change occurs.
        ApplyMachineEnabledState();
        UpdateBrewLoopAudioState();
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CoffeeMachine SetEnabled changing %d -> %d"),
        CoffeeMachineUpgradesFlowLogPrefix,
        bCoffeeMachineEnabledByUpgrade ? 1 : 0,
        bEnabled ? 1 : 0);
    bCoffeeMachineEnabledByUpgrade = bEnabled;

    if (bCoffeeMachineEnabledByUpgrade)
    {
        EnsurePitcherSpawned();
    }
    else if (OwnedPitcher)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine disabling: destroying owned pitcher %s"), CoffeeMachineUpgradesFlowLogPrefix, *GetNameSafe(OwnedPitcher));
        OwnedPitcher->Destroy();
        OwnedPitcher = nullptr;
    }

    OnRep_CoffeeMachineEnabled();
    UpdateBrewLoopAudioState();
}

void AEMRCoffeeMachineActor::EnsurePitcherSpawned()
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine EnsurePitcherSpawned ignored: no authority."), CoffeeMachineUpgradesFlowLogPrefix);
        return;
    }

    if (!OwnedPitcher && PitcherClass)
    {
        const FTransform SpawnTransform = PitcherAnchor ? PitcherAnchor->GetComponentTransform() : GetActorTransform();
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        OwnedPitcher = GetWorld()->SpawnActor<AEMRCoffeePitcherItemActor>(PitcherClass, SpawnTransform, SpawnParams);
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CoffeeMachine spawned owned pitcher class=%s actor=%s"),
            CoffeeMachineUpgradesFlowLogPrefix,
            *GetNameSafe(PitcherClass),
            *GetNameSafe(OwnedPitcher));
    }

    if (OwnedPitcher)
    {
        OwnedPitcher->SetOwningMachine(this);
        if (PitcherAnchor)
        {
            OwnedPitcher->SetAnchorTransform(PitcherAnchor->GetComponentTransform());
        }
        OwnedPitcher->SetReturnTraceComponent(MachineMesh);
        OwnedPitcher->ConfigureCoffeePayload(MaxServingsPerFill, PatienceRestorePercentOfMax);
        OwnedPitcher->ReturnToAnchor();
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CoffeeMachine initialized owned pitcher servings=%d restorePct=%.2f"),
            CoffeeMachineUpgradesFlowLogPrefix,
            MaxServingsPerFill,
            PatienceRestorePercentOfMax);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s CoffeeMachine EnsurePitcherSpawned failed: PitcherClass missing or spawn failed."), CoffeeMachineUpgradesFlowLogPrefix);
    }
}

void AEMRCoffeeMachineActor::ApplyMachineEnabledState()
{
    if (InteractableComponent)
    {
        InteractableComponent->SetEnabled(bCoffeeMachineEnabledByUpgrade);
    }

    if (HasActorBegunPlay())
    {
        SetActorHiddenInGame(!bCoffeeMachineEnabledByUpgrade);
    }

    UpdateMachineVisibilityTraceResponse();
    UpdateBrewLoopAudioState();
}

void AEMRCoffeeMachineActor::OnRep_CoffeeMachineEnabled()
{
    ApplyMachineEnabledState();
    UpdateBrewLoopAudioState();
}

void AEMRCoffeeMachineActor::UpdateMachineVisibilityTraceResponse()
{
    if (!MachineMesh)
    {
        return;
    }

    const bool bShouldEnableCollisionQuery = bCoffeeMachineEnabledByUpgrade;
    const bool bPitcherBrewing = OwnedPitcher && OwnedPitcher->IsBrewing();
    const bool bShouldEnableMachineCollision = bShouldEnableCollisionQuery && !bPitcherBrewing;
    const bool bShouldBlockVisibility = bShouldEnableMachineCollision && !ShouldHideMachineForPickup();
    const bool bCollisionQueryEnabledNow = MachineMesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
    const bool bVisibilityBlocksNow = MachineMesh->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block;
    if (bMachineVisibilityBlocksTrace == bShouldBlockVisibility
        && bMachineCollisionQueryEnabled == bShouldEnableMachineCollision
        && bCollisionQueryEnabledNow == bShouldEnableMachineCollision
        && bVisibilityBlocksNow == bShouldBlockVisibility)
    {
        return;
    }

    bMachineVisibilityBlocksTrace = bShouldBlockVisibility;
    bMachineCollisionQueryEnabled = bShouldEnableMachineCollision;
    MachineMesh->SetCollisionEnabled(bShouldEnableMachineCollision ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    MachineMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    MachineMesh->SetCollisionResponseToChannel(ECC_Visibility, bShouldBlockVisibility ? ECR_Block : ECR_Ignore);
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s CoffeeMachine collision state changed machine=%s queryEnabled=%d blockVisibility=%d enabled=%d ownedPitcher=%s pitcherCarried=%d pitcherFull=%d pitcherBrewing=%d"),
        CoffeeMachineUpgradesFlowLogPrefix,
        *GetNameSafe(this),
        bShouldEnableMachineCollision ? 1 : 0,
        bShouldBlockVisibility ? 1 : 0,
        bCoffeeMachineEnabledByUpgrade ? 1 : 0,
        *GetNameSafe(OwnedPitcher),
        (OwnedPitcher && OwnedPitcher->IsCurrentlyCarried()) ? 1 : 0,
        (OwnedPitcher && OwnedPitcher->IsFull()) ? 1 : 0,
        bPitcherBrewing ? 1 : 0);
}

bool AEMRCoffeeMachineActor::ShouldHideMachineForPickup() const
{
    return bCoffeeMachineEnabledByUpgrade
    && OwnedPitcher
    && OwnedPitcher->IsFull()
    && !OwnedPitcher->IsCurrentlyCarried();
}

void AEMRCoffeeMachineActor::UpdateBrewLoopAudioState()
{
    if (!BrewLoopAudioComponent)
    {
        bBrewLoopAudioPlaying = false;
        return;
    }

    USoundBase* LoopSound = BrewStartSound.Get()
    ? BrewStartSound.Get()
    : BrewLoopAudioComponent->GetSound();
    const bool bShouldPlayLoop = bCoffeeMachineEnabledByUpgrade
    && LoopSound
    && OwnedPitcher
    && OwnedPitcher->IsBrewing();

    if (bShouldPlayLoop)
    {
        if (BrewLoopAudioComponent->GetSound() != LoopSound)
        {
            BrewLoopAudioComponent->SetSound(LoopSound);
        }
        if (!bBrewLoopAudioPlaying || !BrewLoopAudioComponent->IsPlaying())
        {
            BrewLoopAudioComponent->Play();
            bBrewLoopAudioPlaying = true;
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("%s CoffeeMachine brew loop sound started machine=%s sound=%s"),
                CoffeeMachineUpgradesFlowLogPrefix,
                *GetNameSafe(this),
                *GetNameSafe(LoopSound));
        }
        return;
    }

    if (bBrewLoopAudioPlaying || BrewLoopAudioComponent->IsPlaying())
    {
        BrewLoopAudioComponent->Stop();
        bBrewLoopAudioPlaying = false;
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s CoffeeMachine brew loop sound stopped machine=%s"),
            CoffeeMachineUpgradesFlowLogPrefix,
            *GetNameSafe(this));
    }
}
