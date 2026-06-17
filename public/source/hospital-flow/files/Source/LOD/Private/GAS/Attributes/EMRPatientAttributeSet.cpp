
#include "GAS/Attributes/EMRPatientAttributeSet.h"

#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Characters/Patient/EMRPatient.h"


void UEMRPatientAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);


    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, HeartRate, COND_None, REPNOTIFY_Always)
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, BloodPressureSystolic, COND_None, REPNOTIFY_Always)
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, BloodPressureDiastolic, COND_None, REPNOTIFY_Always)
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, OxygenSaturation, COND_None, REPNOTIFY_Always)
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, RespiratoryRate, COND_None, REPNOTIFY_Always)
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Temperature, COND_None, REPNOTIFY_Always)
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, GlasgowScore, COND_None, REPNOTIFY_Always)


    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Patience, COND_None, REPNOTIFY_Always)
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxPatience, COND_None, REPNOTIFY_Always)
}


void UEMRPatientAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
	
    if (Attribute == GetPatienceAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPatience());
    }
}


void UEMRPatientAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetPatienceAttribute())
    {
        SetPatience(FMath::Clamp(GetPatience(), 0.f, GetMaxPatience()));

        if (GetPatience() <= 0.f)
        {
            // The treatment subsystem listens for Patience reaching zero via
            // attribute change delegates and handles the exit flow server-side.
        }
    }
}



void UEMRPatientAttributeSet::OnRep_HeartRate(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, HeartRate, OldValue)
}

void UEMRPatientAttributeSet::OnRep_BloodPressureSystolic(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, BloodPressureSystolic, OldValue)
}

void UEMRPatientAttributeSet::OnRep_BloodPressureDiastolic(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, BloodPressureDiastolic, OldValue)
}

void UEMRPatientAttributeSet::OnRep_OxygenSaturation(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, OxygenSaturation, OldValue)
}

void UEMRPatientAttributeSet::OnRep_RespiratoryRate(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, RespiratoryRate, OldValue)
}

void UEMRPatientAttributeSet::OnRep_Temperature(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Temperature, OldValue)
}

void UEMRPatientAttributeSet::OnRep_GlasgowScore(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, GlasgowScore, OldValue)
}

void UEMRPatientAttributeSet::OnRep_Patience(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Patience, OldValue)
}

void UEMRPatientAttributeSet::OnRep_MaxPatience(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxPatience, OldValue)
}