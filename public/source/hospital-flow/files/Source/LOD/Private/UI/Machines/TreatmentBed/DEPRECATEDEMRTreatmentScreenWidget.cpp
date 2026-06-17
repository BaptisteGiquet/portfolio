
#include "UI/Machines/TreatmentBed/DEPRECATEDEMRTreatmentScreenWidget.h"

#include "UI/Patient/EMRPatientFolderWidget.h"
#include "UI/Patient/EMRPatientUIController.h"


void UDEPRECATEDEMRTreatmentScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
}


void UDEPRECATEDEMRTreatmentScreenWidget::NativeDestruct()
{
	ClearPatientData();
	Super::NativeDestruct();
}


void UDEPRECATEDEMRTreatmentScreenWidget::BindToPatient(UEMRPatientUIController* InUIController)
{
	if (!InUIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreatmentScreen] Cannot bind to null UIController"));
		return;
	}

	ClearPatientData();

	UIController = InUIController;

	if (!Widget_PatientFolder)
	{
		UE_LOG(LogTemp, Error, TEXT("[TreatmentScreen] Widget_PatientFolder is NULL (not bound)"));
		return;
	}
	

	Widget_PatientFolder->InitializePatientFolder(UIController);
	
	UIController->StartMonitoring();
}


void UDEPRECATEDEMRTreatmentScreenWidget::ClearPatientData()
{
	if (UIController)
	{
		UIController->StopMonitoring();
	}

	if (Widget_PatientFolder)
	{
		Widget_PatientFolder->Cleanup();
	}

	UIController = nullptr;
}

