#include "Subsystems/AGASSModsSubsystem.h"

#include "Algo/Sort.h"
#include "Algo/Unique.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Data/AGASSMapDefinition.h"
#include "Data/AGASSModManifest.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Modules/ModuleManager.h"
#include "Settings/AGASSModsDeveloperSettings.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSMods, Log, All);

namespace
{
	constexpr const TCHAR* GameplaySessionTravelOption = TEXT("AGASS_GAMEPLAY_SESSION");
	constexpr const TCHAR* LegacyTowerSessionTravelOption = TEXT("AGASS_TOWER_SESSION");

	FString MakeLookupKey(const FString& Value)
	{
		return Value.TrimStartAndEnd().ToLower();
	}

	FString MakeWorldPackagePathKey(const FString& WorldPackagePath)
	{
		return MakeLookupKey(UWorld::RemovePIEPrefix(WorldPackagePath));
	}

	FString NormalizeDirectoryPath(const FString& Path)
	{
		FString FullPath = FPaths::ConvertRelativePathToFull(Path);
		FPaths::NormalizeDirectoryName(FullPath);
		return FullPath;
	}

	bool IsUnderDirectoryOrSame(const FString& Path, const FString& RootDirectory)
	{
		if (RootDirectory.IsEmpty())
		{
			return false;
		}

		return Path.Equals(RootDirectory, ESearchCase::IgnoreCase) || FPaths::IsUnderDirectory(Path, RootDirectory);
	}

	FString JoinValues(const TArray<FString>& Values, const TCHAR* Separator)
	{
		FString Result;
		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			if (Index > 0)
			{
				Result += Separator;
			}

			Result += Values[Index];
		}

		return Result;
	}

	void NormalizeTagArray(TArray<FString>& InOutTags)
	{
		for (FString& Tag : InOutTags)
		{
			Tag = Tag.TrimStartAndEnd();
		}

		InOutTags.RemoveAll([](const FString& Tag) { return Tag.IsEmpty(); });
		InOutTags.Sort([](const FString& Left, const FString& Right)
		{
			return Left.Compare(Right, ESearchCase::IgnoreCase) < 0;
		});
		InOutTags.SetNum(Algo::Unique(InOutTags));
	}

	void AppendValidationMessage(FString& InOutMessage, const FString& Message)
	{
		if (Message.IsEmpty())
		{
			return;
		}

		if (!InOutMessage.IsEmpty())
		{
			InOutMessage += TEXT(" ");
		}

		InOutMessage += Message;
	}

	FString MakeSoftObjectPathKey(const FSoftObjectPath& SoftObjectPath)
	{
		FString Key = SoftObjectPath.ToString().TrimStartAndEnd();
		Key.ToLowerInline();
		return Key;
	}

	void AppendUniqueSoftObjectPaths(
		const TArray<FSoftObjectPath>& SourcePaths,
		TArray<FSoftObjectPath>& OutPaths,
		TSet<FString>& InOutSeenPathKeys)
	{
		for (const FSoftObjectPath& SourcePath : SourcePaths)
		{
			if (SourcePath.IsNull())
			{
				continue;
			}

			const FString PathKey = MakeSoftObjectPathKey(SourcePath);
			if (PathKey.IsEmpty() || InOutSeenPathKeys.Contains(PathKey))
			{
				continue;
			}

			InOutSeenPathKeys.Add(PathKey);
			OutPaths.Add(SourcePath);
		}
	}

	bool TryLoadCookedPluginAssetRegistry(const TSharedRef<IPlugin>& Plugin, FAssetRegistryState& OutPluginState, FString* OutFailureReason = nullptr)
	{
		const FString PluginAssetRegistryPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("AssetRegistry.bin"));
		if (!FPaths::FileExists(PluginAssetRegistryPath))
		{
			if (OutFailureReason != nullptr)
			{
				*OutFailureReason = FString::Printf(TEXT("Missing plugin asset registry: %s"), *PluginAssetRegistryPath);
			}
			return false;
		}

		if (!FAssetRegistryState::LoadFromDisk(*PluginAssetRegistryPath, FAssetRegistryLoadOptions(), OutPluginState))
		{
			if (OutFailureReason != nullptr)
			{
				*OutFailureReason = FString::Printf(TEXT("Failed to load plugin asset registry: %s"), *PluginAssetRegistryPath);
			}
			return false;
		}

		if (OutFailureReason != nullptr)
		{
			OutFailureReason->Reset();
		}
		return true;
	}

	bool IsAssetUnderMountedPath(const FAssetData& AssetData, const FString& MountedAssetPath)
	{
		const FString AssetPackagePath = AssetData.PackagePath.ToString();
		return AssetPackagePath.Equals(MountedAssetPath, ESearchCase::IgnoreCase)
			|| AssetPackagePath.StartsWith(MountedAssetPath + TEXT("/"), ESearchCase::IgnoreCase);
	}

	void GatherManifestAssetsFromPluginState(
		const FAssetRegistryState& PluginState,
		const FString& MountedAssetPath,
		TArray<FAssetData>& OutManifestAssets)
	{
		const FTopLevelAssetPath ManifestClassPath = UAGASSModManifest::StaticClass()->GetClassPathName();
		OutManifestAssets.Reset();
		PluginState.EnumerateAllAssets([&](const FAssetData& AssetData)
		{
			if (AssetData.AssetClassPath == ManifestClassPath && IsAssetUnderMountedPath(AssetData, MountedAssetPath))
			{
				OutManifestAssets.Add(AssetData);
			}
		});

		OutManifestAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
		{
			return Left.GetObjectPathString().Compare(Right.GetObjectPathString(), ESearchCase::IgnoreCase) < 0;
		});
	}

	bool TryAppendCookedPluginAssetRegistry(const FAssetRegistryState& PluginState, IAssetRegistry& AssetRegistry)
	{
		AssetRegistry.AppendState(PluginState, UE::AssetRegistry::EAppendMode::Append);
		return true;
	}

	bool TryMountCookedPluginPaks(const TSharedRef<IPlugin>& Plugin, TSet<FString>& InOutMountedPakFiles, FString* OutFailureReason = nullptr)
	{
		if (!FPlatformProperties::RequiresCookedData())
		{
			if (OutFailureReason != nullptr)
			{
				OutFailureReason->Reset();
			}
			return true;
		}

		if (!FCoreDelegates::MountPak.IsBound())
		{
			if (OutFailureReason != nullptr)
			{
				*OutFailureReason = FString::Printf(TEXT("MountPak delegate is not available for plugin '%s'."), *Plugin->GetName());
			}
			return false;
		}

		const FString PluginPakRoot = FPaths::Combine(Plugin->GetContentDir(), TEXT("Paks"), FPlatformProperties::PlatformName());
		TArray<FString> FoundPakFiles;
		IFileManager::Get().FindFilesRecursive(FoundPakFiles, *PluginPakRoot, TEXT("*.pak"), true, false, false);
		FoundPakFiles.Sort();

		if (FoundPakFiles.IsEmpty())
		{
			if (OutFailureReason != nullptr)
			{
				*OutFailureReason = FString::Printf(TEXT("No cooked plugin pak files were found under '%s'."), *PluginPakRoot);
			}
			return false;
		}

		for (FString& PakFile : FoundPakFiles)
		{
			FPaths::NormalizeFilename(PakFile);
			if (InOutMountedPakFiles.Contains(PakFile))
			{
				continue;
			}

			if (FCoreDelegates::MountPak.Execute(PakFile, 0) == nullptr)
			{
				if (OutFailureReason != nullptr)
				{
					*OutFailureReason = FString::Printf(TEXT("Failed to mount cooked plugin pak '%s'."), *PakFile);
				}
				return false;
			}

			InOutMountedPakFiles.Add(PakFile);
		}

		if (OutFailureReason != nullptr)
		{
			OutFailureReason->Reset();
		}
		return true;
	}
}

void UAGASSModsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RefreshRegistry();
}

void UAGASSModsSubsystem::Deinitialize()
{
	DiscoveredMods.Reset();
	AvailableMaps.Reset();
	ModIndexById.Reset();
	MapIndexById.Reset();
	MapIndexByWorldPackagePath.Reset();
	AddedPluginSearchPaths.Reset();
	MountedPluginPakFiles.Reset();
	ActiveModIds.Reset();
	SelectedMapId.Reset();

	Super::Deinitialize();
}

void UAGASSModsSubsystem::RefreshRegistry()
{
	RegisterConfiguredPluginSearchPaths();
	RebuildRegistry();
	ValidateSelectionState();
	RefreshDynamicFlags();
	RegistryChangedEvent.Broadcast();
	SelectionChangedEvent.Broadcast();
}

const TArray<FAGASSDiscoveredModInfo>& UAGASSModsSubsystem::GetDiscoveredMods() const
{
	return DiscoveredMods;
}

const TArray<FAGASSAvailableMapInfo>& UAGASSModsSubsystem::GetAvailableMaps() const
{
	return AvailableMaps;
}

const TArray<FString>& UAGASSModsSubsystem::GetActiveModIds() const
{
	return ActiveModIds;
}

FString UAGASSModsSubsystem::GetSelectedMapId() const
{
	return SelectedMapId;
}

bool UAGASSModsSubsystem::IsModActive(const FString& ModId) const
{
	return ActiveModIds.ContainsByPredicate([&ModId](const FString& ActiveModId)
	{
		return ActiveModId.Equals(ModId, ESearchCase::IgnoreCase);
	});
}

const FAGASSAvailableMapInfo* UAGASSModsSubsystem::GetSelectedMapInfo() const
{
	if (const int32* MapIndex = MapIndexById.Find(MakeLookupKey(SelectedMapId)))
	{
		return AvailableMaps.IsValidIndex(*MapIndex) ? &AvailableMaps[*MapIndex] : nullptr;
	}

	return nullptr;
}

const FAGASSAvailableMapInfo* UAGASSModsSubsystem::FindMapInfoByWorldPackagePath(const FString& WorldPackagePath) const
{
	if (const int32* MapIndex = MapIndexByWorldPackagePath.Find(MakeWorldPackagePathKey(WorldPackagePath)))
	{
		return AvailableMaps.IsValidIndex(*MapIndex) ? &AvailableMaps[*MapIndex] : nullptr;
	}

	return nullptr;
}

bool UAGASSModsSubsystem::CanChangeSelection(FString& OutFailureMessage) const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		OutFailureMessage.Reset();
		return true;
	}

	if (IsGameplaySessionWorld(*World)
		|| World->GetNetMode() == NM_Client
		|| World->GetNetMode() == NM_ListenServer
		|| World->GetNetMode() == NM_DedicatedServer)
	{
		OutFailureMessage = NSLOCTEXT(
			"AGASSMods",
			"SelectionLockedWhileSessionActive",
			"Mod activation and map selection can only change from the frontend or another idle state.").ToString();
		return false;
	}

	OutFailureMessage.Reset();
	return true;
}

bool UAGASSModsSubsystem::SetModActive(const FString& ModId, const bool bActive, FString& OutFailureMessage)
{
	if (!CanChangeSelection(OutFailureMessage))
	{
		return false;
	}

	const int32* ModIndex = ModIndexById.Find(MakeLookupKey(ModId));
	if (ModIndex == nullptr || !DiscoveredMods.IsValidIndex(*ModIndex))
	{
		OutFailureMessage = NSLOCTEXT("AGASSMods", "UnknownModFailure", "That mod is no longer available. Refresh the registry and try again.").ToString();
		return false;
	}

	const FAGASSDiscoveredModInfo& ModInfo = DiscoveredMods[*ModIndex];
	if (!ModInfo.bIsValid)
	{
		OutFailureMessage = ModInfo.ValidationError.IsEmpty()
			? NSLOCTEXT("AGASSMods", "InvalidModFailure", "That mod is not valid for runtime use.").ToString()
			: ModInfo.ValidationError;
		return false;
	}

	const bool bWasActive = IsModActive(ModInfo.ModId);
	if (bWasActive == bActive)
	{
		OutFailureMessage.Reset();
		return true;
	}

	if (bActive)
	{
		ActiveModIds.Add(ModInfo.ModId);
		NormalizeStringArray(ActiveModIds);
	}
	else
	{
		ActiveModIds.RemoveAll([&ModInfo](const FString& ActiveModId)
		{
			return ActiveModId.Equals(ModInfo.ModId, ESearchCase::IgnoreCase);
		});
	}

	ValidateSelectionState();
	RefreshDynamicFlags();
	SelectionChangedEvent.Broadcast();
	OutFailureMessage.Reset();
	return true;
}

bool UAGASSModsSubsystem::SetSelectedMapId(const FString& MapId, FString& OutFailureMessage)
{
	if (!CanChangeSelection(OutFailureMessage))
	{
		return false;
	}

	const int32* MapIndex = MapIndexById.Find(MakeLookupKey(MapId));
	if (MapIndex == nullptr || !AvailableMaps.IsValidIndex(*MapIndex))
	{
		OutFailureMessage = NSLOCTEXT("AGASSMods", "UnknownMapFailure", "That map is no longer available. Refresh the registry and try again.").ToString();
		return false;
	}

	const FAGASSAvailableMapInfo& MapInfo = AvailableMaps[*MapIndex];
	if (!MapInfo.OwningModId.IsEmpty() && !IsModActive(MapInfo.OwningModId))
	{
		ActiveModIds.Add(MapInfo.OwningModId);
		NormalizeStringArray(ActiveModIds);
	}

	if (SelectedMapId.Equals(MapInfo.MapId, ESearchCase::IgnoreCase))
	{
		RefreshDynamicFlags();
		OutFailureMessage.Reset();
		return true;
	}

	SelectedMapId = MapInfo.MapId;
	ValidateSelectionState();
	RefreshDynamicFlags();
	SelectionChangedEvent.Broadcast();
	OutFailureMessage.Reset();
	return true;
}

bool UAGASSModsSubsystem::TryBuildHostedSelection(FAGASSResolvedContentSelection& OutSelection, FString& OutFailureMessage) const
{
	return TryResolveSelection(SelectedMapId, ActiveModIds, OutSelection, OutFailureMessage);
}

bool UAGASSModsSubsystem::TryBuildRuntimeSelectionForWorld(
	const UWorld& World,
	FAGASSResolvedContentSelection& OutSelection,
	FString& OutFailureMessage) const
{
	if (const FAGASSAvailableMapInfo* MapInfo = FindMapInfoByWorldPackagePath(World.GetOutermost()->GetName()))
	{
		TArray<FString> RequestedActiveModIds = ActiveModIds;
		if (!MapInfo->OwningModId.IsEmpty()
			&& !RequestedActiveModIds.ContainsByPredicate([MapInfo](const FString& ActiveModId)
			{
				return ActiveModId.Equals(MapInfo->OwningModId, ESearchCase::IgnoreCase);
			}))
		{
			RequestedActiveModIds.Add(MapInfo->OwningModId);
		}

		NormalizeStringArray(RequestedActiveModIds);
		return TryResolveSelection(MapInfo->MapId, RequestedActiveModIds, OutSelection, OutFailureMessage);
	}

	if (World.URL.HasOption(GameplaySessionTravelOption) || World.URL.HasOption(LegacyTowerSessionTravelOption))
	{
		return TryBuildHostedSelection(OutSelection, OutFailureMessage);
	}

	OutSelection = FAGASSResolvedContentSelection();
	OutFailureMessage = NSLOCTEXT("AGASSMods", "RuntimeSelectionUnknownWorld", "The current gameplay world is not registered as an available map.").ToString();
	return false;
}

bool UAGASSModsSubsystem::EvaluateJoinCompatibility(
	const FString& RemoteMapId,
	const FString& RemoteMapDisplayName,
	const FString& RemoteActiveModIdsCsv,
	const FString& RemoteContentCompatibilityHash,
	FAGASSJoinCompatibilityResult& OutResult) const
{
	OutResult = FAGASSJoinCompatibilityResult();
	OutResult.RequiredMapId = RemoteMapId;
	OutResult.RequiredMapDisplayName = RemoteMapDisplayName;
	OutResult.RequiredActiveModIdsCsv = RemoteActiveModIdsCsv;
	OutResult.RemoteContentCompatibilityHash = RemoteContentCompatibilityHash;

	const TArray<FString> RequiredModIds = ParseDelimitedIds(RemoteActiveModIdsCsv);
	TArray<FString> LocalActiveMods = ActiveModIds;
	NormalizeStringArray(LocalActiveMods);

	TSet<FString> LocalActiveKeys;
	for (const FString& LocalActiveModId : LocalActiveMods)
	{
		LocalActiveKeys.Add(MakeLookupKey(LocalActiveModId));
	}

	TSet<FString> RequiredModKeys;
	for (const FString& RequiredModId : RequiredModIds)
	{
		RequiredModKeys.Add(MakeLookupKey(RequiredModId));
		if (!LocalActiveKeys.Contains(MakeLookupKey(RequiredModId)))
		{
			OutResult.MissingRequiredModIds.Add(RequiredModId);
		}
	}

	for (const FString& LocalActiveModId : LocalActiveMods)
	{
		if (!RequiredModKeys.Contains(MakeLookupKey(LocalActiveModId)))
		{
			OutResult.UnexpectedActiveModIds.Add(LocalActiveModId);
		}
	}

	if (OutResult.MissingRequiredModIds.Num() > 0 || OutResult.UnexpectedActiveModIds.Num() > 0)
	{
		const FString MissingList = JoinIds(OutResult.MissingRequiredModIds);
		const FString ExtraList = JoinIds(OutResult.UnexpectedActiveModIds);
		if (!MissingList.IsEmpty() && !ExtraList.IsEmpty())
		{
			OutResult.FriendlyFailureMessage = FString::Format(
				*NSLOCTEXT("AGASSMods", "JoinMismatchMissingAndExtra", "Select the required mods before joining. Missing: {0}. Disable: {1}.").ToString(),
				{MissingList, ExtraList});
		}
		else if (!MissingList.IsEmpty())
		{
			OutResult.FriendlyFailureMessage = FString::Format(
				*NSLOCTEXT("AGASSMods", "JoinMismatchMissingOnly", "Select the required mods before joining: {0}.").ToString(),
				{MissingList});
		}
		else
		{
			OutResult.FriendlyFailureMessage = FString::Format(
				*NSLOCTEXT("AGASSMods", "JoinMismatchExtraOnly", "Disable the current active mods before joining: {0}.").ToString(),
				{ExtraList});
		}

		return false;
	}

	FAGASSResolvedContentSelection LocalSelection;
	if (!TryResolveSelection(RemoteMapId, LocalActiveMods, LocalSelection, OutResult.FriendlyFailureMessage))
	{
		const FString MapLabel = RemoteMapDisplayName.IsEmpty() ? RemoteMapId : RemoteMapDisplayName;
		if (OutResult.FriendlyFailureMessage.IsEmpty())
		{
			OutResult.FriendlyFailureMessage = FString::Format(
				*NSLOCTEXT("AGASSMods", "JoinMismatchMapUnavailable", "The selected session map is not available locally: {0}.").ToString(),
				{MapLabel});
		}

		return false;
	}

	OutResult.LocalContentCompatibilityHash = LocalSelection.ContentCompatibilityHash;
	if (!RemoteContentCompatibilityHash.IsEmpty()
		&& !OutResult.LocalContentCompatibilityHash.Equals(RemoteContentCompatibilityHash, ESearchCase::IgnoreCase))
	{
		OutResult.FriendlyFailureMessage = NSLOCTEXT(
			"AGASSMods",
			"JoinMismatchCompatibilityHash",
			"The selected map or active mods do not match this session's compatibility requirements.").ToString();
		return false;
	}

	OutResult.bIsCompatible = true;
	return true;
}

FAGASSModsRegistryChangedEvent& UAGASSModsSubsystem::OnRegistryChanged()
{
	return RegistryChangedEvent;
}

FAGASSModsSelectionChangedEvent& UAGASSModsSubsystem::OnSelectionChanged()
{
	return SelectionChangedEvent;
}

bool UAGASSModsSubsystem::IsGameplaySessionWorld(const UWorld& World) const
{
	if (World.URL.HasOption(GameplaySessionTravelOption) || World.URL.HasOption(LegacyTowerSessionTravelOption))
	{
		return true;
	}

	return FindMapInfoByWorldPackagePath(World.GetOutermost()->GetName()) != nullptr;
}

void UAGASSModsSubsystem::RegisterConfiguredPluginSearchPaths()
{
	const UAGASSModsDeveloperSettings* Settings = UAGASSModsDeveloperSettings::Get();
	if (Settings == nullptr)
	{
		return;
	}

	IPluginManager& PluginManager = IPluginManager::Get();
	bool bNeedsRefresh = false;

	auto RegisterPathArray = [this, &PluginManager, &bNeedsRefresh](const TArray<FDirectoryPath>& DirectoryPaths)
	{
		for (const FDirectoryPath& DirectoryPath : DirectoryPaths)
		{
			if (DirectoryPath.Path.IsEmpty())
			{
				continue;
			}

			const FString FullPath = NormalizeDirectoryPath(DirectoryPath.Path);
			if (AddedPluginSearchPaths.Contains(FullPath))
			{
				continue;
			}

			PluginManager.AddPluginSearchPath(FullPath, false);
			AddedPluginSearchPaths.Add(FullPath);
			bNeedsRefresh = true;
		}
	};

	RegisterPathArray(Settings->AdditionalLocalModPluginRoots);
	RegisterPathArray(Settings->WorkshopStagingPluginRoots);
	RegisterPathArray(Settings->WorkshopInstallPluginRoots);

	if (bNeedsRefresh)
	{
		PluginManager.RefreshPluginsList();
	}
}

void UAGASSModsSubsystem::RebuildRegistry()
{
	DiscoveredMods.Reset();
	AvailableMaps.Reset();
	ModIndexById.Reset();
	MapIndexById.Reset();
	MapIndexByWorldPackagePath.Reset();

	AddConfiguredBaseMaps();

	const UAGASSModsDeveloperSettings* Settings = UAGASSModsDeveloperSettings::Get();
	if (Settings == nullptr)
	{
		return;
	}

	TArray<FString> ProjectModRoots;
	if (Settings->bScanProjectModsDirectory)
	{
		ProjectModRoots.Add(NormalizeDirectoryPath(FPaths::ProjectModsDir()));
	}

	TArray<FString> AdditionalLocalRoots;
	for (const FDirectoryPath& DirectoryPath : Settings->AdditionalLocalModPluginRoots)
	{
		if (!DirectoryPath.Path.IsEmpty())
		{
			AdditionalLocalRoots.Add(NormalizeDirectoryPath(DirectoryPath.Path));
		}
	}

	TArray<FString> WorkshopStagingRoots;
	for (const FDirectoryPath& DirectoryPath : Settings->WorkshopStagingPluginRoots)
	{
		if (!DirectoryPath.Path.IsEmpty())
		{
			WorkshopStagingRoots.Add(NormalizeDirectoryPath(DirectoryPath.Path));
		}
	}

	TArray<FString> WorkshopInstallRoots;
	for (const FDirectoryPath& DirectoryPath : Settings->WorkshopInstallPluginRoots)
	{
		if (!DirectoryPath.Path.IsEmpty())
		{
			WorkshopInstallRoots.Add(NormalizeDirectoryPath(DirectoryPath.Path));
		}
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.WaitForCompletion();

	TSet<FString> SeenModIds;
	TSet<FString> SeenMapIds;
	for (const FAGASSAvailableMapInfo& ExistingMap : AvailableMaps)
	{
		SeenMapIds.Add(MakeLookupKey(ExistingMap.MapId));
	}

	TArray<TSharedRef<IPlugin>> DiscoveredPlugins = IPluginManager::Get().GetDiscoveredPlugins();
	DiscoveredPlugins.Sort([](const TSharedRef<IPlugin>& Left, const TSharedRef<IPlugin>& Right)
	{
		return Left->GetName().Compare(Right->GetName(), ESearchCase::IgnoreCase) < 0;
	});

	for (const TSharedRef<IPlugin>& Plugin : DiscoveredPlugins)
	{
		const FString PluginBaseDir = NormalizeDirectoryPath(Plugin->GetBaseDir());
		const FString DescriptorFilePath = FPaths::ConvertRelativePathToFull(Plugin->GetDescriptorFileName());
		EAGASSModSourceType SourceType = EAGASSModSourceType::Unknown;

		for (const FString& Root : ProjectModRoots)
		{
			if (IsUnderDirectoryOrSame(PluginBaseDir, Root))
			{
				SourceType = EAGASSModSourceType::ProjectModsDirectory;
				break;
			}
		}

		if (SourceType == EAGASSModSourceType::Unknown)
		{
			for (const FString& Root : AdditionalLocalRoots)
			{
				if (IsUnderDirectoryOrSame(PluginBaseDir, Root))
				{
					SourceType = EAGASSModSourceType::AdditionalLocalDirectory;
					break;
				}
			}
		}

		if (SourceType == EAGASSModSourceType::Unknown)
		{
			for (const FString& Root : WorkshopStagingRoots)
			{
				if (IsUnderDirectoryOrSame(PluginBaseDir, Root))
				{
					SourceType = EAGASSModSourceType::WorkshopStagingDirectory;
					break;
				}
			}
		}

		if (SourceType == EAGASSModSourceType::Unknown)
		{
			for (const FString& Root : WorkshopInstallRoots)
			{
				if (IsUnderDirectoryOrSame(PluginBaseDir, Root))
				{
					SourceType = EAGASSModSourceType::WorkshopInstallDirectory;
					break;
				}
			}
		}

		if (SourceType == EAGASSModSourceType::Unknown)
		{
			continue;
		}

		// Only enumerate loose installed mods that still have a real descriptor on disk.
		// This avoids surfacing project test mods that were staged into the base build.
		if (!FPaths::DirectoryExists(PluginBaseDir) || !FPaths::FileExists(DescriptorFilePath))
		{
			continue;
		}

		FAGASSDiscoveredModInfo ModInfo;
		ModInfo.PluginName = Plugin->GetName();
		ModInfo.DescriptorFilePath = DescriptorFilePath;
		ModInfo.MountedAssetPath = Plugin->GetMountedAssetPath();
		ModInfo.SourceType = SourceType;
		ModInfo.bIsMounted = Plugin->IsMounted();
		ModInfo.bIsValid = false;

		const FPluginDescriptor& Descriptor = Plugin->GetDescriptor();
		if (!Plugin->CanContainContent())
		{
			ModInfo.ValidationError = NSLOCTEXT("AGASSMods", "PluginMustContainContent", "Mods must be content plugins.").ToString();
			DiscoveredMods.Add(ModInfo);
			continue;
		}

		if (Descriptor.Modules.Num() > 0)
		{
			ModInfo.ValidationError = NSLOCTEXT("AGASSMods", "PluginMustNotContainModules", "Native-code mods are not supported. Remove all modules from this plugin.").ToString();
			DiscoveredMods.Add(ModInfo);
			continue;
		}

		if (!Descriptor.bExplicitlyLoaded)
		{
			ModInfo.ValidationError = NSLOCTEXT("AGASSMods", "PluginMustBeExplicitlyLoaded", "Mods must set ExplicitlyLoaded=true so AGASS can control the runtime selection path.").ToString();
			DiscoveredMods.Add(ModInfo);
			continue;
		}

		if (!Plugin->IsMounted())
		{
			if (!Settings->bMountDiscoveredModsDuringRefresh || !IPluginManager::Get().MountExplicitlyLoadedPlugin(Plugin->GetName()))
			{
				ModInfo.ValidationError = NSLOCTEXT("AGASSMods", "PluginMountFailed", "The mod plugin could not be mounted for manifest discovery.").ToString();
				DiscoveredMods.Add(ModInfo);
				continue;
			}

			ModInfo.bIsMounted = true;
		}

		FString PluginPakMountFailure;
		if (!TryMountCookedPluginPaks(Plugin, MountedPluginPakFiles, &PluginPakMountFailure))
		{
			ModInfo.ValidationError = PluginPakMountFailure.IsEmpty()
				? NSLOCTEXT("AGASSMods", "PluginPakMountFailed", "The mod plugin content containers could not be mounted for runtime loading.").ToString()
				: PluginPakMountFailure;
			DiscoveredMods.Add(ModInfo);
			continue;
		}

		TArray<FString> PathsToScan;
		PathsToScan.Add(Plugin->GetMountedAssetPath());
		AssetRegistry.ScanPathsSynchronous(PathsToScan, true);

		FARFilter ManifestFilter;
		ManifestFilter.PackagePaths.Add(*Plugin->GetMountedAssetPath());
		ManifestFilter.ClassPaths.Add(UAGASSModManifest::StaticClass()->GetClassPathName());
		ManifestFilter.bRecursivePaths = true;
		ManifestFilter.bRecursiveClasses = true;

		TArray<FAssetData> ManifestAssets;
		AssetRegistry.GetAssets(ManifestFilter, ManifestAssets);
		ManifestAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
		{
			return Left.GetObjectPathString().Compare(Right.GetObjectPathString(), ESearchCase::IgnoreCase) < 0;
		});

		if (ManifestAssets.Num() == 0)
		{
			FAssetRegistryState PluginAssetRegistryState;
			FString AssetRegistryLoadFailure;
			if (TryLoadCookedPluginAssetRegistry(Plugin, PluginAssetRegistryState, &AssetRegistryLoadFailure))
			{
				TryAppendCookedPluginAssetRegistry(PluginAssetRegistryState, AssetRegistry);

				ManifestAssets.Reset();
				AssetRegistry.GetAssets(ManifestFilter, ManifestAssets);
				ManifestAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
				{
					return Left.GetObjectPathString().Compare(Right.GetObjectPathString(), ESearchCase::IgnoreCase) < 0;
				});

				if (ManifestAssets.Num() > 0)
				{
					UE_LOG(LogAGASSMods, Display, TEXT("Loaded runtime asset registry for discovered mod plugin '%s' from '%s'."), *Plugin->GetName(), *FPaths::Combine(Plugin->GetBaseDir(), TEXT("AssetRegistry.bin")));
				}
				else
				{
					GatherManifestAssetsFromPluginState(PluginAssetRegistryState, Plugin->GetMountedAssetPath(), ManifestAssets);
					if (ManifestAssets.Num() > 0)
					{
						UE_LOG(LogAGASSMods, Display, TEXT("Discovered runtime manifest entries for mod plugin '%s' directly from '%s'."), *Plugin->GetName(), *FPaths::Combine(Plugin->GetBaseDir(), TEXT("AssetRegistry.bin")));
					}
				}
			}
			else if (!AssetRegistryLoadFailure.IsEmpty())
			{
				UE_LOG(LogAGASSMods, Verbose, TEXT("%s"), *AssetRegistryLoadFailure);
			}
		}

		if (ManifestAssets.Num() != 1)
		{
			ModInfo.ValidationError = ManifestAssets.Num() == 0
				? NSLOCTEXT("AGASSMods", "PluginMissingManifest", "The mod plugin must contain exactly one UAGASSModManifest asset.").ToString()
				: NSLOCTEXT("AGASSMods", "PluginMultipleManifests", "The mod plugin contains multiple UAGASSModManifest assets. Keep exactly one runtime manifest.").ToString();
			DiscoveredMods.Add(ModInfo);
			continue;
		}

		UAGASSModManifest* Manifest = Cast<UAGASSModManifest>(ManifestAssets[0].GetAsset());
		if (Manifest == nullptr)
		{
			ModInfo.ValidationError = NSLOCTEXT("AGASSMods", "ManifestLoadFailed", "The mod manifest asset could not be loaded.").ToString();
			DiscoveredMods.Add(ModInfo);
			continue;
		}

		ModInfo.ModId = Manifest->ModId.TrimStartAndEnd();
		ModInfo.DisplayName = Manifest->DisplayName.IsEmpty() ? FText::FromString(Plugin->GetFriendlyName()) : Manifest->DisplayName;
		ModInfo.Description = Manifest->Description;
		ModInfo.Version = Manifest->Version.TrimStartAndEnd();
		ModInfo.CompatibilityVersion = Manifest->CompatibilityVersion.TrimStartAndEnd();
		ModInfo.CompatibilityTags = Manifest->CompatibilityTags;
		ModInfo.DeclaredSessionEventDefinitionAssets = Manifest->SessionEventDefinitions;
		NormalizeTagArray(ModInfo.CompatibilityTags);

		if (ModInfo.ModId.IsEmpty())
		{
			ModInfo.ValidationError = NSLOCTEXT("AGASSMods", "ManifestMissingModId", "The mod manifest must declare a stable ModId.").ToString();
			DiscoveredMods.Add(ModInfo);
			continue;
		}

		if (ModInfo.GetEffectiveCompatibilityVersion().IsEmpty())
		{
			ModInfo.ValidationError = NSLOCTEXT("AGASSMods", "ManifestMissingCompatibilityVersion", "The mod manifest must declare a Version or CompatibilityVersion.").ToString();
			DiscoveredMods.Add(ModInfo);
			continue;
		}

		if (SeenModIds.Contains(MakeLookupKey(ModInfo.ModId)))
		{
			ModInfo.ValidationError = FString::Format(
				*NSLOCTEXT("AGASSMods", "DuplicateModId", "Another discovered mod already uses ModId '{0}'.").ToString(),
				{ModInfo.ModId});
			DiscoveredMods.Add(ModInfo);
			continue;
		}

		SeenModIds.Add(MakeLookupKey(ModInfo.ModId));
		ModInfo.bIsValid = true;

		TArray<FAGASSAvailableMapInfo> PluginMaps;
		for (const TSoftObjectPtr<UAGASSMapDefinition>& MapDefinitionPtr : Manifest->MapDefinitions)
		{
			UAGASSMapDefinition* MapDefinition = MapDefinitionPtr.LoadSynchronous();
			if (MapDefinition == nullptr)
			{
				AppendValidationMessage(
					ModInfo.ValidationError,
					NSLOCTEXT("AGASSMods", "ManifestInvalidMapReference", "One declared map definition could not be loaded and was ignored.").ToString());
				continue;
			}

			FAGASSAvailableMapInfo MapInfo;
			MapInfo.MapId = MapDefinition->MapId.TrimStartAndEnd();
			MapInfo.DisplayName = MapDefinition->DisplayName.IsEmpty() ? FText::FromString(MapInfo.MapId) : MapDefinition->DisplayName;
			MapInfo.Description = MapDefinition->Description;
			MapInfo.Version = MapDefinition->Version.TrimStartAndEnd();
			MapInfo.CompatibilityVersion = MapDefinition->CompatibilityVersion.TrimStartAndEnd();
			MapInfo.CompatibilityTags = MapDefinition->CompatibilityTags;
			MapInfo.SessionEventDefinitionAssets = MapDefinition->SessionEventDefinitions;
			NormalizeTagArray(MapInfo.CompatibilityTags);
			MapInfo.WorldPackagePath = MapDefinition->WorldAsset.ToSoftObjectPath().GetLongPackageName();
			MapInfo.OwningModId = ModInfo.ModId;
			MapInfo.OwningPluginName = Plugin->GetName();

			if (MapInfo.MapId.IsEmpty() || MapInfo.WorldPackagePath.IsEmpty())
			{
				AppendValidationMessage(
					ModInfo.ValidationError,
					NSLOCTEXT("AGASSMods", "ManifestInvalidMapMetadata", "One declared map definition was missing MapId or WorldAsset and was ignored.").ToString());
				continue;
			}

			if (SeenMapIds.Contains(MakeLookupKey(MapInfo.MapId)))
			{
				AppendValidationMessage(
					ModInfo.ValidationError,
					FString::Format(
						*NSLOCTEXT("AGASSMods", "DuplicateMapId", "The map id '{0}' conflicts with another discovered map and was ignored.").ToString(),
						{MapInfo.MapId}));
				continue;
			}

			SeenMapIds.Add(MakeLookupKey(MapInfo.MapId));
			PluginMaps.Add(MapInfo);
			ModInfo.DeclaredMapIds.Add(MapInfo.MapId);
		}

		DiscoveredMods.Add(ModInfo);
		for (const FAGASSAvailableMapInfo& MapInfo : PluginMaps)
		{
			AvailableMaps.Add(MapInfo);
		}
	}

	DiscoveredMods.Sort([](const FAGASSDiscoveredModInfo& Left, const FAGASSDiscoveredModInfo& Right)
	{
		return Left.GetDisplayLabel().Compare(Right.GetDisplayLabel(), ESearchCase::IgnoreCase) < 0;
	});

	AvailableMaps.Sort([](const FAGASSAvailableMapInfo& Left, const FAGASSAvailableMapInfo& Right)
	{
		if (Left.bIsBaseGameMap != Right.bIsBaseGameMap)
		{
			return Left.bIsBaseGameMap;
		}

		return Left.GetDisplayLabel().Compare(Right.GetDisplayLabel(), ESearchCase::IgnoreCase) < 0;
	});

	for (int32 Index = 0; Index < DiscoveredMods.Num(); ++Index)
	{
		if (!DiscoveredMods[Index].ModId.IsEmpty())
		{
			ModIndexById.Add(MakeLookupKey(DiscoveredMods[Index].ModId), Index);
		}
	}

	for (int32 Index = 0; Index < AvailableMaps.Num(); ++Index)
	{
		if (!AvailableMaps[Index].MapId.IsEmpty())
		{
			MapIndexById.Add(MakeLookupKey(AvailableMaps[Index].MapId), Index);
		}

		if (!AvailableMaps[Index].WorldPackagePath.IsEmpty())
		{
			MapIndexByWorldPackagePath.Add(MakeWorldPackagePathKey(AvailableMaps[Index].WorldPackagePath), Index);
		}
	}
}

void UAGASSModsSubsystem::AddConfiguredBaseMaps()
{
	const UAGASSModsDeveloperSettings* Settings = UAGASSModsDeveloperSettings::Get();
	if (Settings == nullptr)
	{
		return;
	}

	TSet<FString> AddedBaseMapIds;
	auto TryAddBaseMap = [this, &AddedBaseMapIds](FAGASSAvailableMapInfo&& BaseMapInfo)
	{
		BaseMapInfo.MapId = BaseMapInfo.MapId.TrimStartAndEnd();
		BaseMapInfo.WorldPackagePath = UWorld::RemovePIEPrefix(BaseMapInfo.WorldPackagePath.TrimStartAndEnd());
		NormalizeTagArray(BaseMapInfo.CompatibilityTags);
		BaseMapInfo.bIsBaseGameMap = true;

		if (BaseMapInfo.MapId.IsEmpty() || BaseMapInfo.WorldPackagePath.IsEmpty())
		{
			return;
		}

		const FString MapIdKey = MakeLookupKey(BaseMapInfo.MapId);
		if (AddedBaseMapIds.Contains(MapIdKey))
		{
			return;
		}

		AddedBaseMapIds.Add(MapIdKey);
		AvailableMaps.Add(MoveTemp(BaseMapInfo));
	};

	for (const TSoftObjectPtr<UAGASSMapDefinition>& BaseMapDefinitionPtr : Settings->BaseGameMapDefinitions)
	{
		const UAGASSMapDefinition* BaseMapDefinition = BaseMapDefinitionPtr.LoadSynchronous();
		if (BaseMapDefinition == nullptr)
		{
			continue;
		}

		FAGASSAvailableMapInfo BaseMapInfo;
		BaseMapInfo.MapId = BaseMapDefinition->MapId.TrimStartAndEnd();
		BaseMapInfo.DisplayName = BaseMapDefinition->DisplayName;
		BaseMapInfo.Description = BaseMapDefinition->Description;
		BaseMapInfo.Version = BaseMapDefinition->Version.TrimStartAndEnd();
		BaseMapInfo.CompatibilityVersion = BaseMapDefinition->CompatibilityVersion.TrimStartAndEnd();
		BaseMapInfo.CompatibilityTags = BaseMapDefinition->CompatibilityTags;
		BaseMapInfo.SessionEventDefinitionAssets = BaseMapDefinition->SessionEventDefinitions;
		BaseMapInfo.WorldPackagePath = BaseMapDefinition->WorldAsset.ToSoftObjectPath().GetLongPackageName();
		TryAddBaseMap(MoveTemp(BaseMapInfo));
	}

	if (AvailableMaps.Num() > 0)
	{
		return;
	}

	FAGASSAvailableMapInfo LegacyBaseMapInfo;
	if (const UAGASSMapDefinition* BaseMapDefinition = Settings->DefaultBaseMapDefinition.LoadSynchronous())
	{
		LegacyBaseMapInfo.MapId = BaseMapDefinition->MapId.TrimStartAndEnd();
		LegacyBaseMapInfo.DisplayName = BaseMapDefinition->DisplayName.IsEmpty()
			? Settings->DefaultBaseMapDisplayName
			: BaseMapDefinition->DisplayName;
		LegacyBaseMapInfo.Description = BaseMapDefinition->Description.IsEmpty()
			? Settings->DefaultBaseMapDescription
			: BaseMapDefinition->Description;
		LegacyBaseMapInfo.Version = BaseMapDefinition->Version.TrimStartAndEnd();
		LegacyBaseMapInfo.CompatibilityVersion = BaseMapDefinition->CompatibilityVersion.TrimStartAndEnd();
		LegacyBaseMapInfo.CompatibilityTags = BaseMapDefinition->CompatibilityTags;
		LegacyBaseMapInfo.SessionEventDefinitionAssets = BaseMapDefinition->SessionEventDefinitions;
		LegacyBaseMapInfo.WorldPackagePath = BaseMapDefinition->WorldAsset.ToSoftObjectPath().GetLongPackageName();
	}
	else
	{
		LegacyBaseMapInfo.MapId = Settings->DefaultBaseMapId.TrimStartAndEnd();
		LegacyBaseMapInfo.DisplayName = Settings->DefaultBaseMapDisplayName;
		LegacyBaseMapInfo.Description = Settings->DefaultBaseMapDescription;
		LegacyBaseMapInfo.Version = Settings->DefaultBaseMapVersion.TrimStartAndEnd();
		LegacyBaseMapInfo.CompatibilityVersion = Settings->DefaultBaseMapCompatibilityVersion.TrimStartAndEnd();
		LegacyBaseMapInfo.WorldPackagePath = Settings->DefaultBaseGameMap.ToSoftObjectPath().GetLongPackageName();
	}

	TryAddBaseMap(MoveTemp(LegacyBaseMapInfo));
}

void UAGASSModsSubsystem::RefreshDynamicFlags()
{
	for (FAGASSDiscoveredModInfo& ModInfo : DiscoveredMods)
	{
		ModInfo.bIsActive = IsModActive(ModInfo.ModId);
	}

	for (FAGASSAvailableMapInfo& MapInfo : AvailableMaps)
	{
		MapInfo.bIsCurrentlySelected = MapInfo.MapId.Equals(SelectedMapId, ESearchCase::IgnoreCase);
		MapInfo.bOwningModIsActive = MapInfo.OwningModId.IsEmpty() || IsModActive(MapInfo.OwningModId);
	}
}

void UAGASSModsSubsystem::ValidateSelectionState()
{
	TArray<FString> PrunedActiveMods;
	for (const FString& ActiveModId : ActiveModIds)
	{
		const int32* ModIndex = ModIndexById.Find(MakeLookupKey(ActiveModId));
		if (ModIndex != nullptr && DiscoveredMods.IsValidIndex(*ModIndex) && DiscoveredMods[*ModIndex].bIsValid)
		{
			PrunedActiveMods.Add(DiscoveredMods[*ModIndex].ModId);
		}
	}

	ActiveModIds = MoveTemp(PrunedActiveMods);
	NormalizeStringArray(ActiveModIds);

	const FAGASSAvailableMapInfo* SelectedMapInfo = GetSelectedMapInfo();
	if (SelectedMapInfo == nullptr)
	{
		const UAGASSModsDeveloperSettings* Settings = UAGASSModsDeveloperSettings::Get();
		if (Settings != nullptr)
		{
			if (const int32* DefaultMapIndex = MapIndexById.Find(MakeLookupKey(Settings->DefaultSelectedMapId)))
			{
				if (AvailableMaps.IsValidIndex(*DefaultMapIndex))
				{
					SelectedMapId = AvailableMaps[*DefaultMapIndex].MapId;
					SelectedMapInfo = &AvailableMaps[*DefaultMapIndex];
				}
			}
		}

		if (SelectedMapInfo == nullptr && AvailableMaps.Num() > 0)
		{
			SelectedMapId = AvailableMaps[0].MapId;
			SelectedMapInfo = &AvailableMaps[0];
		}
		else
		{
			SelectedMapId.Reset();
			return;
		}
	}

	if (SelectedMapInfo != nullptr && !SelectedMapInfo->OwningModId.IsEmpty() && !IsModActive(SelectedMapInfo->OwningModId))
	{
		ActiveModIds.Add(SelectedMapInfo->OwningModId);
		NormalizeStringArray(ActiveModIds);
	}
}

bool UAGASSModsSubsystem::TryResolveSelection(
	const FString& MapId,
	const TArray<FString>& RequestedActiveModIds,
	FAGASSResolvedContentSelection& OutSelection,
	FString& OutFailureMessage) const
{
	OutSelection = FAGASSResolvedContentSelection();
	OutFailureMessage.Reset();

	const int32* MapIndex = MapIndexById.Find(MakeLookupKey(MapId));
	if (MapIndex == nullptr || !AvailableMaps.IsValidIndex(*MapIndex))
	{
		OutFailureMessage = NSLOCTEXT("AGASSMods", "ResolveSelectionUnknownMap", "The selected map is no longer available.").ToString();
		return false;
	}

	const FAGASSAvailableMapInfo& MapInfo = AvailableMaps[*MapIndex];
	TArray<const FAGASSDiscoveredModInfo*> ResolvedMods;
	if (!TryResolveActiveMods(RequestedActiveModIds, ResolvedMods, OutFailureMessage))
	{
		return false;
	}

	if (!MapInfo.OwningModId.IsEmpty()
		&& !ResolvedMods.ContainsByPredicate([&MapInfo](const FAGASSDiscoveredModInfo* ModInfo)
		{
			return ModInfo != nullptr && ModInfo->ModId.Equals(MapInfo.OwningModId, ESearchCase::IgnoreCase);
		}))
	{
		OutFailureMessage = FString::Format(
			*NSLOCTEXT("AGASSMods", "ResolveSelectionMissingOwningMod", "Map '{0}' requires the active mod '{1}'.").ToString(),
			{MapInfo.GetDisplayLabel(), MapInfo.OwningModId});
		return false;
	}

	OutSelection.bIsValid = true;
	OutSelection.SelectedMap = MapInfo;

	TSet<FString> SeenSessionEventAssetKeys;
	AppendUniqueSoftObjectPaths(MapInfo.SessionEventDefinitionAssets, OutSelection.SessionEventDefinitionAssets, SeenSessionEventAssetKeys);

	TArray<FString> ActiveModIdsForSelection;
	for (const FAGASSDiscoveredModInfo* ModInfo : ResolvedMods)
	{
		if (ModInfo == nullptr)
		{
			continue;
		}

		FAGASSActiveModCompatibilityInfo ActiveModInfo;
		ActiveModInfo.ModId = ModInfo->ModId;
		ActiveModInfo.DisplayName = ModInfo->DisplayName;
		ActiveModInfo.Version = ModInfo->Version;
		ActiveModInfo.CompatibilityVersion = ModInfo->CompatibilityVersion;
		ActiveModInfo.CompatibilityTags = ModInfo->CompatibilityTags;
		ActiveModInfo.PluginName = ModInfo->PluginName;
		OutSelection.ActiveMods.Add(ActiveModInfo);
		ActiveModIdsForSelection.Add(ModInfo->ModId);
		AppendUniqueSoftObjectPaths(
			ModInfo->DeclaredSessionEventDefinitionAssets,
			OutSelection.SessionEventDefinitionAssets,
			SeenSessionEventAssetKeys);
	}

	NormalizeStringArray(ActiveModIdsForSelection);
	OutSelection.ActiveModIdsCsv = JoinIds(ActiveModIdsForSelection);
	OutSelection.ContentCompatibilityHash = BuildContentCompatibilityHash(MapInfo, ResolvedMods);
	return true;
}

bool UAGASSModsSubsystem::TryResolveActiveMods(
	const TArray<FString>& RequestedActiveModIds,
	TArray<const FAGASSDiscoveredModInfo*>& OutResolvedMods,
	FString& OutFailureMessage) const
{
	OutResolvedMods.Reset();
	OutFailureMessage.Reset();

	TArray<FString> NormalizedRequestedIds = RequestedActiveModIds;
	NormalizeStringArray(NormalizedRequestedIds);

	for (const FString& RequestedModId : NormalizedRequestedIds)
	{
		const int32* ModIndex = ModIndexById.Find(MakeLookupKey(RequestedModId));
		if (ModIndex == nullptr || !DiscoveredMods.IsValidIndex(*ModIndex))
		{
			OutFailureMessage = FString::Format(
				*NSLOCTEXT("AGASSMods", "ResolveSelectionMissingMod", "The active mod '{0}' is no longer available.").ToString(),
				{RequestedModId});
			return false;
		}

		const FAGASSDiscoveredModInfo& ModInfo = DiscoveredMods[*ModIndex];
		if (!ModInfo.bIsValid)
		{
			OutFailureMessage = ModInfo.ValidationError.IsEmpty()
				? FString::Format(
					*NSLOCTEXT("AGASSMods", "ResolveSelectionInvalidMod", "The active mod '{0}' is not valid for runtime use.").ToString(),
					{ModInfo.ModId})
				: ModInfo.ValidationError;
			return false;
		}

		OutResolvedMods.Add(&ModInfo);
	}

	Algo::Sort(OutResolvedMods, [](const FAGASSDiscoveredModInfo* Left, const FAGASSDiscoveredModInfo* Right)
	{
		if (Left == nullptr || Right == nullptr)
		{
			return Left != nullptr;
		}

		return Left->ModId.Compare(Right->ModId, ESearchCase::IgnoreCase) < 0;
	});

	return true;
}

FString UAGASSModsSubsystem::BuildContentCompatibilityHash(
	const FAGASSAvailableMapInfo& MapInfo,
	const TArray<const FAGASSDiscoveredModInfo*>& ActiveMods) const
{
	TArray<FString> CompatibilityTokens;

	TArray<FString> MapTags = MapInfo.CompatibilityTags;
	NormalizeTagArray(MapTags);
	CompatibilityTokens.Add(FString::Printf(TEXT("MapId=%s"), *MapInfo.MapId));
	CompatibilityTokens.Add(FString::Printf(TEXT("MapCompat=%s"), *MapInfo.GetEffectiveCompatibilityVersion()));
	CompatibilityTokens.Add(FString::Printf(TEXT("MapTags=%s"), *JoinValues(MapTags, TEXT("+"))));

	for (const FAGASSDiscoveredModInfo* ModInfo : ActiveMods)
	{
		if (ModInfo == nullptr)
		{
			continue;
		}

		TArray<FString> ModTags = ModInfo->CompatibilityTags;
		NormalizeTagArray(ModTags);
		CompatibilityTokens.Add(FString::Printf(TEXT("Mod=%s@%s[%s]"), *ModInfo->ModId, *ModInfo->GetEffectiveCompatibilityVersion(), *JoinValues(ModTags, TEXT("+"))));
	}

	CompatibilityTokens.Sort([](const FString& Left, const FString& Right)
	{
		return Left.Compare(Right, ESearchCase::IgnoreCase) < 0;
	});

	return FMD5::HashAnsiString(*JoinValues(CompatibilityTokens, TEXT("|")));
}

void UAGASSModsSubsystem::NormalizeStringArray(TArray<FString>& InOutValues)
{
	for (FString& Value : InOutValues)
	{
		Value = Value.TrimStartAndEnd();
	}

	InOutValues.RemoveAll([](const FString& Value)
	{
		return Value.IsEmpty();
	});

	InOutValues.Sort([](const FString& Left, const FString& Right)
	{
		return Left.Compare(Right, ESearchCase::IgnoreCase) < 0;
	});
	InOutValues.SetNum(Algo::Unique(InOutValues));
}

TArray<FString> UAGASSModsSubsystem::ParseDelimitedIds(const FString& DelimitedValue)
{
	TArray<FString> Values;
	DelimitedValue.ParseIntoArray(Values, TEXT(","), true);
	NormalizeStringArray(Values);
	return Values;
}

FString UAGASSModsSubsystem::JoinIds(const TArray<FString>& Values)
{
	return JoinValues(Values, TEXT(","));
}
