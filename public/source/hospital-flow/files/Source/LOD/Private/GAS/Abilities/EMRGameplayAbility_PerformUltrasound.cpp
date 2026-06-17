#include "GAS/Abilities/EMRGameplayAbility_PerformUltrasound.h"

#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRExamQueueSubsystem.h"

UEMRGameplayAbility_PerformUltrasound::UEMRGameplayAbility_PerformUltrasound()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Abilities::Exam::Ultrasound;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    UltrasoundExamTag = EMRTags::Abilities::Exam::Ultrasound;
}

void UEMRGameplayAbility_PerformUltrasound::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
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
        AEMRPatient* Patient = const_cast<AEMRPatient*>(Cast<AEMRPatient>(TriggerEventData ? TriggerEventData->Target : nullptr));
        if (!Patient)
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
                    ExamQueue->CompleteExamForPatient(Patient, UltrasoundExamTag, true);
                }
            }
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
