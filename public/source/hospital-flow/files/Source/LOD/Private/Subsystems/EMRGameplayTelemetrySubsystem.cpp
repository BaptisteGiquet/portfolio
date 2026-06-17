#include "Subsystems/EMRGameplayTelemetrySubsystem.h"

#include "GAS/EMRTags.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    constexpr int32 TelemetrySchemaVersion = 3;
    constexpr TCHAR TelemetryFileName[] = TEXT("nightshift_metrics.csv");
    constexpr TCHAR TelemetryFolderName[] = TEXT("Telemetry");

    struct FCanonicalTagColumn
    {
        FGameplayTag Tag;
        const TCHAR* ColumnName = TEXT("");
    };

    TArray<FCanonicalTagColumn> GetCanonicalExamColumns()
    {
        return {
            {EMRTags::Abilities::Exam::CTScan, TEXT("exam_ct_scan_count")},
            {EMRTags::Abilities::Exam::XRay, TEXT("exam_xray_count")},
            {EMRTags::Abilities::Exam::Ultrasound, TEXT("exam_ultrasound_count")},
            {EMRTags::Abilities::Exam::Electrocardiogram, TEXT("exam_electrocardiogram_count")},
            {EMRTags::Abilities::Exam::LabAnalyzer::Root, TEXT("exam_lab_analyzer_count")}
        };
    }

    TArray<FCanonicalTagColumn> GetCanonicalTreatmentColumns()
    {
        return {
            {EMRTags::Abilities::Treatment::Bandage, TEXT("treatment_bandage_count")},
            {EMRTags::Abilities::Treatment::Suture, TEXT("treatment_suture_count")},
            {EMRTags::Abilities::Treatment::Cast, TEXT("treatment_cast_count")},
            {EMRTags::Abilities::Treatment::Splint, TEXT("treatment_splint_count")},
            {EMRTags::Abilities::Treatment::Oxygen, TEXT("treatment_oxygen_count")},
            {EMRTags::Abilities::Treatment::IntravenousMedication, TEXT("treatment_intravenous_medication_count")},
            {EMRTags::Abilities::Treatment::OralMedication, TEXT("treatment_oral_medication_count")}
        };
    }

    FString FormatMetricFloat(const float Value)
    {
        return FString::Printf(TEXT("%.3f"), Value);
    }
}

void UEMRGameplayTelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    WrittenRecordIds.Reset();
    CachedSessionGameNumber = 0;
    bSessionGameNumberInitialized = false;
    ActiveTelemetryCsvPath = GetTelemetryCsvPath();

    if (!EnsureCsvHeaderExists(ActiveTelemetryCsvPath))
    {
        ActiveTelemetryCsvPath = GetFallbackTelemetryCsvPath();
        EnsureCsvHeaderExists(ActiveTelemetryCsvPath);
    }
}

void UEMRGameplayTelemetrySubsystem::WriteNightShiftTelemetryLocal(const FEMRNightShiftTelemetryRecord& Record)
{
    if (ShouldSkipRecord(Record.RecordId))
    {
        return;
    }

    if (ActiveTelemetryCsvPath.IsEmpty())
    {
        ActiveTelemetryCsvPath = GetTelemetryCsvPath();
    }

    if (!EnsureCsvHeaderExists(ActiveTelemetryCsvPath))
    {
        const FString FallbackCsvPath = GetFallbackTelemetryCsvPath();
        if (!FallbackCsvPath.Equals(ActiveTelemetryCsvPath, ESearchCase::IgnoreCase) && EnsureCsvHeaderExists(FallbackCsvPath))
        {
            ActiveTelemetryCsvPath = FallbackCsvPath;
            bSessionGameNumberInitialized = false;
            CachedSessionGameNumber = 0;
        }
    }

    const int32 SessionGameNumber = GetNextLocalSessionGameNumber();
    const FString Row = BuildCsvRow(Record, SessionGameNumber) + LINE_TERMINATOR;

    if (!FFileHelper::SaveStringToFile(Row, *ActiveTelemetryCsvPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append))
    {
        bool bWrittenToFallback = false;
        const FString FallbackCsvPath = GetFallbackTelemetryCsvPath();
        if (!FallbackCsvPath.Equals(ActiveTelemetryCsvPath, ESearchCase::IgnoreCase) && EnsureCsvHeaderExists(FallbackCsvPath))
        {
            CachedSessionGameNumber = FMath::Max(0, CachedSessionGameNumber - 1);
            ActiveTelemetryCsvPath = FallbackCsvPath;
            bSessionGameNumberInitialized = false;
            CachedSessionGameNumber = 0;

            const int32 FallbackSessionGameNumber = GetNextLocalSessionGameNumber();
            const FString FallbackRow = BuildCsvRow(Record, FallbackSessionGameNumber) + LINE_TERMINATOR;
            bWrittenToFallback = FFileHelper::SaveStringToFile(
                FallbackRow,
                *ActiveTelemetryCsvPath,
                FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
                &IFileManager::Get(),
                FILEWRITE_Append);
        }

        if (!bWrittenToFallback)
        {
            CachedSessionGameNumber = FMath::Max(0, CachedSessionGameNumber - 1);
            if (Record.RecordId.IsValid())
            {
                WrittenRecordIds.Remove(Record.RecordId);
            }

            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[Telemetry] Failed to append telemetry row to '%s' and fallback '%s'."),
                *GetTelemetryCsvPath(),
                *FallbackCsvPath);
        }
    }
}

int32 UEMRGameplayTelemetrySubsystem::GetNextLocalSessionGameNumber()
{
    EnsureSessionGameNumberInitialized();
    ++CachedSessionGameNumber;
    return CachedSessionGameNumber;
}

void UEMRGameplayTelemetrySubsystem::ResetRuntimeState()
{
    WrittenRecordIds.Reset();
    CachedSessionGameNumber = 0;
    bSessionGameNumberInitialized = false;
    ActiveTelemetryCsvPath.Reset();

    UE_LOG(LogTemp, Warning, TEXT("[TempFlowReset] Telemetry runtime cache reset."));
}

FString UEMRGameplayTelemetrySubsystem::GetTelemetryCsvPath() const
{
#if WITH_DEV_AUTOMATION_TESTS
    if (!TelemetryCsvPathOverride.IsEmpty())
    {
        return TelemetryCsvPathOverride;
    }
#endif

    const FString BuildRootDirectory = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("../../")));
    return FPaths::Combine(BuildRootDirectory, TelemetryFolderName, TelemetryFileName);
}

FString UEMRGameplayTelemetrySubsystem::GetFallbackTelemetryCsvPath() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TelemetryFolderName, TelemetryFileName);
}

bool UEMRGameplayTelemetrySubsystem::EnsureCsvHeaderExists(const FString& CsvPath)
{
    IFileManager& FileManager = IFileManager::Get();
    const FString DirectoryPath = FPaths::GetPath(CsvPath);
    if (!FileManager.DirectoryExists(*DirectoryPath) && !FileManager.MakeDirectory(*DirectoryPath, true))
    {
        return false;
    }

    if (FileManager.FileExists(*CsvPath))
    {
        const int64 FileSize = FileManager.FileSize(*CsvPath);
        if (FileSize > 0)
        {
            TArray<FString> ExistingLines;
            if (FFileHelper::LoadFileToStringArray(ExistingLines, *CsvPath) && ExistingLines.Num() > 0)
            {
                const FString ExpectedHeader = BuildCsvHeader();
                if (ExistingLines[0].TrimStartAndEnd().Equals(ExpectedHeader, ESearchCase::CaseSensitive))
                {
                    return true;
                }

                const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
                const FString BackupPath = FPaths::Combine(
                    DirectoryPath,
                    FString::Printf(TEXT("%s_pre_schema_%d_%s.csv"), *FPaths::GetBaseFilename(CsvPath), TelemetrySchemaVersion, *Timestamp));
                const bool bMoved = FileManager.Move(*BackupPath, *CsvPath, true, true, false, true);
                if (!bMoved)
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    }

    const FString HeaderWithNewline = BuildCsvHeader() + LINE_TERMINATOR;
    if (!FFileHelper::SaveStringToFile(HeaderWithNewline, *CsvPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        return false;
    }

    return true;
}

FString UEMRGameplayTelemetrySubsystem::BuildCsvRow(const FEMRNightShiftTelemetryRecord& Record, const int32 SessionGameNumber) const
{
    TMap<FGameplayTag, int32> ExamCountByTag;
    TMap<FGameplayTag, int32> TreatmentCountByTag;

    for (const FEMRGameplayTelemetryTagCount& TagCount : Record.ExamCounts)
    {
        if (!TagCount.Tag.IsValid() || TagCount.Count <= 0)
        {
            continue;
        }

        const FGameplayTag NormalizedExamTag = TagCount.Tag.MatchesTag(EMRTags::Abilities::Exam::LabAnalyzer::Root)
            ? EMRTags::Abilities::Exam::LabAnalyzer::Root
            : TagCount.Tag;
        ExamCountByTag.FindOrAdd(NormalizedExamTag) += TagCount.Count;
    }

    for (const FEMRGameplayTelemetryTagCount& TagCount : Record.TreatmentCounts)
    {
        if (!TagCount.Tag.IsValid() || TagCount.Count <= 0)
        {
            continue;
        }

        TreatmentCountByTag.FindOrAdd(TagCount.Tag) += TagCount.Count;
    }

    int32 ExamOtherCount = 0;
    int32 TreatmentOtherCount = 0;

    const TArray<FCanonicalTagColumn> CanonicalExamColumns = GetCanonicalExamColumns();
    const TArray<FCanonicalTagColumn> CanonicalTreatmentColumns = GetCanonicalTreatmentColumns();

    TSet<FGameplayTag> CanonicalExamTags;
    CanonicalExamTags.Reserve(CanonicalExamColumns.Num());
    for (const FCanonicalTagColumn& Column : CanonicalExamColumns)
    {
        CanonicalExamTags.Add(Column.Tag);
    }

    TSet<FGameplayTag> CanonicalTreatmentTags;
    CanonicalTreatmentTags.Reserve(CanonicalTreatmentColumns.Num());
    for (const FCanonicalTagColumn& Column : CanonicalTreatmentColumns)
    {
        CanonicalTreatmentTags.Add(Column.Tag);
    }

    for (const TPair<FGameplayTag, int32>& Pair : ExamCountByTag)
    {
        if (!CanonicalExamTags.Contains(Pair.Key))
        {
            ExamOtherCount += Pair.Value;
        }
    }

    for (const TPair<FGameplayTag, int32>& Pair : TreatmentCountByTag)
    {
        if (!CanonicalTreatmentTags.Contains(Pair.Key))
        {
            TreatmentOtherCount += Pair.Value;
        }
    }

    const UEnum* DifficultyTierEnum = StaticEnum<EEMRNightShiftDifficultyTier>();
    const FString DifficultyName = DifficultyTierEnum
    ? DifficultyTierEnum->GetNameStringByValue(static_cast<int64>(Record.DifficultyTier))
    : TEXT("Unknown");

    TArray<FString> Columns;
    Columns.Reserve(64);
    Columns.Add(FString::FromInt(TelemetrySchemaVersion));
    Columns.Add(Record.RecordId.ToString(EGuidFormats::DigitsWithHyphensLower));
    Columns.Add(Record.UtcTimestampIso8601);
    Columns.Add(FString::FromInt(SessionGameNumber));
    Columns.Add(FString::FromInt(Record.CycleNumber));
    Columns.Add(FString::FromInt(Record.NightShiftNumberInCycle));
    Columns.Add(Record.bSkippedRemainingNightShiftsInCycle ? TEXT("1") : TEXT("0"));
    Columns.Add(FormatMetricFloat(Record.CycleQuota));
    Columns.Add(DifficultyName);
    Columns.Add(FormatMetricFloat(Record.TotalNightShiftSeconds));
    Columns.Add(FormatMetricFloat(Record.OvertimeSeconds));
    Columns.Add(FormatMetricFloat(Record.RevenueCollectedTotal));
    Columns.Add(FormatMetricFloat(Record.RevenueCollectedBeforeOvertime));
    Columns.Add(FormatMetricFloat(Record.RevenueCollectedAfterOvertime));
    Columns.Add(FormatMetricFloat(Record.ReputationLostTotal));
    Columns.Add(FormatMetricFloat(Record.ReputationLostBeforeOvertime));
    Columns.Add(FormatMetricFloat(Record.ReputationLostAfterOvertime));
    Columns.Add(FString::FromInt(Record.PatientsSpawnedTotal));
    Columns.Add(FString::FromInt(Record.PatientsSpawnedBeforeOvertime));
    Columns.Add(FString::FromInt(Record.PatientsSpawnedAfterOvertime));
    Columns.Add(FString::FromInt(Record.PatientsFullyCuredTotal));
    Columns.Add(FString::FromInt(Record.PatientsFullyCuredBeforeOvertime));
    Columns.Add(FString::FromInt(Record.PatientsFullyCuredAfterOvertime));
    Columns.Add(FString::FromInt(Record.PatientsLeftByPatienceTotal));
    Columns.Add(FString::FromInt(Record.PatientsLeftByPatienceBeforeOvertime));
    Columns.Add(FString::FromInt(Record.PatientsLeftByPatienceAfterOvertime));
    Columns.Add(FormatMetricFloat(Record.AverageSpawnToCureSeconds));

    for (const FCanonicalTagColumn& Column : CanonicalExamColumns)
    {
        Columns.Add(FString::FromInt(ExamCountByTag.FindRef(Column.Tag)));
    }
    Columns.Add(FString::FromInt(ExamOtherCount));

    for (const FCanonicalTagColumn& Column : CanonicalTreatmentColumns)
    {
        Columns.Add(FString::FromInt(TreatmentCountByTag.FindRef(Column.Tag)));
    }
    Columns.Add(FString::FromInt(TreatmentOtherCount));

    for (FString& Column : Columns)
    {
        Column = EscapeCsvCell(Column);
    }

    return FString::Join(Columns, TEXT(","));
}

bool UEMRGameplayTelemetrySubsystem::ShouldSkipRecord(const FGuid& RecordId)
{
    if (!RecordId.IsValid())
    {
        return false;
    }

    if (WrittenRecordIds.Contains(RecordId))
    {
        return true;
    }

    WrittenRecordIds.Add(RecordId);
    return false;
}

void UEMRGameplayTelemetrySubsystem::EnsureSessionGameNumberInitialized()
{
    if (bSessionGameNumberInitialized)
    {
        return;
    }

    if (ActiveTelemetryCsvPath.IsEmpty())
    {
        ActiveTelemetryCsvPath = GetTelemetryCsvPath();
    }

    CachedSessionGameNumber = CountDataRowsInExistingCsv(ActiveTelemetryCsvPath);
    bSessionGameNumberInitialized = true;
}

int32 UEMRGameplayTelemetrySubsystem::CountDataRowsInExistingCsv(const FString& CsvPath) const
{
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *CsvPath))
    {
        return 0;
    }

    int32 DataRows = 0;
    for (int32 Index = 1; Index < Lines.Num(); ++Index)
    {
        if (!Lines[Index].TrimStartAndEnd().IsEmpty())
        {
            ++DataRows;
        }
    }
    return DataRows;
}

FString UEMRGameplayTelemetrySubsystem::BuildCsvHeader()
{
    TArray<FString> Columns = {
        TEXT("schema_version"),
        TEXT("record_id"),
        TEXT("utc_timestamp_iso8601"),
        TEXT("session_game_number"),
        TEXT("cycle_number"),
        TEXT("nightshift_number_in_cycle"),
        TEXT("skipped_remaining_nightshifts_in_cycle"),
        TEXT("cycle_quota"),
        TEXT("difficulty_tier"),
        TEXT("total_nightshift_seconds"),
        TEXT("overtime_seconds"),
        TEXT("revenue_collected_total"),
        TEXT("revenue_collected_before_overtime"),
        TEXT("revenue_collected_after_overtime"),
        TEXT("reputation_lost_total"),
        TEXT("reputation_lost_before_overtime"),
        TEXT("reputation_lost_after_overtime"),
        TEXT("patients_spawned_total"),
        TEXT("patients_spawned_before_overtime"),
        TEXT("patients_spawned_after_overtime"),
        TEXT("patients_fully_cured_total"),
        TEXT("patients_fully_cured_before_overtime"),
        TEXT("patients_fully_cured_after_overtime"),
        TEXT("patients_left_by_patience_total"),
        TEXT("patients_left_by_patience_before_overtime"),
        TEXT("patients_left_by_patience_after_overtime"),
        TEXT("average_spawn_to_cure_seconds")
    };

    for (const FCanonicalTagColumn& Column : GetCanonicalExamColumns())
    {
        Columns.Add(Column.ColumnName);
    }
    Columns.Add(TEXT("exam_other_count"));

    for (const FCanonicalTagColumn& Column : GetCanonicalTreatmentColumns())
    {
        Columns.Add(Column.ColumnName);
    }
    Columns.Add(TEXT("treatment_other_count"));

    return FString::Join(Columns, TEXT(","));
}

FString UEMRGameplayTelemetrySubsystem::EscapeCsvCell(const FString& Value)
{
    if (!Value.Contains(TEXT(",")) && !Value.Contains(TEXT("\"")) && !Value.Contains(TEXT("\n")) && !Value.Contains(TEXT("\r")))
    {
        return Value;
    }

    FString EscapedValue = Value;
    EscapedValue.ReplaceInline(TEXT("\""), TEXT("\"\""));
    return FString::Printf(TEXT("\"%s\""), *EscapedValue);
}

#if WITH_DEV_AUTOMATION_TESTS
void UEMRGameplayTelemetrySubsystem::SetTelemetryCsvPathOverrideForTests(const FString& InPath)
{
    TelemetryCsvPathOverride = InPath;
    ActiveTelemetryCsvPath = InPath;
    CachedSessionGameNumber = 0;
    bSessionGameNumberInitialized = false;
    WrittenRecordIds.Reset();
}

FString UEMRGameplayTelemetrySubsystem::BuildCsvHeaderForTests()
{
    return BuildCsvHeader();
}

FString UEMRGameplayTelemetrySubsystem::BuildCsvRowForTests(const FEMRNightShiftTelemetryRecord& Record, const int32 SessionGameNumber)
{
    UEMRGameplayTelemetrySubsystem* TempSubsystem = NewObject<UEMRGameplayTelemetrySubsystem>(GetTransientPackage());
    return TempSubsystem->BuildCsvRow(Record, SessionGameNumber);
}
#endif
