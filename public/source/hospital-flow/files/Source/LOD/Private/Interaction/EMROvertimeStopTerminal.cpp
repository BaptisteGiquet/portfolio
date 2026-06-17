
#include "Interaction/EMROvertimeStopTerminal.h"

#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Interaction/EMRInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LOD/EMRCollisionChannels.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"

namespace
{
    constexpr float CoverRotationToleranceDegrees = 0.1f;
    constexpr float ButtonLocationTolerance = 0.01f;
}


AEMROvertimeStopTerminal::AEMROvertimeStopTerminal()
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(false);

    TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
    SetRootComponent(TerminalMesh);
    TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TerminalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    TerminalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TerminalMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    CoverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoverMesh"));
    CoverMesh->SetupAttachment(TerminalMesh);
    CoverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
    ButtonMesh->SetupAttachment(TerminalMesh);
    ButtonMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ButtonMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    ButtonMesh->SetCollisionResponseToChannel(ECC_InteractableWidgets, ECR_Block);
    ButtonMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    ButtonMesh->SetMobility(EComponentMobility::Movable);

    InteractableComponent = CreateDefaultSubobject<UEMRInteractableComponent>(TEXT("InteractableComponent"));
    InteractableComponent->SetDisplayName(FText::FromString(TEXT("Stop Overtime")));
}


void AEMROvertimeStopTerminal::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisClass, bInteractionEnabled);
    DOREPLIFETIME(ThisClass, bInteractionConsumed);
}


void AEMROvertimeStopTerminal::BeginPlay()
{
    Super::BeginPlay();

    CacheButtonLocations();
    ApplyButtonPressedState(bInteractionConsumed);
    ApplyInteractionEnabledState(IsNightShiftInOvertime());

    if (UWorld* World = GetWorld())
    {
        if (AEMRNightShiftGameState* NightShiftGameState = World->GetGameState<AEMRNightShiftGameState>())
        {
            if (!OvertimeStartedHandle.IsValid())
            {
                OvertimeStartedHandle = NightShiftGameState->OnNightShiftOvertimeStarted().AddUObject(this, &ThisClass::HandleNightShiftOvertimeStarted);
            }

            if (!PendingTravelChangedHandle.IsValid())
            {
                PendingTravelChangedHandle = NightShiftGameState->OnPendingTravelChanged().AddUObject(this, &ThisClass::HandlePendingTravelChanged);
            }
        }
    }
}


void AEMROvertimeStopTerminal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (OvertimeStartedHandle.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            if (AEMRNightShiftGameState* NightShiftGameState = World->GetGameState<AEMRNightShiftGameState>())
            {
                NightShiftGameState->OnNightShiftOvertimeStarted().Remove(OvertimeStartedHandle);
            }
        }
        OvertimeStartedHandle.Reset();
    }

    if (PendingTravelChangedHandle.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            if (AEMRNightShiftGameState* NightShiftGameState = World->GetGameState<AEMRNightShiftGameState>())
            {
                NightShiftGameState->OnPendingTravelChanged().Remove(PendingTravelChangedHandle);
            }
        }
        PendingTravelChangedHandle.Reset();
    }

    StopOvertimeStopAlertSound();

    Super::EndPlay(EndPlayReason);
}


void AEMROvertimeStopTerminal::ApplyInteractionEnabledState(bool bEnable)
{
    const bool bWasEnabled = bInteractionEnabled;
    bInteractionEnabled = bEnable;
    RefreshInteractionAvailability();
    ApplyCoverRotation(bInteractionEnabled);

    if (HasAuthority() && !bWasEnabled && bInteractionEnabled)
    {
        PlayTerminalSound(LidOpenSound);
    }
}


bool AEMROvertimeStopTerminal::IsNightShiftInOvertime() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const AEMRNightShiftGameState* RunGS = World->GetGameState<AEMRNightShiftGameState>())
        {
            return RunGS->IsInNightShiftOvertime();
        }
    }

    return false;
}


void AEMROvertimeStopTerminal::HandleNightShiftOvertimeStarted()
{
    ApplyInteractionEnabledState(true);
}


void AEMROvertimeStopTerminal::OnRep_InteractionEnabled()
{
    ApplyInteractionEnabledState(bInteractionEnabled);
}

void AEMROvertimeStopTerminal::OnRep_InteractionConsumed()
{
    RefreshInteractionAvailability();
    ApplyButtonPressedState(bInteractionConsumed);
}


void AEMROvertimeStopTerminal::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bIsCoverAnimating)
    {
        UpdateCoverRotation(DeltaSeconds);
    }

    if (bIsButtonAnimating)
    {
        UpdateButtonLocation(DeltaSeconds);
    }
}


void AEMROvertimeStopTerminal::ApplyCoverRotation(bool bOpen)
{
    if (!CoverMesh)
    {
        return;
    }

    const FRotator TargetCoverRotation = bOpen ? CoverOpenRotation : CoverClosedRotation;
    TargetCoverRotationQuat = TargetCoverRotation.Quaternion();

    const FQuat CurrentCoverRotationQuat = CoverMesh->GetRelativeTransform().GetRotation();
    const float AngleDeltaRadians = CurrentCoverRotationQuat.AngularDistance(TargetCoverRotationQuat);
    if (CoverInterpSpeed <= 0.0f || AngleDeltaRadians <= FMath::DegreesToRadians(CoverRotationToleranceDegrees))
    {
        CoverMesh->SetRelativeRotation(TargetCoverRotationQuat, false, nullptr, ETeleportType::None);
        bIsCoverAnimating = false;
        UpdateTickEnabled();
        return;
    }

    bIsCoverAnimating = true;
    UpdateTickEnabled();
}


void AEMROvertimeStopTerminal::UpdateCoverRotation(float DeltaSeconds)
{
    if (!CoverMesh)
    {
        bIsCoverAnimating = false;
        UpdateTickEnabled();
        return;
    }

    const FQuat CurrentRotationQuat = CoverMesh->GetRelativeTransform().GetRotation();
    const float InterpAlpha = FMath::Clamp(DeltaSeconds * CoverInterpSpeed, 0.0f, 1.0f);
    const FQuat NewRotationQuat = FQuat::Slerp(CurrentRotationQuat, TargetCoverRotationQuat, InterpAlpha).GetNormalized();
    CoverMesh->SetRelativeRotation(NewRotationQuat, false, nullptr, ETeleportType::None);

    const float AngleDeltaRadians = NewRotationQuat.AngularDistance(TargetCoverRotationQuat);
    if (AngleDeltaRadians <= FMath::DegreesToRadians(CoverRotationToleranceDegrees))
    {
        CoverMesh->SetRelativeRotation(TargetCoverRotationQuat, false, nullptr, ETeleportType::None);
        bIsCoverAnimating = false;
        UpdateTickEnabled();
    }
}


void AEMROvertimeStopTerminal::UpdateTickEnabled()
{
    SetActorTickEnabled(bIsCoverAnimating || bIsButtonAnimating);
}

void AEMROvertimeStopTerminal::RefreshInteractionAvailability()
{
    if (InteractableComponent)
    {
        InteractableComponent->SetInteractionEventTag(FGameplayTag());
        InteractableComponent->SetEnabled(false);
    }
}

void AEMROvertimeStopTerminal::CacheButtonLocations()
{
    if (!ButtonMesh)
    {
        return;
    }

    ButtonRestRelativeLocation = ButtonMesh->GetRelativeLocation();
    ButtonPressedRelativeLocation = ButtonRestRelativeLocation + ButtonPressedOffset;
    TargetButtonRelativeLocation = ButtonRestRelativeLocation;
    bButtonLocationsCached = true;
}

void AEMROvertimeStopTerminal::ApplyButtonPressedState(bool bPressed)
{
    if (!ButtonMesh)
    {
        return;
    }

    if (!bButtonLocationsCached)
    {
        CacheButtonLocations();
    }

    TargetButtonRelativeLocation = bPressed ? ButtonPressedRelativeLocation : ButtonRestRelativeLocation;

    const FVector CurrentLocation = ButtonMesh->GetRelativeLocation();
    if (ButtonInterpSpeed <= 0.0f || CurrentLocation.Equals(TargetButtonRelativeLocation, ButtonLocationTolerance))
    {
        ButtonMesh->SetRelativeLocation(TargetButtonRelativeLocation, false, nullptr, ETeleportType::None);
        bIsButtonAnimating = false;
        UpdateTickEnabled();
        return;
    }

    bIsButtonAnimating = true;
    UpdateTickEnabled();
}

void AEMROvertimeStopTerminal::UpdateButtonLocation(float DeltaSeconds)
{
    if (!ButtonMesh)
    {
        bIsButtonAnimating = false;
        UpdateTickEnabled();
        return;
    }

    const FVector CurrentLocation = ButtonMesh->GetRelativeLocation();
    const FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetButtonRelativeLocation, DeltaSeconds, ButtonInterpSpeed);
    ButtonMesh->SetRelativeLocation(NewLocation, false, nullptr, ETeleportType::None);

    if (NewLocation.Equals(TargetButtonRelativeLocation, ButtonLocationTolerance))
    {
        ButtonMesh->SetRelativeLocation(TargetButtonRelativeLocation, false, nullptr, ETeleportType::None);
        bIsButtonAnimating = false;
        UpdateTickEnabled();
    }
}

void AEMROvertimeStopTerminal::PlayTerminalSound(USoundBase* Sound)
{
    if (!HasAuthority())
    {
        return;
    }

    Multicast_PlayTerminalSound(Sound);
}

void AEMROvertimeStopTerminal::PlayOvertimeStopAlertSound()
{
    if (!HasAuthority())
    {
        return;
    }

    Multicast_PlayOvertimeStopAlertSound(OvertimeStopAlertSound);
}

void AEMROvertimeStopTerminal::StopOvertimeStopAlertSound()
{
    if (UAudioComponent* AudioComponent = ActiveOvertimeStopAlertAudio.Get())
    {
        AudioComponent->Stop();
    }

    ActiveOvertimeStopAlertAudio.Reset();
}

void AEMROvertimeStopTerminal::HandlePendingTravelChanged()
{
    const UWorld* World = GetWorld();
    const AEMRNightShiftGameState* RunGS = World ? World->GetGameState<AEMRNightShiftGameState>() : nullptr;
    if (!RunGS)
    {
        return;
    }

    if (!RunGS->GetPendingTravelURL().IsEmpty())
    {
        StopOvertimeStopAlertSound();
    }
}

void AEMROvertimeStopTerminal::Multicast_PlayTerminalSound_Implementation(USoundBase* Sound)
{
    if (GetNetMode() == NM_DedicatedServer || !Sound)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
}

void AEMROvertimeStopTerminal::Multicast_PlayOvertimeStopAlertSound_Implementation(USoundBase* Sound)
{
    if (GetNetMode() == NM_DedicatedServer || !Sound)
    {
        return;
    }

    StopOvertimeStopAlertSound();
    ActiveOvertimeStopAlertAudio = UGameplayStatics::SpawnSound2D(this, Sound);
}

bool AEMROvertimeStopTerminal::ConsumeTerminalInteraction()
{
    if (!HasAuthority() || !bInteractionEnabled || bInteractionConsumed)
    {
        return false;
    }

    bInteractionConsumed = true;
    OnRep_InteractionConsumed();
    PlayTerminalSound(InteractionSound);
    PlayOvertimeStopAlertSound();
    ForceNetUpdate();
    return true;
}

bool AEMROvertimeStopTerminal::TryConsumeTerminalInteraction()
{
    return ConsumeTerminalInteraction();
}

bool AEMROvertimeStopTerminal::IsButtonComponent(const UPrimitiveComponent* Component) const
{
    return Component && ButtonMesh && Component == ButtonMesh;
}

bool AEMROvertimeStopTerminal::IsConfirmButtonInteractionEnabled() const
{
    return bInteractionEnabled && !bInteractionConsumed;
}

FGameplayEventData AEMROvertimeStopTerminal::GetInteractionEventData_Implementation(AActor* InterActor) const
{
    if (InteractableComponent)
    {
        FGameplayEventData EventData = InteractableComponent->BuildInteractionEventData(InterActor);
        EventData.EventTag = FGameplayTag();
        return EventData;
    }

    return FGameplayEventData();
}

FText AEMROvertimeStopTerminal::GetInteractableDisplayName_Implementation() const
{
    return InteractableComponent ? InteractableComponent->GetDisplayName() : FText::FromString(GetName());
}

bool AEMROvertimeStopTerminal::IsInteractableEnabled_Implementation() const
{
    return false;
}


