#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EMRDeveloperToolsTypes.generated.h"

UENUM(BlueprintType)
enum class EEMRDeveloperToolAction : uint8
{
    SpawnPatient UMETA(DisplayName = "Spawn Patient"),
    WinNightShift UMETA(DisplayName = "Win Nightshift"),
    LoseNightShift UMETA(DisplayName = "Lose Nightshift"),
    EarnRevenue1000 UMETA(DisplayName = "Earn 1000 TotalRevenue"),
    EarnReputation10 UMETA(DisplayName = "Earn 10 Reputation"),
    LoseReputation10 UMETA(DisplayName = "Lose 10 Reputation"),
    SpeedUpGameX2 UMETA(DisplayName = "Speed Up x2"),
    SlowDownGameX2 UMETA(DisplayName = "Slow Down x2"),
    RerollHubUpgrades UMETA(DisplayName = "Reroll Hub Upgrades"),
    GiveSpecificUpgrade UMETA(DisplayName = "Give Specific Upgrade")
};

USTRUCT(BlueprintType)
struct FEMRDeveloperToolActionRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "EMR|DevTools")
    EEMRDeveloperToolAction Action = EEMRDeveloperToolAction::SpawnPatient;

    UPROPERTY(BlueprintReadWrite, Category = "EMR|DevTools")
    FGameplayTag UpgradeTag;
};

USTRUCT(BlueprintType)
struct FEMRDeveloperToolActionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|DevTools")
    EEMRDeveloperToolAction Action = EEMRDeveloperToolAction::SpawnPatient;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|DevTools")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|DevTools")
    FString Message;
};

USTRUCT(BlueprintType)
struct FEMRDeveloperToolUpgradeOption
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EMR|DevTools")
    FGameplayTag UpgradeTag;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|DevTools")
    FName UpgradeId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "EMR|DevTools")
    FString DisplayLabel;
};
