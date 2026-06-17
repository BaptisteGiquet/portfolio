#include "UI/Lobby/EMRQualitySettingsWidget.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/GameUserSettings.h"

namespace
{
	Scalability::FQualityLevels ToEngineLevels(const FEMRQualityLevelsData& Data)
    {
	    Scalability::FQualityLevels Levels;
        Levels.ViewDistanceQuality = Data.ViewDistanceQuality;
        Levels.AntiAliasingQuality = Data.AntiAliasingQuality;
        Levels.ShadowQuality = Data.ShadowQuality;
        Levels.GlobalIlluminationQuality = Data.GlobalIlluminationQuality;
        Levels.ReflectionQuality = Data.ReflectionQuality;
        Levels.PostProcessQuality = Data.PostProcessQuality;
        Levels.TextureQuality = Data.TextureQuality;
        Levels.EffectsQuality = Data.EffectsQuality;
        Levels.FoliageQuality = Data.FoliageQuality;
        Levels.ShadingQuality = Data.ShadingQuality;
        return Levels;
    }

    FEMRQualityPreset MakePreset(const FString& DisplayName, const FEMRQualityLevelsData& Levels, float ResolutionScale, bool bUseVSync, bool bUseDynamicResolution)
    {
        FEMRQualityPreset Preset;
        Preset.DisplayName = FText::FromString(DisplayName);
        Preset.QualityLevels = Levels;
        Preset.ResolutionScaleNormalized = ResolutionScale;
        Preset.bUseVSync = bUseVSync;
        Preset.bUseDynamicResolution = bUseDynamicResolution;
        return Preset;
    }

    FEMRQualityLevelsData MakeUniformLevels(int32 QualityLevel)
    {
        FEMRQualityLevelsData Levels;
        Levels.ViewDistanceQuality = QualityLevel;
        Levels.AntiAliasingQuality = QualityLevel;
        Levels.ShadowQuality = QualityLevel;
        Levels.GlobalIlluminationQuality = QualityLevel;
        Levels.ReflectionQuality = QualityLevel;
        Levels.PostProcessQuality = QualityLevel;
        Levels.TextureQuality = QualityLevel;
        Levels.EffectsQuality = QualityLevel;
        Levels.FoliageQuality = QualityLevel;
        Levels.ShadingQuality = QualityLevel;
        return Levels;
    }

    FEMRQualityLevelsData MakePerformanceLevels(int32 ViewDistanceQuality, int32 ShadowQuality, int32 TextureQuality, int32 FoliageQuality)
    {
        FEMRQualityLevelsData Levels = MakeUniformLevels(2);
        Levels.ViewDistanceQuality = ViewDistanceQuality;
        Levels.ShadowQuality = ShadowQuality;
        Levels.TextureQuality = TextureQuality;
        Levels.FoliageQuality = FoliageQuality;
        return Levels;
    }
}

void UEMRQualitySettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    BuildDefaultPresets();
    PopulateComboBoxOptions();

    if (ComboBox_QualityPreset)
    {
        ComboBox_QualityPreset->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleSelectionChanged);
    }
	
}

void UEMRQualitySettingsWidget::NativeDestruct()
{
    if (ComboBox_QualityPreset)
    {
        ComboBox_QualityPreset->OnSelectionChanged.RemoveDynamic(this, &ThisClass::HandleSelectionChanged);
    }
	

    Super::NativeDestruct();
}

void UEMRQualitySettingsWidget::ApplyPresetByName(FName PresetName)
{
    const FEMRQualityPreset* FoundPreset = QualityPresets.Find(PresetName);
    if (FoundPreset)
    {
        ActivePresetName = PresetName;
        ApplyPresetInternal(*FoundPreset);

        if (ComboBox_QualityPreset)
        {
            const FString* OptionLabel = OptionLabelToPresetName.FindKey(PresetName);
            if (OptionLabel)
            {
                ComboBox_QualityPreset->SetSelectedOption(*OptionLabel);
            }
        }
    }
}

void UEMRQualitySettingsWidget::ApplySelectedPreset()
{
    if (!ComboBox_QualityPreset)
    {
        return;
    }

    const FString SelectedLabel = ComboBox_QualityPreset->GetSelectedOption();
    if (const FName* PresetName = OptionLabelToPresetName.Find(SelectedLabel))
    {
        ApplyPresetByName(*PresetName);
    }
}

void UEMRQualitySettingsWidget::BuildDefaultPresets()
{
    if (QualityPresets.Num() > 0)
    {
        return;
    }

    PresetOrder = {
        TEXT("Epic"),
        TEXT("High"),
        TEXT("Medium"),
        TEXT("Low"),
        TEXT("Minimum"),
    };

    QualityPresets.Add(TEXT("Epic"), MakePreset(TEXT("Épique"), MakeUniformLevels(3), 1.0f, true, false));
    QualityPresets.Add(TEXT("High"), MakePreset(TEXT("Élevée"), MakeUniformLevels(2), 1.0f, true, false));
    QualityPresets.Add(TEXT("Medium"), MakePreset(TEXT("Moyenne"), MakePerformanceLevels(1, 1, 1, 1), 0.60f, false, false));
    QualityPresets.Add(TEXT("Low"), MakePreset(TEXT("Basse"), MakePerformanceLevels(0, 0, 1, 0), 0.42f, false, true));
    QualityPresets.Add(TEXT("Minimum"), MakePreset(TEXT("Minimum"), MakePerformanceLevels(0, 0, 0, 0), 0.35f, false, true));
}

void UEMRQualitySettingsWidget::PopulateComboBoxOptions()
{
    if (!ComboBox_QualityPreset)
    {
        return;
    }

    ComboBox_QualityPreset->ClearOptions();
    OptionLabelToPresetName.Reset();

    for (const FName PresetName : PresetOrder)
    {
        if (const FEMRQualityPreset* Preset = QualityPresets.Find(PresetName))
        {
            const FString OptionLabel = Preset->DisplayName.ToString();
            OptionLabelToPresetName.Add(OptionLabel, PresetName);
            ComboBox_QualityPreset->AddOption(OptionLabel);
        }
    }

    const FString* DefaultOption = OptionLabelToPresetName.FindKey(DefaultPresetName);
    if (!DefaultOption && OptionLabelToPresetName.Num() > 0)
    {
        DefaultPresetName = OptionLabelToPresetName.CreateConstIterator()->Value;
        DefaultOption = OptionLabelToPresetName.FindKey(DefaultPresetName);
    }

    if (DefaultOption)
    {
        ComboBox_QualityPreset->SetSelectedOption(*DefaultOption);
        if (bApplyOnSelection)
        {
            ApplySelectedPreset();
        }
    }
}

void UEMRQualitySettingsWidget::ApplyPresetInternal(const FEMRQualityPreset& Preset)
{
    if (!GEngine)
    {
        return;
    }

    if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
    {
        const Scalability::FQualityLevels EngineLevels = ToEngineLevels(Preset.QualityLevels);

        UserSettings->SetViewDistanceQuality(EngineLevels.ViewDistanceQuality);
        UserSettings->SetAntiAliasingQuality(EngineLevels.AntiAliasingQuality);
        UserSettings->SetShadowQuality(EngineLevels.ShadowQuality);
        UserSettings->SetGlobalIlluminationQuality(EngineLevels.GlobalIlluminationQuality);
        UserSettings->SetReflectionQuality(EngineLevels.ReflectionQuality);
        UserSettings->SetPostProcessingQuality(EngineLevels.PostProcessQuality);
        UserSettings->SetTextureQuality(EngineLevels.TextureQuality);
        UserSettings->SetVisualEffectQuality(EngineLevels.EffectsQuality);
        UserSettings->SetFoliageQuality(EngineLevels.FoliageQuality);
        UserSettings->SetShadingQuality(EngineLevels.ShadingQuality);

        UserSettings->SetResolutionScaleNormalized(Preset.ResolutionScaleNormalized);
        UserSettings->SetVSyncEnabled(Preset.bUseVSync);
        UserSettings->SetDynamicResolutionEnabled(Preset.bUseDynamicResolution);

        UserSettings->ApplySettings(false);
        UserSettings->SaveSettings();
    }
}

void UEMRQualitySettingsWidget::HandleSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (!bApplyOnSelection)
    {
        return;
    }

    if (const FName* PresetName = OptionLabelToPresetName.Find(SelectedItem))
    {
        ApplyPresetByName(*PresetName);
    }
}

void UEMRQualitySettingsWidget::HandleApplyButtonClicked()
{
    ApplySelectedPreset();
}
