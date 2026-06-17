#include "Characters/NPC/EMRHubertCharacter.h"

#include "AIController.h"
#include "Characters/NPC/EMRHubertAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"

namespace
{
const TCHAR* HubertStateToString_Character(const EEMRHubertState State)
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


AEMRHubertCharacter::AEMRHubertCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetReplicateMovement(true);
    bAlwaysRelevant = true;

    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->bOrientRotationToMovement = true;
    }

    if (USkeletalMeshComponent* MeshComponent = GetMesh())
    {
        MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    }
}


void AEMRHubertCharacter::BeginPlay()
{
    Super::BeginPlay();

    PushStateToAnimInstance();
}


void AEMRHubertCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, HubertState);
}


void AEMRHubertCharacter::SetHubertState(const EEMRHubertState NewState)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] SetHubertState rejected on non-authority character=%s"), *GetNameSafe(this));
        return;
    }

    if (HubertState == NewState)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] SetHubertState ignored (already %s) actor=%s"),
            HubertStateToString_Character(NewState),
            *GetNameSafe(this));
        return;
    }


    HubertState = NewState;
    OnRep_HubertState();
    ForceNetUpdate();
}


void AEMRHubertCharacter::PlayVoiceLineForAllPlayers(USoundBase* VoiceLine)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] PlayVoiceLineForAllPlayers rejected on non-authority character=%s"), *GetNameSafe(this));
        return;
    }

    if (!VoiceLine)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Missing voice line for playback on %s"), *GetNameSafe(this));
        return;
    }

    Multicast_PlayVoiceLineAtHubert(VoiceLine);
}


void AEMRHubertCharacter::OnRep_HubertState()
{
    PushStateToAnimInstance();
}

void AEMRHubertCharacter::Multicast_PlayVoiceLineAtHubert_Implementation(USoundBase* VoiceLine)
{
    if (!VoiceLine)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Multicast_PlayVoiceLineAtHubert actor=%s voice=%s"),
        *GetNameSafe(this),
        *GetNameSafe(VoiceLine));
    UGameplayStatics::PlaySoundAtLocation(this, VoiceLine, GetActorLocation());
}


void AEMRHubertCharacter::PushStateToAnimInstance() const
{
    USkeletalMeshComponent* MeshComponent = GetMesh();
    if (!MeshComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HUBERT_FLOW] Missing mesh component on %s while applying state=%d"), *GetNameSafe(this), static_cast<int32>(HubertState));
        return;
    }

    if (HubertAnimInstanceClass && MeshComponent->GetAnimClass() != HubertAnimInstanceClass)
    {
        MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        MeshComponent->SetAnimInstanceClass(HubertAnimInstanceClass);
    }
	
    if (UEMRHubertAnimInstance* HubertAnimInstance = Cast<UEMRHubertAnimInstance>(MeshComponent->GetAnimInstance()))
    {
        HubertAnimInstance->SetHubertState(HubertState);
    }
    else if (MeshComponent->GetAnimClass())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[HUBERT_FLOW] Anim class on %s is not UEMRHubertAnimInstance. CurrentAnimClass=%s"),
            *GetNameSafe(this),
            *GetNameSafe(MeshComponent->GetAnimClass()));
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[HUBERT_FLOW] No anim class configured on %s. Assign HubertAnimInstanceClass or set AnimClass in BP."),
            *GetNameSafe(this));
    }
}
