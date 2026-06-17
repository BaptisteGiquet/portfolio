#include "UI/Machines/Triage/EMRTriagePatientCardObject.h"

#include "Characters/Patient/EMRPatient.h"
#include "UI/Patient/EMRPatientUIController.h"

void UEMRTriagePatientCardObject::Initialize(AEMRPatient* InPatient, UEMRPatientUIController* InController)
{
	Patient = InPatient;
	UIController = InController;
}

void UEMRTriagePatientCardObject::Cleanup()
{
	if (UIController)
	{
		UIController->StopMonitoring();
	}

	UIController = nullptr;
	Patient.Reset();
}
