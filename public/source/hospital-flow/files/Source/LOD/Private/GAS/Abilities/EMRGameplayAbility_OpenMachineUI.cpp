#include "GAS/Abilities/EMRGameplayAbility_OpenMachineUI.h"

#include "GAS/EMRTags.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerController.h"


UEMRGameplayAbility_OpenMachineUI::UEMRGameplayAbility_OpenMachineUI()
{
    InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Machine::Any;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}


void UEMRGameplayAbility_OpenMachineUI::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    const AActor* TargetActor = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;
    AEMRPatient* PatientActor = const_cast<AEMRPatient*>(Cast<AEMRPatient>(TargetActor));
    if (!PatientActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[OpenMachineUI] No patient in event data"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
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

    const FGameplayTag MachineType = TriggerEventData ? TriggerEventData->EventTag : FGameplayTag();
    if (!MachineType.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[OpenMachineUI] Invalid machine type tag"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (ActorInfo && ActorInfo->IsLocallyControlled())
    {
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
