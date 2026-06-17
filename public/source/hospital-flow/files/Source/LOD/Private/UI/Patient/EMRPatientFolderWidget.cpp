
#include "UI/Patient/EMRPatientFolderWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Subsystems/EMRLocalizationSubsystem.h"
#include "UI/Patient/EMRPatientUIController.h"


void UEMRPatientFolderWidget::NativeConstruct()
{
    Super::NativeConstruct();
}


void UEMRPatientFolderWidget::NativeDestruct()
{
    Cleanup();
    Super::NativeDestruct();
}


void UEMRPatientFolderWidget::InitializePatientFolder(UEMRPatientUIController* InUIController)
{
    if (!InUIController)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PatientFolder] InitializePatientFolder called with null UIController"));
        return;
    }

    // Cleanup previous bindings if any
    Cleanup();

    UIController = InUIController;

    // Bind to UIController events
    UIController->OnVitalStatsUpdated.AddDynamic(this, &UEMRPatientFolderWidget::OnVitalStatsUpdated);

    // Initial update
    FEMRPatientUIData InitialData = UIController->GetPatientUIData();
    OnVitalStatsUpdated(InitialData);
}


void UEMRPatientFolderWidget::Cleanup()
{
    if (UIController)
    {
        UIController->OnVitalStatsUpdated.RemoveDynamic(this, &UEMRPatientFolderWidget::OnVitalStatsUpdated);
        UIController = nullptr;
    }
}


void UEMRPatientFolderWidget::OnVitalStatsUpdated(const FEMRPatientUIData& UIData)
{
    if (!UIData.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[PatientFolder] Received invalid UIData"));
        return;
    }

    if (!UIData.bFolderDisplayRegistered)
    {
        const FText HiddenLabel = NotRegisteredText.IsEmpty() ? FText::FromString(TEXT("Not Registered")) : NotRegisteredText;

        if (Text_FullName)
        {
            Text_FullName->SetText(HiddenLabel);
        }

        if (Text_Age)
        {
            Text_Age->SetText(FText::GetEmpty());
        }

        if (Text_BloodType)
        {
            Text_BloodType->SetText(FText::GetEmpty());
        }

        if (Text_PatientType)
        {
            Text_PatientType->SetText(FText::GetEmpty());
        }

        if (Text_Pathologies)
        {
            Text_Pathologies->SetText(FText::GetEmpty());
        }

        if (Text_Symptoms)
        {
            Text_Symptoms->SetText(FText::GetEmpty());
        }

        if (Text_HeartRate)
        {
            Text_HeartRate->SetText(FText::GetEmpty());
        }

        if (Text_BloodPressure)
        {
            Text_BloodPressure->SetText(FText::GetEmpty());
        }

        if (Text_OxygenSaturation)
        {
            Text_OxygenSaturation->SetText(FText::GetEmpty());
        }

        if (Text_RespiratoryRate)
        {
            Text_RespiratoryRate->SetText(FText::GetEmpty());
        }

        if (Text_Temperature)
        {
            Text_Temperature->SetText(FText::GetEmpty());
        }

        if (Text_GlasgowScore)
        {
            Text_GlasgowScore->SetText(FText::GetEmpty());
        }

        if (ProgressBar_Patience)
        {
            ProgressBar_Patience->SetPercent(0.0f);
        }

        FEMRPatientUIData HiddenUIData = UIData;
        HiddenUIData.FullName = HiddenLabel;
        HiddenUIData.Age = 0;
        HiddenUIData.BloodType = FGameplayTag::EmptyTag;
        HiddenUIData.PatientType = FGameplayTag::EmptyTag;
        HiddenUIData.Portrait.Reset();
        HiddenUIData.Pathologies.Reset();
        HiddenUIData.Symptoms.Reset();
        HiddenUIData.CurrentPhase = FGameplayTag::EmptyTag;
        HiddenUIData.HeartRate = 0.0f;
        HiddenUIData.BloodPressureSystolic = 0.0f;
        HiddenUIData.BloodPressureDiastolic = 0.0f;
        HiddenUIData.OxygenSaturation = 0.0f;
        HiddenUIData.RespiratoryRate = 0.0f;
        HiddenUIData.Temperature = 0.0f;
        HiddenUIData.GlasgowScore = 0.0f;
        HiddenUIData.Patience = 0.0f;
        HiddenUIData.MaxPatience = 0.0f;

        // Call optional Blueprint hook with scrubbed data to avoid leaking hidden info.
        BP_OnPostVitalStatsUpdated(HiddenUIData);
        return;
    }
	
	
    UEMRLocalizationSubsystem* LocalizationSubsystem = GetGameInstance()->GetSubsystem<UEMRLocalizationSubsystem>();
    if (!LocalizationSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("[PatientFolder] LocalizationSubsystem not available"));
        return;
    }

    // Identity
    if (Text_FullName)
    {
        Text_FullName->SetText(UIData.FullName);
    }

    if (Text_Age)
    {
        Text_Age->SetText(FText::AsNumber(UIData.Age));
    }

    if (Text_BloodType)
    {
        Text_BloodType->SetText(LocalizationSubsystem->GetLocalizedTag(UIData.BloodType));
    }

    if (Text_PatientType)
    {
        Text_PatientType->SetText(LocalizationSubsystem->GetLocalizedTag(UIData.PatientType));
    }

 

    // Medical
    if (Text_Pathologies)
    {
    	TArray<FGameplayTag> PathologiesArray;
    	UIData.Pathologies.GetGameplayTagArray(PathologiesArray);
    	Text_Pathologies->SetText(LocalizationSubsystem->GetLocalizedList(PathologiesArray));
    }

    if (Text_Symptoms)
    {
    	TArray<FGameplayTag> SymptomsArray;
    	UIData.Symptoms.GetGameplayTagArray(SymptomsArray);
    	Text_Symptoms->SetText(LocalizationSubsystem->GetLocalizedList(SymptomsArray));
    }
	
	
    // Vital Stats
    if (Text_HeartRate)
    {
        Text_HeartRate->SetText(FText::AsNumber(FMath::RoundToInt(UIData.HeartRate)));
    }

    if (Text_BloodPressure)
    {
        Text_BloodPressure->SetText(UIData.GetFormattedBloodPressure());
    }

    if (Text_OxygenSaturation)
    {
        Text_OxygenSaturation->SetText(FText::AsPercent(UIData.OxygenSaturation / 100.0f));
    }

    if (Text_RespiratoryRate)
    {
        Text_RespiratoryRate->SetText(FText::AsNumber(FMath::RoundToInt(UIData.RespiratoryRate)));
    }

    if (Text_Temperature)
    {
        Text_Temperature->SetText(FText::Format(
            FText::FromString("{0}°C"),
            FText::AsNumber(UIData.Temperature, &FNumberFormattingOptions::DefaultNoGrouping())
        ));
    }

    if (Text_GlasgowScore)
    {
        Text_GlasgowScore->SetText(FText::AsNumber(FMath::RoundToInt(UIData.GlasgowScore)));
    }

    if (ProgressBar_Patience)
    {
        ProgressBar_Patience->SetPercent(UIData.GetPatienceRatio());
    }

    // Call optional Blueprint hook
    BP_OnPostVitalStatsUpdated(UIData);
}
