#include "GAS/Abilities/EMRGameplayAbility_UseSplintTreatment.h"

#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Characters/Player/EMRPlayerController.h"
#include "GAS/EMRTags.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "LOD/EMRCollisionChannels.h"
#include "Subsystems/EMRTreatmentSubsystem.h"
#include "Interaction/EMRCarryableComponent.h"
#include "Shop/EMRItemActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Shop/EMRItemData.h"


UEMRGameplayAbility_UseSplintTreatment::UEMRGameplayAbility_UseSplintTreatment()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = EMRTags::Abilities::Treatment::Splint;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}


void UEMRGameplayAbility_UseSplintTreatment::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!TriggerEventData)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRItemActor* SplintItem = const_cast<AEMRItemActor*>(Cast<AEMRItemActor>(TriggerEventData->OptionalObject));
    if (!SplintItem)
    {
        SplintItem = const_cast<AEMRItemActor*>(Cast<AEMRItemActor>(TriggerEventData->Target));
    }

    if (!SplintItem)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRPlayerController* PlayerController = Cast<AEMRPlayerController>(GetAvatarActorFromActorInfo()->GetInstigatorController());
    if (!PlayerController)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    FHitResult HitResult = GetHitResultFromLineTrace(PlayerController);
    if (HitResult.bBlockingHit == false)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AActor* InteractableActor = HitResult.GetActor();
    if (!InteractableActor)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    IEMRInteractableInterface* Interactable = Cast<IEMRInteractableInterface>(InteractableActor);
    if (!Interactable)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    FGameplayEventData InteractionData = Interactable->Execute_GetInteractionEventData(InteractableActor, GetAvatarActorFromActorInfo());

    AEMRPatient* Patient = const_cast<AEMRPatient*>(Cast<AEMRPatient>(InteractionData.Target));
    if (!Patient)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!Patient->CanReceiveTreatment())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    const UEMRPatientData* PatientData = Patient->GetPatientData();
    if (!PatientData)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!GetWorld() || !GetWorld()->GetGameInstance())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    UEMRTreatmentSubsystem* TreatmentSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>();
    if (!TreatmentSubsystem)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    FGameplayTagContainer PatientTreatmentsNeeded = TreatmentSubsystem->GetTreatmentsFromPatient(Patient);
    if (!PatientTreatmentsNeeded.IsValid())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (HasAuthority(&ActivationInfo))
    {
        if (!AttachSplintToPatient(*SplintItem, *Patient))
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        TryConsumeTriggerItemForWaitingPatient(TriggerEventData, Patient, ActivationInfo);

        if (PatientTreatmentsNeeded.HasTagExact(TriggerEventData->EventTag))
        {
            TreatmentSubsystem->CompleteTreatmentForPatient(Patient, TriggerEventData->EventTag, true);
            TryPlayTriggerItemUseSoundForInstigator(TriggerEventData, ActivationInfo);
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


FHitResult UEMRGameplayAbility_UseSplintTreatment::GetHitResultFromLineTrace(APlayerController* InPlayerController) const
{
    APlayerController* PC = Cast<APlayerController>(InPlayerController);
    if (!PC) return FHitResult();

    FVector PlayerViewLocation;
    FRotator PlayerViewRotation;
    PC->GetPlayerViewPoint(PlayerViewLocation, PlayerViewRotation);

    const float TraceDistance = 10000.f;
    const FVector TraceStart = PlayerViewLocation;
    const FVector TraceEnd = TraceStart + (PlayerViewRotation.Vector() * TraceDistance);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetAvatarActorFromActorInfo());

    FHitResult HitResult;
    GetWorld()->LineTraceSingleByChannel(HitResult,TraceStart,TraceEnd, ECC_Patient, QueryParams);

    if (bDrawDebug)
    {
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 2.f, 0, 2.f);
    }

    return HitResult;
}


USkeletalMeshComponent* UEMRGameplayAbility_UseSplintTreatment::FindPatientMeshForSocket(AEMRPatient& Patient) const
{
    if (USkeletalMeshComponent* PatientMesh = Patient.GetMesh())
    {
        if (SplintSocketName.IsNone() || PatientMesh->DoesSocketExist(SplintSocketName))
        {
            return PatientMesh;
        }
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshes;
    Patient.GetComponents(SkeletalMeshes);

    for (USkeletalMeshComponent* MeshComponent : SkeletalMeshes)
    {
        if (MeshComponent && (SplintSocketName.IsNone() || MeshComponent->DoesSocketExist(SplintSocketName)))
        {
            return MeshComponent;
        }
    }

    return nullptr;
}


bool UEMRGameplayAbility_UseSplintTreatment::AttachSplintToPatient(AEMRItemActor& SplintActor, AEMRPatient& Patient) const
{
    if (SplintSocketName.IsNone())
    {
        return false;
    }
	
	USkeletalMeshComponent* PatientMesh = FindPatientMeshForSocket(Patient);
	if (!PatientMesh)
	{
		return false;
	}
	
	const UEMRItemData* SplintItemData = SplintActor.GetItemData();
	if (!SplintItemData)
	{
		return false;
	}

	UStaticMesh* SplintMesh = SplintItemData->GetWorldItemMesh();
	if (!SplintMesh)
	{
		return false;
	}

    UStaticMeshComponent* SplintMeshComponent = NewObject<UStaticMeshComponent>(&Patient);
    if (!SplintMeshComponent)
    {
        return false;
    }

    SplintMeshComponent->SetStaticMesh(SplintMesh);
    SplintMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SplintMeshComponent->SetGenerateOverlapEvents(false);
    SplintMeshComponent->SetIsReplicated(true);
    SplintMeshComponent->ComponentTags.Add(EMRTags::Component::TreatmentAttachment.GetModuleName());
    SplintMeshComponent->AttachToComponent(PatientMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SplintSocketName);
    SplintMeshComponent->RegisterComponent();

    return true;
}
