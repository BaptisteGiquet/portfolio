#include "GAS/Abilities/EMRGameplayAbility_PerformCTScan.h"

#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerController.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRExamQueueSubsystem.h"

UEMRGameplayAbility_PerformCTScan::UEMRGameplayAbility_PerformCTScan()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Abilities::Exam::CTScan;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    CTScanExamTag = EMRTags::Abilities::Exam::CTScan;
}

void UEMRGameplayAbility_PerformCTScan::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (HasAuthority(&ActivationInfo))
    {
        AEMRPatient* Patient = TriggerEventData ? const_cast<AEMRPatient*>(Cast<AEMRPatient>(TriggerEventData->Target)) : nullptr;
        if (!Patient)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent();
        if (!PatientASC)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
        if (!SourceASC)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        if (UWorld* World = GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
            {
                if (UEMRExamQueueSubsystem* ExamQueue = GameInstance->GetSubsystem<UEMRExamQueueSubsystem>())
                {
                    ExamQueue->CompleteExamForPatient(Patient, CTScanExamTag, true);
                }
            }
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UEMRGameplayAbility_PerformCTScan::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
