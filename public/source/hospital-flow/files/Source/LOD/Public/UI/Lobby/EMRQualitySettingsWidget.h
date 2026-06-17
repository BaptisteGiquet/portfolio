#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "Engine/EngineTypes.h"
#include "EMRQualitySettingsWidget.generated.h"

class UButton;
class UComboBoxString;


USTRUCT(BlueprintType)
struct FEMRQualityLevelsData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 ViewDistanceQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 AntiAliasingQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 ShadowQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 GlobalIlluminationQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 ReflectionQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 PostProcessQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 TextureQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 EffectsQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 FoliageQuality = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    int32 ShadingQuality = 3;
};

USTRUCT(BlueprintType)
struct FEMRQualityPreset
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    FText DisplayName = FText::GetEmpty();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    FEMRQualityLevelsData QualityLevels;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float ResolutionScaleNormalized = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    bool bUseVSync = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    bool bUseDynamicResolution = false;
};

/**
 * Simple widget to expose a handful of predefined scalability presets for the lobby.
 */
UCLASS()
class LOD_API UEMRQualitySettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

    /** Apply the preset matching the provided name ("Epic", "High", "Medium", "Low", "Minimum"). */
    UFUNCTION(BlueprintCallable, Category = "EMR|Quality")
    void ApplyPresetByName(FName PresetName);

    /** Apply the currently selected preset option. */
    UFUNCTION(BlueprintCallable, Category = "EMR|Quality")
    void ApplySelectedPreset();

protected:
    void BuildDefaultPresets();
    void PopulateComboBoxOptions();
    void ApplyPresetInternal(const FEMRQualityPreset& Preset);

    UFUNCTION()
    void HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleApplyButtonClicked();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    FName DefaultPresetName = TEXT("High");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    TMap<FName, FEMRQualityPreset> QualityPresets;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    TArray<FName> PresetOrder;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EMR|Quality")
    bool bApplyOnSelection = true;

private:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ComboBox_QualityPreset;
	
    TMap<FString, FName> OptionLabelToPresetName;
    FName ActivePresetName = NAME_None;
};
