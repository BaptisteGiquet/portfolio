#include "GAS/Abilities/EMRGameplayAbility_UseLabAnalyzerTube.h"

#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "EMRLabAnalyzerLog.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRLabAnalyzerMachine.h"
#include "Interaction/EMRLabAnalyzerTrash.h"
#include "Interaction/EMRLabAnalyzerTube.h"
#include "EngineUtils.h"

UEMRGameplayAbility_UseLabAnalyzerTube::UEMRGameplayAbility_UseLabAnalyzerTube()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Event::LabAnalyzer::UseTube;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_UseLabAnalyzerTube::ActivateAbility(
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

    AEMRPlayerCharacter* Player = ActorInfo ? Cast<AEMRPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    AEMRLabAnalyzerTube* Tube = const_cast<AEMRLabAnalyzerTube*>(Cast<AEMRLabAnalyzerTube>(TriggerEventData->OptionalObject));
    if (!Tube)
    {
        Tube = const_cast<AEMRLabAnalyzerTube*>(Cast<AEMRLabAnalyzerTube>(TriggerEventData->Target));
    }

    if (!Tube)
    {
        EMR_LAB_LOG(Log, "UseTube ability: no tube found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRLabAnalyzerMachine* OperatorMachine = nullptr;
    auto IsLabAnalyzerOperator = [&OperatorMachine](AEMRPlayerCharacter* InPlayer) -> bool
    {
        OperatorMachine = nullptr;
        if (!InPlayer)
        {
            return false;
        }

        UWorld* World = InPlayer->GetWorld();
        if (!World)
        {
            return false;
        }

        for (TActorIterator<AEMRLabAnalyzerMachine> It(World); It; ++It)
        {
            if (It->GetCurrentOperator() == InPlayer)
            {
                OperatorMachine = *It;
                return true;
            }
        }

        return false;
    };

    if (!IsLabAnalyzerOperator(Player))
    {
        EMR_LAB_LOG(Log, "UseTube rejected: player not seated at LabAnalyzer");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AActor* TargetActor = const_cast<AActor*>(Cast<AActor>(TriggerEventData->Target));
    if (AEMRLabAnalyzerMachine* Machine = Cast<AEMRLabAnalyzerMachine>(TargetActor))
    {
        if (Machine != OperatorMachine)
        {
            EMR_LAB_LOG(Log, "UseTube rejected: machine mismatch operator=%s target=%s",
                *GetNameSafe(OperatorMachine), *GetNameSafe(Machine));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        const bool bInserted = Machine->TryInsertTube(Tube);
        EMR_LAB_LOG(Log, "UseTube -> Machine insert=%s tube=%s machine=%s",
            bInserted ? TEXT("true") : TEXT("false"),
            *GetNameSafe(Tube),
            *GetNameSafe(Machine));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (Cast<AEMRLabAnalyzerTrash>(TargetActor))
    {
        if (Tube->IsBalanceTube())
        {
            if (OperatorMachine)
            {
                OperatorMachine->PlayTrashAttemptCue(Player, true);
            }
            EMR_LAB_LOG(Log, "UseTube -> Trash rejected balance tube=%s", *GetNameSafe(Tube));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        if (!Tube->CanBeTrashed())
        {
            EMR_LAB_LOG(Log, "UseTube -> Trash rejected tube=%s", *GetNameSafe(Tube));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        AEMRPatient* Patient = Tube->GetOwningPatient();
        if (!Patient || Tube->IsAbandoned())
        {
            EMR_LAB_LOG(Log, "UseTube -> Trash abandoned or no patient tube=%s", *GetNameSafe(Tube));
        }

        if (OperatorMachine)
        {
            OperatorMachine->PlayTrashAttemptCue(Player, false);
        }

        Tube->Destroy();
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    EMR_LAB_LOG(Log, "UseTube target not handled tube=%s target=%s", *GetNameSafe(Tube), *GetNameSafe(TargetActor));
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
