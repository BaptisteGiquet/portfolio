#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "EMRPatientUIData.h"

#include "EMRPatientUIController.generated.h"

class AEMRPatient;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVitalStatsUpdated, const FEMRPatientUIData&, UIData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, FGameplayTag, NewPhase);

/**
 * UI Controller for a Patient
 * Handle data updates and inform widget
 */

UCLASS(BlueprintType)
class LOD_API UEMRPatientUIController : public UObject
{
    GENERATED_BODY()

public:
    /** Bind to current patient and listen for Tags */
    UFUNCTION(BlueprintCallable, Category = "EMR|UI")
    void BindToPatient(const AEMRPatient* InPatient);

    void SetRespectFolderDisplayRegistration(bool bInRespectFolderDisplayRegistration)
    {
        bRespectFolderDisplayRegistration = bInRespectFolderDisplayRegistration;
    }

    /** Start monitoring of VitalStats */
    UFUNCTION(BlueprintCallable, Category = "EMR|UI")
    void StartMonitoring();


    UFUNCTION(BlueprintCallable, Category = "EMR|UI")
    void StopMonitoring();
	
    UFUNCTION(BlueprintPure, Category = "EMR|UI")
    FEMRPatientUIData GetPatientUIData() const;


    UPROPERTY(BlueprintAssignable, Category = "EMR|UI")
    FOnVitalStatsUpdated OnVitalStatsUpdated;


    UPROPERTY(BlueprintAssignable, Category = "EMR|UI")
    FOnPhaseChanged OnPhaseChanged;

private:
    void UpdateVitalStats();
	
    void OnPhaseTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	
    FGameplayTag GetCurrentPhase() const;

    UPROPERTY()
    TObjectPtr<const AEMRPatient> Patient;

    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> PatientASC;

    FTimerHandle MonitoringTimerHandle;

    UPROPERTY(EditDefaultsOnly, Category = "EMR|UI")
    float VitalStatsUpdateFrequency = 1.0f; 

    FDelegateHandle PhaseTagDelegateHandle;
    bool bRespectFolderDisplayRegistration = true;
};
