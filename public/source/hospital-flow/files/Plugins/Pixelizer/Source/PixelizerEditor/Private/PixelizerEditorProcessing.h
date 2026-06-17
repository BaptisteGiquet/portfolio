#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture.h"
#include "Misc/PackageName.h"
#include "PixelizerEditorModule.h"
#include "PixelizerTypes.h"
#include "UObject/UObjectGlobals.h"

class AActor;
class UMaterialInterface;
class UMaterialInstanceConstant;
class UStaticMeshComponent;
class UTexture2D;
class UPixelizerGeneratedRegistryAsset;

namespace PixelizerEditorProcessing
{
	inline constexpr TCHAR RootFolder[] = TEXT("/Game/_Pixelizer");
	inline constexpr TCHAR GeneratedTexturesFolder[] = TEXT("/Game/_Pixelizer/Generated/Textures");
	inline constexpr TCHAR GeneratedMaterialsFolder[] = TEXT("/Game/_Pixelizer/Generated/Materials");
	inline constexpr TCHAR RegistryFolder[] = TEXT("/Game/_Pixelizer/Registry");
	inline constexpr TCHAR RegistryAssetName[] = TEXT("DA_PixelizerRegistry");

	struct FResolvedSelection
	{
		AActor* Actor = nullptr;
		UStaticMeshComponent* Component = nullptr;
		FString Key;
	};

	struct FTextureProcessOutput
	{
		int32 Width = 0;
		int32 Height = 0;
		ETextureSourceFormat SourceFormat = TSF_Invalid;
		TArray64<uint8> Bytes;
		FString Error;
	};

	struct FApplyComponentResult
	{
		bool bAnyApplied = false;
		bool bAnySupported = false;
		FString Status;
		FString Warnings;
		TArray<TWeakObjectPtr<UMaterialInterface>> OverrideMaterialsBySlot;
	};

	FString SanitizeAssetName(FString InName);
	FString StableHashHex(const FString& Input);
	FString BuildEffectHash(const FPixelizerEffectSettings& Settings, const FPixelizerPaletteData& Palette, const FPixelizerOutlineSettings& Outline);

	template <typename TAsset>
	TAsset* LoadOrCreateDataAsset(const FString& Folder, const FString& RawAssetName);

	FPixelizerActorAssignmentRecord* FindActorRecord(UPixelizerGeneratedRegistryAsset* Registry, const FSoftObjectPath& ActorPath);
	FPixelizerActorAssignmentRecord& FindOrAddActorRecord(UPixelizerGeneratedRegistryAsset* Registry, const FSoftObjectPath& ActorPath);

	bool ResolveSelectedStaticMeshComponent(FResolvedSelection& OutSelection, FString* OutWarnings = nullptr);
	bool TryResolveActorToComponent(AActor* Actor, UStaticMeshComponent*& OutComponent);
	FString GetSlotName(UStaticMeshComponent* Component, int32 SlotIndex);

	void AnalyzeComponentMaterials(
		UStaticMeshComponent* Component,
		TArray<FPixelizerMaterialSlotAnalysis>& OutAnalyses,
		FString& OutMaterialReport,
		FString& OutWarnings,
		const TArray<TWeakObjectPtr<UMaterialInterface>>* SourceMaterialsOverride = nullptr);

	bool ReadAndProcessTextureSource(
		UTexture2D* SourceTexture,
		const FPixelizerEffectSettings& Settings,
		const TArray<FLinearColor>& PaletteColors,
		const FPixelizerOutlineSettings& Outline,
		FTextureProcessOutput& Out);

	bool WriteTextureSource(UTexture2D* Texture, const FTextureProcessOutput& ProcessOutput);

	UTexture2D* DuplicateOrReuseTextureAsset(
		UPixelizerGeneratedRegistryAsset* Registry,
		const FString& TextureKeyHash,
		UTexture2D* SourceTexture,
		const FTextureProcessOutput& ProcessOutput,
		const FString& PresetHash,
		FString& InOutWarnings);

	UMaterialInstanceConstant* CreateOrReuseMaterialOverride(
		UPixelizerGeneratedRegistryAsset* Registry,
		const FString& MaterialKeyHash,
		UMaterialInterface* SourceMaterial,
		const FMaterialParameterInfo& TextureParamInfo,
		UTexture2D* GeneratedTexture,
		const FString& PresetHash,
		FString& InOutWarnings);

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
		FApplyComponentResult& OutResult);

	void AssignMaterialsToComponent(UStaticMeshComponent* Component, const TArray<TWeakObjectPtr<UMaterialInterface>>& Materials);

	template <typename TAsset>
	TAsset* LoadOrCreateDataAsset(const FString& Folder, const FString& RawAssetName)
	{
		const FString AssetName = SanitizeAssetName(RawAssetName);
		const FString PackageName = Folder.EndsWith(TEXT("/")) ? Folder + AssetName : Folder + TEXT("/") + AssetName;
		const FString ObjectPath = PackageName + TEXT(".") + AssetName;

		if (TAsset* Existing = LoadObject<TAsset>(nullptr, *ObjectPath))
		{
			return Existing;
		}

		if (!FPackageName::IsValidLongPackageName(PackageName))
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}

		TAsset* Asset = NewObject<TAsset>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
		Package->MarkPackageDirty();
		return Asset;
	}
}
