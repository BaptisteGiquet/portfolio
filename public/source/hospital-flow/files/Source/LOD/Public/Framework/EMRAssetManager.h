#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Engine/AssetManager.h"
#include "Data/EMRRunRulesData.h"

#include "EMRAssetManager.generated.h"


class UEMREconomySystemGenerics;
class UEMRPatientData;
class UEMRRunRulesData;
class UEMRSubsystemConfig;


UCLASS()
class LOD_API UEMRAssetManager : public UAssetManager
{
	GENERATED_BODY()


public:
	static UEMRAssetManager& Get();

	/*******************************************/
	/******************PATIENTS*****************/
	/*******************************************/

public:
	void LoadPatients(const FStreamableDelegate& LoadFinishedCallback);
	bool CollectLoadedPatients(TArray<const UEMRPatientData*>& OutLoadedPatients);
	const UEMRPatientData* GetRandomPatient() const;
	const UEMRPatientData* GetRandomPatientWithPathology(const FGameplayTag& PathologyTag) const;

private:
	void OnPatientsLoadFinished(FStreamableDelegate Callback);

	UPROPERTY()
	TArray<const UEMRPatientData*> LoadedPatients;
	

	/*******************************************/
	/******************ECONOMY******************/
	/*******************************************/

public:
	void LoadEconomySystemGenerics(const FStreamableDelegate& LoadFinishedCallback);
	bool CollectLoadedEconomySystemGenerics(TArray<const UEMREconomySystemGenerics*>& OutLoadedEconomySystemGenerics);

	UPROPERTY()
	TArray<const UEMREconomySystemGenerics*> LoadedEconomySystemGenerics;
	
private:
	void OnEconomySystemGenericsLoadFinished(FStreamableDelegate Callback);
	
	
	/*******************************************/
	/******************SUBSYSTEM****************/
	/*******************************************/

public:
    void LoadSubsystemConfig(const FStreamableDelegate& LoadFinishedCallback);
	bool CollectLoadedSubsystemConfig(TArray<const UEMRSubsystemConfig*>& OutLoadedSubsystemConfig);

	UPROPERTY()
	TArray<const UEMRSubsystemConfig*> LoadedSubsystemConfig;

private:
    void OnSubsystemConfigLoadFinished(FStreamableDelegate Callback);


    /*******************************************/
    /*****************RUN RULES*****************/
    /*******************************************/

public:
    void LoadRunRulesData(const FStreamableDelegate& LoadFinishedCallback);
	bool CollectLoadedRunRulesData(TArray<const UEMRRunRulesData*>& OutLoadedRunRulesData);

private:
    void OnRunRulesDataLoadFinished(FStreamableDelegate Callback);

	UPROPERTY()
	TArray<const UEMRRunRulesData*> LoadedRunRulesData;

};
