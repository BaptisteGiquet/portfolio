#include "GAS/Abilities/EMRGameplayAbility_UseOralTreatment.h"

#include "GAS/EMRTags.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Subsystems/EMRTreatmentSubsystem.h"
#include "Interfaces/EMRInteractableInterface.h"
#include "LOD/EMRCollisionChannels.h"
#include "Characters/Player/EMRPlayerController.h"
#include "GameFramework/Actor.h"


UEMRGameplayAbility_UseOralTreatment::UEMRGameplayAbility_UseOralTreatment()
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = EMRTags::Abilities::Treatment::OralMedication;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}



void UEMRGameplayAbility_UseOralTreatment::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	AEMRPatient* Patient = TriggerEventData
	? const_cast<AEMRPatient*>(Cast<AEMRPatient>(TriggerEventData->Target))
	: nullptr;

	if (!Patient)
	{
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
		Patient = const_cast<AEMRPatient*>(Cast<AEMRPatient>(InteractionData.Target));
	}

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
		TryConsumeTriggerItemForWaitingPatient(TriggerEventData, Patient, ActivationInfo);

		const FGameplayTag IncomingTag = TriggerEventData ? TriggerEventData->EventTag : FGameplayTag();
		if (PatientTreatmentsNeeded.HasTagExact(IncomingTag))
		{
			TreatmentSubsystem->CompleteTreatmentForPatient(Patient, IncomingTag, true);
			TryPlayTriggerItemUseSoundForInstigator(TriggerEventData, ActivationInfo);
		}
	}
	
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

	

FHitResult UEMRGameplayAbility_UseOralTreatment::GetHitResultFromLineTrace(APlayerController* InPlayerController) const
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
