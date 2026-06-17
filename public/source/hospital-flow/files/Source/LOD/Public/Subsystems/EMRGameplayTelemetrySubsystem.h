#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Telemetry/EMRGameplayTelemetryTypes.h"
#include "EMRGameplayTelemetrySubsystem.generated.h"

UCLASS()
class LOD_API UEMRGameplayTelemetrySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void WriteNightShiftTelemetryLocal(const FEMRNightShiftTelemetryRecord& Record);
    int32 GetNextLocalSessionGameNumber();
    void ResetRuntimeState();

#if WITH_DEV_AUTOMATION_TESTS
    void SetTelemetryCsvPathOverrideForTests(const FString& InPath);
    static FString BuildCsvHeaderForTests();
    static FString BuildCsvRowForTests(const FEMRNightShiftTelemetryRecord& Record, int32 SessionGameNumber);
#endif

private:
    FString GetTelemetryCsvPath() const;
    FString GetFallbackTelemetryCsvPath() const;
    bool EnsureCsvHeaderExists(const FString& CsvPath);
    FString BuildCsvRow(const FEMRNightShiftTelemetryRecord& Record, int32 SessionGameNumber) const;
    bool ShouldSkipRecord(const FGuid& RecordId);
    void EnsureSessionGameNumberInitialized();
    int32 CountDataRowsInExistingCsv(const FString& CsvPath) const;

    static FString BuildCsvHeader();
    static FString EscapeCsvCell(const FString& Value);

private:
    TSet<FGuid> WrittenRecordIds;
    int32 CachedSessionGameNumber = 0;
    bool bSessionGameNumberInitialized = false;
    FString ActiveTelemetryCsvPath;

#if WITH_DEV_AUTOMATION_TESTS
    FString TelemetryCsvPathOverride;
#endif
};
