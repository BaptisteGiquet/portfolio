#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"

#include "DEPRECATEDEMRPatientSpawnerTest.generated.h"


class UEMRPatientData;
class AEMRPatient;

UCLASS()
class ADEPRECATEDEMRPatientSpawnerTest : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EMR|Spawner")
	AEMRPatient* SpawnRandomPatient(const FVector& Location);
	
	AEMRPatient* SpawnPatientWithPathology(const FGameplayTag& PathologyTag, const FVector& Location);
	void ReturnPatientToPool(AEMRPatient* Patient);

	
protected:
	virtual void BeginPlay() override;

private:
	void OnPatientsDataLoaded();
	void CreatePatientPool(int32 InPoolSize);
	AEMRPatient* GetPatientFromPool();
	void ActivatePatient(AEMRPatient* Patient, const UEMRPatientData* Data, const FVector& Location);

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Spawner")
	TSubclassOf<AEMRPatient> PatientClass;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Spawner")
	int32 PoolSize = 10;

	UPROPERTY()
	TArray<TObjectPtr<AEMRPatient>> PatientPool;

	UPROPERTY()
	TArray<TObjectPtr<AEMRPatient>> AvailablePatients;
};