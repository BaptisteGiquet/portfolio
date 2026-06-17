
#include "Framework/EMRAssetManager.h"

#include "Data/EMREconomySystemGenerics.h"
#include "Data/EMRRunRulesData.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Subsystems/EMRSubsystemConfig.h"


UEMRAssetManager& UEMRAssetManager::Get()
{
	UEMRAssetManager* Singleton = Cast<UEMRAssetManager>(GEngine->AssetManager.Get());
	if (Singleton)
	{
		return *Singleton;
	}

	UE_LOG(LogLoad, Fatal, TEXT("Asset Manager needs to be of type UEMRAssetManager"))
	return (*NewObject<UEMRAssetManager>());
}





/*******************************************/
/******************PATIENTS*****************/
/*******************************************/


void UEMRAssetManager::LoadPatients(const FStreamableDelegate& LoadFinishedCallback)
{
	LoadPrimaryAssetsWithType(
	UEMRPatientData::GetPatientAssetType(), 
	TArray<FName>(), 
	FStreamableDelegate::CreateUObject(this, &UEMRAssetManager::OnPatientsLoadFinished, LoadFinishedCallback)
);
}


void UEMRAssetManager::OnPatientsLoadFinished(FStreamableDelegate Callback)
{
    LoadedPatients.Reset();
	CollectLoadedPatients(LoadedPatients);
	Callback.ExecuteIfBound();
}


bool UEMRAssetManager::CollectLoadedPatients(TArray<const UEMRPatientData*>& OutLoadedPatients)
{
	TArray<UObject*> LoadedObjects;
	const bool bIsLoaded = GetPrimaryAssetObjectList(UEMRPatientData::GetPatientAssetType(), LoadedObjects);

	if (bIsLoaded)
	{
		for (UObject* ObjectLoaded : LoadedObjects)
		{
			OutLoadedPatients.Add(Cast<UEMRPatientData>(ObjectLoaded));
		}
	}

	return bIsLoaded;
}


const UEMRPatientData* UEMRAssetManager::GetRandomPatient() const
{
	if (LoadedPatients.Num() == 0)
	{
		return nullptr;
	}

	const int32 RandomIndex = FMath::RandRange(0, LoadedPatients.Num() - 1);
	return LoadedPatients[RandomIndex];
}


const UEMRPatientData* UEMRAssetManager::GetRandomPatientWithPathology(const FGameplayTag& PathologyTag) const
{
	if (LoadedPatients.Num() == 0 || !PathologyTag.IsValid())
	{
		return nullptr;
	}


	TArray<const UEMRPatientData*> FilteredPatients;
	for (const UEMRPatientData* Patient : LoadedPatients)
	{
		if (Patient && Patient->GetPathologies().HasTagExact(PathologyTag))
		{
			FilteredPatients.Add(Patient);
		}
	}

	
	if (FilteredPatients.Num() == 0)
	{
		return nullptr;
	}
	
	const int32 RandomIndex = FMath::RandRange(0, FilteredPatients.Num() - 1);
	return FilteredPatients[RandomIndex];
}




/*******************************************/
/******************ECONOMY******************/
/*******************************************/


void UEMRAssetManager::LoadEconomySystemGenerics(const FStreamableDelegate& LoadFinishedCallback)
{
	LoadPrimaryAssetsWithType(
		UEMREconomySystemGenerics::GetEconomySystemGenericsAssetType(),
		TArray<FName>(),
		FStreamableDelegate::CreateUObject(this, &UEMRAssetManager::OnEconomySystemGenericsLoadFinished, LoadFinishedCallback)
		);
}


void UEMRAssetManager::OnEconomySystemGenericsLoadFinished(FStreamableDelegate Callback)
{
	LoadedEconomySystemGenerics.Reset();
	CollectLoadedEconomySystemGenerics(LoadedEconomySystemGenerics);
	Callback.ExecuteIfBound();
}


bool UEMRAssetManager::CollectLoadedEconomySystemGenerics(TArray<const UEMREconomySystemGenerics*>& OutLoadedEconomySystemGenerics)
{
	TArray<UObject*> LoadedObjects;
	const bool bIsLoaded = GetPrimaryAssetObjectList(UEMREconomySystemGenerics::GetEconomySystemGenericsAssetType(), LoadedObjects);

	if (bIsLoaded)
	{
		for (UObject* ObjectLoaded : LoadedObjects)
		{
			OutLoadedEconomySystemGenerics.Add(Cast<UEMREconomySystemGenerics>(ObjectLoaded));
		}
	}

	return bIsLoaded;
}


/*******************************************/
/*****************SUBSYSTEMS****************/
/*******************************************/

void UEMRAssetManager::LoadSubsystemConfig(const FStreamableDelegate& LoadFinishedCallback)
{
	UE_LOG(LogTemp, Log, TEXT("[AssetManager] LoadSubsystemConfig requested."));
	LoadPrimaryAssetsWithType(UEMRSubsystemConfig::GetSubsystemConfigAssetType(), TArray<FName>(), FStreamableDelegate::CreateUObject(this, &UEMRAssetManager::OnSubsystemConfigLoadFinished, LoadFinishedCallback));
}


void UEMRAssetManager::OnSubsystemConfigLoadFinished(FStreamableDelegate Callback)
{
	LoadedSubsystemConfig.Reset();
	CollectLoadedSubsystemConfig(LoadedSubsystemConfig);
	UE_LOG(LogTemp, Log, TEXT("[AssetManager] OnSubsystemConfigLoadFinished loaded=%d"), LoadedSubsystemConfig.Num());
	if (LoadedSubsystemConfig.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AssetManager] No SubsystemConfig primary assets loaded."));
	}
	for (const UEMRSubsystemConfig* Config : LoadedSubsystemConfig)
	{
		if (Config)
		{
			Config->PreloadRuntimeAssets();
		}
	}
	Callback.ExecuteIfBound();
}


bool UEMRAssetManager::CollectLoadedSubsystemConfig(TArray<const UEMRSubsystemConfig*>& OutLoadedSubsystemConfig)
{
	TArray<UObject*> LoadedObjects;
	const bool bIsLoaded = GetPrimaryAssetObjectList(UEMRSubsystemConfig::GetSubsystemConfigAssetType(), LoadedObjects);

	if (bIsLoaded)
	{
		for (UObject* ObjectLoaded : LoadedObjects)
		{
			OutLoadedSubsystemConfig.Add(Cast<UEMRSubsystemConfig>(ObjectLoaded));
		}
	}

	return bIsLoaded;
}


/*******************************************/
/******************RUN RULES****************/
/*******************************************/

void UEMRAssetManager::LoadRunRulesData(const FStreamableDelegate& LoadFinishedCallback)
{
    LoadPrimaryAssetsWithType(UEMRRunRulesData::GetRunRulesAssetType(), TArray<FName>(), FStreamableDelegate::CreateUObject(this, &ThisClass::OnRunRulesDataLoadFinished, LoadFinishedCallback));
}


void UEMRAssetManager::OnRunRulesDataLoadFinished(FStreamableDelegate Callback)
{
    LoadedRunRulesData.Reset();
    CollectLoadedRunRulesData(LoadedRunRulesData);
    Callback.ExecuteIfBound();
}


bool UEMRAssetManager::CollectLoadedRunRulesData(TArray<const UEMRRunRulesData*>& OutLoadedRunRulesData)
{
    TArray<UObject*> LoadedObjects;
    const bool bIsLoaded = GetPrimaryAssetObjectList(UEMRRunRulesData::GetRunRulesAssetType(), LoadedObjects);

    if (bIsLoaded)
    {
        for (UObject* ObjectLoaded : LoadedObjects)
        {
            OutLoadedRunRulesData.Add(Cast<UEMRRunRulesData>(ObjectLoaded));
        }
    }

    return bIsLoaded;
}

