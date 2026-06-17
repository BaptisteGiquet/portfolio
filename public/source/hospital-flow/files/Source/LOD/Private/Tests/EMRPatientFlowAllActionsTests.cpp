#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/AutomationCommon.h"

#include "Framework/EMRAssetManager.h"
#include "Subsystems/EMRSubsystemConfig.h"
#include "Subsystems/EMRExamQueueSubsystem.h"
#include "Subsystems/EMRTreatmentSubsystem.h"
#include "Characters/Patient/EMRPatient.h"
#include "Characters/Patient/EMRPatientData.h"
#include "Interaction/EMRBaseMachine.h"
#include "Interaction/EMRTreatmentBed.h"
#include "Data/EMRExamRequirementsForPathologyMapping.h"
#include "Data/EMRExamQueueMapping.h"
#include "Data/EMRExamMachineCompletionMapping.h"
#include "Data/EMRTreatmentForPathologyMapping.h"
#include "Data/EMRTreatmentQueueMapping.h"
#include "GAS/EMRTags.h"

#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace EMRPatientFlowAllActionsTests
{
	static const TCHAR* Context = TEXT("EMRPatientFlowAllActionsTests");

	template <typename RowType>
	static TArray<const RowType*> GetRowsFromDataTable(UDataTable* DataTable)
	{
		TArray<const RowType*> Rows;
		if (!DataTable)
		{
			return Rows;
		}

		TArray<RowType*> MutableRows;
		DataTable->GetAllRows<RowType>(Context, MutableRows);
		Rows.Reserve(MutableRows.Num());
		for (const RowType* Row : MutableRows)
		{
			if (Row)
			{
				Rows.Add(Row);
			}
		}
		return Rows;
	}

	static bool LoadCoreAssets(
		FAutomationTestBase& Test,
		const UEMRSubsystemConfig*& OutSubsystemConfig,
		TArray<const UEMRPatientData*>& OutPatients)
	{
		OutSubsystemConfig = nullptr;
		OutPatients.Reset();

		UEMRAssetManager& AssetManager = UEMRAssetManager::Get();
		AssetManager.LoadSubsystemConfig(FStreamableDelegate());
		AssetManager.LoadPatients(FStreamableDelegate());
		AssetManager.LoadEconomySystemGenerics(FStreamableDelegate());
		FlushAsyncLoading();

		TArray<const UEMRSubsystemConfig*> LoadedConfigs;
		AssetManager.CollectLoadedSubsystemConfig(LoadedConfigs);
		if (LoadedConfigs.Num() == 0 || !LoadedConfigs[0])
		{
			Test.AddError(TEXT("No SubsystemConfig asset was loaded."));
			return false;
		}

		AssetManager.CollectLoadedPatients(OutPatients);
		if (OutPatients.Num() == 0)
		{
			Test.AddError(TEXT("No PatientData assets were loaded."));
			return false;
		}

		OutSubsystemConfig = LoadedConfigs[0];
		return true;
	}

	static TArray<FGameplayTag> ToSortedTagArray(const FGameplayTagContainer& Tags)
	{
		TArray<FGameplayTag> Result;
		for (const FGameplayTag& Tag : Tags)
		{
			if (Tag.IsValid())
			{
				Result.Add(Tag);
			}
		}

		Result.Sort([](const FGameplayTag& A, const FGameplayTag& B)
		{
			return A.ToString() < B.ToString();
		});
		return Result;
	}

	static FString JoinTags(const TArray<FGameplayTag>& Tags)
	{
		TArray<FString> AsStrings;
		AsStrings.Reserve(Tags.Num());
		for (const FGameplayTag& Tag : Tags)
		{
			AsStrings.Add(Tag.ToString());
		}
		return FString::Join(AsStrings, TEXT(", "));
	}

	static const UEMRPatientData* SelectPatientForPathology(
		const TArray<const UEMRPatientData*>& Patients,
		const FGameplayTag& Pathology)
	{
		const UEMRPatientData* Fallback = nullptr;
		for (const UEMRPatientData* PatientData : Patients)
		{
			if (!PatientData)
			{
				continue;
			}

			const FGameplayTagContainer Pathologies = PatientData->GetPathologies();
			if (!Pathologies.HasTagExact(Pathology))
			{
				continue;
			}

			const int32 NumPathologies = Pathologies.Num();
			if (NumPathologies == 1)
			{
				return PatientData;
			}

			if (!Fallback)
			{
				Fallback = PatientData;
			}
		}

		return Fallback;
	}

	static FGameplayTagContainer GatherRequiredExamsForPatient(
		const UEMRPatientData* PatientData,
		const TMap<FGameplayTag, FGameplayTagContainer>& ExamsByPathology)
	{
		FGameplayTagContainer RequiredExams;
		if (!PatientData)
		{
			return RequiredExams;
		}

		for (const FGameplayTag& Pathology : PatientData->GetPathologies())
		{
			if (const FGameplayTagContainer* Exams = ExamsByPathology.Find(Pathology))
			{
				RequiredExams.AppendTags(*Exams);
			}
		}

		RequiredExams.RemoveTag(EMRTags::Abilities::Exam::None);
		return RequiredExams;
	}

	static FGameplayTagContainer GatherRequiredExamsForPathology(
		const FGameplayTag& Pathology,
		const TMap<FGameplayTag, FGameplayTagContainer>& ExamsByPathology)
	{
		FGameplayTagContainer RequiredExams;
		if (const FGameplayTagContainer* Exams = ExamsByPathology.Find(Pathology))
		{
			RequiredExams.AppendTags(*Exams);
		}

		RequiredExams.RemoveTag(EMRTags::Abilities::Exam::None);
		return RequiredExams;
	}

	static FGameplayTagContainer GatherRequiredTreatmentsForPatient(
		const UEMRPatientData* PatientData,
		const TMap<FGameplayTag, FGameplayTagContainer>& TreatmentsByPathology)
	{
		FGameplayTagContainer RequiredTreatments;
		if (!PatientData)
		{
			return RequiredTreatments;
		}

		for (const FGameplayTag& Pathology : PatientData->GetPathologies())
		{
			if (const FGameplayTagContainer* Treatments = TreatmentsByPathology.Find(Pathology))
			{
				RequiredTreatments.AppendTags(*Treatments);
			}
		}

		return RequiredTreatments;
	}

	static FGameplayTagContainer GatherRequiredTreatmentsForPathology(
		const FGameplayTag& Pathology,
		const TMap<FGameplayTag, FGameplayTagContainer>& TreatmentsByPathology)
	{
		FGameplayTagContainer RequiredTreatments;
		if (const FGameplayTagContainer* Treatments = TreatmentsByPathology.Find(Pathology))
		{
			RequiredTreatments.AppendTags(*Treatments);
		}
		return RequiredTreatments;
	}

	static const UEMRPatientData* GetAnyPatientData(const TArray<const UEMRPatientData*>& Patients)
	{
		for (const UEMRPatientData* PatientData : Patients)
		{
			if (PatientData)
			{
				return PatientData;
			}
		}
		return nullptr;
	}

	static UEMRPatientData* CreateSinglePathologyPatientData(
		const UEMRPatientData* TemplatePatientData,
		const FGameplayTag& Pathology,
		FAutomationTestBase& Test)
	{
		if (!TemplatePatientData || !Pathology.IsValid())
		{
			return nullptr;
		}

		UEMRPatientData* SyntheticData = DuplicateObject<UEMRPatientData>(
			const_cast<UEMRPatientData*>(TemplatePatientData),
			GetTransientPackage());
		if (!SyntheticData)
		{
			Test.AddError(FString::Printf(TEXT("Failed to duplicate patient data for pathology '%s'."), *Pathology.ToString()));
			return nullptr;
		}

		SyntheticData->SetFlags(RF_Transient);
		FProperty* PathologiesProperty = UEMRPatientData::StaticClass()->FindPropertyByName(TEXT("Pathologies"));
		FStructProperty* PathologiesStructProperty = CastField<FStructProperty>(PathologiesProperty);
		if (!PathologiesStructProperty)
		{
			Test.AddError(TEXT("UEMRPatientData.Pathologies property not found via reflection."));
			return nullptr;
		}

		FGameplayTagContainer* PathologiesValue = PathologiesStructProperty->ContainerPtrToValuePtr<FGameplayTagContainer>(SyntheticData);
		if (!PathologiesValue)
		{
			Test.AddError(TEXT("Could not access synthetic patient pathologies container."));
			return nullptr;
		}

		PathologiesValue->Reset();
		PathologiesValue->AddTag(Pathology);
		return SyntheticData;
	}

	static bool HasLabAnalyzerRootQueueMapping(const TArray<const FEMRExamQueueMapping*>& QueueMappings)
	{
		for (const FEMRExamQueueMapping* Mapping : QueueMappings)
		{
			if (!Mapping || !Mapping->ExamTag.IsValid() || !Mapping->QueueTag.IsValid() || !Mapping->MachineTypeTag.IsValid())
			{
				continue;
			}

			if (Mapping->ExamTag.MatchesTagExact(EMRTags::Abilities::Exam::LabAnalyzer::Root))
			{
				return true;
			}
		}

		return false;
	}

	static bool HasLabAnalyzerRootCompletionMapping(const TArray<const FEMRExamMachineCompletionMapping*>& CompletionMappings)
	{
		for (const FEMRExamMachineCompletionMapping* Mapping : CompletionMappings)
		{
			if (!Mapping || !Mapping->ExamTag.IsValid() || !Mapping->CompletionTag.IsValid())
			{
				continue;
			}

			const bool bRootMatch = Mapping->ExamTag.MatchesTagExact(EMRTags::Abilities::Exam::LabAnalyzer::Root);
			if (!bRootMatch)
			{
				continue;
			}

			if (Mapping->bMatchExamByHierarchy || Mapping->bMatchCompletionByHierarchy)
			{
				return true;
			}
		}

		return false;
	}

	static bool HasExamQueueMapping(
		const FGameplayTag& ExamTag,
		const TArray<const FEMRExamQueueMapping*>& QueueMappings)
	{
		for (const FEMRExamQueueMapping* Mapping : QueueMappings)
		{
			if (!Mapping || !Mapping->ExamTag.IsValid() || !Mapping->QueueTag.IsValid() || !Mapping->MachineTypeTag.IsValid())
			{
				continue;
			}

			const bool bMatch = Mapping->bMatchByHierarchy
				? ExamTag.MatchesTag(Mapping->ExamTag)
				: ExamTag.MatchesTagExact(Mapping->ExamTag);
			if (bMatch)
			{
				return true;
			}
		}

		return false;
	}

	static bool HasExamCompletionMapping(
		const FGameplayTag& ExamTag,
		const TArray<const FEMRExamMachineCompletionMapping*>& CompletionMappings)
	{
		for (const FEMRExamMachineCompletionMapping* Mapping : CompletionMappings)
		{
			if (!Mapping || !Mapping->ExamTag.IsValid() || !Mapping->CompletionTag.IsValid())
			{
				continue;
			}

			const bool bMatch = Mapping->bMatchExamByHierarchy
				? ExamTag.MatchesTag(Mapping->ExamTag)
				: ExamTag.MatchesTagExact(Mapping->ExamTag);
			if (bMatch)
			{
				return true;
			}
		}

		return false;
	}

	static bool HasExactTreatmentQueueMapping(
		const FGameplayTag& TreatmentTag,
		const TArray<const FEMRTreatmentQueueMapping*>& QueueMappings)
	{
		for (const FEMRTreatmentQueueMapping* Mapping : QueueMappings)
		{
			if (!Mapping || !Mapping->TreatmentTag.IsValid() || !Mapping->QueueTag.IsValid())
			{
				continue;
			}

			if (TreatmentTag.MatchesTagExact(Mapping->TreatmentTag))
			{
				return true;
			}
		}

		return false;
	}

	static AEMRPatient* SpawnAndInitializePatient(UWorld* World, const UEMRPatientData* PatientData, FAutomationTestBase& Test, const FVector& SpawnLocation = FVector::ZeroVector)
	{
		if (!World || !PatientData)
		{
			return nullptr;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags = RF_Transient;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AEMRPatient* Patient = World->SpawnActor<AEMRPatient>(AEMRPatient::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
		if (!Patient)
		{
			Test.AddError(TEXT("Failed to spawn AEMRPatient."));
			return nullptr;
		}

		Patient->InitializeFromPatientData(PatientData);
		return Patient;
	}

	static bool SetMachineTypeTagOnMachine(AEMRBaseMachine* Machine, const FGameplayTag& MachineTag, FAutomationTestBase& Test)
	{
		if (!Machine || !MachineTag.IsValid())
		{
			return false;
		}

		FProperty* MachineTypeTagProperty = AEMRBaseMachine::StaticClass()->FindPropertyByName(TEXT("MachineTypeTag"));
		FStructProperty* MachineTypeTagStructProperty = CastField<FStructProperty>(MachineTypeTagProperty);
		if (!MachineTypeTagStructProperty)
		{
			Test.AddError(TEXT("AEMRBaseMachine.MachineTypeTag property was not found through reflection."));
			return false;
		}

		FGameplayTag* MachineTypeTagValue = MachineTypeTagStructProperty->ContainerPtrToValuePtr<FGameplayTag>(Machine);
		if (!MachineTypeTagValue)
		{
			Test.AddError(TEXT("Could not access AEMRBaseMachine.MachineTypeTag value."));
			return false;
		}

		*MachineTypeTagValue = MachineTag;
		return true;
	}

	static AEMRBaseMachine* SpawnMachineWithTag(
		UWorld* World,
		const FGameplayTag& MachineTag,
		const FVector& SpawnLocation,
		FAutomationTestBase& Test)
	{
		if (!World || !MachineTag.IsValid())
		{
			return nullptr;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags = RF_Transient;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AEMRBaseMachine* Machine = World->SpawnActor<AEMRBaseMachine>(AEMRBaseMachine::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
		if (!Machine)
		{
			Test.AddError(TEXT("Failed to spawn test machine actor."));
			return nullptr;
		}

		if (!SetMachineTypeTagOnMachine(Machine, MachineTag, Test))
		{
			Machine->Destroy();
			return nullptr;
		}

		return Machine;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEMRPatientFlowAllActionsMappingCoverageTest,
	"Project.PatientFlow.AllActions.MappingCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRPatientFlowAllActionsMappingCoverageTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("String Table not found"), EAutomationExpectedErrorFlags::Contains, 1);

	const UEMRSubsystemConfig* SubsystemConfig = nullptr;
	TArray<const UEMRPatientData*> Patients;
	if (!EMRPatientFlowAllActionsTests::LoadCoreAssets(*this, SubsystemConfig, Patients))
	{
		return false;
	}

	UDataTable* ExamRequirementsTable = SubsystemConfig->GetExamRequirementsFromPathologyTable();
	UDataTable* ExamQueueTable = SubsystemConfig->GetExamQueueMappingTable();
	UDataTable* ExamCompletionTable = SubsystemConfig->GetExamCompletionMappingTable();
	UDataTable* TreatmentsForPathologyTable = SubsystemConfig->GetTreatmentsFromPathologyMappingTable();
	UDataTable* TreatmentQueueTable = SubsystemConfig->GetTreatmentQueueMappingTable();

	if (!ExamRequirementsTable || !ExamQueueTable || !ExamCompletionTable || !TreatmentsForPathologyTable || !TreatmentQueueTable)
	{
		AddError(TEXT("One or more required subsystem data tables are missing from SubsystemConfig."));
		return false;
	}

	const TArray<const FEMRExamRequirementsForPathologyMapping*> ExamRequirementsRows =
	EMRPatientFlowAllActionsTests::GetRowsFromDataTable<FEMRExamRequirementsForPathologyMapping>(ExamRequirementsTable);
	const TArray<const FEMRExamQueueMapping*> ExamQueueRows =
	EMRPatientFlowAllActionsTests::GetRowsFromDataTable<FEMRExamQueueMapping>(ExamQueueTable);
	const TArray<const FEMRExamMachineCompletionMapping*> ExamCompletionRows =
	EMRPatientFlowAllActionsTests::GetRowsFromDataTable<FEMRExamMachineCompletionMapping>(ExamCompletionTable);
	const TArray<const FEMRTreatmentForPathologyMapping*> TreatmentRows =
	EMRPatientFlowAllActionsTests::GetRowsFromDataTable<FEMRTreatmentForPathologyMapping>(TreatmentsForPathologyTable);
	const TArray<const FEMRTreatmentQueueMapping*> TreatmentQueueRows =
	EMRPatientFlowAllActionsTests::GetRowsFromDataTable<FEMRTreatmentQueueMapping>(TreatmentQueueTable);

	FGameplayTagContainer AllExamTags;
	FGameplayTagContainer AllTreatmentTags;
	FGameplayTagContainer AllMappedPathologies;
	for (const FEMRExamRequirementsForPathologyMapping* Row : ExamRequirementsRows)
	{
		if (!Row || !Row->Pathology.IsValid())
		{
			continue;
		}

		AllMappedPathologies.AddTag(Row->Pathology);
		AllExamTags.AppendTags(Row->RequiredExams);
	}

	AllExamTags.RemoveTag(EMRTags::Abilities::Exam::None);

	for (const FEMRTreatmentForPathologyMapping* Row : TreatmentRows)
	{
		if (!Row || !Row->Pathology.IsValid())
		{
			continue;
		}

		AllMappedPathologies.AddTag(Row->Pathology);
		AllTreatmentTags.AppendTags(Row->Treatments);
	}

	for (const FGameplayTag& ExamTag : AllExamTags)
	{
		const bool bHasQueue = EMRPatientFlowAllActionsTests::HasExamQueueMapping(ExamTag, ExamQueueRows);
		TestTrue(FString::Printf(TEXT("Exam '%s' must have an exam queue mapping"), *ExamTag.ToString()), bHasQueue);

		const bool bHasCompletion = EMRPatientFlowAllActionsTests::HasExamCompletionMapping(ExamTag, ExamCompletionRows);
		TestTrue(FString::Printf(TEXT("Exam '%s' must have an exam completion mapping"), *ExamTag.ToString()), bHasCompletion);
	}

	for (const FGameplayTag& TreatmentTag : AllTreatmentTags)
	{
		const bool bHasQueue = EMRPatientFlowAllActionsTests::HasExactTreatmentQueueMapping(TreatmentTag, TreatmentQueueRows);
		TestTrue(FString::Printf(TEXT("Treatment '%s' must have an exact treatment queue mapping"), *TreatmentTag.ToString()), bHasQueue);
	}

	const bool bHasLabQueueRootMapping = EMRPatientFlowAllActionsTests::HasLabAnalyzerRootQueueMapping(ExamQueueRows);
	const bool bHasLabCompletionRootMapping = EMRPatientFlowAllActionsTests::HasLabAnalyzerRootCompletionMapping(ExamCompletionRows);
	TestTrue(TEXT("Lab analyzer queue root mapping must exist (or all children must be mapped explicitly)."), bHasLabQueueRootMapping);
	TestTrue(TEXT("Lab analyzer completion root mapping must support hierarchy matching."), bHasLabCompletionRootMapping);

	int32 MissingPatientDataCount = 0;
	for (const FGameplayTag& Pathology : AllMappedPathologies)
	{
		const UEMRPatientData* MatchingPatientData = EMRPatientFlowAllActionsTests::SelectPatientForPathology(Patients, Pathology);
		if (!MatchingPatientData)
		{
			++MissingPatientDataCount;
			AddWarning(FString::Printf(TEXT("Pathology '%s' has no authored patient asset. End-to-end test will use synthetic data."), *Pathology.ToString()));
		}
	}
	AddInfo(FString::Printf(TEXT("Pathologies without authored patient data: %d"), MissingPatientDataCount));

	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEMRPatientFlowAllActionsEndToEndTest,
	"Project.PatientFlow.AllActions.EndToEndAllMappedPathologies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRPatientFlowAllActionsEndToEndTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("String Table not found"), EAutomationExpectedErrorFlags::Contains, 1);

	const UEMRSubsystemConfig* SubsystemConfig = nullptr;
	TArray<const UEMRPatientData*> Patients;
	if (!EMRPatientFlowAllActionsTests::LoadCoreAssets(*this, SubsystemConfig, Patients))
	{
		return false;
	}

	UDataTable* ExamRequirementsTable = SubsystemConfig->GetExamRequirementsFromPathologyTable();
	UDataTable* TreatmentsForPathologyTable = SubsystemConfig->GetTreatmentsFromPathologyMappingTable();
	if (!ExamRequirementsTable || !TreatmentsForPathologyTable)
	{
		AddError(TEXT("Required mapping tables are missing from SubsystemConfig."));
		return false;
	}

	const TArray<const FEMRExamRequirementsForPathologyMapping*> ExamRequirementsRows =
	EMRPatientFlowAllActionsTests::GetRowsFromDataTable<FEMRExamRequirementsForPathologyMapping>(ExamRequirementsTable);
	const TArray<const FEMRTreatmentForPathologyMapping*> TreatmentRows =
	EMRPatientFlowAllActionsTests::GetRowsFromDataTable<FEMRTreatmentForPathologyMapping>(TreatmentsForPathologyTable);

	TMap<FGameplayTag, FGameplayTagContainer> ExamsByPathology;
	TMap<FGameplayTag, FGameplayTagContainer> TreatmentsByPathology;
	FGameplayTagContainer AllExamTags;
	FGameplayTagContainer AllTreatmentTags;

	for (const FEMRExamRequirementsForPathologyMapping* Row : ExamRequirementsRows)
	{
		if (!Row || !Row->Pathology.IsValid())
		{
			continue;
		}

		FGameplayTagContainer& Exams = ExamsByPathology.FindOrAdd(Row->Pathology);
		Exams.AppendTags(Row->RequiredExams);
		AllExamTags.AppendTags(Row->RequiredExams);
	}

	AllExamTags.RemoveTag(EMRTags::Abilities::Exam::None);

	for (const FEMRTreatmentForPathologyMapping* Row : TreatmentRows)
	{
		if (!Row || !Row->Pathology.IsValid())
		{
			continue;
		}

		FGameplayTagContainer& Treatments = TreatmentsByPathology.FindOrAdd(Row->Pathology);
		Treatments.AppendTags(Row->Treatments);
		AllTreatmentTags.AppendTags(Row->Treatments);
	}

	TArray<FGameplayTag> PathologiesToTest;
	for (const TPair<FGameplayTag, FGameplayTagContainer>& Pair : ExamsByPathology)
	{
		PathologiesToTest.AddUnique(Pair.Key);
	}
	for (const TPair<FGameplayTag, FGameplayTagContainer>& Pair : TreatmentsByPathology)
	{
		PathologiesToTest.AddUnique(Pair.Key);
	}

	PathologiesToTest.Sort([](const FGameplayTag& A, const FGameplayTag& B)
	{
		return A.ToString() < B.ToString();
	});

	FTestWorldWrapper WorldWrapper;
	if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
	{
		AddError(TEXT("Failed to create test world."));
		return false;
	}

	if (!WorldWrapper.BeginPlayInTestWorld())
	{
		WorldWrapper.ForwardErrorMessages(this);
		AddError(TEXT("Failed to begin play in test world."));
		return false;
	}

	UWorld* World = WorldWrapper.GetTestWorld();
	if (!World)
	{
		AddError(TEXT("Test world is null."));
		return false;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		AddError(TEXT("Test world has no game instance."));
		return false;
	}

	UEMRExamQueueSubsystem* ExamQueueSubsystem = GameInstance->GetSubsystem<UEMRExamQueueSubsystem>();
	UEMRTreatmentSubsystem* TreatmentSubsystem = GameInstance->GetSubsystem<UEMRTreatmentSubsystem>();
	if (!ExamQueueSubsystem || !TreatmentSubsystem)
	{
		AddError(TEXT("Required patient-flow subsystems are missing."));
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags = RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEMRTreatmentBed* Bed = World->SpawnActor<AEMRTreatmentBed>(AEMRTreatmentBed::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!Bed)
	{
		AddError(TEXT("Failed to spawn treatment bed required by treatment flow."));
		return false;
	}

	WorldWrapper.TickTestWorld();

	FGameplayTagContainer CoveredExamTags;
	FGameplayTagContainer CoveredTreatmentTags;
	const UEMRPatientData* FallbackTemplatePatientData = EMRPatientFlowAllActionsTests::GetAnyPatientData(Patients);
	if (!FallbackTemplatePatientData)
	{
		AddError(TEXT("No valid patient data template available."));
		return false;
	}

	for (const FGameplayTag& Pathology : PathologiesToTest)
	{
		const UEMRPatientData* PatientTemplateData = EMRPatientFlowAllActionsTests::SelectPatientForPathology(Patients, Pathology);
		if (!PatientTemplateData)
		{
			PatientTemplateData = FallbackTemplatePatientData;
			AddWarning(FString::Printf(TEXT("Pathology '%s' has no authored patient data. Using synthetic duplicate from '%s'."),
				*Pathology.ToString(),
				*GetNameSafe(PatientTemplateData)));
		}

		TStrongObjectPtr<UEMRPatientData> ScopedSyntheticData(
			EMRPatientFlowAllActionsTests::CreateSinglePathologyPatientData(PatientTemplateData, Pathology, *this));
		if (!ScopedSyntheticData.IsValid())
		{
			continue;
		}

		AEMRPatient* Patient = EMRPatientFlowAllActionsTests::SpawnAndInitializePatient(World, ScopedSyntheticData.Get(), *this);
		if (!Patient)
		{
			continue;
		}

		UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent();
		if (!PatientASC)
		{
			AddError(FString::Printf(TEXT("Patient '%s' has no ability system component."), *GetNameSafe(Patient)));
			Patient->Destroy();
			continue;
		}

		const FGameplayTagContainer RequiredExams = EMRPatientFlowAllActionsTests::GatherRequiredExamsForPathology(Pathology, ExamsByPathology);
		const FGameplayTagContainer RequiredTreatments = EMRPatientFlowAllActionsTests::GatherRequiredTreatmentsForPathology(Pathology, TreatmentsByPathology);
		const TArray<FGameplayTag> RequiredExamArray = EMRPatientFlowAllActionsTests::ToSortedTagArray(RequiredExams);
		const TArray<FGameplayTag> RequiredTreatmentArray = EMRPatientFlowAllActionsTests::ToSortedTagArray(RequiredTreatments);

		for (const FGameplayTag& ExamTag : RequiredExamArray)
		{
			CoveredExamTags.AddTag(ExamTag);
		}
		for (const FGameplayTag& TreatmentTag : RequiredTreatmentArray)
		{
			CoveredTreatmentTags.AddTag(TreatmentTag);
		}

		if (RequiredExamArray.Num() > 0)
		{
			ExamQueueSubsystem->AddPatientToExamQueue(Patient, RequiredExamArray);
			TestTrue(
				FString::Printf(TEXT("Patient '%s' should enter WaitingExam phase"), *GetNameSafe(Patient)),
				PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::WaitingExam));

			for (const FGameplayTag& ExamTag : RequiredExamArray)
			{
				ExamQueueSubsystem->CompleteExamForPatient(Patient, ExamTag, true);
			}
		}
		else
		{
			ExamQueueSubsystem->BypassExamQueueToTreatment(Patient);
		}

		WorldWrapper.TickTestWorld();

		for (const FGameplayTag& TreatmentTag : RequiredTreatmentArray)
		{
			TreatmentSubsystem->CompleteTreatmentForPatient(Patient, TreatmentTag, true);
		}

		WorldWrapper.TickTestWorld();

		TestTrue(
			FString::Printf(TEXT("Patient '%s' should reach Leaving phase after full treatment flow"), *GetNameSafe(Patient)),
			PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving));

		TestFalse(
			FString::Printf(TEXT("Patient '%s' should not keep pathology root tag after full treatment flow"), *GetNameSafe(Patient)),
			PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Pathology::Root));

		ExamQueueSubsystem->RemovePatientFromAllQueues(Patient);
		Patient->Destroy();
		WorldWrapper.TickTestWorld();
	}

	for (const FGameplayTag& ExamTag : AllExamTags)
	{
		TestTrue(
			FString::Printf(TEXT("Coverage must include exam '%s'"), *ExamTag.ToString()),
			CoveredExamTags.HasTagExact(ExamTag));
	}

	for (const FGameplayTag& TreatmentTag : AllTreatmentTags)
	{
		TestTrue(
			FString::Printf(TEXT("Coverage must include treatment '%s'"), *TreatmentTag.ToString()),
			CoveredTreatmentTags.HasTagExact(TreatmentTag));
	}

	const TArray<FGameplayTag> CoveredExamArray = EMRPatientFlowAllActionsTests::ToSortedTagArray(CoveredExamTags);
	const TArray<FGameplayTag> CoveredTreatmentArray = EMRPatientFlowAllActionsTests::ToSortedTagArray(CoveredTreatmentTags);
	AddInfo(FString::Printf(TEXT("Covered Exams (%d): %s"), CoveredExamArray.Num(), *EMRPatientFlowAllActionsTests::JoinTags(CoveredExamArray)));
	AddInfo(FString::Printf(TEXT("Covered Treatments (%d): %s"), CoveredTreatmentArray.Num(), *EMRPatientFlowAllActionsTests::JoinTags(CoveredTreatmentArray)));

	WorldWrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEMRPatientFlowAllActionsNegativeInvalidTreatmentTest,
	"Project.PatientFlow.AllActions.Negative.InvalidTreatmentNoExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRPatientFlowAllActionsNegativeInvalidTreatmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("String Table not found"), EAutomationExpectedErrorFlags::Contains, 1);

	const UEMRSubsystemConfig* SubsystemConfig = nullptr;
	TArray<const UEMRPatientData*> Patients;
	if (!EMRPatientFlowAllActionsTests::LoadCoreAssets(*this, SubsystemConfig, Patients))
	{
		return false;
	}

	UDataTable* TreatmentsForPathologyTable = SubsystemConfig->GetTreatmentsFromPathologyMappingTable();
	if (!TreatmentsForPathologyTable)
	{
		AddError(TEXT("Treatment mapping table is missing from SubsystemConfig."));
		return false;
	}

	const TArray<const FEMRTreatmentForPathologyMapping*> TreatmentRows =
	EMRPatientFlowAllActionsTests::GetRowsFromDataTable<FEMRTreatmentForPathologyMapping>(TreatmentsForPathologyTable);

	TMap<FGameplayTag, FGameplayTagContainer> TreatmentsByPathology;
	for (const FEMRTreatmentForPathologyMapping* Row : TreatmentRows)
	{
		if (!Row || !Row->Pathology.IsValid())
		{
			continue;
		}

		FGameplayTagContainer& Treatments = TreatmentsByPathology.FindOrAdd(Row->Pathology);
		Treatments.AppendTags(Row->Treatments);
	}

	const UEMRPatientData* SelectedPatientData = nullptr;
	FGameplayTag SelectedPathology = FGameplayTag::EmptyTag;
	for (const TPair<FGameplayTag, FGameplayTagContainer>& Pair : TreatmentsByPathology)
	{
		if (Pair.Value.IsEmpty())
		{
			continue;
		}

		SelectedPatientData = EMRPatientFlowAllActionsTests::SelectPatientForPathology(Patients, Pair.Key);
		if (SelectedPatientData)
		{
			SelectedPathology = Pair.Key;
			break;
		}
	}

	const UEMRPatientData* FallbackTemplatePatientData = EMRPatientFlowAllActionsTests::GetAnyPatientData(Patients);
	if (!SelectedPatientData)
	{
		SelectedPatientData = FallbackTemplatePatientData;
		for (const TPair<FGameplayTag, FGameplayTagContainer>& Pair : TreatmentsByPathology)
		{
			if (!Pair.Value.IsEmpty())
			{
				SelectedPathology = Pair.Key;
				break;
			}
		}
	}
	if (!SelectedPatientData || !SelectedPathology.IsValid())
	{
		AddError(TEXT("Could not resolve patient template/pathology for negative treatment test."));
		return false;
	}

	FTestWorldWrapper WorldWrapper;
	if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
	{
		WorldWrapper.ForwardErrorMessages(this);
		AddError(TEXT("Failed to create and start test world."));
		return false;
	}

	UWorld* World = WorldWrapper.GetTestWorld();
	if (!World || !World->GetGameInstance())
	{
		AddError(TEXT("Test world or game instance is invalid."));
		return false;
	}

	UEMRTreatmentSubsystem* TreatmentSubsystem = World->GetGameInstance()->GetSubsystem<UEMRTreatmentSubsystem>();
	if (!TreatmentSubsystem)
	{
		AddError(TEXT("Treatment subsystem is missing."));
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags = RF_Transient;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEMRTreatmentBed* Bed = World->SpawnActor<AEMRTreatmentBed>(AEMRTreatmentBed::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!Bed)
	{
		AddError(TEXT("Failed to spawn treatment bed for negative path test."));
		return false;
	}

	TStrongObjectPtr<UEMRPatientData> ScopedSyntheticData(
		EMRPatientFlowAllActionsTests::CreateSinglePathologyPatientData(SelectedPatientData, SelectedPathology, *this));
	if (!ScopedSyntheticData.IsValid())
	{
		return false;
	}

	AEMRPatient* Patient = EMRPatientFlowAllActionsTests::SpawnAndInitializePatient(World, ScopedSyntheticData.Get(), *this);
	if (!Patient)
	{
		return false;
	}

	UAbilitySystemComponent* PatientASC = Patient->GetAbilitySystemComponent();
	if (!PatientASC)
	{
		AddError(TEXT("Negative path patient has no ASC."));
		return false;
	}

	TreatmentSubsystem->AddPatientToTreatmentQueue(Patient);
	WorldWrapper.TickTestWorld();

	TreatmentSubsystem->CompleteTreatmentForPatient(Patient, EMRTags::Abilities::Treatment::Root, true);
	WorldWrapper.TickTestWorld();

	TestFalse(
		TEXT("Invalid treatment completion must not force patient to exit."),
		PatientASC->HasMatchingGameplayTag(EMRTags::Patient::Phase::Leaving));

	TreatmentSubsystem->HandlePatientAbandonment(Patient);
	Patient->Destroy();
	WorldWrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEMRPatientFlowAllActionsRoutingCompleteExamAvailableAlternativeTest,
	"Project.PatientFlow.AllActions.Routing.CompleteExamUsesAvailableAlternative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRPatientFlowAllActionsRoutingCompleteExamAvailableAlternativeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("String Table not found"), EAutomationExpectedErrorFlags::Contains, 1);

	const UEMRSubsystemConfig* SubsystemConfig = nullptr;
	TArray<const UEMRPatientData*> Patients;
	if (!EMRPatientFlowAllActionsTests::LoadCoreAssets(*this, SubsystemConfig, Patients))
	{
		return false;
	}

	const UEMRPatientData* TemplatePatientData = EMRPatientFlowAllActionsTests::GetAnyPatientData(Patients);
	if (!TemplatePatientData)
	{
		AddError(TEXT("No patient template available for routing test."));
		return false;
	}

	FTestWorldWrapper WorldWrapper;
	if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
	{
		AddError(TEXT("Failed to create test world."));
		return false;
	}

	UWorld* World = WorldWrapper.GetTestWorld();
	if (!World || !World->GetGameInstance())
	{
		AddError(TEXT("Test world or game instance is invalid."));
		return false;
	}

	UEMRExamQueueSubsystem* ExamQueueSubsystem = World->GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>();
	if (!ExamQueueSubsystem)
	{
		AddError(TEXT("Exam queue subsystem is missing."));
		return false;
	}

	const FGameplayTag UltrasoundMachineTag = ExamQueueSubsystem->GetMachineTagForExam(EMRTags::Abilities::Exam::Ultrasound);
	const FGameplayTag CTMachineTag = ExamQueueSubsystem->GetMachineTagForExam(EMRTags::Abilities::Exam::CTScan);
	const FGameplayTag XRayMachineTag = ExamQueueSubsystem->GetMachineTagForExam(EMRTags::Abilities::Exam::XRay);
	if (!UltrasoundMachineTag.IsValid() || !CTMachineTag.IsValid() || !XRayMachineTag.IsValid())
	{
		AddError(TEXT("Could not resolve one or more machine tags for exam routing test."));
		return false;
	}

	AEMRBaseMachine* UltrasoundMachine = EMRPatientFlowAllActionsTests::SpawnMachineWithTag(World, UltrasoundMachineTag, FVector(100.f, 0.f, 0.f), *this);
	AEMRBaseMachine* CTMachine = EMRPatientFlowAllActionsTests::SpawnMachineWithTag(World, CTMachineTag, FVector(300.f, 0.f, 0.f), *this);
	AEMRBaseMachine* XRayMachine = EMRPatientFlowAllActionsTests::SpawnMachineWithTag(World, XRayMachineTag, FVector(600.f, 0.f, 0.f), *this);
	if (!UltrasoundMachine || !CTMachine || !XRayMachine)
	{
		return false;
	}

	if (!WorldWrapper.BeginPlayInTestWorld())
	{
		WorldWrapper.ForwardErrorMessages(this);
		AddError(TEXT("Failed to begin play for routing test world."));
		return false;
	}

	AEMRPatient* CTBlocker = EMRPatientFlowAllActionsTests::SpawnAndInitializePatient(World, TemplatePatientData, *this, FVector(900.f, 0.f, 0.f));
	AEMRPatient* XRayBlocker = EMRPatientFlowAllActionsTests::SpawnAndInitializePatient(World, TemplatePatientData, *this, FVector(950.f, 0.f, 0.f));
	AEMRPatient* Patient = EMRPatientFlowAllActionsTests::SpawnAndInitializePatient(World, TemplatePatientData, *this, FVector::ZeroVector);
	if (!CTBlocker || !XRayBlocker || !Patient)
	{
		AddError(TEXT("Failed to spawn one or more patients for routing test."));
		return false;
	}

	CTMachine->SetOccupiedByPatient(CTBlocker);
	XRayMachine->SetOccupiedByPatient(XRayBlocker);

	TArray<FGameplayTag> ExamSequence;
	ExamSequence.Add(EMRTags::Abilities::Exam::Ultrasound);
	ExamSequence.Add(EMRTags::Abilities::Exam::CTScan);
	ExamSequence.Add(EMRTags::Abilities::Exam::XRay);

	ExamQueueSubsystem->AddPatientToExamQueue(Patient, ExamSequence);
	WorldWrapper.TickTestWorld();

	TestTrue(TEXT("Patient should initially be assigned to Ultrasound machine."), UltrasoundMachine->GetAssignedPatient() == Patient);

	XRayMachine->ClearOccupiedPatient(XRayBlocker);
	ExamQueueSubsystem->CompleteExamForPatient(Patient, EMRTags::Abilities::Exam::Ultrasound, true);
	WorldWrapper.TickTestWorld();

	TestTrue(TEXT("Patient should be assigned to XRay when CTScan is occupied and XRay is free."), XRayMachine->GetAssignedPatient() == Patient);
	TestTrue(TEXT("CTScan should remain occupied by blocker patient."), CTMachine->GetAssignedPatient() == CTBlocker);

	ExamQueueSubsystem->RemovePatientFromAllQueues(Patient);
	CTMachine->ClearOccupiedPatient(CTBlocker);
	XRayMachine->ClearOccupiedPatient(Patient);
	UltrasoundMachine->ClearOccupiedPatient(Patient);

	Patient->Destroy();
	CTBlocker->Destroy();
	XRayBlocker->Destroy();
	WorldWrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEMRPatientFlowAllActionsRoutingNearestAvailableExamTest,
	"Project.PatientFlow.AllActions.Routing.ChoosesNearestAvailableExam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRPatientFlowAllActionsRoutingNearestAvailableExamTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("String Table not found"), EAutomationExpectedErrorFlags::Contains, 1);

	const UEMRSubsystemConfig* SubsystemConfig = nullptr;
	TArray<const UEMRPatientData*> Patients;
	if (!EMRPatientFlowAllActionsTests::LoadCoreAssets(*this, SubsystemConfig, Patients))
	{
		return false;
	}

	const UEMRPatientData* TemplatePatientData = EMRPatientFlowAllActionsTests::GetAnyPatientData(Patients);
	if (!TemplatePatientData)
	{
		AddError(TEXT("No patient template available for nearest routing test."));
		return false;
	}

	FTestWorldWrapper WorldWrapper;
	if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
	{
		AddError(TEXT("Failed to create test world."));
		return false;
	}

	UWorld* World = WorldWrapper.GetTestWorld();
	if (!World || !World->GetGameInstance())
	{
		AddError(TEXT("Test world or game instance is invalid."));
		return false;
	}

	UEMRExamQueueSubsystem* ExamQueueSubsystem = World->GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>();
	if (!ExamQueueSubsystem)
	{
		AddError(TEXT("Exam queue subsystem is missing."));
		return false;
	}

	const FGameplayTag UltrasoundMachineTag = ExamQueueSubsystem->GetMachineTagForExam(EMRTags::Abilities::Exam::Ultrasound);
	const FGameplayTag CTMachineTag = ExamQueueSubsystem->GetMachineTagForExam(EMRTags::Abilities::Exam::CTScan);
	const FGameplayTag XRayMachineTag = ExamQueueSubsystem->GetMachineTagForExam(EMRTags::Abilities::Exam::XRay);
	if (!UltrasoundMachineTag.IsValid() || !CTMachineTag.IsValid() || !XRayMachineTag.IsValid())
	{
		AddError(TEXT("Could not resolve one or more machine tags for nearest routing test."));
		return false;
	}

	AEMRBaseMachine* UltrasoundMachine = EMRPatientFlowAllActionsTests::SpawnMachineWithTag(World, UltrasoundMachineTag, FVector(100.f, 0.f, 0.f), *this);
	AEMRBaseMachine* CTMachine = EMRPatientFlowAllActionsTests::SpawnMachineWithTag(World, CTMachineTag, FVector(250.f, 0.f, 0.f), *this);
	AEMRBaseMachine* XRayMachine = EMRPatientFlowAllActionsTests::SpawnMachineWithTag(World, XRayMachineTag, FVector(1400.f, 0.f, 0.f), *this);
	if (!UltrasoundMachine || !CTMachine || !XRayMachine)
	{
		return false;
	}

	if (!WorldWrapper.BeginPlayInTestWorld())
	{
		WorldWrapper.ForwardErrorMessages(this);
		AddError(TEXT("Failed to begin play for nearest routing test world."));
		return false;
	}

	AEMRPatient* CTBlocker = EMRPatientFlowAllActionsTests::SpawnAndInitializePatient(World, TemplatePatientData, *this, FVector(900.f, 0.f, 0.f));
	AEMRPatient* XRayBlocker = EMRPatientFlowAllActionsTests::SpawnAndInitializePatient(World, TemplatePatientData, *this, FVector(950.f, 0.f, 0.f));
	AEMRPatient* Patient = EMRPatientFlowAllActionsTests::SpawnAndInitializePatient(World, TemplatePatientData, *this, FVector::ZeroVector);
	if (!CTBlocker || !XRayBlocker || !Patient)
	{
		AddError(TEXT("Failed to spawn one or more patients for nearest routing test."));
		return false;
	}

	CTMachine->SetOccupiedByPatient(CTBlocker);
	XRayMachine->SetOccupiedByPatient(XRayBlocker);

	TArray<FGameplayTag> ExamSequence;
	ExamSequence.Add(EMRTags::Abilities::Exam::Ultrasound);
	ExamSequence.Add(EMRTags::Abilities::Exam::CTScan);
	ExamSequence.Add(EMRTags::Abilities::Exam::XRay);

	ExamQueueSubsystem->AddPatientToExamQueue(Patient, ExamSequence);
	WorldWrapper.TickTestWorld();

	TestTrue(TEXT("Patient should initially be assigned to Ultrasound machine."), UltrasoundMachine->GetAssignedPatient() == Patient);

	CTMachine->ClearOccupiedPatient(CTBlocker);
	XRayMachine->ClearOccupiedPatient(XRayBlocker);
	ExamQueueSubsystem->CompleteExamForPatient(Patient, EMRTags::Abilities::Exam::Ultrasound, true);
	WorldWrapper.TickTestWorld();

	TestTrue(TEXT("Patient should be assigned to nearest available machine (CTScan)."), CTMachine->GetAssignedPatient() == Patient);
	TestFalse(TEXT("Patient should not be assigned to farther XRay machine when CTScan is available and closer."), XRayMachine->GetAssignedPatient() == Patient);

	ExamQueueSubsystem->RemovePatientFromAllQueues(Patient);
	CTMachine->ClearOccupiedPatient(Patient);
	UltrasoundMachine->ClearOccupiedPatient(Patient);

	Patient->Destroy();
	CTBlocker->Destroy();
	XRayBlocker->Destroy();
	WorldWrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEMRPatientFlowAllActionsRoutingNoMidPathRerouteTest,
	"Project.PatientFlow.AllActions.Routing.NoMidPathReroute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRPatientFlowAllActionsRoutingNoMidPathRerouteTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("String Table not found"), EAutomationExpectedErrorFlags::Contains, 1);

	const UEMRSubsystemConfig* SubsystemConfig = nullptr;
	TArray<const UEMRPatientData*> Patients;
	if (!EMRPatientFlowAllActionsTests::LoadCoreAssets(*this, SubsystemConfig, Patients))
	{
		return false;
	}

	const UEMRPatientData* TemplatePatientData = EMRPatientFlowAllActionsTests::GetAnyPatientData(Patients);
	if (!TemplatePatientData)
	{
		AddError(TEXT("No patient template available for no-reroute test."));
		return false;
	}

	FTestWorldWrapper WorldWrapper;
	if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
	{
		AddError(TEXT("Failed to create test world."));
		return false;
	}

	UWorld* World = WorldWrapper.GetTestWorld();
	if (!World || !World->GetGameInstance())
	{
		AddError(TEXT("Test world or game instance is invalid."));
		return false;
	}

	UEMRExamQueueSubsystem* ExamQueueSubsystem = World->GetGameInstance()->GetSubsystem<UEMRExamQueueSubsystem>();
	if (!ExamQueueSubsystem)
	{
		AddError(TEXT("Exam queue subsystem is missing."));
		return false;
	}

	const FGameplayTag CTMachineTag = ExamQueueSubsystem->GetMachineTagForExam(EMRTags::Abilities::Exam::CTScan);
	const FGameplayTag XRayMachineTag = ExamQueueSubsystem->GetMachineTagForExam(EMRTags::Abilities::Exam::XRay);
	if (!CTMachineTag.IsValid() || !XRayMachineTag.IsValid())
	{
		AddError(TEXT("Could not resolve one or more machine tags for no-reroute test."));
		return false;
	}

	AEMRBaseMachine* XRayMachine = EMRPatientFlowAllActionsTests::SpawnMachineWithTag(World, XRayMachineTag, FVector(120.f, 0.f, 0.f), *this);
	AEMRBaseMachine* CTMachine = EMRPatientFlowAllActionsTests::SpawnMachineWithTag(World, CTMachineTag, FVector(1000.f, 0.f, 0.f), *this);
	if (!XRayMachine || !CTMachine)
	{
		return false;
	}

	if (!WorldWrapper.BeginPlayInTestWorld())
	{
		WorldWrapper.ForwardErrorMessages(this);
		AddError(TEXT("Failed to begin play for no-reroute test world."));
		return false;
	}

	AEMRPatient* Patient = EMRPatientFlowAllActionsTests::SpawnAndInitializePatient(World, TemplatePatientData, *this, FVector::ZeroVector);
	if (!Patient)
	{
		AddError(TEXT("Failed to spawn patient for no-reroute test."));
		return false;
	}

	TArray<FGameplayTag> ExamSequence;
	ExamSequence.Add(EMRTags::Abilities::Exam::XRay);
	ExamSequence.Add(EMRTags::Abilities::Exam::CTScan);
	ExamQueueSubsystem->AddPatientToExamQueue(Patient, ExamSequence);
	WorldWrapper.TickTestWorld();

	TestTrue(TEXT("Patient should be assigned to XRay machine first."), XRayMachine->GetAssignedPatient() == Patient);

	AEMRPatient* NextForCT = ExamQueueSubsystem->GetNextPatientForMachine(CTMachineTag);
	TestNull(TEXT("CT machine should not pull patient already assigned to a different machine."), NextForCT);
	TestTrue(TEXT("Patient assignment should remain on XRay machine after CT dequeue attempt."), XRayMachine->GetAssignedPatient() == Patient);

	ExamQueueSubsystem->RemovePatientFromAllQueues(Patient);
	XRayMachine->ClearOccupiedPatient(Patient);
	CTMachine->ClearOccupiedPatient(Patient);
	Patient->Destroy();
	WorldWrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

#endif
