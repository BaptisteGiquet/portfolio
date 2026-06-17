#include "Environment/EMRToiletSlotActor.h"

#include "Characters/Patient/EMRPatient.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Interaction/EMRToiletCleaner.h"
#include "Subsystems/EMRToiletSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AEMRToiletSlotActor::AEMRToiletSlotActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    ApproachArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ApproachArrow"));
    ApproachArrow->SetupAttachment(SceneRoot);

    CleanerAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("CleanerAnchor"));
    CleanerAnchor->SetupAttachment(SceneRoot);

    CleanerAnchorTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("CleanerAnchorTrigger"));
    CleanerAnchorTrigger->SetupAttachment(CleanerAnchor);
    CleanerAnchorTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CleanerAnchorTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    CleanerAnchorTrigger->SetGenerateOverlapEvents(false);


    UseTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("UseTrigger"));
    UseTrigger->SetupAttachment(SceneRoot);
    UseTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    UseTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    UseTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    UseTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleUseTriggerOverlap);

    DirtDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("DirtDecal"));
    DirtDecal->SetupAttachment(SceneRoot);
    DirtDecal->SetHiddenInGame(true);
    DirtDecal->SetVisibility(false);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
    if (InteractableComponent)
    {
        InteractableComponent->SetDisplayName(FText::FromString(TEXT("Dirty Toilet")));
        InteractableComponent->SetEnabled(false);
    }
}

void AEMRToiletSlotActor::BeginPlay()
{
    Super::BeginPlay();

    UpdateDirtDecal();
    UpdateInteractableState();

    if (HasAuthority())
    {
        if (UEMRToiletSubsystem* ToiletSubsystem = GetWorld()->GetSubsystem<UEMRToiletSubsystem>())
        {
            ToiletSubsystem->RegisterSlot(this);
        }

        if (ToiletCleaner && CleanerAnchor)
        {
            ToiletCleaner->SetAnchorTransform(CleanerAnchor->GetComponentTransform());
            ToiletCleaner->SetReturnTraceComponent(CleanerAnchorTrigger);
            ToiletCleaner->ReturnToAnchor();
        }
    }
}

void AEMRToiletSlotActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
    {
        if (UEMRToiletSubsystem* ToiletSubsystem = GetWorld()->GetSubsystem<UEMRToiletSubsystem>())
        {
            ToiletSubsystem->UnregisterSlot(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void AEMRToiletSlotActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEMRToiletSlotActor, bIsDirty);
    DOREPLIFETIME(AEMRToiletSlotActor, bIsOccupied);
}

FGameplayEventData AEMRToiletSlotActor::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    return InteractableComponent ? InteractableComponent->BuildInteractionEventData(InterActor) : FGameplayEventData();
}

FText AEMRToiletSlotActor::GetInteractableDisplayName_Implementation() const
{
    return InteractableComponent ? InteractableComponent->GetDisplayName() : FText::FromString(TEXT("Toilet"));
}

bool AEMRToiletSlotActor::IsInteractableEnabled_Implementation() const
{
    return bIsDirty && !bIsOccupied;
}

void AEMRToiletSlotActor::AssignToPatient(AEMRPatient* Patient)
{
    AssignedPatient = Patient;
}

void AEMRToiletSlotActor::ClearAssignment()
{
    AssignedPatient.Reset();
}

bool AEMRToiletSlotActor::IsAvailable() const
{
    return !AssignedPatient.IsValid() && !bIsOccupied;
}

void AEMRToiletSlotActor::SetOccupied(bool bNewOccupied)
{
    if (bIsOccupied == bNewOccupied)
    {
        return;
    }

    bIsOccupied = bNewOccupied;
    OnRep_Occupied();
    ForceNetUpdate();
}

void AEMRToiletSlotActor::SetDirty(bool bNewDirty)
{
    if (bIsDirty == bNewDirty)
    {
        return;
    }

    bIsDirty = bNewDirty;
    OnRep_Dirty();
    ForceNetUpdate();
}

FTransform AEMRToiletSlotActor::GetApproachTransform() const
{
    if (ApproachArrow)
    {
        return ApproachArrow->GetComponentTransform();
    }

    return GetActorTransform();
}

void AEMRToiletSlotActor::PlayUseOutcomeSound(bool bBecameDirty)
{
    if (!HasAuthority())
    {
        return;
    }

    Multicast_PlayToiletUseOutcomeSound(bBecameDirty);
}

void AEMRToiletSlotActor::HandleUseTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || bIsOccupied)
    {
        return;
    }

    if (AssignedPatient.IsValid() && OtherActor == AssignedPatient.Get())
    {
        if (UEMRToiletSubsystem* ToiletSubsystem = GetWorld()->GetSubsystem<UEMRToiletSubsystem>())
        {
            ToiletSubsystem->NotifySlotArrival(this, AssignedPatient.Get());
        }
    }
}

void AEMRToiletSlotActor::OnRep_Dirty()
{
    UpdateDirtDecal();
    UpdateInteractableState();
}

void AEMRToiletSlotActor::OnRep_Occupied()
{
    UpdateInteractableState();
}

void AEMRToiletSlotActor::Multicast_PlayToiletUseOutcomeSound_Implementation(bool bBecameDirty)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    USoundBase* SoundToPlay = bBecameDirty ? ToiletUseDirtySound : ToiletUseCleanSound;
    if (!SoundToPlay)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());
}

void AEMRToiletSlotActor::UpdateDirtDecal()
{
    if (DirtDecal)
    {
        DirtDecal->SetHiddenInGame(!bIsDirty);
        DirtDecal->SetVisibility(bIsDirty, true);
    }
}

void AEMRToiletSlotActor::UpdateInteractableState()
{
    if (InteractableComponent)
    {
        InteractableComponent->SetEnabled(bIsDirty && !bIsOccupied);
    }
}
