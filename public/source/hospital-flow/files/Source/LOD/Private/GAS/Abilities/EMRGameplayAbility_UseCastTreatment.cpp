#include "GAS/Abilities/EMRGameplayAbility_UseCastTreatment.h"

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
#include "Components/StaticMeshComponent.h"
#include "Shop/EMRItemData.h"


UEMRGameplayAbility_UseCastTreatment::UEMRGameplayAbility_UseCastTreatment()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = EMRTags::Abilities::Treatment::Cast;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}


void UEMRGameplayAbility_UseCastTreatment::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

    AEMRItemActor* CastItem = const_cast<AEMRItemActor*>(Cast<AEMRItemActor>(TriggerEventData->OptionalObject));
    if (!CastItem)
    {
        CastItem = const_cast<AEMRItemActor*>(Cast<AEMRItemActor>(TriggerEventData->Target));
    }

    if (!CastItem)
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
        if (!AttachCastToPatient(*CastItem, *Patient))
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


FHitResult UEMRGameplayAbility_UseCastTreatment::GetHitResultFromLineTrace(APlayerController* InPlayerController) const
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


USkeletalMeshComponent* UEMRGameplayAbility_UseCastTreatment::FindPatientMeshForSocket(AEMRPatient& Patient) const
{
    if (USkeletalMeshComponent* PatientMesh = Patient.GetMesh())
    {
        if (CastSocketName.IsNone() || PatientMesh->DoesSocketExist(CastSocketName))
        {
            return PatientMesh;
        }
    }

    TArray<USkeletalMeshComponent*> SkeletalMeshes;
    Patient.GetComponents(SkeletalMeshes);

    for (USkeletalMeshComponent* MeshComponent : SkeletalMeshes)
    {
        if (MeshComponent && (CastSocketName.IsNone() || MeshComponent->DoesSocketExist(CastSocketName)))
        {
            return MeshComponent;
        }
    }

    return nullptr;
}


bool UEMRGameplayAbility_UseCastTreatment::AttachCastToPatient(AEMRItemActor& CastActor, AEMRPatient& Patient) const
{
	if (CastSocketName.IsNone())
	{
		return false;
	}

	USkeletalMeshComponent* PatientMesh = FindPatientMeshForSocket(Patient);
	if (!PatientMesh)
	{
		return false;
	}

	const UEMRItemData* CastItemData = CastActor.GetItemData();
	if (!CastItemData)
	{
		return false;
	}

	UStaticMesh* CastMesh = CastItemData->GetWorldItemMesh();
	if (!CastMesh)
	{
		return false;
	}

    UStaticMeshComponent* CastMeshComponent = NewObject<UStaticMeshComponent>(&Patient);
    if (!CastMeshComponent)
    {
        return false;
    }

    CastMeshComponent->SetStaticMesh(CastMesh);
    CastMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CastMeshComponent->SetGenerateOverlapEvents(false);
    CastMeshComponent->SetIsReplicated(true);
    CastMeshComponent->ComponentTags.Add(EMRTags::Component::TreatmentAttachment.GetModuleName());
    CastMeshComponent->AttachToComponent(PatientMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, CastSocketName);
    CastMeshComponent->RegisterComponent();

    return true;
}
