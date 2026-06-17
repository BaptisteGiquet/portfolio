#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "EMRTriagePatientDetailWidget.generated.h"

class UEMRTriagePatientCard;
class UEMRFrontendCommonButtonBase;
class UEMRPatientUIController;
class UEMRExamWaitingManagerComponent;
class UEMRWaitingSeatComponent;
class UEMRTriageValidationErrorWidget;
class UCheckBox;
class AEMRPatient;
class AEMRPlayerController;
class AActor;
enum class EEMRReceptionValidationResult : uint8;

DECLARE_MULTICAST_DELEGATE(FOnTriagePatientDetailClosed);

UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRTriagePatientDetailWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetPatient(AEMRPatient* InPatient);
	void ClearPatient();

	AEMRPatient* GetActivePatient() const { return ActivePatient.Get(); }

	FOnTriagePatientDetailClosed OnCloseRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void HandleCloseClicked();
	void HandleValidateClicked();
	void HandleNoExamClicked();
	void HandleCTScanClicked();
	void HandleXRayClicked();
	void HandleUltrasoundClicked();
	void HandleLabAnalyzerClicked();
	void HandleReceptionValidationResult(AEMRPatient* Patient, EEMRReceptionValidationResult Result);
	void HandleValidationErrorDismissRequested();
	void ResetRegisterPatientFolderSelection();
	void UpdateRegisterPatientFolderCheckboxState();

	void ToggleExamSelection(const FGameplayTag& ExamTag);
	void ResetExamSelection();
	void UpdateExamButtonSelectionState();
	void UpdateValidateButtonState();
	void ShowValidationError(EEMRReceptionValidationResult Result);
	void HideValidationError();
	void BindToOwningPlayerController();
	void UnbindFromOwningPlayerController();
	void BindToExamWaitingManagers();
	void UnbindFromExamWaitingManagers();

	UFUNCTION()
	void HandleExamWaitingSeatAssigned(UEMRWaitingSeatComponent* Seat, AActor* PatientActor);

	UFUNCTION()
	void HandleExamWaitingSeatReleased(UEMRWaitingSeatComponent* Seat);

	UFUNCTION()
	void HandleRegisterPatientFolderCheckStateChanged(bool bIsChecked);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRTriagePatientCard> Widget_TriagePatientCard = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Validate = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Close = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_NoExam = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_CTScan = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_XRay = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_Ultrasound = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEMRFrontendCommonButtonBase> CommonButton_LabAnalyzer = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEMRTriageValidationErrorWidget> Widget_ValidationError = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CheckBox_RegisterPatientFolder = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI|Triage")
	FText WrongExamValidationErrorText;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI|Triage")
	FText WaitingSeatUnavailableValidationErrorText;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI|Triage")
	FText GenericValidationErrorText;

	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UEMRFrontendCommonButtonBase>> ExamButtonMap;

	UPROPERTY()
	TSet<FGameplayTag> SelectedExamTags;

	UPROPERTY()
	bool bNoExamNeeded = false;

	UPROPERTY()
	bool bRegisterPatientFolder = false;

	UPROPERTY()
	TObjectPtr<UEMRPatientUIController> ActiveUIController = nullptr;

	TWeakObjectPtr<AEMRPlayerController> CachedPlayerController;
	TArray<TWeakObjectPtr<UEMRExamWaitingManagerComponent>> BoundExamWaitingManagers;

	bool bWaitingValidationResult = false;
	bool bValidationErrorVisible = false;

	TWeakObjectPtr<AEMRPatient> ActivePatient;
};
