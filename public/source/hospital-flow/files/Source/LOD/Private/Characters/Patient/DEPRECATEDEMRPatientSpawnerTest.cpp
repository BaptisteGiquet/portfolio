#include "Characters/Patient/DEPRECATEDEMRPatientSpawnerTest.h"

#include "Characters/Patient/EMRPatient.h"
#include "Framework/EMRAssetManager.h"


void ADEPRECATEDEMRPatientSpawnerTest::BeginPlay()
{
    Super::BeginPlay();

   /* UEMRAssetManager::Get().LoadPatients(
    FStreamableDelegate::CreateUObject(this, &ADEPRECATEDEMRPatientSpawnerTest::OnPatientsDataLoaded)
    );*/
}


void ADEPRECATEDEMRPatientSpawnerTest::OnPatientsDataLoaded()
{
	if (!HasAuthority()) return;

   // CreatePatientPool(PoolSize);
}


void ADEPRECATEDEMRPatientSpawnerTest::CreatePatientPool(int32 InPoolSize)
{
    const FVector StorageLocation(-10000.f, -10000.f, -10000.f);

    for (int32 PatientNumber = 0; PatientNumber < InPoolSize; ++PatientNumber)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AEMRPatient* Patient = GetWorld()->SpawnActor<AEMRPatient>(
            PatientClass, StorageLocation, FRotator::ZeroRotator, SpawnParams
        );

        if (Patient)
        {
            Patient->SetActorHiddenInGame(true);
            Patient->SetActorEnableCollision(false);
            Patient->SetActorTickEnabled(false);

            PatientPool.Add(Patient);
            AvailablePatients.Add(Patient);
        }
    }
}


AEMRPatient* ADEPRECATEDEMRPatientSpawnerTest::SpawnRandomPatient(const FVector& Location)
{
	if (!HasAuthority()) return nullptr;
	
    const UEMRPatientData* Data = UEMRAssetManager::Get().GetRandomPatient();
    if (!Data)
    {
        return nullptr;
    }

    AEMRPatient* Patient = GetPatientFromPool();
    if (!Patient)
    {
        return nullptr;
    }

    ActivatePatient(Patient, Data, Location);
    return Patient;
}


AEMRPatient* ADEPRECATEDEMRPatientSpawnerTest::GetPatientFromPool()
{
    if (AvailablePatients.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Spawner] No patients available"));
        return nullptr;
    }

    AEMRPatient* Patient = AvailablePatients[0];
    AvailablePatients.RemoveAt(0);
    return Patient;
}


void ADEPRECATEDEMRPatientSpawnerTest::ActivatePatient(AEMRPatient* Patient, const UEMRPatientData* Data, const FVector& Location)
{
    Patient->SetActorLocation(Location);
    Patient->SetActorHiddenInGame(false);
    Patient->SetActorEnableCollision(true);
    Patient->SetActorTickEnabled(true);

    Patient->InitializeFromPatientData(Data);
}


void ADEPRECATEDEMRPatientSpawnerTest::ReturnPatientToPool(AEMRPatient* Patient)
{
    if (!Patient || !PatientPool.Contains(Patient))
    {
        return;
    }

    const FVector StorageLocation(-10000.f, -10000.f, -10000.f);
    Patient->SetActorLocation(StorageLocation);
    Patient->SetActorHiddenInGame(true);
    Patient->SetActorEnableCollision(false);
    Patient->SetActorTickEnabled(false);

    AvailablePatients.Add(Patient);
}
