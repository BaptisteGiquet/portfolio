#include "GAS/Abilities/EMRGameplayAbility_UseCoffeePitcher.h"

#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerCharacter.h"
#include "GAS/Attributes/EMRPatientAttributeSet.h"
#include "GAS/EMRTags.h"
#include "Interaction/EMRCoffeeMachineActor.h"
#include "Interaction/EMRCoffeePitcherItemActor.h"
#include "Sound/SoundBase.h"

namespace
{
    constexpr TCHAR UseCoffeePitcherUpgradesFlowLogPrefix[] = TEXT("[UpgradesFlow]");
}

UEMRGameplayAbility_UseCoffeePitcher::UEMRGameplayAbility_UseCoffeePitcher()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = EMRTags::Abilities::UseCoffeePitcher;
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    FAbilityTriggerData CoffeeMachineInteractTrigger;
    CoffeeMachineInteractTrigger.TriggerTag = EMRTags::Machine::CoffeeMachine;
    CoffeeMachineInteractTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(CoffeeMachineInteractTrigger);
}

void UEMRGameplayAbility_UseCoffeePitcher::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("%s UseCoffeePitcher ActivateAbility instigator=%s target=%s optional=%s"),
        UseCoffeePitcherUpgradesFlowLogPrefix,
        *GetNameSafe(TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr),
        *GetNameSafe(TriggerEventData ? TriggerEventData->Target.Get() : nullptr),
        *GetNameSafe(TriggerEventData ? TriggerEventData->OptionalObject.Get() : nullptr));

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s UseCoffeePitcher commit failed."), UseCoffeePitcherUpgradesFlowLogPrefix);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s UseCoffeePitcher rejected: no authority/prediction key."), UseCoffeePitcherUpgradesFlowLogPrefix);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AEMRPlayerCharacter* InstigatorCharacter = TriggerEventData
    ? const_cast<AEMRPlayerCharacter*>(Cast<const AEMRPlayerCharacter>(TriggerEventData->Instigator))
    : nullptr;
    if (!InstigatorCharacter)
    {
        InstigatorCharacter = Cast<AEMRPlayerCharacter>(GetAvatarActorFromActorInfo());
    }

    AEMRCoffeePitcherItemActor* Pitcher = TriggerEventData
    ? const_cast<AEMRCoffeePitcherItemActor*>(Cast<const AEMRCoffeePitcherItemActor>(TriggerEventData->OptionalObject))
    : nullptr;
    if (!Pitcher && TriggerEventData)
    {
        Pitcher = const_cast<AEMRCoffeePitcherItemActor*>(Cast<const AEMRCoffeePitcherItemActor>(TriggerEventData->Target));
    }

    const AActor* RawTargetActor = TriggerEventData ? Cast<const AActor>(TriggerEventData->Target) : nullptr;
    AActor* TargetActor = const_cast<AActor*>(RawTargetActor);

    if (AEMRCoffeeMachineActor* CoffeeMachine = Cast<AEMRCoffeeMachineActor>(TargetActor))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("%s UseCoffeePitcher machine branch machine=%s pitcher=%s instigator=%s"),
            UseCoffeePitcherUpgradesFlowLogPrefix,
            *GetNameSafe(CoffeeMachine),
            *GetNameSafe(Pitcher),
            *GetNameSafe(InstigatorCharacter));

        if (!InstigatorCharacter)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s UseCoffeePitcher machine branch aborted: instigator missing."), UseCoffeePitcherUpgradesFlowLogPrefix);
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        if (Pitcher)
        {
            CoffeeMachine->TryDockPitcher(InstigatorCharacter, Pitcher);
        }
        else
        {
            CoffeeMachine->TryStartBrew(InstigatorCharacter);
        }

        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (AEMRPatient* Patient = Cast<AEMRPatient>(TargetActor))
    {
        if (!Pitcher)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s UseCoffeePitcher patient branch aborted: pitcher missing."), UseCoffeePitcherUpgradesFlowLogPrefix);
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent();
        if (!PatientASC)
        {
            UE_LOG(LogTemp, Warning, TEXT("%s UseCoffeePitcher patient branch aborted: patient ASC missing."), UseCoffeePitcherUpgradesFlowLogPrefix);
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        const float MaxPatience = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetMaxPatienceAttribute());
        const float CurrentPatience = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetPatienceAttribute());

        float PatienceRestoreAmount = 0.0f;
        if (Pitcher->ConsumeServingAndGetRestoreAmount(MaxPatience, PatienceRestoreAmount))
        {
            const float NewPatience = FMath::Clamp(CurrentPatience + PatienceRestoreAmount, 0.0f, MaxPatience);
            PatientASC->SetNumericAttributeBase(UEMRPatientAttributeSet::GetPatienceAttribute(), NewPatience);
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("%s UseCoffeePitcher patient restored patient=%s restore=%.2f old=%.2f new=%.2f max=%.2f"),
                UseCoffeePitcherUpgradesFlowLogPrefix,
                *GetNameSafe(Patient),
                PatienceRestoreAmount,
                CurrentPatience,
                NewPatience,
                MaxPatience);

            bool bPlayedPatientUseSound = false;
            if (USoundBase* PatientUseSound = Pitcher->GetPatientUseSound())
            {
                if (InstigatorCharacter)
                {
                    InstigatorCharacter->PlayWorldSoundForAllPlayers(PatientUseSound, Patient->GetActorLocation());
                    UE_LOG(
                        LogTemp,
                        Warning,
                        TEXT("%s UseCoffeePitcher patient use sound played sound=%s location=%s"),
                        UseCoffeePitcherUpgradesFlowLogPrefix,
                        *GetNameSafe(PatientUseSound),
                        *Patient->GetActorLocation().ToCompactString());
                    bPlayedPatientUseSound = true;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("%s UseCoffeePitcher patient use sound skipped: instigator missing."), UseCoffeePitcherUpgradesFlowLogPrefix);
                }
            }

            if (!bPlayedPatientUseSound)
            {
                TryPlayTriggerItemUseSoundForInstigator(TriggerEventData, ActivationInfo);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("%s UseCoffeePitcher patient branch no restore applied (pitcher empty or invalid)."), UseCoffeePitcherUpgradesFlowLogPrefix);
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
