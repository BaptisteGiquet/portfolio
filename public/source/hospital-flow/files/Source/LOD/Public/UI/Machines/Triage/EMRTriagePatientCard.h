#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "EMRTriagePatientCard.generated.h"

class UAnalogSlider;
class UCommonTextBlock;
class UCommonLazyImage;
class UBorder;
class UProgressBar;
class UEMRTriagePatientCardCommonButton;
class UEMRPatientUIController;
class AEMRPatient;
struct FEMRPatientUIData;
/**
 * 
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNativeTick))
class LOD_API UEMRTriagePatientCard : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	void InitializeCard(AEMRPatient* InPatient, UEMRPatientUIController* InUIController);
	void ResetCard();
	void SetLabAnalyzerWaitingPathologyOverride(bool bEnabled, const FText& InText);

protected:
	virtual void NativeDestruct() override;

private:
	void Cleanup();
	void UpdateSeverityBorder();
	void SetPatienceLevelVisual(float InNormalizedValue);
	bool ShouldShowLabAnalyzerWaitingPathologyOverride() const;
	FText GetLabAnalyzerWaitingPathologyText() const;

	UFUNCTION()
	void HandleVitalStatsUpdated(const FEMRPatientUIData& UIData);

	//***** Bound Widgets *****//
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_CardColor;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonLazyImage> CommonLazyImage_Portrait = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Name;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Age;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_BloodGroup;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Symptoms;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> CommonTextBlock_Pathologies;
	
	// Preferred widget for performance (lighter than AnalogSlider in repeated list items).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar_PatienceLevel = nullptr;

	// Backward-compatible fallback for existing widgets that haven't been migrated yet.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UAnalogSlider> AnalogSlider_PatienceLevel = nullptr;
	//***** Bound Widgets *****//

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|UI|Triage", meta = (AllowPrivateAccess = true))
	FLinearColor SeverityRedColor = FLinearColor(0.82f, 0.16f, 0.16f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|UI|Triage", meta = (AllowPrivateAccess = true))
	FLinearColor SeverityYellowColor = FLinearColor(0.93f, 0.83f, 0.12f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|UI|Triage", meta = (AllowPrivateAccess = true))
	FLinearColor SeverityGreenColor = FLinearColor(0.13f, 0.65f, 0.18f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|UI|Triage", meta = (AllowPrivateAccess = true))
	FLinearColor SeverityBlackColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|UI|Triage", meta = (AllowPrivateAccess = true))
	FText NotRegisteredText;

	UPROPERTY()
	TObjectPtr<UEMRPatientUIController> UIController;

	TWeakObjectPtr<AEMRPatient> Patient;
	bool bIsShowingUnregisteredState = false;
	bool bLabAnalyzerWaitingPathologyOverrideEnabled = false;
	FText LabAnalyzerWaitingPathologyOverrideText;
};
