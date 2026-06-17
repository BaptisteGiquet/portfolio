#include "UI/Machines/Triage/EMRTriagePatientCard.h"

#include "AnalogSlider.h"
#include "AbilitySystemComponent.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRPatientData.h"
#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "GAS/EMRTags.h"
#include "Styling/SlateBrush.h"
#include "Subsystems/EMRLocalizationSubsystem.h"
#include "UI/Patient/EMRPatientUIController.h"

void UEMRTriagePatientCard::InitializeCard(AEMRPatient* InPatient, UEMRPatientUIController* InUIController)
{
	if (!InPatient || !InUIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TriagePatientCard] InitializeCard called with invalid data"));
		return;
	}

	Cleanup();

	Patient = InPatient;
	UIController = InUIController;

	UIController->OnVitalStatsUpdated.AddDynamic(this, &ThisClass::HandleVitalStatsUpdated);
	UIController->StartMonitoring();

	HandleVitalStatsUpdated(UIController->GetPatientUIData());
	UpdateSeverityBorder();
}

void UEMRTriagePatientCard::SetLabAnalyzerWaitingPathologyOverride(bool bEnabled, const FText& InText)
{
	bLabAnalyzerWaitingPathologyOverrideEnabled = bEnabled;
	LabAnalyzerWaitingPathologyOverrideText = InText;
}

void UEMRTriagePatientCard::ResetCard()
{
	Cleanup();
}

void UEMRTriagePatientCard::NativeDestruct()
{
	Cleanup();
	Super::NativeDestruct();
}

void UEMRTriagePatientCard::Cleanup()
{
	if (UIController)
	{
		UIController->OnVitalStatsUpdated.RemoveDynamic(this, &ThisClass::HandleVitalStatsUpdated);
		UIController->StopMonitoring();
		UIController = nullptr;
	}

	Patient.Reset();
	bIsShowingUnregisteredState = false;

	if (Border_CardColor)
	{
		Border_CardColor->SetBrushColor(FLinearColor::Transparent);
	}

	if (CommonLazyImage_Portrait)
	{
		CommonLazyImage_Portrait->SetBrush(FSlateBrush());
	}

	if (CommonTextBlock_Name)
	{
		CommonTextBlock_Name->SetText(FText::GetEmpty());
	}

	if (CommonTextBlock_Age)
	{
		CommonTextBlock_Age->SetText(FText::GetEmpty());
	}

	if (CommonTextBlock_BloodGroup)
	{
		CommonTextBlock_BloodGroup->SetText(FText::GetEmpty());
	}

	if (CommonTextBlock_Symptoms)
	{
		CommonTextBlock_Symptoms->SetText(FText::GetEmpty());
	}

	if (CommonTextBlock_Pathologies)
	{
		CommonTextBlock_Pathologies->SetText(FText::GetEmpty());
	}

	SetPatienceLevelVisual(0.0f);
}

bool UEMRTriagePatientCard::ShouldShowLabAnalyzerWaitingPathologyOverride() const
{
	if (!bLabAnalyzerWaitingPathologyOverrideEnabled)
	{
		return false;
	}

	const AEMRPatient* PatientPtr = Patient.Get();
	if (!PatientPtr)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = PatientPtr->GetAbilitySystemComponent();
	if (!ASC || !ASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::WaitingExam))
	{
		return false;
	}

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	bool bHasLabAnalyzerQueueTag = false;
	bool bHasOtherExamQueueTag = false;

	TArray<FGameplayTag> OwnedTagArray;
	OwnedTags.GetGameplayTagArray(OwnedTagArray);
	for (const FGameplayTag& OwnedTag : OwnedTagArray)
	{
		if (!OwnedTag.IsValid() || !OwnedTag.MatchesTag(EMRTags::Patient::ExamQueue::Root))
		{
			continue;
		}

		if (OwnedTag.MatchesTagExact(EMRTags::Patient::ExamQueue::Root))
		{
			continue;
		}

		if (OwnedTag.MatchesTag(EMRTags::Patient::ExamQueue::WaitingForLabAnalyzer))
		{
			bHasLabAnalyzerQueueTag = true;
		}
		else
		{
			bHasOtherExamQueueTag = true;
		}
	}

	return bHasLabAnalyzerQueueTag && !bHasOtherExamQueueTag;
}

FText UEMRTriagePatientCard::GetLabAnalyzerWaitingPathologyText() const
{
	if (!LabAnalyzerWaitingPathologyOverrideText.IsEmpty())
	{
		return LabAnalyzerWaitingPathologyOverrideText;
	}

	return FText::FromString(TEXT("Waiting for Lab Analyzer"));
}

void UEMRTriagePatientCard::SetPatienceLevelVisual(float InNormalizedValue)
{
	const float ClampedValue = FMath::Clamp(InNormalizedValue, 0.0f, 1.0f);

	if (ProgressBar_PatienceLevel)
	{
		ProgressBar_PatienceLevel->SetPercent(ClampedValue);
	}

	if (AnalogSlider_PatienceLevel)
	{
		AnalogSlider_PatienceLevel->SetValue(ClampedValue);
	}
}

void UEMRTriagePatientCard::UpdateSeverityBorder()
{
	if (!Border_CardColor)
	{
		return;
	}

	FLinearColor BorderColor = FLinearColor::Transparent;

	if (bIsShowingUnregisteredState)
	{
		Border_CardColor->SetBrushColor(BorderColor);
		return;
	}

	if (const AEMRPatient* PatientPtr = Patient.Get())
	{
		if (const UEMRPatientData* PatientData = PatientPtr->GetPatientData())
		{
			const FGameplayTag SeverityTag = PatientData->GetSeverity();

			if (SeverityTag.MatchesTag(EMRTags::Patient::Severity::Red))
			{
				BorderColor = SeverityRedColor;
			}
			else if (SeverityTag.MatchesTag(EMRTags::Patient::Severity::Yellow))
			{
				BorderColor = SeverityYellowColor;
			}
			else if (SeverityTag.MatchesTag(EMRTags::Patient::Severity::Green))
			{
				BorderColor = SeverityGreenColor;
			}
			else if (SeverityTag.MatchesTag(EMRTags::Patient::Severity::Black))
			{
				BorderColor = SeverityBlackColor;
			}
		}
	}

	Border_CardColor->SetBrushColor(BorderColor);
}

void UEMRTriagePatientCard::HandleVitalStatsUpdated(const FEMRPatientUIData& UIData)
{
	if (!UIData.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TriagePatientCard] Received invalid UIData"));
		return;
	}

	const bool bShowLabAnalyzerWaitingPathology = ShouldShowLabAnalyzerWaitingPathologyOverride();
	const FText LabAnalyzerWaitingPathologyText = bShowLabAnalyzerWaitingPathology
	? GetLabAnalyzerWaitingPathologyText()
	: FText::GetEmpty();

	if (!UIData.bFolderDisplayRegistered)
	{
		bIsShowingUnregisteredState = true;
		const FText HiddenLabel = NotRegisteredText.IsEmpty() ? FText::FromString(TEXT("Not Registered")) : NotRegisteredText;

		if (CommonLazyImage_Portrait)
		{
			CommonLazyImage_Portrait->SetBrush(FSlateBrush());
		}

		if (CommonTextBlock_Name)
		{
			CommonTextBlock_Name->SetText(HiddenLabel);
		}

		if (CommonTextBlock_Age)
		{
			CommonTextBlock_Age->SetText(FText::GetEmpty());
		}

		if (CommonTextBlock_BloodGroup)
		{
			CommonTextBlock_BloodGroup->SetText(FText::GetEmpty());
		}

		if (CommonTextBlock_Symptoms)
		{
			CommonTextBlock_Symptoms->SetText(FText::GetEmpty());
		}

		if (CommonTextBlock_Pathologies)
		{
			CommonTextBlock_Pathologies->SetText(LabAnalyzerWaitingPathologyText);
		}

		SetPatienceLevelVisual(0.0f);

		UpdateSeverityBorder();
		return;
	}

	bIsShowingUnregisteredState = false;
	UEMRLocalizationSubsystem* LocalizationSubsystem = GetGameInstance()->GetSubsystem<UEMRLocalizationSubsystem>();
	if (!LocalizationSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[TriagePatientCard] LocalizationSubsystem not available"));
		return;
	}

	if (CommonLazyImage_Portrait)
	{
		if (UIData.Portrait.IsNull())
		{
			CommonLazyImage_Portrait->SetBrush(FSlateBrush());
		}
		else
		{
			CommonLazyImage_Portrait->SetBrushFromLazyTexture(UIData.Portrait);
		}
	}

	if (CommonTextBlock_Name)
	{
		CommonTextBlock_Name->SetText(UIData.FullName);
	}

	if (CommonTextBlock_Age)
	{
		CommonTextBlock_Age->SetText(FText::AsNumber(UIData.Age));
	}

	if (CommonTextBlock_BloodGroup)
	{
		CommonTextBlock_BloodGroup->SetText(LocalizationSubsystem->GetLocalizedTag(UIData.BloodType));
	}

	if (CommonTextBlock_Symptoms)
	{
		TArray<FGameplayTag> SymptomsArray;
		UIData.Symptoms.GetGameplayTagArray(SymptomsArray);
		CommonTextBlock_Symptoms->SetText(LocalizationSubsystem->GetLocalizedList(SymptomsArray));
	}

	if (CommonTextBlock_Pathologies)
	{
		if (bShowLabAnalyzerWaitingPathology)
		{
			CommonTextBlock_Pathologies->SetText(LabAnalyzerWaitingPathologyText);
		}
		else
		{
			TArray<FGameplayTag> PathologiesArray;
			UIData.Pathologies.GetGameplayTagArray(PathologiesArray);
			CommonTextBlock_Pathologies->SetText(LocalizationSubsystem->GetLocalizedList(PathologiesArray));
		}
	}

	SetPatienceLevelVisual(UIData.GetPatienceRatio());

	UpdateSeverityBorder();
}
