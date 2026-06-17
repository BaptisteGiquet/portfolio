

#include "UI/Patient/EMRPatientUIController.h"

#include "GAS/Attributes/EMRPatientAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRPatientData.h"
#include "GAS/EMRTags.h"


void UEMRPatientUIController::BindToPatient(const AEMRPatient* InPatient)
{
    if (!InPatient)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UIController] Attempted to bind to null patient"));
        return;
    }

    // Cleanup ancien binding
    StopMonitoring();

    Patient = InPatient;
    PatientASC = Patient->GetAbilitySystemComponent();

    if (!PatientASC)
    {
        UE_LOG(LogTemp, Error, TEXT("[UIController] Patient has no PatientASC"));
        return;
    }

    // Bind to Phase changes (EMR.Patient.Phase.*)
    PhaseTagDelegateHandle = PatientASC->RegisterGameplayTagEvent(
        EMRTags::Patient::Phase::Root,
        EGameplayTagEventType::NewOrRemoved
    ).AddUObject(this, &UEMRPatientUIController::OnPhaseTagChanged);
}


void UEMRPatientUIController::StartMonitoring()
{
    if (!Patient || !PatientASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UIController] Cannot start monitoring without patient"));
        return;
    }

    // First update
    UpdateVitalStats();

	
    // Timer for futures updates
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            MonitoringTimerHandle,
            this,
            &ThisClass::UpdateVitalStats, VitalStatsUpdateFrequency,
            true
        );
    }
}


void UEMRPatientUIController::StopMonitoring()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MonitoringTimerHandle);
    }

    if (PatientASC && PhaseTagDelegateHandle.IsValid())
    {
        PatientASC->RegisterGameplayTagEvent(EMRTags::Patient::Phase::Root).Remove(PhaseTagDelegateHandle);
        PhaseTagDelegateHandle.Reset();
    }
}


void UEMRPatientUIController::UpdateVitalStats()
{
	FEMRPatientUIData UIData = GetPatientUIData();
	OnVitalStatsUpdated.Broadcast(UIData);
}


FEMRPatientUIData UEMRPatientUIController::GetPatientUIData() const
{
    FEMRPatientUIData UIData;

    if (!Patient || !PatientASC)
    {
        return UIData;
    }

    const UEMRPatientData* PatientData = Patient->GetPatientData();
    if (!PatientData)
    {
        return UIData;
    }

    // Static Data
    UIData.bFolderDisplayRegistered = !bRespectFolderDisplayRegistration || Patient->IsFolderDisplayRegistered();
    UIData.FullName = Patient->GetFullName();
    UIData.Age = Patient->GetAge();
    UIData.BloodType = Patient->GetBloodType();
    UIData.PatientType = PatientData->GetParticularity();
    UIData.Pathologies = PatientData->GetPathologies();
    UIData.Symptoms = PatientData->GetSymptoms();
    UIData.Portrait = PatientData->GetPortrait();


	// Dynamic Data
    UIData.CurrentPhase = GetCurrentPhase();
	
    UIData.HeartRate = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetHeartRateAttribute());
    UIData.BloodPressureSystolic = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetBloodPressureSystolicAttribute());
    UIData.BloodPressureDiastolic = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetBloodPressureDiastolicAttribute());
    UIData.OxygenSaturation = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetOxygenSaturationAttribute());
    UIData.RespiratoryRate = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetRespiratoryRateAttribute());
    UIData.Temperature = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetTemperatureAttribute());
    UIData.GlasgowScore = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetGlasgowScoreAttribute());
    UIData.Patience = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetPatienceAttribute());
    UIData.MaxPatience = PatientASC->GetNumericAttribute(UEMRPatientAttributeSet::GetMaxPatienceAttribute());

    return UIData;
}


void UEMRPatientUIController::OnPhaseTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
    if (NewCount > 0) // New Phase Tag added
    {
        OnPhaseChanged.Broadcast(CallbackTag);
    }
}


FGameplayTag UEMRPatientUIController::GetCurrentPhase() const
{
    if (!PatientASC)
    {
        return FGameplayTag::EmptyTag;
    }

    FGameplayTagContainer OwnedTags;
    PatientASC->GetOwnedGameplayTags(OwnedTags);

    // Search first tag that start with "EMR.Patient.Phase"
    FGameplayTag PhaseParentTag = EMRTags::Patient::Phase::Root;
    for (const FGameplayTag& Tag : OwnedTags)
    {
        if (Tag.MatchesTag(PhaseParentTag) && Tag != PhaseParentTag)
        {
            return Tag;
        }
    }

    return FGameplayTag::EmptyTag;
}
