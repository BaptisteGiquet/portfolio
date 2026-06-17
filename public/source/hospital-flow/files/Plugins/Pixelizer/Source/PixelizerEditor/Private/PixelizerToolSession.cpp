#include "PixelizerToolSession.h"

namespace PixelizerToolSessionInternal
{
	static TArray<FLinearColor> MakeRetro16()
	{
		return {
			FLinearColor::FromSRGBColor(FColor(0x14, 0x0C, 0x1C)),
			FLinearColor::FromSRGBColor(FColor(0x44, 0x24, 0x34)),
			FLinearColor::FromSRGBColor(FColor(0x30, 0x34, 0x6D)),
			FLinearColor::FromSRGBColor(FColor(0x4E, 0x4A, 0x4E)),
			FLinearColor::FromSRGBColor(FColor(0x85, 0x4C, 0x30)),
			FLinearColor::FromSRGBColor(FColor(0x34, 0x65, 0x24)),
			FLinearColor::FromSRGBColor(FColor(0xD0, 0x46, 0x48)),
			FLinearColor::FromSRGBColor(FColor(0x75, 0x71, 0x61)),
			FLinearColor::FromSRGBColor(FColor(0x59, 0x7D, 0xCE)),
			FLinearColor::FromSRGBColor(FColor(0xD2, 0x7D, 0x2C)),
			FLinearColor::FromSRGBColor(FColor(0x85, 0x95, 0xA1)),
			FLinearColor::FromSRGBColor(FColor(0x6D, 0xAA, 0x2C)),
			FLinearColor::FromSRGBColor(FColor(0xD2, 0xAA, 0x99)),
			FLinearColor::FromSRGBColor(FColor(0x6D, 0xC2, 0xCA)),
			FLinearColor::FromSRGBColor(FColor(0xDA, 0xD4, 0x5E)),
			FLinearColor::FromSRGBColor(FColor(0xDE, 0xEE, 0xD6))
		};
	}

	static TArray<FLinearColor> MakeGameBoy()
	{
		return {
			FLinearColor::FromSRGBColor(FColor(15, 56, 15)),
			FLinearColor::FromSRGBColor(FColor(48, 98, 48)),
			FLinearColor::FromSRGBColor(FColor(139, 172, 15)),
			FLinearColor::FromSRGBColor(FColor(155, 188, 15))
		};
	}

	static TArray<FLinearColor> MakeGrayscale4()
	{
		return {
			FLinearColor(0.05f, 0.05f, 0.05f, 1.0f),
			FLinearColor(0.30f, 0.30f, 0.30f, 1.0f),
			FLinearColor(0.65f, 0.65f, 0.65f, 1.0f),
			FLinearColor(0.95f, 0.95f, 0.95f, 1.0f)
		};
	}

	static TArray<FLinearColor> MakeVibrant8()
	{
		return {
			FLinearColor::FromSRGBColor(FColor(28, 28, 28)),
			FLinearColor::FromSRGBColor(FColor(255, 77, 109)),
			FLinearColor::FromSRGBColor(FColor(255, 176, 0)),
			FLinearColor::FromSRGBColor(FColor(253, 255, 0)),
			FLinearColor::FromSRGBColor(FColor(0, 200, 83)),
			FLinearColor::FromSRGBColor(FColor(0, 180, 216)),
			FLinearColor::FromSRGBColor(FColor(131, 56, 236)),
			FLinearColor::FromSRGBColor(FColor(245, 245, 245))
		};
	}
}

UPixelizerToolSession::UPixelizerToolSession()
{
	ApplyBuiltInPalette();
}

void UPixelizerToolSession::ApplyBuiltInPalette()
{
	switch (BuiltInPalette)
	{
	case EPixelizerBuiltInPalette::Retro16:
		WorkingPaletteName = TEXT("Retro16");
		WorkingPaletteColors = PixelizerToolSessionInternal::MakeRetro16();
		break;
	case EPixelizerBuiltInPalette::GameBoy:
		WorkingPaletteName = TEXT("GameBoy");
		WorkingPaletteColors = PixelizerToolSessionInternal::MakeGameBoy();
		break;
	case EPixelizerBuiltInPalette::Grayscale4:
		WorkingPaletteName = TEXT("Grayscale4");
		WorkingPaletteColors = PixelizerToolSessionInternal::MakeGrayscale4();
		break;
	case EPixelizerBuiltInPalette::Vibrant8:
		WorkingPaletteName = TEXT("Vibrant8");
		WorkingPaletteColors = PixelizerToolSessionInternal::MakeVibrant8();
		break;
	default:
		break;
	}
}

FPixelizerPaletteData UPixelizerToolSession::MakePaletteData() const
{
	FPixelizerPaletteData PaletteData;
	PaletteData.PaletteName = WorkingPaletteName;
	PaletteData.Colors = WorkingPaletteColors;
	return PaletteData;
}

void UPixelizerToolSession::LoadFromPaletteAsset()
{
	if (!SelectedPaletteAsset)
	{
		return;
	}

	WorkingPaletteName = SelectedPaletteAsset->Palette.PaletteName;
	WorkingPaletteColors = SelectedPaletteAsset->Palette.Colors;
}

void UPixelizerToolSession::WriteToPaletteAsset(UPixelizerPaletteAsset* Asset) const
{
	if (!Asset)
	{
		return;
	}

	Asset->Palette = MakePaletteData();
}

void UPixelizerToolSession::LoadFromPresetAsset()
{
	if (!SelectedPresetAsset)
	{
		return;
	}

	EffectSettings = SelectedPresetAsset->EffectSettings;
	OutlineSettings = SelectedPresetAsset->OutlineSettings;
	WorkingPaletteName = SelectedPresetAsset->Palette.PaletteName;
	WorkingPaletteColors = SelectedPresetAsset->Palette.Colors;
}

void UPixelizerToolSession::WriteToPresetAsset(UPixelizerEffectPresetAsset* Asset) const
{
	if (!Asset)
	{
		return;
	}

	Asset->PresetName = PresetAssetName;
	Asset->EffectSettings = EffectSettings;
	Asset->Palette = MakePaletteData();
	Asset->OutlineSettings = OutlineSettings;
}

UPixelizerEffectPresetAsset* UPixelizerToolSession::GetEffectiveBatchPreset() const
{
	if (bBatchUseSelectedPresetAsset && BatchPresetAsset)
	{
		return BatchPresetAsset;
	}

	return SelectedPresetAsset;
}
