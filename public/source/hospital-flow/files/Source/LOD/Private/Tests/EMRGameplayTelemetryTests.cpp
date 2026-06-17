#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Subsystems/EMRGameplayTelemetrySubsystem.h"
#include "GAS/EMRTags.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace EMRGameplayTelemetryTests
{
    static TArray<FString> ParseCsvLine(const FString& Line)
    {
        TArray<FString> Values;
        FString CurrentValue;
        bool bInsideQuotes = false;

        for (int32 Index = 0; Index < Line.Len(); ++Index)
        {
            const TCHAR Char = Line[Index];
            if (Char == TEXT('\"'))
            {
                if (bInsideQuotes && Index + 1 < Line.Len() && Line[Index + 1] == TEXT('\"'))
                {
                    CurrentValue.AppendChar(TEXT('\"'));
                    ++Index;
                }
                else
                {
                    bInsideQuotes = !bInsideQuotes;
                }
                continue;
            }

            if (Char == TEXT(',') && !bInsideQuotes)
            {
                Values.Add(CurrentValue);
                CurrentValue.Reset();
                continue;
            }

            CurrentValue.AppendChar(Char);
        }

        Values.Add(CurrentValue);
        return Values;
    }

    static TMap<FString, int32> BuildColumnIndexByName(const TArray<FString>& HeaderColumns)
    {
        TMap<FString, int32> Result;
        for (int32 Index = 0; Index < HeaderColumns.Num(); ++Index)
        {
            Result.Add(HeaderColumns[Index], Index);
        }
        return Result;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEMRGameplayTelemetryCsvSchemaTest,
    "Project.Telemetry.NightShift.CsvSchema",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRGameplayTelemetryCsvSchemaTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FString HeaderLine = UEMRGameplayTelemetrySubsystem::BuildCsvHeaderForTests();
    const TArray<FString> HeaderColumns = EMRGameplayTelemetryTests::ParseCsvLine(HeaderLine);
    const TMap<FString, int32> ColumnIndex = EMRGameplayTelemetryTests::BuildColumnIndexByName(HeaderColumns);

    TestTrue(TEXT("Schema should include schema_version"), ColumnIndex.Contains(TEXT("schema_version")));
    TestTrue(TEXT("Schema should include session_game_number"), ColumnIndex.Contains(TEXT("session_game_number")));
    TestTrue(TEXT("Schema should include cycle_number"), ColumnIndex.Contains(TEXT("cycle_number")));
    TestTrue(TEXT("Schema should include skipped_remaining_nightshifts_in_cycle"), ColumnIndex.Contains(TEXT("skipped_remaining_nightshifts_in_cycle")));
    TestTrue(TEXT("Schema should include overtime_seconds"), ColumnIndex.Contains(TEXT("overtime_seconds")));
    TestTrue(TEXT("Schema should include exam_other_count"), ColumnIndex.Contains(TEXT("exam_other_count")));
    TestTrue(TEXT("Schema should include treatment_other_count"), ColumnIndex.Contains(TEXT("treatment_other_count")));
    TestTrue(TEXT("Schema should include exam_ct_scan_count"), ColumnIndex.Contains(TEXT("exam_ct_scan_count")));
    TestTrue(TEXT("Schema should include exam_lab_analyzer_count"), ColumnIndex.Contains(TEXT("exam_lab_analyzer_count")));
    TestTrue(TEXT("Schema should include treatment_oral_medication_count"), ColumnIndex.Contains(TEXT("treatment_oral_medication_count")));

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEMRGameplayTelemetryCsvRowSerializationTest,
    "Project.Telemetry.NightShift.CsvRowSerialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRGameplayTelemetryCsvRowSerializationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEMRNightShiftTelemetryRecord Record;
    Record.RecordId = FGuid::NewGuid();
    Record.UtcTimestampIso8601 = TEXT("2026-02-11T12:34:56Z,UTC");
    Record.CycleNumber = 2;
    Record.NightShiftNumberInCycle = 3;
    Record.bSkippedRemainingNightShiftsInCycle = true;
    Record.CycleQuota = 25000.0f;
    Record.DifficultyTier = EEMRNightShiftDifficultyTier::Critical;
    Record.TotalNightShiftSeconds = 820.5f;
    Record.OvertimeSeconds = 133.25f;
    Record.RevenueCollectedBeforeOvertime = 4000.0f;
    Record.RevenueCollectedAfterOvertime = 500.0f;
    Record.RevenueCollectedTotal = 4500.0f;
    Record.ReputationLostBeforeOvertime = 3.0f;
    Record.ReputationLostAfterOvertime = 2.0f;
    Record.ReputationLostTotal = 5.0f;
    Record.PatientsSpawnedBeforeOvertime = 14;
    Record.PatientsSpawnedAfterOvertime = 5;
    Record.PatientsSpawnedTotal = 19;
    Record.PatientsFullyCuredBeforeOvertime = 10;
    Record.PatientsFullyCuredAfterOvertime = 3;
    Record.PatientsFullyCuredTotal = 13;
    Record.PatientsLeftByPatienceBeforeOvertime = 2;
    Record.PatientsLeftByPatienceAfterOvertime = 1;
    Record.PatientsLeftByPatienceTotal = 3;
    Record.AverageSpawnToCureSeconds = 92.75f;
    Record.ExamCounts = {
        {EMRTags::Abilities::Exam::CTScan, 2},
        {EMRTags::Abilities::Exam::LabAnalyzer::CBC, 3},
        {EMRTags::Abilities::Exam::LabAnalyzer::Lactate, 2},
        {EMRTags::Abilities::Exam::Triage, 4}
    };
    Record.TreatmentCounts = {
        {EMRTags::Abilities::Treatment::Bandage, 6},
        {EMRTags::Abilities::Treatment::Root, 1}
    };

    const FString HeaderLine = UEMRGameplayTelemetrySubsystem::BuildCsvHeaderForTests();
    const FString RowLine = UEMRGameplayTelemetrySubsystem::BuildCsvRowForTests(Record, 42);

    const TArray<FString> HeaderColumns = EMRGameplayTelemetryTests::ParseCsvLine(HeaderLine);
    const TArray<FString> RowColumns = EMRGameplayTelemetryTests::ParseCsvLine(RowLine);
    TestEqual(TEXT("Header/row column count should match"), RowColumns.Num(), HeaderColumns.Num());

    const TMap<FString, int32> ColumnIndex = EMRGameplayTelemetryTests::BuildColumnIndexByName(HeaderColumns);
    auto GetValue = [&ColumnIndex, &RowColumns](const FString& ColumnName) -> FString
    {
        if (const int32* FoundIndex = ColumnIndex.Find(ColumnName))
        {
            return RowColumns.IsValidIndex(*FoundIndex) ? RowColumns[*FoundIndex] : FString();
        }
        return FString();
    };

    TestEqual(TEXT("Session game number should serialize"), GetValue(TEXT("session_game_number")), FString(TEXT("42")));
    TestEqual(TEXT("Skip flag should serialize"), GetValue(TEXT("skipped_remaining_nightshifts_in_cycle")), FString(TEXT("1")));
    TestEqual(TEXT("Timestamp should preserve comma via escaping"), GetValue(TEXT("utc_timestamp_iso8601")), FString(TEXT("2026-02-11T12:34:56Z,UTC")));
    TestEqual(TEXT("Revenue before overtime should serialize"), GetValue(TEXT("revenue_collected_before_overtime")), FString(TEXT("4000.000")));
    TestEqual(TEXT("Revenue after overtime should serialize"), GetValue(TEXT("revenue_collected_after_overtime")), FString(TEXT("500.000")));
    TestEqual(TEXT("Average spawn-to-cure should serialize"), GetValue(TEXT("average_spawn_to_cure_seconds")), FString(TEXT("92.750")));
    TestEqual(TEXT("Known exam count should map to explicit column"), GetValue(TEXT("exam_ct_scan_count")), FString(TEXT("2")));
    TestEqual(TEXT("All LabAnalyzer variants should aggregate into one column"), GetValue(TEXT("exam_lab_analyzer_count")), FString(TEXT("5")));
    TestEqual(TEXT("Unknown exam count should map to other column"), GetValue(TEXT("exam_other_count")), FString(TEXT("4")));
    TestEqual(TEXT("Known treatment count should map to explicit column"), GetValue(TEXT("treatment_bandage_count")), FString(TEXT("6")));
    TestEqual(TEXT("Unknown treatment count should map to other column"), GetValue(TEXT("treatment_other_count")), FString(TEXT("1")));

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEMRGameplayTelemetryDedupeWriteTest,
    "Project.Telemetry.NightShift.DedupeWrite",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEMRGameplayTelemetryDedupeWriteTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FString TestCsvPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Telemetry"), TEXT("nightshift_metrics_test.csv"));
    IFileManager::Get().Delete(*TestCsvPath);

    UEMRGameplayTelemetrySubsystem* TelemetrySubsystem = NewObject<UEMRGameplayTelemetrySubsystem>(GetTransientPackage());
    TelemetrySubsystem->SetTelemetryCsvPathOverrideForTests(TestCsvPath);

    FEMRNightShiftTelemetryRecord Record;
    Record.RecordId = FGuid::NewGuid();
    Record.UtcTimestampIso8601 = TEXT("2026-02-11T20:00:00Z");
    Record.CycleNumber = 1;
    Record.NightShiftNumberInCycle = 1;

    TelemetrySubsystem->WriteNightShiftTelemetryLocal(Record);
    TelemetrySubsystem->WriteNightShiftTelemetryLocal(Record);

    TArray<FString> FileLines;
    const bool bLoaded = FFileHelper::LoadFileToStringArray(FileLines, *TestCsvPath);
    TestTrue(TEXT("Telemetry CSV should be created"), bLoaded);

    int32 DataRows = 0;
    for (int32 Index = 1; Index < FileLines.Num(); ++Index)
    {
        if (!FileLines[Index].TrimStartAndEnd().IsEmpty())
        {
            ++DataRows;
        }
    }

    TestEqual(TEXT("Duplicate record id should only write one data row"), DataRows, 1);

    IFileManager::Get().Delete(*TestCsvPath);
    return !HasAnyErrors();
}

#endif
