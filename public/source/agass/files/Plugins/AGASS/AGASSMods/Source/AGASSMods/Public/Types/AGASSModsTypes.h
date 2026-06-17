#pragma once

#include "CoreMinimal.h"
#include "AGASSModsTypes.generated.h"

UENUM(BlueprintType)
enum class EAGASSModSourceType : uint8
{
	BaseGame UMETA(DisplayName = "Base Game"),
	ProjectModsDirectory UMETA(DisplayName = "Project Mods"),
	AdditionalLocalDirectory UMETA(DisplayName = "Additional Local Directory"),
	WorkshopStagingDirectory UMETA(DisplayName = "Workshop Staging"),
	WorkshopInstallDirectory UMETA(DisplayName = "Workshop Install"),
	Unknown UMETA(DisplayName = "Unknown")
};

USTRUCT(BlueprintType)
struct AGASSMODS_API FAGASSDiscoveredModInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString ModId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FText Description;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString Version;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString CompatibilityVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FString> CompatibilityTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString PluginName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString DescriptorFilePath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString MountedAssetPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	EAGASSModSourceType SourceType = EAGASSModSourceType::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	bool bIsMounted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	bool bIsValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	bool bIsActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString ValidationError;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FString> DeclaredMapIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FSoftObjectPath> DeclaredSessionEventDefinitionAssets;

	FString GetEffectiveCompatibilityVersion() const
	{
		return CompatibilityVersion.IsEmpty() ? Version : CompatibilityVersion;
	}

	FString GetDisplayLabel() const
	{
		if (!DisplayName.IsEmpty())
		{
			return DisplayName.ToString();
		}

		if (!ModId.IsEmpty())
		{
			return ModId;
		}

		if (!PluginName.IsEmpty())
		{
			return PluginName;
		}

		return DescriptorFilePath;
	}
};

USTRUCT(BlueprintType)
struct AGASSMODS_API FAGASSAvailableMapInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString MapId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FText Description;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString Version;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString CompatibilityVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FString> CompatibilityTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString WorldPackagePath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString OwningModId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString OwningPluginName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	bool bIsBaseGameMap = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	bool bIsCurrentlySelected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	bool bOwningModIsActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FSoftObjectPath> SessionEventDefinitionAssets;

	FString GetEffectiveCompatibilityVersion() const
	{
		return CompatibilityVersion.IsEmpty() ? Version : CompatibilityVersion;
	}

	FString GetDisplayLabel() const
	{
		return DisplayName.IsEmpty() ? MapId : DisplayName.ToString();
	}
};

USTRUCT(BlueprintType)
struct AGASSMODS_API FAGASSActiveModCompatibilityInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString ModId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString Version;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString CompatibilityVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FString> CompatibilityTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString PluginName;
};

USTRUCT(BlueprintType)
struct AGASSMODS_API FAGASSResolvedContentSelection
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	bool bIsValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FAGASSAvailableMapInfo SelectedMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FAGASSActiveModCompatibilityInfo> ActiveMods;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString ActiveModIdsCsv;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString ContentCompatibilityHash;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FSoftObjectPath> SessionEventDefinitionAssets;

	FString BuildTimedRunLeaderboardKey() const
	{
		FString SourceValue = !ContentCompatibilityHash.IsEmpty() ? ContentCompatibilityHash : SelectedMap.MapId;
		SourceValue = SourceValue.TrimStartAndEnd().ToLower();
		for (TCHAR& Character : SourceValue)
		{
			if (!FChar::IsAlnum(Character))
			{
				Character = TEXT('_');
			}
		}

		while (SourceValue.ReplaceInline(TEXT("__"), TEXT("_")) > 0)
		{
		}

		while (SourceValue.StartsWith(TEXT("_")))
		{
			SourceValue.RightChopInline(1, EAllowShrinking::No);
		}

		while (SourceValue.EndsWith(TEXT("_")))
		{
			SourceValue.LeftChopInline(1, EAllowShrinking::No);
		}

		if (SourceValue.IsEmpty())
		{
			return FString();
		}

		return FString::Printf(TEXT("timedrun_%s"), *SourceValue.Left(56));
	}
};

USTRUCT(BlueprintType)
struct AGASSMODS_API FAGASSJoinCompatibilityResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	bool bIsCompatible = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString FriendlyFailureMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString RequiredMapId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString RequiredMapDisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString RequiredActiveModIdsCsv;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString LocalContentCompatibilityHash;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	FString RemoteContentCompatibilityHash;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FString> MissingRequiredModIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AGASS|Mods")
	TArray<FString> UnexpectedActiveModIds;
};
