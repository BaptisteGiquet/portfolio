#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EMRTriagePatientCardObject.generated.h"

class AEMRPatient;
class UEMRPatientUIController;

/**
 * List item data object for triage patient cards.
 */
UCLASS()
class LOD_API UEMRTriagePatientCardObject : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AEMRPatient* InPatient, UEMRPatientUIController* InController);

	AEMRPatient* GetPatient() const { return Patient.Get(); }
	UEMRPatientUIController* GetUIController() const { return UIController; }

	void Cleanup();

private:
	UPROPERTY()
	TWeakObjectPtr<AEMRPatient> Patient;

	UPROPERTY()
	TObjectPtr<UEMRPatientUIController> UIController;
};
