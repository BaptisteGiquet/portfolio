#include "UI/Machines/Triage/EMRTriagePatientDetailWidget.h"

#include "UI/Machines/Triage/EMRTriagePatientCard.h"
#include "UI/Machines/Triage/EMRTriageValidationErrorWidget.h"
#include "UI/Frontend/Core/EMRFrontendCommonButtonBase.h"
#include "UI/Patient/EMRPatientUIController.h"
#include "Components/CheckBox.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Player/EMRPlayerController.h"
#include "EngineUtils.h"
#include "Environment/EMRExamWaitingArea.h"
#include "Environment/EMRExamWaitingManagerComponent.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "GAS/EMRTags.h"

void UEMRTriagePatientDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ExamButtonMap.Reset();
	bNoExamNeeded = false;
	bRegisterPatientFolder = false;
	bValidationErrorVisible = false;
	if (CommonButton_NoExam)
	{
		CommonButton_NoExam->OnClicked().RemoveAll(this);
		CommonButton_NoExam->OnClicked().AddUObject(this, &ThisClass::HandleNoExamClicked);
		ExamButtonMap.Add(EMRTags::Abilities::Exam::None, CommonButton_NoExam);
	}

	if (CommonButton_CTScan)
	{
		CommonButton_CTScan->OnClicked().RemoveAll(this);
		CommonButton_CTScan->OnClicked().AddUObject(this, &ThisClass::HandleCTScanClicked);
		ExamButtonMap.Add(EMRTags::Abilities::Exam::CTScan, CommonButton_CTScan);
	}

	if (CommonButton_XRay)
	{
		CommonButton_XRay->OnClicked().RemoveAll(this);
		CommonButton_XRay->OnClicked().AddUObject(this, &ThisClass::HandleXRayClicked);
		ExamButtonMap.Add(EMRTags::Abilities::Exam::XRay, CommonButton_XRay);
	}

	if (CommonButton_Ultrasound)
	{
		CommonButton_Ultrasound->OnClicked().RemoveAll(this);
		CommonButton_Ultrasound->OnClicked().AddUObject(this, &ThisClass::HandleUltrasoundClicked);
		ExamButtonMap.Add(EMRTags::Abilities::Exam::Ultrasound, CommonButton_Ultrasound);
	}

	if (CommonButton_LabAnalyzer)
	{
		CommonButton_LabAnalyzer->OnClicked().RemoveAll(this);
		CommonButton_LabAnalyzer->OnClicked().AddUObject(this, &ThisClass::HandleLabAnalyzerClicked);
		ExamButtonMap.Add(EMRTags::Abilities::Exam::LabAnalyzer::Root, CommonButton_LabAnalyzer);
	}

	if (CommonButton_Validate)
	{
		CommonButton_Validate->OnClicked().RemoveAll(this);
		CommonButton_Validate->OnClicked().AddUObject(this, &ThisClass::HandleValidateClicked);
	}

	if (CommonButton_Close)
	{
		CommonButton_Close->OnClicked().RemoveAll(this);
		CommonButton_Close->OnClicked().AddUObject(this, &ThisClass::HandleCloseClicked);
	}

	if (Widget_ValidationError)
	{
		Widget_ValidationError->OnDismissRequested.RemoveAll(this);
		Widget_ValidationError->OnDismissRequested.AddUObject(this, &ThisClass::HandleValidationErrorDismissRequested);
	}

	if (CheckBox_RegisterPatientFolder)
	{
		CheckBox_RegisterPatientFolder->OnCheckStateChanged.RemoveDynamic(this, &ThisClass::HandleRegisterPatientFolderCheckStateChanged);
		CheckBox_RegisterPatientFolder->OnCheckStateChanged.AddDynamic(this, &ThisClass::HandleRegisterPatientFolderCheckStateChanged);
	}

	ResetExamSelection();
	ResetRegisterPatientFolderSelection();
	HideValidationError();
	BindToOwningPlayerController();
	BindToExamWaitingManagers();
	UpdateValidateButtonState();
}

void UEMRTriagePatientDetailWidget::NativeDestruct()
{
	if (CommonButton_Validate)
	{
		CommonButton_Validate->OnClicked().RemoveAll(this);
	}

	if (CommonButton_NoExam)
	{
		CommonButton_NoExam->OnClicked().RemoveAll(this);
	}

	if (CommonButton_CTScan)
	{
		CommonButton_CTScan->OnClicked().RemoveAll(this);
	}

	if (CommonButton_XRay)
	{
		CommonButton_XRay->OnClicked().RemoveAll(this);
	}

	if (CommonButton_Ultrasound)
	{
		CommonButton_Ultrasound->OnClicked().RemoveAll(this);
	}

	if (CommonButton_LabAnalyzer)
	{
		CommonButton_LabAnalyzer->OnClicked().RemoveAll(this);
	}

	if (CommonButton_Close)
	{
		CommonButton_Close->OnClicked().RemoveAll(this);
	}

	if (Widget_ValidationError)
	{
		Widget_ValidationError->OnDismissRequested.RemoveAll(this);
	}

	if (CheckBox_RegisterPatientFolder)
	{
		CheckBox_RegisterPatientFolder->OnCheckStateChanged.RemoveDynamic(this, &ThisClass::HandleRegisterPatientFolderCheckStateChanged);
	}

	UnbindFromOwningPlayerController();
	UnbindFromExamWaitingManagers();

	ExamButtonMap.Reset();
	SelectedExamTags.Reset();
	bNoExamNeeded = false;
	bRegisterPatientFolder = false;
	bWaitingValidationResult = false;
	bValidationErrorVisible = false;

	ClearPatient();

	Super::NativeDestruct();
}

void UEMRTriagePatientDetailWidget::SetPatient(AEMRPatient* InPatient)
{
	if (!InPatient || !Widget_TriagePatientCard)
	{
		ClearPatient();
		return;
	}

	if (ActivePatient.Get() == InPatient && ActiveUIController)
	{
		return;
	}

	ClearPatient();

	ActivePatient = InPatient;
	ResetExamSelection();
	ResetRegisterPatientFolderSelection();
	HideValidationError();
	ActiveUIController = NewObject<UEMRPatientUIController>(this);
	if (ActiveUIController)
	{
		// Reception triage always needs full data visible so the player can choose exams.
		ActiveUIController->SetRespectFolderDisplayRegistration(false);
		ActiveUIController->BindToPatient(InPatient);
	}

	Widget_TriagePatientCard->InitializeCard(InPatient, ActiveUIController);
	UpdateValidateButtonState();
}

void UEMRTriagePatientDetailWidget::ClearPatient()
{
	if (Widget_TriagePatientCard)
	{
		Widget_TriagePatientCard->ResetCard();
	}

	if (ActiveUIController)
	{
		ActiveUIController->StopMonitoring();
	}

	ActiveUIController = nullptr;
	ActivePatient = nullptr;
	bWaitingValidationResult = false;
	ResetExamSelection();
	ResetRegisterPatientFolderSelection();
	HideValidationError();
	UpdateValidateButtonState();
}

void UEMRTriagePatientDetailWidget::HandleCloseClicked()
{
	OnCloseRequested.Broadcast();
}

void UEMRTriagePatientDetailWidget::HandleValidateClicked()
{
	if (!ActivePatient.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TriageDetail] Validate clicked without active patient."));
		return;
	}

	if (!bNoExamNeeded && SelectedExamTags.Num() == 0)
	{
		UpdateValidateButtonState();
		return;
	}

	if (bWaitingValidationResult)
	{
		return;
	}

	AEMRPlayerController* PC = Cast<AEMRPlayerController>(GetOwningPlayer());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TriageDetail] Owning player controller is not EMRPlayerController."));
		return;
	}

	if (CachedPlayerController.Get() != PC)
	{
		BindToOwningPlayerController();
	}

	const TArray<FGameplayTag> ExamTags = bNoExamNeeded
	? TArray<FGameplayTag>{EMRTags::Abilities::Exam::None}
	: SelectedExamTags.Array();

	HideValidationError();
	bWaitingValidationResult = true;
	UpdateValidateButtonState();
	PC->Server_CompleteReceptionTask(ActivePatient.Get(), ExamTags, bNoExamNeeded, bRegisterPatientFolder);
}

void UEMRTriagePatientDetailWidget::HandleNoExamClicked()
{
	if (bValidationErrorVisible)
	{
		return;
	}

	bNoExamNeeded = !bNoExamNeeded;
	if (bNoExamNeeded)
	{
		SelectedExamTags.Reset();
	}

	UpdateExamButtonSelectionState();
	UpdateValidateButtonState();
}

void UEMRTriagePatientDetailWidget::HandleCTScanClicked()
{
	ToggleExamSelection(EMRTags::Abilities::Exam::CTScan);
}

void UEMRTriagePatientDetailWidget::HandleXRayClicked()
{
	ToggleExamSelection(EMRTags::Abilities::Exam::XRay);
}

void UEMRTriagePatientDetailWidget::HandleUltrasoundClicked()
{
	ToggleExamSelection(EMRTags::Abilities::Exam::Ultrasound);
}

void UEMRTriagePatientDetailWidget::HandleLabAnalyzerClicked()
{
	ToggleExamSelection(EMRTags::Abilities::Exam::LabAnalyzer::Root);
}

void UEMRTriagePatientDetailWidget::ToggleExamSelection(const FGameplayTag& ExamTag)
{
	if (!ActivePatient.IsValid() || !ExamTag.IsValid())
	{
		return;
	}

	if (bNoExamNeeded || bValidationErrorVisible)
	{
		return;
	}

	if (SelectedExamTags.Contains(ExamTag))
	{
		SelectedExamTags.Remove(ExamTag);
	}
	else
	{
		SelectedExamTags.Add(ExamTag);
	}

	UpdateExamButtonSelectionState();
	UpdateValidateButtonState();
}

void UEMRTriagePatientDetailWidget::ResetExamSelection()
{
	SelectedExamTags.Reset();
	bNoExamNeeded = false;

	for (const TPair<FGameplayTag, TObjectPtr<UEMRFrontendCommonButtonBase>>& Pair : ExamButtonMap)
	{
		if (Pair.Value)
		{
			Pair.Value->ClearSelection();
		}
	}

	UpdateExamButtonSelectionState();
}

void UEMRTriagePatientDetailWidget::ResetRegisterPatientFolderSelection()
{
	bRegisterPatientFolder = false;
	UpdateRegisterPatientFolderCheckboxState();
}

void UEMRTriagePatientDetailWidget::UpdateRegisterPatientFolderCheckboxState()
{
	if (CheckBox_RegisterPatientFolder)
	{
		CheckBox_RegisterPatientFolder->SetIsChecked(bRegisterPatientFolder);
	}
}

void UEMRTriagePatientDetailWidget::UpdateExamButtonSelectionState()
{
	for (const TPair<FGameplayTag, TObjectPtr<UEMRFrontendCommonButtonBase>>& Pair : ExamButtonMap)
	{
		if (Pair.Value)
		{
			const bool bIsNoneTag = Pair.Key.MatchesTagExact(EMRTags::Abilities::Exam::None);
			const bool bHasExamSelection = SelectedExamTags.Num() > 0;
			const bool bEnabled = !bValidationErrorVisible && (bIsNoneTag ? !bHasExamSelection : !bNoExamNeeded);
			Pair.Value->SetIsEnabled(bEnabled);
		}
	}
}

void UEMRTriagePatientDetailWidget::UpdateValidateButtonState()
{
	if (CommonButton_Validate)
	{
		const bool bHasSelection = bNoExamNeeded || SelectedExamTags.Num() > 0;
		const bool bHasPatient = ActivePatient.IsValid();
		CommonButton_Validate->SetIsEnabled(bHasSelection && bHasPatient && !bWaitingValidationResult);
	}
}

void UEMRTriagePatientDetailWidget::ShowValidationError(EEMRReceptionValidationResult Result)
{
	if (!Widget_ValidationError)
	{
		return;
	}

	FText Message;
	switch (Result)
	{
	case EEMRReceptionValidationResult::WrongExamSelection:
		Message = WrongExamValidationErrorText;
		break;
	case EEMRReceptionValidationResult::NoExamWaitingSeatAvailable:
		Message = WaitingSeatUnavailableValidationErrorText;
		break;
	default:
		Message = GenericValidationErrorText;
		break;
	}

	if (Message.IsEmpty())
	{
		Message = GenericValidationErrorText;
	}

	if (Message.IsEmpty())
	{
		HideValidationError();
		return;
	}

	bValidationErrorVisible = true;
	Widget_ValidationError->ShowMessage(Message);
	UpdateExamButtonSelectionState();
}

void UEMRTriagePatientDetailWidget::HideValidationError()
{
	bValidationErrorVisible = false;

	if (Widget_ValidationError)
	{
		Widget_ValidationError->ClearMessage();
	}

	UpdateExamButtonSelectionState();
}

void UEMRTriagePatientDetailWidget::HandleValidationErrorDismissRequested()
{
	HideValidationError();
}

void UEMRTriagePatientDetailWidget::HandleRegisterPatientFolderCheckStateChanged(bool bIsChecked)
{
	bRegisterPatientFolder = bIsChecked;
}

void UEMRTriagePatientDetailWidget::BindToOwningPlayerController()
{
	AEMRPlayerController* PlayerController = Cast<AEMRPlayerController>(GetOwningPlayer());
	if (!PlayerController)
	{
		return;
	}

	PlayerController->OnReceptionValidationResult.RemoveAll(this);
	PlayerController->OnReceptionValidationResult.AddUObject(this, &ThisClass::HandleReceptionValidationResult);
	CachedPlayerController = PlayerController;
}

void UEMRTriagePatientDetailWidget::UnbindFromOwningPlayerController()
{
	if (CachedPlayerController.IsValid())
	{
		CachedPlayerController->OnReceptionValidationResult.RemoveAll(this);
	}

	CachedPlayerController = nullptr;
}

void UEMRTriagePatientDetailWidget::BindToExamWaitingManagers()
{
	UnbindFromExamWaitingManagers();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AEMRExamWaitingArea> It(World); It; ++It)
	{
		if (AEMRExamWaitingArea* WaitingArea = *It)
		{
			if (UEMRExamWaitingManagerComponent* Manager = WaitingArea->GetManagerComponent())
			{
				Manager->OnSeatAssigned.RemoveDynamic(this, &ThisClass::HandleExamWaitingSeatAssigned);
				Manager->OnSeatReleased.RemoveDynamic(this, &ThisClass::HandleExamWaitingSeatReleased);
				Manager->OnSeatAssigned.AddDynamic(this, &ThisClass::HandleExamWaitingSeatAssigned);
				Manager->OnSeatReleased.AddDynamic(this, &ThisClass::HandleExamWaitingSeatReleased);
				BoundExamWaitingManagers.Add(Manager);
			}
		}
	}
}

void UEMRTriagePatientDetailWidget::UnbindFromExamWaitingManagers()
{
	for (TWeakObjectPtr<UEMRExamWaitingManagerComponent>& ManagerPtr : BoundExamWaitingManagers)
	{
		if (UEMRExamWaitingManagerComponent* Manager = ManagerPtr.Get())
		{
			Manager->OnSeatAssigned.RemoveDynamic(this, &ThisClass::HandleExamWaitingSeatAssigned);
			Manager->OnSeatReleased.RemoveDynamic(this, &ThisClass::HandleExamWaitingSeatReleased);
		}
	}

	BoundExamWaitingManagers.Reset();
}

void UEMRTriagePatientDetailWidget::HandleExamWaitingSeatAssigned(UEMRWaitingSeatComponent* Seat, AActor* PatientActor)
{
	UpdateValidateButtonState();
}

void UEMRTriagePatientDetailWidget::HandleExamWaitingSeatReleased(UEMRWaitingSeatComponent* Seat)
{
	UpdateValidateButtonState();
}

void UEMRTriagePatientDetailWidget::HandleReceptionValidationResult(AEMRPatient* Patient, EEMRReceptionValidationResult Result)
{
	if (!ActivePatient.IsValid() || ActivePatient.Get() != Patient)
	{
		return;
	}

	bWaitingValidationResult = false;
	if (Result == EEMRReceptionValidationResult::Success)
	{
		HideValidationError();
		OnCloseRequested.Broadcast();
		return;
	}

	if (Result == EEMRReceptionValidationResult::WrongExamSelection)
	{
		ResetExamSelection();
	}

	ShowValidationError(Result);
	UpdateValidateButtonState();
}
