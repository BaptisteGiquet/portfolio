
#include "Interaction/EMRTreatmentBed.h"

#include "Subsystems/EMRTreatmentSubsystem.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRAIController.h"
#include "Framework/EMRNightShiftGameState.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UI/Machines/TreatmentBed/DEPRECATEDEMRTreatmentScreenWidget.h"
#include "UI/Patient/EMRPatientUIController.h"
#include "Animation/AnimInstance.h"
#include "Interaction/EMRTreatmentBedFolder.h"
#include "TimerManager.h"


AEMRTreatmentBed::AEMRTreatmentBed()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
	
    BedRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BedRoot"));
    SetRootComponent(BedRoot);
	
    BedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BedMesh"));
    BedMesh->SetupAttachment(GetRootComponent());

    // Patient position marker (offset above bed)
    PatientPositionMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("PatientPositionMarker"));
    PatientPositionMarker->SetupAttachment(GetRootComponent());

    PatientAttachPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("PatientAttachPoint"));
    PatientAttachPoint->SetupAttachment(GetRootComponent());

    PatientAttachTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PatientAttachTrigger"));
    PatientAttachTrigger->SetupAttachment(GetRootComponent());
    PatientAttachTrigger->SetSphereRadius(140.0f);
    PatientAttachTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PatientAttachTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    PatientAttachTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	TreatmentScreenComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TreatmentScreen"));
	TreatmentScreenComponent->SetupAttachment(GetRootComponent());
	TreatmentScreenComponent->SetWidgetSpace(EWidgetSpace::World);
	TreatmentScreenComponent->SetDrawSize(FVector2D(1920.0f, 1080.0f));

    PatientFolderAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("PatientFolderAnchor"));
    PatientFolderAnchor->SetupAttachment(GetRootComponent());
}


void AEMRTreatmentBed::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEMRTreatmentBed, CurrentPatient);
	DOREPLIFETIME(AEMRTreatmentBed, AttachedPatient);
    DOREPLIFETIME(AEMRTreatmentBed, bTreatmentBedEnabledByUpgrade);
}


void AEMRTreatmentBed::BeginPlay()
{
    Super::BeginPlay();

    const bool bUsesUpgradeGate = RequiredUpgradeTag.IsValid();
    if (HasAuthority())
    {
        TreatmentSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>() : nullptr;
        if (bUsesUpgradeGate)
        {
            const AEMRNightShiftGameState* RunGS = GetWorld() ? GetWorld()->GetGameState<AEMRNightShiftGameState>() : nullptr;
            const int32 CurrentUpgradeStackCount = RunGS ? RunGS->GetActiveRunUpgradeStackCount(RequiredUpgradeTag) : 0;
            bTreatmentBedEnabledByUpgrade = CurrentUpgradeStackCount >= GetRequiredUpgradeStackCount();
        }
        else
        {
            bTreatmentBedEnabledByUpgrade = true;
        }

        if (bTreatmentBedEnabledByUpgrade)
        {
            RegisterWithTreatmentSubsystemIfNeeded();
        }

        if (PatientAttachTrigger)
        {
            PatientAttachTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleAttachTriggerOverlap);
        }

        if (PatientFolder)
        {
            if (PatientFolderAnchor)
            {
                PatientFolder->SetAnchorTransform(PatientFolderAnchor->GetComponentTransform());
            }
            PatientFolder->ReturnToAnchor();
            PatientFolder->ClearAssignedPatient();
        }
    }
    ApplyTreatmentBedEnabledState();
}


void AEMRTreatmentBed::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
    {
        UnregisterFromTreatmentSubsystemIfNeeded();
    }

    Super::EndPlay(EndPlayReason);
}


void AEMRTreatmentBed::AssignPatient(AEMRPatient* Patient)
{
    if (!Patient || CurrentPatient)
    {
        return;
    }

    CurrentPatient = Patient;

	if (!PatientUIController)
	{
		PatientUIController = NewObject<UEMRPatientUIController>(this);
	}

	PatientUIController->BindToPatient(Patient);
	PatientUIController->StartMonitoring();

	UpdateScreenDisplay();

    if (PatientFolder)
    {
        PatientFolder->ReturnToAnchor();
        PatientFolder->SetAssignedPatient(Patient);
    }

    if (PatientAttachTrigger && PatientAttachTrigger->IsOverlappingActor(Patient))
    {
        AttachPatientToBed(Patient);
    }
}


void AEMRTreatmentBed::ReleasePatient()
{
    if (!CurrentPatient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] Bed ReleasePatient ignored. Bed=%s Patient=null"),
            *GetNameSafe(this));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Bed ReleasePatient start. Bed=%s Patient=%s"),
        *GetNameSafe(this),
        *GetNameSafe(CurrentPatient));
    DetachPatientFromBed(CurrentPatient);

    if (PatientFolder)
    {
        PatientFolder->ClearAssignedPatient();
        PatientFolder->ReturnToAnchor();
    }

    CurrentPatient = nullptr;

    // Notify subsystem
    if (TreatmentSubsystem && bTreatmentBedRegisteredWithSubsystem)
    {
        TreatmentSubsystem->OnBedFreed(this);
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Bed ReleasePatient end. Bed=%s"), *GetNameSafe(this));
}

void AEMRTreatmentBed::SetTreatmentBedEnabledByUpgrade(const bool bEnabled)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bTreatmentBedEnabledByUpgrade == bEnabled)
    {
        if (bTreatmentBedEnabledByUpgrade)
        {
            RegisterWithTreatmentSubsystemIfNeeded();
        }
        else
        {
            UnregisterFromTreatmentSubsystemIfNeeded();
        }
        ApplyTreatmentBedEnabledState();
        return;
    }

    bTreatmentBedEnabledByUpgrade = bEnabled;
    if (bTreatmentBedEnabledByUpgrade)
    {
        RegisterWithTreatmentSubsystemIfNeeded();
    }
    else
    {
        UnregisterFromTreatmentSubsystemIfNeeded();
        ClearAssignedPatientWithoutFreeNotification();
    }

    ApplyTreatmentBedEnabledState();
    ForceNetUpdate();
}


FVector AEMRTreatmentBed::GetPatientWaitPointLocation() const
{
	return PatientPositionMarker->GetComponentLocation();
}


FRotator AEMRTreatmentBed::GetPatientWaitPointRotation() const
{
	return PatientPositionMarker->GetComponentRotation();
}


void AEMRTreatmentBed::UpdateScreenDisplay()
{
	if (!TreatmentScreenComponent || !PatientUIController) return;

	UDEPRECATEDEMRTreatmentScreenWidget* ScreenWidget = Cast<UDEPRECATEDEMRTreatmentScreenWidget>(TreatmentScreenComponent->GetWidget());

	if (!ScreenWidget && TreatmentScreenWidgetClass)
	{
		TreatmentScreenComponent->SetWidgetClass(TreatmentScreenWidgetClass);
		ScreenWidget = Cast<UDEPRECATEDEMRTreatmentScreenWidget>(TreatmentScreenComponent->GetWidget());
	}

	if (ScreenWidget)
	{
		ScreenWidget->BindToPatient(PatientUIController);
	}
}


void AEMRTreatmentBed::OnRep_CurrentPatient()
{
	if (CurrentPatient)
	{
		if (!PatientUIController)
		{
			PatientUIController = NewObject<UEMRPatientUIController>(this);
		}

		PatientUIController->BindToPatient(CurrentPatient);
		PatientUIController->StartMonitoring();

		UpdateScreenDisplay();
	}
}

void AEMRTreatmentBed::OnRep_AttachedPatient()
{
    HandleAttachedPatientChanged();
}

void AEMRTreatmentBed::OnRep_TreatmentBedEnabledByUpgrade()
{
    ApplyTreatmentBedEnabledState();
}

void AEMRTreatmentBed::HandleAttachTriggerOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority() || !CurrentPatient)
    {
        return;
    }

    if (OtherActor == CurrentPatient)
    {
        AttachPatientToBed(CurrentPatient);
    }
}

void AEMRTreatmentBed::AttachPatientToBed(AEMRPatient* Patient)
{
    if (!HasAuthority() || !Patient || AttachedPatient == Patient)
    {
        return;
    }

    USceneComponent* AttachComponent = PatientAttachPoint ? PatientAttachPoint : GetRootComponent();
    if (!AttachComponent)
    {
        return;
    }

    if (!bHasCachedPatientTransform || CachedPatientTransformOwner.Get() != Patient)
    {
        CachedPatientTransformOwner = Patient;
        CachedPatientTransform = Patient->GetActorTransform();
        bHasCachedPatientTransform = true;
    }

    const FTransform AttachTransform = AttachComponent->GetComponentTransform();
    Patient->SetActorLocationAndRotation(
        AttachTransform.GetLocation(),
        AttachTransform.GetRotation().Rotator(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
    Patient->AttachToComponent(AttachComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

    if (AController* Controller = Patient->GetController())
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

    if (UCharacterMovementComponent* MovementComponent = Patient->GetCharacterMovement())
    {
        MovementComponent->DisableMovement();
    }

    AttachedPatient = Patient;
    ForceNetUpdate();

    Multicast_PlayPatientLayMontage(Patient);
}

void AEMRTreatmentBed::DetachPatientFromBed(AEMRPatient* Patient)
{
    if (!HasAuthority() || !Patient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRPatientLeave] Bed DetachPatient ignored. Bed=%s Patient=%s HasAuthority=%s"),
            *GetNameSafe(this),
            *GetNameSafe(Patient),
            HasAuthority() ? TEXT("true") : TEXT("false"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Bed DetachPatient start. Bed=%s Patient=%s"),
        *GetNameSafe(this),
        *GetNameSafe(Patient));
    Patient->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    if (bHasCachedPatientTransform && CachedPatientTransformOwner.Get() == Patient)
    {
        Patient->SetActorLocationAndRotation(
            CachedPatientTransform.GetLocation(),
            CachedPatientTransform.GetRotation().Rotator(),
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Bed DetachPatient restored transform. Bed=%s Patient=%s Location=%s Rotation=%s"),
            *GetNameSafe(this),
            *GetNameSafe(Patient),
            *CachedPatientTransform.GetLocation().ToCompactString(),
            *CachedPatientTransform.GetRotation().Rotator().ToCompactString());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Bed DetachPatient no cached transform. Bed=%s Patient=%s"),
            *GetNameSafe(this),
            *GetNameSafe(Patient));
    }

    CachedPatientTransformOwner.Reset();
    bHasCachedPatientTransform = false;

    Multicast_StopPatientLayMontage(Patient);

    if (AttachedPatient == Patient)
    {
        AttachedPatient = nullptr;
        ForceNetUpdate();
    }

    if (UCharacterMovementComponent* MovementComponent = Patient->GetCharacterMovement())
    {
        MovementComponent->SetMovementMode(MOVE_Walking);
    }

    UE_LOG(LogTemp, Log, TEXT("[EMRPatientLeave] Bed DetachPatient end. Bed=%s Patient=%s"),
        *GetNameSafe(this),
        *GetNameSafe(Patient));
}

void AEMRTreatmentBed::PlayPatientLayMontageFor(AEMRPatient* Patient)
{
    if (!Patient || !PatientLayOnBedMontage || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (USkeletalMeshComponent* TargetMesh = ResolvePatientMeshFor(Patient))
    {
        if (UAnimInstance* AnimInstance = TargetMesh->GetAnimInstance())
        {
            if (!AnimInstance->Montage_IsPlaying(PatientLayOnBedMontage))
            {
                AnimInstance->Montage_Play(PatientLayOnBedMontage);
            }
        }
    }
}

void AEMRTreatmentBed::StopPatientLayMontageFor(AEMRPatient* Patient)
{
    if (!Patient || !PatientLayOnBedMontage || GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (USkeletalMeshComponent* TargetMesh = ResolvePatientMeshFor(Patient))
    {
        if (UAnimInstance* AnimInstance = TargetMesh->GetAnimInstance())
        {
            constexpr float BlendOutTime = 0.2f;
            AnimInstance->Montage_Stop(BlendOutTime, PatientLayOnBedMontage);
        }
    }
}

void AEMRTreatmentBed::HandleAttachedPatientChanged()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AttachedPatientMontageRetryTimer);
    }

    if (LastAttachedPatient.IsValid() && LastAttachedPatient != AttachedPatient)
    {
        StopPatientLayMontageFor(LastAttachedPatient.Get());
    }

    LastAttachedPatient = AttachedPatient;

    if (!AttachedPatient)
    {
        return;
    }

    if (!TryPlayAttachedPatientMontage() && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            AttachedPatientMontageRetryTimer,
            this,
            &ThisClass::HandleAttachedPatientChanged,
            0.1f,
            false);
    }
}

bool AEMRTreatmentBed::TryPlayAttachedPatientMontage()
{
    if (!AttachedPatient || !PatientLayOnBedMontage || GetNetMode() == NM_DedicatedServer)
    {
        return true;
    }

    USkeletalMeshComponent* TargetMesh = ResolvePatientMeshFor(AttachedPatient);
    if (!TargetMesh)
    {
        return false;
    }

    UAnimInstance* AnimInstance = TargetMesh->GetAnimInstance();
    if (!AnimInstance)
    {
        return false;
    }

    if (!AnimInstance->Montage_IsPlaying(PatientLayOnBedMontage))
    {
        AnimInstance->Montage_Play(PatientLayOnBedMontage);
    }

    return true;
}

USkeletalMeshComponent* AEMRTreatmentBed::ResolvePatientMeshFor(const AEMRPatient* Patient) const
{
    if (!Patient)
    {
        return nullptr;
    }

    if (CachedPatientMeshOwner.Get() == Patient && CachedPatientMesh.IsValid())
    {
        if (CachedPatientMesh->GetOwner() == Patient)
        {
            return CachedPatientMesh.Get();
        }

        CachedPatientMesh.Reset();
        CachedPatientMeshOwner.Reset();
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
                CachedPatientMeshOwner = const_cast<AEMRPatient*>(Patient);
                CachedPatientMesh = TaggedMesh;
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
                CachedPatientMeshOwner = const_cast<AEMRPatient*>(Patient);
                CachedPatientMesh = MeshComponent;
                return MeshComponent;
            }
        }
    }

    if (USkeletalMeshComponent* DefaultMesh = Patient->GetMesh())
    {
        CachedPatientMeshOwner = const_cast<AEMRPatient*>(Patient);
        CachedPatientMesh = DefaultMesh;
        return DefaultMesh;
    }

    return nullptr;
}

void AEMRTreatmentBed::Multicast_PlayPatientLayMontage_Implementation(AEMRPatient* Patient)
{
    PlayPatientLayMontageFor(Patient);
}

void AEMRTreatmentBed::Multicast_StopPatientLayMontage_Implementation(AEMRPatient* Patient)
{
    StopPatientLayMontageFor(Patient);
}

void AEMRTreatmentBed::ApplyTreatmentBedEnabledState()
{
    SetActorHiddenInGame(!bTreatmentBedEnabledByUpgrade);
    SetActorEnableCollision(bTreatmentBedEnabledByUpgrade);

    if (BedMesh)
    {
        BedMesh->SetCollisionEnabled(bTreatmentBedEnabledByUpgrade ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }

    if (PatientAttachTrigger)
    {
        PatientAttachTrigger->SetCollisionEnabled(bTreatmentBedEnabledByUpgrade ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    }

    if (TreatmentScreenComponent)
    {
        TreatmentScreenComponent->SetVisibility(bTreatmentBedEnabledByUpgrade, true);
    }

    if (PatientFolder)
    {
        PatientFolder->SetActorHiddenInGame(!bTreatmentBedEnabledByUpgrade);
        PatientFolder->SetActorEnableCollision(bTreatmentBedEnabledByUpgrade);
    }
}

void AEMRTreatmentBed::RegisterWithTreatmentSubsystemIfNeeded()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!TreatmentSubsystem)
    {
        TreatmentSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>() : nullptr;
    }

    if (TreatmentSubsystem && !bTreatmentBedRegisteredWithSubsystem)
    {
        TreatmentSubsystem->RegisterTreatmentBed(this);
        bTreatmentBedRegisteredWithSubsystem = true;
    }
}

void AEMRTreatmentBed::UnregisterFromTreatmentSubsystemIfNeeded()
{
    if (!HasAuthority())
    {
        return;
    }

    if (TreatmentSubsystem && bTreatmentBedRegisteredWithSubsystem)
    {
        TreatmentSubsystem->UnregisterTreatmentBed(this);
        bTreatmentBedRegisteredWithSubsystem = false;
    }
}

void AEMRTreatmentBed::ClearAssignedPatientWithoutFreeNotification()
{
    if (!HasAuthority() || !CurrentPatient)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[TreatmentBedUpgrade] Clearing patient without free notification. Bed=%s Patient=%s"),
        *GetNameSafe(this),
        *GetNameSafe(CurrentPatient));

    DetachPatientFromBed(CurrentPatient);

    if (PatientFolder)
    {
        PatientFolder->ClearAssignedPatient();
        PatientFolder->ReturnToAnchor();
    }

    CurrentPatient = nullptr;
}
