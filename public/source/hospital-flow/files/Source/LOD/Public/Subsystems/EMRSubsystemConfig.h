
#pragma once

#include "CoreMinimal.h"
#include "EMRSubsystemConfig.generated.h"


class UDataTable;
class UEMRDifficultyTuningData;
class UEMRSpecialEventDefinition;
class UEMRToiletConfig;
struct FEMRExamQueueMapping;
struct FEMROralTreatmentMappingRow;
struct FEMRTreatmentForPathologyMapping;
struct FEMRExamRequirementsForPathologyMapping;
struct FEMRExamMachineCompletionMapping;
struct FEMRNavigationTargetData;
struct FEMRXRayTargetData;
struct FEMRUltrasoundTargetData;
struct FEMRCTScanTargetData;

/**
 * Centralized configuration asset listing data tables required by EMR subsystems.
 */
UCLASS()
class LOD_API UEMRSubsystemConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	static FPrimaryAssetType GetSubsystemConfigAssetType();

	UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
	UDataTable* GetAttributeToTagMapping() const { return AttributeToTagMapping; }
	
	UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
	UDataTable* GetExamQueueMappingTable() const { return ExamQueueMappingTable; }

	UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
	UDataTable* GetTreatmentQueueMappingTable() const { return TreatmentQueueMappingTable; }
	
    UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
    UDataTable* GetTreatmentsFromPathologyMappingTable() const { return TreatmentsForPathologyTable; }

    UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
    UDataTable* GetExamRequirementsFromPathologyTable() const { return ExamRequirementsFromPathologyTable; }

    UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
    UDataTable* GetExamCompletionMappingTable() const { return ExamCompletionMappingTable; }

    UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
    UDataTable* GetXRayTargetsFromPathologyTable() const { return XRayTargetsFromPathologyTable; }

    UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
    UDataTable* GetUltrasoundTargetsFromPathologyTable() const { return UltrasoundTargetsFromPathologyTable; }

    UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
    UDataTable* GetCTScanTargetsFromPathologyTable() const { return CTScanTargetsFromPathologyTable; }

    UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
    UDataTable* GetNavigationIntentMappingTable() const { return NavigationIntentMappingTable; }

    UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
    const UEMRDifficultyTuningData* GetDifficultyTuning() const;

    UFUNCTION(BlueprintPure, Category = "EMR|Subsystems")
    const UEMRToiletConfig* GetToiletConfig() const;

    /** Preload soft references used in gameplay runtime paths to avoid first-use hitching. */
    void PreloadRuntimeAssets() const;

    void GetSpecialEventDefinitions(TArray<const UEMRSpecialEventDefinition*>& OutSpecialEventDefinitions) const;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRAttributeToTagMapping"))
	TObjectPtr<UDataTable> AttributeToTagMapping = nullptr;

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRExamQueueMapping"))
	TObjectPtr<UDataTable> ExamQueueMappingTable = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRTreatmentQueueMapping"))
	TObjectPtr<UDataTable> TreatmentQueueMappingTable = nullptr;
	
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRTreatmentForPathology"))
    TObjectPtr<UDataTable> TreatmentsForPathologyTable = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRExamRequirementsForPathologyMapping"))
    TObjectPtr<UDataTable> ExamRequirementsFromPathologyTable = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRExamMachineCompletionMapping"))
    TObjectPtr<UDataTable> ExamCompletionMappingTable = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRXRayTargetData"))
    TObjectPtr<UDataTable> XRayTargetsFromPathologyTable = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRUltrasoundTargetData"))
    TObjectPtr<UDataTable> UltrasoundTargetsFromPathologyTable = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRCTScanTargetData"))
    TObjectPtr<UDataTable> CTScanTargetsFromPathologyTable = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true", RowType = "FEMRNavigationTargetData"))
    TObjectPtr<UDataTable> NavigationIntentMappingTable = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true"))
    TSoftObjectPtr<UEMRDifficultyTuningData> DifficultyTuning;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true"))
    TSoftObjectPtr<UEMRToiletConfig> ToiletConfig;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Subsystems", meta = (AllowPrivateAccess = "true"))
    TArray<TSoftObjectPtr<UEMRSpecialEventDefinition>> SpecialEventDefinitions;

    UPROPERTY(Transient)
    mutable TObjectPtr<UEMRDifficultyTuningData> CachedDifficultyTuningAsset = nullptr;

    UPROPERTY(Transient)
    mutable TArray<TObjectPtr<UEMRSpecialEventDefinition>> CachedSpecialEventDefinitions;
    mutable bool bRuntimeAssetsPreloaded = false;
};

