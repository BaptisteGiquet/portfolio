#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "EMRPatientFolderWidget.generated.h"

class UProgressBar;
class UImage;
class UTextBlock;
class UEMRPatientUIController;
struct FEMRPatientUIData;

/**
 * Main widget for the patient folder
 * Subscribes to UIController events and updates widgets
 */

UCLASS()
class LOD_API UEMRPatientFolderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EMR|UI|Patient")
	void InitializePatientFolder(UEMRPatientUIController* UIController);

	/** Called when widget is closed */
	UFUNCTION(BlueprintCallable, Category = "EMR|UI|Patient")
	void Cleanup();

	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintPure, Category = "EMR|UI|Patient")
	UEMRPatientUIController* GetUIController() const { return UIController; }

	
	/** Optional Blueprint hook called after vital stats are updated */
	UFUNCTION(BlueprintImplementableEvent, Category = "EMR|UI|Patient", meta = (DisplayName = "On Post Vital Stats Updated"))
	void BP_OnPostVitalStatsUpdated(const FEMRPatientUIData& UIData);



private:
	UFUNCTION()
	void OnVitalStatsUpdated(const FEMRPatientUIData& UIData);
	

	/** Builds the cache from the Data Table */
	void BuildLocalizationCache();

	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_FullName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Age;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_BloodType;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PatientType;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Pathologies;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Symptoms; 

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_HeartRate;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_BloodPressure;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_OxygenSaturation;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RespiratoryRate;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Temperature;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_GlasgowScore;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Patience;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|UI|Patient")
	FText NotRegisteredText;
	

	UPROPERTY()
	TObjectPtr<UEMRPatientUIController> UIController;
};
