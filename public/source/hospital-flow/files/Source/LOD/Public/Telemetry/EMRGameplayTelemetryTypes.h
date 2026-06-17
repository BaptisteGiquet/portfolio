#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/EMRNightShiftDefinition.h"
#include "EMRGameplayTelemetryTypes.generated.h"

USTRUCT(BlueprintType)
struct FEMRGameplayTelemetryTagCount
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    FGameplayTag Tag;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FEMRNightShiftTelemetryRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    FGuid RecordId;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    FString UtcTimestampIso8601;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 CycleNumber = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 NightShiftNumberInCycle = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    bool bSkippedRemainingNightShiftsInCycle = false;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float CycleQuota = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    EEMRNightShiftDifficultyTier DifficultyTier = EEMRNightShiftDifficultyTier::Standard;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float TotalNightShiftSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float OvertimeSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float RevenueCollectedTotal = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float RevenueCollectedBeforeOvertime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float RevenueCollectedAfterOvertime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float ReputationLostTotal = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float ReputationLostBeforeOvertime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float ReputationLostAfterOvertime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 PatientsSpawnedTotal = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 PatientsSpawnedBeforeOvertime = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 PatientsSpawnedAfterOvertime = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 PatientsFullyCuredTotal = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 PatientsFullyCuredBeforeOvertime = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 PatientsFullyCuredAfterOvertime = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 PatientsLeftByPatienceTotal = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 PatientsLeftByPatienceBeforeOvertime = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    int32 PatientsLeftByPatienceAfterOvertime = 0;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    float AverageSpawnToCureSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    TArray<FEMRGameplayTelemetryTagCount> ExamCounts;

    UPROPERTY(BlueprintReadOnly, Category="EMR|Telemetry")
    TArray<FEMRGameplayTelemetryTagCount> TreatmentCounts;
};
