#include "PixelizerEditorProcessing.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "MaterialEditingLibrary.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace PixelizerEditorProcessing
{
	static FString JoinPath(const FString& Folder, const FString& AssetName)
	{
		if (Folder.EndsWith(TEXT("/")))
		{
			return Folder + AssetName;
		}
		return Folder + TEXT("/") + AssetName;
	}

	static int32 ScoreTextureParameter(const FName ParamName)
	{
		const FString Name = ParamName.ToString().ToLower();
		int32 Score = 0;

		if (Name.Contains(TEXT("basecolor")) || Name.Contains(TEXT("base_color"))) Score += 100;
		if (Name.Contains(TEXT("albedo"))) Score += 90;
		if (Name.Contains(TEXT("diffuse"))) Score += 80;
		if (Name.Contains(TEXT("color"))) Score += 40;

		if (Name.Contains(TEXT("normal"))) Score -= 200;
		if (Name.Contains(TEXT("orm")) || Name.Contains(TEXT("rma")) || Name.Contains(TEXT("mra"))) Score -= 180;
		if (Name.Contains(TEXT("rough")) || Name.Contains(TEXT("metal")) || Name.Contains(TEXT("ao"))) Score -= 140;
		if (Name.Contains(TEXT("mask")) || Name.Contains(TEXT("opacity"))) Score -= 120;

		return Score;
	}

	FString SanitizeAssetName(FString InName)
	{
		if (InName.IsEmpty())
		{
			return TEXT("PixelizerAsset");
		}

		for (TCHAR& Ch : InName)
		{
			if (!(FChar::IsAlnum(Ch) || Ch == TEXT('_')))
			{
				Ch = TEXT('_');
			}
		}

		while (InName.StartsWith(TEXT("_")))
		{
			InName.RightChopInline(1);
		}

		return InName.IsEmpty() ? TEXT("PixelizerAsset") : InName;
	}

	FString StableHashHex(const FString& Input)
	{
		FTCHARToUTF8 Utf8(*Input);
		FMD5 Md5;
		Md5.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		uint8 Digest[16];
		Md5.Final(Digest);
		return BytesToHex(Digest, UE_ARRAY_COUNT(Digest));
	}

	static FString BuildPaletteHash(const FPixelizerPaletteData& Palette)
	{
		FString Builder = Palette.PaletteName + TEXT("|");
		for (const FLinearColor& Color : Palette.Colors)
		{
			Builder += FString::Printf(TEXT("%.4f,%.4f,%.4f,%.4f;"), Color.R, Color.G, Color.B, Color.A);
		}
		return StableHashHex(Builder);
	}

	FString BuildEffectHash(const FPixelizerEffectSettings& Settings, const FPixelizerPaletteData& Palette, const FPixelizerOutlineSettings& Outline)
	{
		const FString Builder = FString::Printf(
			TEXT("PX=%d|PI=%.4f|DM=%d|DS=%.4f|B=%.4f|E=%.4f|C=%.4f|S=%.4f|BL=%.4f|A=%d|REUSE=%d|UNIQ=%d|OL=%d|OLT=%.4f|OLC=%.4f,%.4f,%.4f,%.4f|PA=%s"),
			Settings.PixelSize,
			Settings.PaletteImpact,
			static_cast<int32>(Settings.DitherMode),
			Settings.DitherStrength,
			Settings.Brightness,
			Settings.Exposure,
			Settings.Contrast,
			Settings.Saturation,
			Settings.BlurAmount,
			Settings.bPreserveAlpha ? 1 : 0,
			Settings.bReuseSharedGeneratedAssets ? 1 : 0,
			Settings.bUniquePerActor ? 1 : 0,
			Outline.bEnableOutline ? 1 : 0,
			Outline.Thickness,
			Outline.Color.R, Outline.Color.G, Outline.Color.B, Outline.Color.A,
			*BuildPaletteHash(Palette));
		return StableHashHex(Builder);
	}

	static FPixelizerGeneratedTextureCacheEntry* FindTextureCache(UPixelizerGeneratedRegistryAsset* Registry, const FString& KeyHash)
	{
		if (!Registry) return nullptr;
		for (FPixelizerGeneratedTextureCacheEntry& Entry : Registry->TextureCache)
		{
			if (Entry.KeyHash == KeyHash)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	static FPixelizerGeneratedMaterialCacheEntry* FindMaterialCache(UPixelizerGeneratedRegistryAsset* Registry, const FString& KeyHash)
	{
		if (!Registry) return nullptr;
		for (FPixelizerGeneratedMaterialCacheEntry& Entry : Registry->MaterialCache)
		{
			if (Entry.KeyHash == KeyHash)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	FPixelizerActorAssignmentRecord* FindActorRecord(UPixelizerGeneratedRegistryAsset* Registry, const FSoftObjectPath& ActorPath)
	{
		if (!Registry) return nullptr;
		for (FPixelizerActorAssignmentRecord& Record : Registry->ActorAssignments)
		{
			if (Record.ActorPath == ActorPath)
			{
				return &Record;
			}
		}
		return nullptr;
	}

	FPixelizerActorAssignmentRecord& FindOrAddActorRecord(UPixelizerGeneratedRegistryAsset* Registry, const FSoftObjectPath& ActorPath)
	{
		if (FPixelizerActorAssignmentRecord* Existing = FindActorRecord(Registry, ActorPath))
		{
			return *Existing;
		}
		FPixelizerActorAssignmentRecord NewRecord;
		NewRecord.ActorPath = ActorPath;
		return Registry->ActorAssignments.Add_GetRef(MoveTemp(NewRecord));
	}

	bool ResolveSelectedStaticMeshComponent(FResolvedSelection& OutSelection, FString* OutWarnings)
	{
		OutSelection = FResolvedSelection();

		if (!GEditor)
		{
			if (OutWarnings) *OutWarnings += TEXT("Editor is not available.\n");
			return false;
		}

		USelection* SelectedActors = GEditor->GetSelectedActors();
		if (!SelectedActors || SelectedActors->Num() == 0)
		{
			if (OutWarnings) *OutWarnings += TEXT("No actor selected.\n");
			return false;
		}

		int32 SelectedCount = 0;
		for (FSelectionIterator It(*SelectedActors); It; ++It)
		{
			++SelectedCount;
			AActor* Actor = Cast<AActor>(*It);
			if (!Actor)
			{
				continue;
			}

			UStaticMeshComponent* Component = nullptr;
			if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(Actor))
			{
				Component = SMA->GetStaticMeshComponent();
			}
			else
			{
				Component = Actor->FindComponentByClass<UStaticMeshComponent>();
			}

			if (!Component)
			{
				continue;
			}

			OutSelection.Actor = Actor;
			OutSelection.Component = Component;
			OutSelection.Key = Actor->GetPathName() + TEXT("|") + Component->GetPathName();
			if (OutWarnings && SelectedCount > 1)
			{
				*OutWarnings += TEXT("Multiple actors selected; using the first supported actor for single-actor editing.\n");
			}
			return true;
		}

		if (OutWarnings) *OutWarnings += TEXT("Selection does not include a supported StaticMeshActor/UStaticMeshComponent.\n");
		return false;
	}

	bool TryResolveActorToComponent(AActor* Actor, UStaticMeshComponent*& OutComponent)
	{
		OutComponent = nullptr;
		if (!Actor) return false;
		if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(Actor))
		{
			OutComponent = SMA->GetStaticMeshComponent();
			return OutComponent != nullptr;
		}
		OutComponent = Actor->FindComponentByClass<UStaticMeshComponent>();
		return OutComponent != nullptr;
	}

	FString GetSlotName(UStaticMeshComponent* Component, const int32 SlotIndex)
	{
		if (!Component)
		{
			return FString::Printf(TEXT("Slot %d"), SlotIndex);
		}
		if (const UStaticMesh* Mesh = Component->GetStaticMesh())
		{
			const TArray<FStaticMaterial>& Mats = Mesh->GetStaticMaterials();
			if (Mats.IsValidIndex(SlotIndex) && !Mats[SlotIndex].MaterialSlotName.IsNone())
			{
				return Mats[SlotIndex].MaterialSlotName.ToString();
			}
		}
		return FString::Printf(TEXT("Slot %d"), SlotIndex);
	}

	static bool AnalyzeMaterialForBaseColor(UMaterialInterface* Material, FPixelizerMaterialSlotAnalysis& InOutAnalysis)
	{
		InOutAnalysis.BaseColorParameterName = NAME_None;
		InOutAnalysis.BaseColorParameterInfo = FMaterialParameterInfo();
		InOutAnalysis.BaseColorTexture = nullptr;
		InOutAnalysis.bSupported = false;

		if (!Material)
		{
			InOutAnalysis.Status = TEXT("No material assigned.");
			InOutAnalysis.Notes.Add(TEXT("Empty slot."));
			return false;
		}

		TArray<FMaterialParameterInfo> ParamInfos;
		TArray<FGuid> ParamIds;
		Material->GetAllTextureParameterInfo(ParamInfos, ParamIds);
		if (ParamInfos.Num() == 0)
		{
			InOutAnalysis.Status = TEXT("Unsupported (no texture parameters exposed).");
			InOutAnalysis.Notes.Add(TEXT("Phase 1 supports materials/material instances with an exposed BaseColor-like texture parameter only."));
			return false;
		}

		int32 BestScore = TNumericLimits<int32>::Min();
		FName BestParam = NAME_None;
		FMaterialParameterInfo BestParamInfo;
		UTexture2D* BestTexture = nullptr;

		for (const FMaterialParameterInfo& Info : ParamInfos)
		{
			UTexture* TextureValue = nullptr;
			const bool bHasValue = Material->GetTextureParameterValue(FHashedMaterialParameterInfo(Info), TextureValue, false);
			if (!bHasValue || !TextureValue)
			{
				continue;
			}

			UTexture2D* Texture2D = Cast<UTexture2D>(TextureValue);
			if (!Texture2D)
			{
				InOutAnalysis.Notes.Add(FString::Printf(TEXT("Texture param '%s' is not a UTexture2D (skipped)."), *Info.Name.ToString()));
				continue;
			}

			const int32 Score = ScoreTextureParameter(Info.Name);
			if (Score > BestScore)
			{
				BestScore = Score;
				BestParam = Info.Name;
				BestParamInfo = Info;
				BestTexture = Texture2D;
			}
		}

		if (!BestTexture || BestParam.IsNone() || BestScore < 0)
		{
			InOutAnalysis.Status = TEXT("Unsupported (could not find a BaseColor-like texture parameter safely).");
			InOutAnalysis.Notes.Add(TEXT("Complex/custom material parameter naming may require Phase 2 improvements."));
			return false;
		}

		InOutAnalysis.BaseColorParameterName = BestParam;
		InOutAnalysis.BaseColorParameterInfo = BestParamInfo;
		InOutAnalysis.BaseColorTexture = BestTexture;
		InOutAnalysis.bSupported = true;
		InOutAnalysis.Status = FString::Printf(
			TEXT("Supported - BaseColor param '%s' (Assoc=%d, Index=%d) -> %s"),
			*BestParam.ToString(),
			static_cast<int32>(BestParamInfo.Association),
			BestParamInfo.Index,
			*BestTexture->GetName());
		return true;
	}

	void AnalyzeComponentMaterials(
		UStaticMeshComponent* Component,
		TArray<FPixelizerMaterialSlotAnalysis>& OutAnalyses,
		FString& OutMaterialReport,
		FString& OutWarnings,
		const TArray<TWeakObjectPtr<UMaterialInterface>>* SourceMaterialsOverride)
	{
		OutAnalyses.Reset();
		OutMaterialReport.Empty();

		if (!Component)
		{
			OutWarnings += TEXT("No static mesh component selected.\n");
			return;
		}

		const int32 NumMaterials = Component->GetNumMaterials();
		OutMaterialReport += FString::Printf(TEXT("Material slots: %d\n"), NumMaterials);
		for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
		{
			FPixelizerMaterialSlotAnalysis Analysis;
			Analysis.SlotIndex = SlotIndex;
			Analysis.SlotName = GetSlotName(Component, SlotIndex);
			if (SourceMaterialsOverride && SourceMaterialsOverride->IsValidIndex(SlotIndex))
			{
				Analysis.AssignedMaterial = (*SourceMaterialsOverride)[SlotIndex];
			}
			else
			{
				Analysis.AssignedMaterial = Component->GetMaterial(SlotIndex);
			}

			AnalyzeMaterialForBaseColor(Analysis.AssignedMaterial.Get(), Analysis);
			OutAnalyses.Add(Analysis);

			OutMaterialReport += FString::Printf(TEXT("[%d] %s\n"), SlotIndex, *Analysis.SlotName);
			OutMaterialReport += FString::Printf(TEXT("  Material: %s\n"), Analysis.AssignedMaterial.IsValid() ? *Analysis.AssignedMaterial->GetPathName() : TEXT("<None>"));
			OutMaterialReport += FString::Printf(TEXT("  Status: %s\n"), *Analysis.Status);
			if (Analysis.BaseColorTexture.IsValid())
			{
				OutMaterialReport += FString::Printf(TEXT("  BaseColor Texture: %s\n"), *Analysis.BaseColorTexture->GetPathName());
			}
			for (const FString& Note : Analysis.Notes)
			{
				OutMaterialReport += FString::Printf(TEXT("  Note: %s\n"), *Note);
			}
			OutMaterialReport += TEXT("\n");

			if (!Analysis.bSupported)
			{
				OutWarnings += FString::Printf(TEXT("[%s] %s\n"), *Analysis.SlotName, *Analysis.Status);
			}
		}
	}

	static FLinearColor ApplyAdjustments(const FLinearColor& InColor, const FPixelizerEffectSettings& Settings)
	{
		FLinearColor C = InColor;
		const float ExposureScale = FMath::Pow(2.0f, Settings.Exposure);
		C.R = C.R * ExposureScale + Settings.Brightness;
		C.G = C.G * ExposureScale + Settings.Brightness;
		C.B = C.B * ExposureScale + Settings.Brightness;

		C.R = (C.R - 0.5f) * Settings.Contrast + 0.5f;
		C.G = (C.G - 0.5f) * Settings.Contrast + 0.5f;
		C.B = (C.B - 0.5f) * Settings.Contrast + 0.5f;

		const float Luma = 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B;
		C.R = FMath::Lerp(Luma, C.R, Settings.Saturation);
		C.G = FMath::Lerp(Luma, C.G, Settings.Saturation);
		C.B = FMath::Lerp(Luma, C.B, Settings.Saturation);

		C.R = FMath::Clamp(C.R, 0.0f, 1.0f);
		C.G = FMath::Clamp(C.G, 0.0f, 1.0f);
		C.B = FMath::Clamp(C.B, 0.0f, 1.0f);
		C.A = FMath::Clamp(C.A, 0.0f, 1.0f);
		return C;
	}

	static float GetDitherValue(const EPixelizerDitherMode Mode, const int32 X, const int32 Y)
	{
		switch (Mode)
		{
		case EPixelizerDitherMode::Ordered2x2:
		{
			static const int32 Matrix[2][2] = {{0, 2}, {3, 1}};
			return (static_cast<float>(Matrix[Y & 1][X & 1]) / 4.0f) - 0.375f;
		}
		case EPixelizerDitherMode::Ordered4x4:
		{
			static const int32 Matrix[4][4] = {
				{0, 8, 2, 10},
				{12, 4, 14, 6},
				{3, 11, 1, 9},
				{15, 7, 13, 5}};
			return (static_cast<float>(Matrix[Y & 3][X & 3]) / 16.0f) - 0.46875f;
		}
		case EPixelizerDitherMode::RandomNoise:
		{
			const uint32 Seed = HashCombine(::GetTypeHash(X), ::GetTypeHash(Y * 92821));
			return (static_cast<float>(Seed & 0xFF) / 255.0f) - 0.5f;
		}
		case EPixelizerDitherMode::None:
		default:
			return 0.0f;
		}
	}

	static FLinearColor QuantizeToPalette(const FLinearColor& InColor, const TArray<FLinearColor>& Palette, const FPixelizerEffectSettings& Settings, const int32 X, const int32 Y)
	{
		if (Palette.Num() == 0 || Settings.PaletteImpact <= KINDA_SMALL_NUMBER)
		{
			return InColor;
		}

		FLinearColor Candidate = InColor;
		const float Dither = GetDitherValue(Settings.DitherMode, X, Y) * Settings.DitherStrength;
		Candidate.R = FMath::Clamp(Candidate.R + Dither, 0.0f, 1.0f);
		Candidate.G = FMath::Clamp(Candidate.G + Dither, 0.0f, 1.0f);
		Candidate.B = FMath::Clamp(Candidate.B + Dither, 0.0f, 1.0f);

		int32 BestIndex = 0;
		float BestDist = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index < Palette.Num(); ++Index)
		{
			const FLinearColor P = Palette[Index];
			const float Dr = Candidate.R - P.R;
			const float Dg = Candidate.G - P.G;
			const float Db = Candidate.B - P.B;
			const float Dist = Dr * Dr + Dg * Dg + Db * Db;
			if (Dist < BestDist)
			{
				BestDist = Dist;
				BestIndex = Index;
			}
		}

		FLinearColor Quantized = Palette[BestIndex];
		Quantized.A = InColor.A;
		return FMath::Lerp(InColor, Quantized, FMath::Clamp(Settings.PaletteImpact, 0.0f, 1.0f));
	}

	static void BoxBlurInPlace(TArray<FLinearColor>& Pixels, const int32 Width, const int32 Height, const int32 Radius)
	{
		if (Radius <= 0 || Width <= 0 || Height <= 0)
		{
			return;
		}

		TArray<FLinearColor> Src = Pixels;
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				FLinearColor Sum = FLinearColor::Transparent;
				int32 Count = 0;
				for (int32 Oy = -Radius; Oy <= Radius; ++Oy)
				{
					const int32 Sy = FMath::Clamp(Y + Oy, 0, Height - 1);
					for (int32 Ox = -Radius; Ox <= Radius; ++Ox)
					{
						const int32 Sx = FMath::Clamp(X + Ox, 0, Width - 1);
						Sum += Src[Sy * Width + Sx];
						++Count;
					}
				}
				Pixels[Y * Width + X] = Sum / static_cast<float>(Count);
			}
		}
	}

	static float ColorEdgeDistanceSq(const FLinearColor& A, const FLinearColor& B)
	{
		const float Dr = A.R - B.R;
		const float Dg = A.G - B.G;
		const float Db = A.B - B.B;
		return (Dr * Dr) + (Dg * Dg) + (Db * Db);
	}

	static void ApplyTextureBakedOutlineInPlace(
		TArray<FLinearColor>& Pixels,
		const int32 Width,
		const int32 Height,
		const FPixelizerOutlineSettings& Outline,
		const bool bPreserveAlpha)
	{
		if (!Outline.bEnableOutline || Pixels.Num() != Width * Height || Width <= 1 || Height <= 1)
		{
			return;
		}

		const int32 Radius = FMath::Clamp(FMath::CeilToInt(Outline.Thickness), 1, 8);
		constexpr float ColorThresholdSq = 0.015f * 0.015f;
		constexpr float AlphaThreshold = 0.10f;

		TBitArray<> EdgeMask(false, Width * Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const int32 Index = (Y * Width) + X;
				const FLinearColor& C = Pixels[Index];
				bool bEdge = false;

				auto CheckNeighbor = [&](const int32 NX, const int32 NY)
				{
					if (bEdge || NX < 0 || NX >= Width || NY < 0 || NY >= Height)
					{
						return;
					}

					const FLinearColor& N = Pixels[(NY * Width) + NX];
					if (ColorEdgeDistanceSq(C, N) > ColorThresholdSq)
					{
						bEdge = true;
						return;
					}

					if (FMath::Abs(C.A - N.A) > AlphaThreshold)
					{
						bEdge = true;
					}
				};

				CheckNeighbor(X - 1, Y);
				CheckNeighbor(X + 1, Y);
				CheckNeighbor(X, Y - 1);
				CheckNeighbor(X, Y + 1);
				EdgeMask[Index] = bEdge;
			}
		}

		TBitArray<> DilatedMask(false, Width * Height);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				bool bHit = false;
				for (int32 Oy = -Radius; Oy <= Radius && !bHit; ++Oy)
				{
					const int32 NY = Y + Oy;
					if (NY < 0 || NY >= Height)
					{
						continue;
					}

					for (int32 Ox = -Radius; Ox <= Radius; ++Ox)
					{
						const int32 NX = X + Ox;
						if (NX < 0 || NX >= Width)
						{
							continue;
						}

						if ((Ox * Ox) + (Oy * Oy) > (Radius * Radius))
						{
							continue;
						}

						if (EdgeMask[(NY * Width) + NX])
						{
							bHit = true;
							break;
						}
					}
				}

				DilatedMask[(Y * Width) + X] = bHit;
			}
		}

		const FLinearColor OutlineColorLinear = FLinearColor(
			FMath::Clamp(Outline.Color.R, 0.0f, 1.0f),
			FMath::Clamp(Outline.Color.G, 0.0f, 1.0f),
			FMath::Clamp(Outline.Color.B, 0.0f, 1.0f),
			FMath::Clamp(Outline.Color.A, 0.0f, 1.0f));

		for (int32 Index = 0; Index < Pixels.Num(); ++Index)
		{
			if (!DilatedMask[Index])
			{
				continue;
			}

			const float OriginalAlpha = Pixels[Index].A;
			FLinearColor Final = FMath::Lerp(Pixels[Index], OutlineColorLinear, OutlineColorLinear.A);
			if (bPreserveAlpha)
			{
				Final.A = OriginalAlpha;
			}
			Pixels[Index] = Final;
		}
	}

	bool ReadAndProcessTextureSource(
		UTexture2D* SourceTexture,
		const FPixelizerEffectSettings& Settings,
		const TArray<FLinearColor>& PaletteColors,
		const FPixelizerOutlineSettings& Outline,
		FTextureProcessOutput& Out)
	{
		Out = FTextureProcessOutput();
		if (!SourceTexture)
		{
			Out.Error = TEXT("Missing source texture.");
			return false;
		}

		if (!SourceTexture->Source.IsValid() || SourceTexture->Source.GetNumMips() <= 0)
		{
			Out.Error = FString::Printf(TEXT("Texture '%s' has no editable source art."), *SourceTexture->GetPathName());
			return false;
		}

		const ETextureSourceFormat SourceFormat = SourceTexture->Source.GetFormat();
		if (SourceFormat != TSF_BGRA8)
		{
			Out.Error = FString::Printf(TEXT("Texture '%s' source format '%d' is unsupported in Phase 1 (expected BGRA8 source art)."), *SourceTexture->GetPathName(), static_cast<int32>(SourceFormat));
			return false;
		}

		const int32 Width = static_cast<int32>(SourceTexture->Source.GetSizeX());
		const int32 Height = static_cast<int32>(SourceTexture->Source.GetSizeY());
		if (Width <= 0 || Height <= 0)
		{
			Out.Error = TEXT("Texture has invalid dimensions.");
			return false;
		}

		TArray64<uint8> RawBytes;
		if (!SourceTexture->Source.GetMipData(RawBytes, 0))
		{
			Out.Error = FString::Printf(TEXT("Failed to read mip0 source data for '%s'."), *SourceTexture->GetPathName());
			return false;
		}

		const int64 RequiredBytes = static_cast<int64>(Width) * static_cast<int64>(Height) * 4;
		if (RawBytes.Num() < RequiredBytes)
		{
			Out.Error = TEXT("Texture source data is smaller than expected.");
			return false;
		}

		TArray<FLinearColor> WorkingPixels;
		WorkingPixels.SetNum(Width * Height);
		const bool bBGRA = true;
		for (int32 Index = 0; Index < Width * Height; ++Index)
		{
			const int32 Offset = Index * 4;
			FColor Srgb;
			if (bBGRA)
			{
				Srgb.B = RawBytes[Offset + 0];
				Srgb.G = RawBytes[Offset + 1];
				Srgb.R = RawBytes[Offset + 2];
				Srgb.A = RawBytes[Offset + 3];
			}
			else
			{
				Srgb.R = RawBytes[Offset + 0];
				Srgb.G = RawBytes[Offset + 1];
				Srgb.B = RawBytes[Offset + 2];
				Srgb.A = RawBytes[Offset + 3];
			}
			WorkingPixels[Index] = FLinearColor::FromSRGBColor(Srgb);
		}

		if (Settings.BlurAmount > KINDA_SMALL_NUMBER)
		{
			const int32 Radius = FMath::Clamp(FMath::RoundToInt(Settings.BlurAmount), 1, 4);
			BoxBlurInPlace(WorkingPixels, Width, Height, Radius);
		}

		TArray<FLinearColor> OutputLinear;
		OutputLinear.SetNum(Width * Height);
		const int32 BlockSize = FMath::Max(1, Settings.PixelSize);
		for (int32 StartY = 0; StartY < Height; StartY += BlockSize)
		{
			for (int32 StartX = 0; StartX < Width; StartX += BlockSize)
			{
				const int32 EndY = FMath::Min(StartY + BlockSize, Height);
				const int32 EndX = FMath::Min(StartX + BlockSize, Width);

				FLinearColor Avg = FLinearColor::Transparent;
				int32 Count = 0;
				for (int32 Y = StartY; Y < EndY; ++Y)
				{
					for (int32 X = StartX; X < EndX; ++X)
					{
						Avg += WorkingPixels[Y * Width + X];
						++Count;
					}
				}
				Avg /= FMath::Max(1, Count);
				const FLinearColor Adjusted = ApplyAdjustments(Avg, Settings);

				for (int32 Y = StartY; Y < EndY; ++Y)
				{
					for (int32 X = StartX; X < EndX; ++X)
					{
						FLinearColor FinalColor = QuantizeToPalette(Adjusted, PaletteColors, Settings, X, Y);
						if (Settings.bPreserveAlpha)
						{
							FinalColor.A = WorkingPixels[Y * Width + X].A;
						}
						OutputLinear[Y * Width + X] = FinalColor;
					}
				}
			}
		}

		ApplyTextureBakedOutlineInPlace(OutputLinear, Width, Height, Outline, Settings.bPreserveAlpha);

		Out.Bytes.SetNumUninitialized(RequiredBytes);
		for (int32 Index = 0; Index < Width * Height; ++Index)
		{
			const FColor Srgb = OutputLinear[Index].ToFColorSRGB();
			const int32 Offset = Index * 4;
			if (bBGRA)
			{
				Out.Bytes[Offset + 0] = Srgb.B;
				Out.Bytes[Offset + 1] = Srgb.G;
				Out.Bytes[Offset + 2] = Srgb.R;
				Out.Bytes[Offset + 3] = Srgb.A;
			}
			else
			{
				Out.Bytes[Offset + 0] = Srgb.R;
				Out.Bytes[Offset + 1] = Srgb.G;
				Out.Bytes[Offset + 2] = Srgb.B;
				Out.Bytes[Offset + 3] = Srgb.A;
			}
		}

		Out.Width = Width;
		Out.Height = Height;
		Out.SourceFormat = SourceFormat;
		return true;
	}

	bool WriteTextureSource(UTexture2D* Texture, const FTextureProcessOutput& ProcessOutput)
	{
		if (!Texture || ProcessOutput.Width <= 0 || ProcessOutput.Height <= 0 || ProcessOutput.Bytes.Num() == 0)
		{
			return false;
		}

		Texture->Modify();
		Texture->PreEditChange(nullptr);
		Texture->Source.Init(ProcessOutput.Width, ProcessOutput.Height, 1, 1, ProcessOutput.SourceFormat, ProcessOutput.Bytes.GetData());
		Texture->PostEditChange();
		Texture->UpdateResource();
		Texture->MarkPackageDirty();
		if (UPackage* Package = Texture->GetOutermost())
		{
			Package->MarkPackageDirty();
		}
		return true;
	}

	UTexture2D* DuplicateOrReuseTextureAsset(UPixelizerGeneratedRegistryAsset* Registry, const FString& TextureKeyHash, UTexture2D* SourceTexture, const FTextureProcessOutput& ProcessOutput, const FString& PresetHash, FString& InOutWarnings)
	{
		if (!SourceTexture) return nullptr;

		if (FPixelizerGeneratedTextureCacheEntry* Cache = FindTextureCache(Registry, TextureKeyHash))
		{
			if (UTexture2D* Reused = Cast<UTexture2D>(Cache->GeneratedTexturePath.TryLoad()))
			{
				return Reused;
			}
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		const FString AssetName = FString::Printf(TEXT("T_Pixel_%s"), *TextureKeyHash.Left(12));
		UObject* DuplicatedObject = AssetTools.DuplicateAsset(AssetName, GeneratedTexturesFolder, SourceTexture);
		UTexture2D* NewTexture = Cast<UTexture2D>(DuplicatedObject);
		if (!NewTexture)
		{
			InOutWarnings += FString::Printf(TEXT("Failed to duplicate texture asset '%s'.\n"), *SourceTexture->GetPathName());
			return nullptr;
		}

		if (!WriteTextureSource(NewTexture, ProcessOutput))
		{
			InOutWarnings += FString::Printf(TEXT("Failed to write processed pixels to '%s'.\n"), *NewTexture->GetPathName());
			return nullptr;
		}

		FPixelizerGeneratedTextureCacheEntry NewEntry;
		NewEntry.KeyHash = TextureKeyHash;
		NewEntry.PresetHash = PresetHash;
		NewEntry.SourceTexturePath = FSoftObjectPath(SourceTexture);
		NewEntry.GeneratedTexturePath = FSoftObjectPath(NewTexture);

		if (FPixelizerGeneratedTextureCacheEntry* Existing = FindTextureCache(Registry, TextureKeyHash))
		{
			*Existing = NewEntry;
		}
		else
		{
			Registry->TextureCache.Add(MoveTemp(NewEntry));
		}

		return NewTexture;
	}

	UMaterialInstanceConstant* CreateOrReuseMaterialOverride(UPixelizerGeneratedRegistryAsset* Registry, const FString& MaterialKeyHash, UMaterialInterface* SourceMaterial, const FMaterialParameterInfo& TextureParamInfo, UTexture2D* GeneratedTexture, const FString& PresetHash, FString& InOutWarnings)
	{
		if (!SourceMaterial || !GeneratedTexture || TextureParamInfo.Name.IsNone())
		{
			return nullptr;
		}

		if (FPixelizerGeneratedMaterialCacheEntry* Cache = FindMaterialCache(Registry, MaterialKeyHash))
		{
			if (UMaterialInstanceConstant* Reused = Cast<UMaterialInstanceConstant>(Cache->GeneratedMaterialPath.TryLoad()))
			{
				return Reused;
			}
		}

		UMaterialInstanceConstant* MaterialInstance = nullptr;
		const FString AssetName = FString::Printf(TEXT("MI_Pixel_%s"), *MaterialKeyHash.Left(12));
		if (UMaterialInstanceConstant* SourceMIC = Cast<UMaterialInstanceConstant>(SourceMaterial))
		{
			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
			MaterialInstance = Cast<UMaterialInstanceConstant>(AssetTools.DuplicateAsset(AssetName, GeneratedMaterialsFolder, SourceMIC));
		}
		else
		{
			MaterialInstance = LoadOrCreateDataAsset<UMaterialInstanceConstant>(GeneratedMaterialsFolder, AssetName);
			if (MaterialInstance)
			{
				UMaterialEditingLibrary::SetMaterialInstanceParent(MaterialInstance, SourceMaterial);
			}
		}

		if (!MaterialInstance)
		{
			InOutWarnings += FString::Printf(TEXT("Failed to create material override for '%s'.\n"), *SourceMaterial->GetPathName());
			return nullptr;
		}

		// MaterialEditingLibrary::SetMaterialInstanceTextureParameterValue() returns false in UE 5.7
		// even after applying the value (engine implementation bug). Use the editor-only API directly
		// and preserve the exact parameter association/index discovered during analysis.
		MaterialInstance->SetTextureParameterValueEditorOnly(TextureParamInfo, GeneratedTexture);
		UMaterialEditingLibrary::UpdateMaterialInstance(MaterialInstance);

		UTexture* ConfirmedTexture = nullptr;
		const bool bResolvedAfterSet = MaterialInstance->GetTextureParameterValue(FHashedMaterialParameterInfo(TextureParamInfo), ConfirmedTexture, false);
		if (!bResolvedAfterSet || ConfirmedTexture != GeneratedTexture)
		{
			InOutWarnings += FString::Printf(
				TEXT("Failed to set texture parameter '%s' (Assoc=%d, Index=%d) on '%s'.\n"),
				*TextureParamInfo.Name.ToString(),
				static_cast<int32>(TextureParamInfo.Association),
				TextureParamInfo.Index,
				*MaterialInstance->GetPathName());
			return nullptr;
		}
		MaterialInstance->MarkPackageDirty();
		if (UPackage* Package = MaterialInstance->GetOutermost())
		{
			Package->MarkPackageDirty();
		}

		FPixelizerGeneratedMaterialCacheEntry NewEntry;
		NewEntry.KeyHash = MaterialKeyHash;
		NewEntry.PresetHash = PresetHash;
		NewEntry.SourceMaterialPath = FSoftObjectPath(SourceMaterial);
		NewEntry.GeneratedMaterialPath = FSoftObjectPath(MaterialInstance);
		if (FPixelizerGeneratedMaterialCacheEntry* Existing = FindMaterialCache(Registry, MaterialKeyHash))
		{
			*Existing = NewEntry;
		}
		else
		{
			Registry->MaterialCache.Add(MoveTemp(NewEntry));
		}

		return MaterialInstance;
	}

	void AssignMaterialsToComponent(UStaticMeshComponent* Component, const TArray<TWeakObjectPtr<UMaterialInterface>>& Materials)
	{
		if (!Component)
		{
			return;
		}

		for (int32 SlotIndex = 0; SlotIndex < Materials.Num(); ++SlotIndex)
		{
			Component->SetMaterial(SlotIndex, Materials[SlotIndex].Get());
		}
		Component->MarkRenderStateDirty();
	}

	bool ApplyToComponentWithAssets(
		UPixelizerGeneratedRegistryAsset* Registry,
		AActor* Actor,
		UStaticMeshComponent* Component,
		const TArray<FPixelizerMaterialSlotAnalysis>& Analyses,
		const TArray<TWeakObjectPtr<UMaterialInterface>>& OriginalMaterialsBySlot,
		const FPixelizerEffectSettings& Settings,
		const FPixelizerPaletteData& Palette,
		const FPixelizerOutlineSettings& Outline,
		const FString& PresetHash,
		FApplyComponentResult& OutResult)
	{
		OutResult = FApplyComponentResult();
		OutResult.Status = TEXT("Nothing applied.");

		if (!Registry)
		{
			OutResult.Warnings += TEXT("Registry asset is missing.\n");
			return false;
		}
		if (!Actor || !Component)
		{
			OutResult.Warnings += TEXT("Invalid actor/component.\n");
			return false;
		}

		TArray<TWeakObjectPtr<UMaterialInterface>> OverrideMaterials = OriginalMaterialsBySlot;
		OverrideMaterials.SetNum(FMath::Max(OverrideMaterials.Num(), Component->GetNumMaterials()));

		int32 AppliedSlotCount = 0;
		for (const FPixelizerMaterialSlotAnalysis& Analysis : Analyses)
		{
			if (!Analysis.bSupported)
			{
				continue;
			}
			OutResult.bAnySupported = true;

			UMaterialInterface* SourceMaterial = OriginalMaterialsBySlot.IsValidIndex(Analysis.SlotIndex) ? OriginalMaterialsBySlot[Analysis.SlotIndex].Get() : Analysis.AssignedMaterial.Get();
			UTexture2D* SourceTexture = Analysis.BaseColorTexture.Get();
			if (!SourceMaterial || !SourceTexture)
			{
				OutResult.Warnings += FString::Printf(TEXT("Slot %d skipped (missing source material/texture).\n"), Analysis.SlotIndex);
				continue;
			}

			FTextureProcessOutput ProcessOutput;
			if (!ReadAndProcessTextureSource(SourceTexture, Settings, Palette.Colors, Outline, ProcessOutput))
			{
				OutResult.Warnings += FString::Printf(TEXT("Slot %d skipped: %s\n"), Analysis.SlotIndex, *ProcessOutput.Error);
				continue;
			}

			const FString UniqueScope = Settings.bUniquePerActor ? Actor->GetPathName() : FString();
			const FString TextureKey = StableHashHex(FString::Printf(TEXT("T|SRC=%s|PRE=%s|SCOPE=%s"), *SourceTexture->GetPathName(), *PresetHash, *UniqueScope));
			UTexture2D* GeneratedTexture = DuplicateOrReuseTextureAsset(Registry, TextureKey, SourceTexture, ProcessOutput, PresetHash, OutResult.Warnings);
			if (!GeneratedTexture)
			{
				continue;
			}

			const FString MaterialKey = StableHashHex(FString::Printf(
				TEXT("M|SRC=%s|TEX=%s|PARAM=%s|ASSOC=%d|PIDX=%d|PRE=%s|SCOPE=%s"),
				*SourceMaterial->GetPathName(),
				*GeneratedTexture->GetPathName(),
				*Analysis.BaseColorParameterName.ToString(),
				static_cast<int32>(Analysis.BaseColorParameterInfo.Association),
				Analysis.BaseColorParameterInfo.Index,
				*PresetHash,
				*UniqueScope));
			UMaterialInstanceConstant* OverrideMIC = CreateOrReuseMaterialOverride(
				Registry,
				MaterialKey,
				SourceMaterial,
				Analysis.BaseColorParameterInfo,
				GeneratedTexture,
				PresetHash,
				OutResult.Warnings);
			if (!OverrideMIC)
			{
				continue;
			}

			if (OverrideMaterials.IsValidIndex(Analysis.SlotIndex))
			{
				OverrideMaterials[Analysis.SlotIndex] = OverrideMIC;
				++AppliedSlotCount;
				OutResult.bAnyApplied = true;
			}
		}

		if (!OutResult.bAnyApplied)
		{
			OutResult.Status = OutResult.bAnySupported
				? TEXT("Apply finished with no changes (all supported slots failed safely).")
				: TEXT("Apply skipped: no supported material slots.");
			return false;
		}

		AssignMaterialsToComponent(Component, OverrideMaterials);
		OutResult.OverrideMaterialsBySlot = OverrideMaterials;
		OutResult.Status = FString::Printf(TEXT("Applied Pixelizer preset to %s (%d slot(s) updated)."), *Actor->GetName(), AppliedSlotCount);

		FPixelizerActorAssignmentRecord& Record = FindOrAddActorRecord(Registry, FSoftObjectPath(Actor));
		Record.LastPresetHash = PresetHash;
		Record.OriginalMaterialsBySlot.SetNum(OriginalMaterialsBySlot.Num());
		for (int32 Index = 0; Index < OriginalMaterialsBySlot.Num(); ++Index)
		{
			Record.OriginalMaterialsBySlot[Index] = FSoftObjectPath(OriginalMaterialsBySlot[Index].Get());
		}
		Record.LastAppliedOverrideMaterialsBySlot.SetNum(OverrideMaterials.Num());
		for (int32 Index = 0; Index < OverrideMaterials.Num(); ++Index)
		{
			Record.LastAppliedOverrideMaterialsBySlot[Index] = FSoftObjectPath(OverrideMaterials[Index].Get());
		}

		Registry->MarkPackageDirty();
		if (UPackage* Package = Registry->GetOutermost())
		{
			Package->MarkPackageDirty();
		}

		if (Outline.bEnableOutline)
		{
			OutResult.Warnings += TEXT("Outline applied as a texture-baked edge overlay (texture details/regions only; not a world-space silhouette outline).\n");
		}

		return true;
	}
}
