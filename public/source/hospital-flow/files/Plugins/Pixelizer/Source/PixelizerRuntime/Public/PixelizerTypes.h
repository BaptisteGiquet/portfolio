#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PixelizerTypes.generated.h"

UENUM(BlueprintType)
enum class EPixelizerDitherMode : uint8
{
	None UMETA(DisplayName = "None"),
	Ordered2x2 UMETA(DisplayName = "Ordered 2x2"),
	Ordered4x4 UMETA(DisplayName = "Ordered 4x4"),
	RandomNoise UMETA(DisplayName = "Random Noise")
};

USTRUCT(BlueprintType)
struct FPixelizerOutlineSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline")
	bool bEnableOutline = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline", meta = (ClampMin = "0.0", ClampMax = "8.0"))
	float Thickness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline")
	FLinearColor Color = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline")
	bool bScreenSpaceOutlinePlanned = false;
};

USTRUCT(BlueprintType)
struct FPixelizerEffectSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixelizer", meta = (ClampMin = "1", ClampMax = "128"))
	int32 PixelSize = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixelizer", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PaletteImpact = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixelizer")
	EPixelizerDitherMode DitherMode = EPixelizerDitherMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pixelizer", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DitherStrength = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustments", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float Brightness = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustments", meta = (ClampMin = "-4.0", ClampMax = "4.0"))
	float Exposure = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustments", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float Contrast = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustments", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float Saturation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Adjustments", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float BlurAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture")
	bool bPreserveAlpha = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Texture")
	bool bProcessEmissiveInFuture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety")
	bool bReuseSharedGeneratedAssets = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safety")
	bool bUniquePerActor = false;
};

USTRUCT(BlueprintType)
struct FPixelizerPaletteData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette")
	FString PaletteName = TEXT("Custom");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette")
	TArray<FLinearColor> Colors;
};

UCLASS(BlueprintType)
class PIXELIZERRUNTIME_API UPixelizerPaletteAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Palette")
	FPixelizerPaletteData Palette;
};

UCLASS(BlueprintType)
class PIXELIZERRUNTIME_API UPixelizerEffectPresetAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FString PresetName = TEXT("PixelPreset");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FPixelizerEffectSettings EffectSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FPixelizerPaletteData Palette;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FPixelizerOutlineSettings OutlineSettings;
};

USTRUCT()
struct FPixelizerGeneratedTextureCacheEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Registry")
	FString KeyHash;

	UPROPERTY(EditAnywhere, Category = "Registry")
	FSoftObjectPath SourceTexturePath;

	UPROPERTY(EditAnywhere, Category = "Registry")
	FSoftObjectPath GeneratedTexturePath;

	UPROPERTY(EditAnywhere, Category = "Registry")
	FString PresetHash;
};

USTRUCT()
struct FPixelizerGeneratedMaterialCacheEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Registry")
	FString KeyHash;

	UPROPERTY(EditAnywhere, Category = "Registry")
	FSoftObjectPath SourceMaterialPath;

	UPROPERTY(EditAnywhere, Category = "Registry")
	FSoftObjectPath GeneratedMaterialPath;

	UPROPERTY(EditAnywhere, Category = "Registry")
	FString PresetHash;
};

USTRUCT()
struct FPixelizerActorAssignmentRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Registry")
	FSoftObjectPath ActorPath;

	UPROPERTY(EditAnywhere, Category = "Registry")
	TArray<FSoftObjectPath> OriginalMaterialsBySlot;

	UPROPERTY(EditAnywhere, Category = "Registry")
	TArray<FSoftObjectPath> LastAppliedOverrideMaterialsBySlot;

	UPROPERTY(EditAnywhere, Category = "Registry")
	FString LastPresetHash;
};

UCLASS(BlueprintType)
class PIXELIZERRUNTIME_API UPixelizerGeneratedRegistryAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Registry")
	TArray<FPixelizerGeneratedTextureCacheEntry> TextureCache;

	UPROPERTY(EditAnywhere, Category = "Registry")
	TArray<FPixelizerGeneratedMaterialCacheEntry> MaterialCache;

	UPROPERTY(EditAnywhere, Category = "Registry")
	TArray<FPixelizerActorAssignmentRecord> ActorAssignments;
};

