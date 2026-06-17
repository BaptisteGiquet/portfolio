#include "GAS/Abilities/EMRGameplayAbility_PerformLabAnalyzerTests.h"

#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "EMRLabAnalyzerLog.h"
#include "GAS/EMRTags.h"
#include "Subsystems/EMRExamQueueSubsystem.h"

UEMRGameplayAbility_PerformLabAnalyzerTests::UEMRGameplayAbility_PerformLabAnalyzerTests()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    const FGameplayTag TestTags[] = {
        EMRTags::Abilities::Exam::LabAnalyzer::CBC,
        EMRTags::Abilities::Exam::LabAnalyzer::Electrolytes,
        EMRTags::Abilities::Exam::LabAnalyzer::Lactate,
        EMRTags::Abilities::Exam::LabAnalyzer::CRP,
        EMRTags::Abilities::Exam::LabAnalyzer::Ddimer,
        EMRTags::Abilities::Exam::LabAnalyzer::Toxicology,
        EMRTags::Abilities::Exam::LabAnalyzer::Troponin,
        EMRTags::Abilities::Exam::LabAnalyzer::Glucose,
        EMRTags::Abilities::Exam::LabAnalyzer::UrineDipstick
    };

    for (const FGameplayTag& Tag : TestTags)
    {
        FAbilityTriggerData TriggerData;
        TriggerData.TriggerTag = Tag;
        TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
        AbilityTriggers.Add(TriggerData);
    }
}

void UEMRGameplayAbility_PerformLabAnalyzerTests::ActivateAbility(
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

    if (!HasAuthority(&ActivationInfo) || !TriggerEventData)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    const FGameplayTag ExamTag = TriggerEventData->EventTag;
    if (!ExamTag.IsValid() || !ExamTag.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root))
    {
        EMR_LAB_LOG(Log, "PerformLabTests rejected: invalid tag %s", *ExamTag.ToString());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRPatient* Patient = const_cast<AEMRPatient*>(Cast<AEMRPatient>(TriggerEventData->Target));
    if (!Patient)
    {
        EMR_LAB_LOG(Log, "PerformLabTests rejected: no patient");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    EMR_LAB_LOG(Log, "PerformLabTests complete exam=%s patient=%s", *ExamTag.ToString(), *GetNameSafe(Patient));
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UEMRExamQueueSubsystem* ExamQueue = GameInstance->GetSubsystem<UEMRExamQueueSubsystem>())
            {
                ExamQueue->CompleteExamForPatient(Patient, ExamTag, true);
            }
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
