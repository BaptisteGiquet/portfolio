#include "GAS/Abilities/EMRGameplayAbility_UseOxygenTreatment.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMROxygenMachine.h"
#include "Interaction/EMROxygenMask.h"

#define OXY_LOG(Format, ...) UE_LOG(LogTemp, Warning, TEXT("[OxygenMachine] " Format), ##__VA_ARGS__)

UEMRGameplayAbility_UseOxygenTreatment::UEMRGameplayAbility_UseOxygenTreatment()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Abilities::Treatment::Oxygen;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UEMRGameplayAbility_UseOxygenTreatment::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        OXY_LOG("UseOxygenTreatment: CommitAbility failed.");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
    {
        OXY_LOG("UseOxygenTreatment: Missing authority/prediction key.");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!TriggerEventData)
    {
        OXY_LOG("UseOxygenTreatment: Missing TriggerEventData.");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRPlayerCharacter* PlayerCharacter = ActorInfo ? Cast<AEMRPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    AEMROxygenMask* Mask = const_cast<AEMROxygenMask*>(Cast<AEMROxygenMask>(TriggerEventData->OptionalObject));
    if (!Mask)
    {
        Mask = const_cast<AEMROxygenMask*>(Cast<AEMROxygenMask>(TriggerEventData->Target));
    }

    AEMRPatient* Patient = const_cast<AEMRPatient*>(Cast<AEMRPatient>(TriggerEventData->Target));
    if (!PlayerCharacter || !Mask || !Patient)
    {
        OXY_LOG("UseOxygenTreatment: Missing actors. Player=%s Mask=%s Patient=%s.",
            *GetNameSafe(PlayerCharacter),
            *GetNameSafe(Mask),
            *GetNameSafe(Patient));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!Mask->IsCarriedBy(PlayerCharacter))
    {
        OXY_LOG("UseOxygenTreatment: Mask not carried by player. Player=%s Mask=%s AttachParent=%s.",
            *GetNameSafe(PlayerCharacter),
            *GetNameSafe(Mask),
            *GetNameSafe(Mask->GetAttachParentActor()));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    FHitResult HitResult;
    if (TriggerEventData->TargetData.Num() > 0)
    {
        const FGameplayAbilityTargetData* TargetData = TriggerEventData->TargetData.Get(0);
        if (TargetData && TargetData->GetScriptStruct() == FGameplayAbilityTargetData_SingleTargetHit::StaticStruct())
        {
            const FGameplayAbilityTargetData_SingleTargetHit* HitData =
                static_cast<const FGameplayAbilityTargetData_SingleTargetHit*>(TargetData);
            HitResult = HitData->HitResult;
        }
    }

    if (!HitResult.bBlockingHit)
    {
        OXY_LOG("UseOxygenTreatment: No blocking hit in TargetData. Player=%s Mask=%s Patient=%s.",
            *GetNameSafe(PlayerCharacter),
            *GetNameSafe(Mask),
            *GetNameSafe(Patient));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMROxygenMachine* Machine = Mask->OwningMachine;
    if (!Machine)
    {
        OXY_LOG("UseOxygenTreatment: Mask has no OwningMachine. Mask=%s.", *GetNameSafe(Mask));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (HasAuthority(&ActivationInfo))
    {
        OXY_LOG("UseOxygenTreatment: Calling TryAttachMaskToPatient. Player=%s Patient=%s Machine=%s.",
            *GetNameSafe(PlayerCharacter),
            *GetNameSafe(Patient),
            *GetNameSafe(Machine));
        Machine->TryAttachMaskToPatient(PlayerCharacter, Patient, HitResult);
    }
    else
    {
        OXY_LOG("UseOxygenTreatment: Skipping TryAttachMaskToPatient (no authority). Player=%s Patient=%s Machine=%s.",
            *GetNameSafe(PlayerCharacter),
            *GetNameSafe(Patient),
            *GetNameSafe(Machine));
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

#undef OXY_LOG
