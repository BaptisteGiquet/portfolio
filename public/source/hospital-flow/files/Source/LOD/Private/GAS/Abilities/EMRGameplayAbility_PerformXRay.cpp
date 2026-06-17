
#include "GAS/Abilities/EMRGameplayAbility_PerformXRay.h"

#include "GAS/EMRTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Subsystems/EMRExamQueueSubsystem.h"
#include "Characters/Player/EMRPlayerController.h"


UEMRGameplayAbility_PerformXRay::UEMRGameplayAbility_PerformXRay()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = EMRTags::Abilities::Exam::XRay;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
    
    XRayExamTag = EMRTags::Abilities::Exam::XRay;
}

void UEMRGameplayAbility_PerformXRay::LogAbilityTriggers(const TCHAR* Context) const
{
	const TCHAR* ContextLabel = Context ? Context : TEXT("Unknown");
	UE_LOG(LogTemp, Log, TEXT("[XRayFlow] AbilityTriggers %s class=%s count=%d netexec=%d instancing=%d replication=%d"),
		ContextLabel,
		*GetNameSafe(GetClass()),
		AbilityTriggers.Num(),
		static_cast<int32>(GetNetExecutionPolicy()),
		static_cast<int32>(GetInstancingPolicy()),
		static_cast<int32>(GetReplicationPolicy()));
	for (int32 TriggerIndex = 0; TriggerIndex < AbilityTriggers.Num(); ++TriggerIndex)
	{
		const FAbilityTriggerData& TriggerData = AbilityTriggers[TriggerIndex];
		UE_LOG(LogTemp, Log, TEXT("[XRayFlow] AbilityTriggers %s [%d] tag=%s source=%d"),
			ContextLabel,
			TriggerIndex,
			*TriggerData.TriggerTag.ToString(),
			static_cast<int32>(TriggerData.TriggerSource));
	}
}


void UEMRGameplayAbility_PerformXRay::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Ability PerformXRay Activate owner=%s avatar=%s authority=%s"),
		ActorInfo ? *GetNameSafe(ActorInfo->OwnerActor.Get()) : TEXT("nullptr"),
		ActorInfo ? *GetNameSafe(ActorInfo->AvatarActor.Get()) : TEXT("nullptr"),
		HasAuthority(&ActivationInfo) ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Log, TEXT("[XRayFlow] Ability PerformXRay NetPolicy netexec=%d locallyControlled=%s"),
		static_cast<int32>(GetNetExecutionPolicy()),
		IsLocallyControlled() ? TEXT("true") : TEXT("false"));
	
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
	
	if (HasAuthority(&ActivationInfo))
    {
	    AEMRPatient* Patient = const_cast<AEMRPatient*>(Cast<AEMRPatient>(TriggerEventData->Target));
	    if (!Patient)
	    {
	        UE_LOG(LogTemp, Error, TEXT("[EMRGameplayAbility_PerformXRay] No patient found"));
	        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	        return;
	    }

	    UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent();
	    if (!PatientASC)
	    {
	        UE_LOG(LogTemp, Error, TEXT("[EMRGameplayAbility_PerformXRay] Patient has no ASC"));
	        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	        return;
	    }

		UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (!SourceASC)
		{
			UE_LOG(LogTemp, Error, TEXT("[EMRGameplayAbility_PerformCTScan] Source ASC missing"));
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
		
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
                if (UEMRExamQueueSubsystem* ExamQueue = GameInstance->GetSubsystem<UEMRExamQueueSubsystem>())
                {
                    ExamQueue->CompleteExamForPatient(Patient, XRayExamTag, true);
                }
            }
        }
    }


	AEMRPlayerController* PlayerController = Cast<AEMRPlayerController>(GetAvatarActorFromActorInfo()->GetInstigatorController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_Interact] No PlayerController (Owner=%s Avatar=%s)"),
			ActorInfo ? *GetNameSafe(ActorInfo->OwnerActor.Get()) : TEXT("nullptr"),
			ActorInfo ? *GetNameSafe(ActorInfo->AvatarActor.Get()) : TEXT("nullptr"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


void UEMRGameplayAbility_PerformXRay::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (bWasCancelled)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EMRGameplayAbility_PerformXRay] Ability cancelled"));
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}