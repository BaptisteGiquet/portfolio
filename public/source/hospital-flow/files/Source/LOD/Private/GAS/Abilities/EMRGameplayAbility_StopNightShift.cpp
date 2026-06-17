#include "GAS/Abilities/EMRGameplayAbility_StopNightShift.h"


#include "Framework/EMRNightShiftGameState.h"
#include "GAS/EMRTags.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/World.h"
#include "Framework/EMRNightShiftGameMode.h"
#include "Interaction/EMROvertimeStopTerminal.h"


UEMRGameplayAbility_StopNightShift::UEMRGameplayAbility_StopNightShift()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Machine::OvertimeStopTerminal;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    FAbilityTriggerData LegacyTriggerData;
    LegacyTriggerData.TriggerTag = EMRTags::Abilities::StopNightShift;
    LegacyTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(LegacyTriggerData);
}

void UEMRGameplayAbility_StopNightShift::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!HasAuthority(&ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (AEMRNightShiftGameMode* NightShiftGameMode = GetNightShiftGameMode())
    {
        if (IsNightShiftInOvertime(*NightShiftGameMode))
        {
            const AActor* TargetActor = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;
            AEMROvertimeStopTerminal* Terminal = const_cast<AEMROvertimeStopTerminal*>(Cast<AEMROvertimeStopTerminal>(TargetActor));
            if (!Terminal)
            {
                UE_LOG(LogTemp, Warning, TEXT("[GA_StopNightShift] Ignored trigger without overtime terminal target"));
                EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
                return;
            }

            if (!Terminal->TryConsumeTerminalInteraction())
            {
                UE_LOG(LogTemp, Warning, TEXT("[GA_StopNightShift] Ignored duplicate or inactive overtime terminal interaction"));
                EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
                return;
            }

            NightShiftGameMode->NotifyNightShiftFinished();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[GA_StopNightShift] Ignored trigger outside overtime"));
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

AEMRNightShiftGameMode* UEMRGameplayAbility_StopNightShift::GetNightShiftGameMode() const
{
    if (const UWorld* World = GetWorld())
    {
        return World->GetAuthGameMode<AEMRNightShiftGameMode>();
    }

    return nullptr;
}

bool UEMRGameplayAbility_StopNightShift::IsNightShiftInOvertime(const AEMRNightShiftGameMode& GameMode) const
{
    if (const AEMRNightShiftGameState* NightShiftGameState = GameMode.GetGameState<AEMRNightShiftGameState>())
    {
        return NightShiftGameState->IsInNightShiftOvertime();
    }

    return false;
}
