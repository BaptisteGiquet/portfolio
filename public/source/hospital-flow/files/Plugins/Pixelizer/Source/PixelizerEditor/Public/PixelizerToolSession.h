#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PixelizerTypes.h"
#include "PixelizerToolSession.generated.h"

UENUM()
enum class EPixelizerBuiltInPalette : uint8
{
	Retro16,
	GameBoy,
	Grayscale4,
	Vibrant8
};

UCLASS(Transient)
class PIXELIZEREDITOR_API UPixelizerToolSession : public UObject
{
	GENERATED_BODY()

public:
	UPixelizerToolSession();

	UPROPERTY(EditAnywhere, Category = "Selection")
	bool bAutoTrackSelection = true;

	UPROPERTY(EditAnywhere, Category = "Selection")
	bool bIncludeAllStaticMeshComponentsOnActor = false;

	UPROPERTY(EditAnywhere, Category = "Pixelizer")
	FPixelizerEffectSettings EffectSettings;

	UPROPERTY(EditAnywhere, Category = "Palette")
	EPixelizerBuiltInPalette BuiltInPalette = EPixelizerBuiltInPalette::Retro16;

	UPROPERTY(EditAnywhere, Category = "Palette")
	FString WorkingPaletteName = TEXT("Custom");

	UPROPERTY(EditAnywhere, Category = "Palette")
	TArray<FLinearColor> WorkingPaletteColors;

	UPROPERTY(EditAnywhere, Category = "Outline")
	FPixelizerOutlineSettings OutlineSettings;

	UPROPERTY(EditAnywhere, Category = "Presets")
	FString PresetAssetName = TEXT("DA_PixelizerPreset");

	UPROPERTY(EditAnywhere, Category = "Presets")
	FString PresetAssetFolder = TEXT("/Game/_Pixelizer/Presets");

	UPROPERTY(EditAnywhere, Category = "Presets")
	TObjectPtr<UPixelizerEffectPresetAsset> SelectedPresetAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "Palettes")
	FString PaletteAssetName = TEXT("DA_PixelizerPalette");

	UPROPERTY(EditAnywhere, Category = "Palettes")
	FString PaletteAssetFolder = TEXT("/Game/_Pixelizer/Palettes");

	UPROPERTY(EditAnywhere, Category = "Palettes")
	TObjectPtr<UPixelizerPaletteAsset> SelectedPaletteAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "Batch")
	bool bBatchUseSelectedPresetAsset = false;

	UPROPERTY(EditAnywhere, Category = "Batch")
	TObjectPtr<UPixelizerEffectPresetAsset> BatchPresetAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "Batch")
	bool bBatchDryRunBeforeApply = true;

	void ApplyBuiltInPalette();
	void LoadFromPaletteAsset();
	void WriteToPaletteAsset(UPixelizerPaletteAsset* Asset) const;
	void LoadFromPresetAsset();
	void WriteToPresetAsset(UPixelizerEffectPresetAsset* Asset) const;

	FPixelizerPaletteData MakePaletteData() const;
	UPixelizerEffectPresetAsset* GetEffectiveBatchPreset() const;
};
