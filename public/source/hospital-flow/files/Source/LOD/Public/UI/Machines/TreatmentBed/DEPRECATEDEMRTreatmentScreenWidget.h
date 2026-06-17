#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "DEPRECATEDEMRTreatmentScreenWidget.generated.h"

class UEMRPatientFolderWidget;
class UEMRPatientUIController;
struct FEMRPatientUIData;

UCLASS()
class LOD_API UDEPRECATEDEMRTreatmentScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
	void BindToPatient(UEMRPatientUIController* InUIController);

	UFUNCTION(BlueprintCallable, Category = "EMR|Treatment")
	void ClearPatientData();

protected:
	virtual void NativeConstruct() override;
	
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UEMRPatientFolderWidget> Widget_PatientFolder;
	
private:
	UPROPERTY()
	TObjectPtr<UEMRPatientUIController> UIController;
};
