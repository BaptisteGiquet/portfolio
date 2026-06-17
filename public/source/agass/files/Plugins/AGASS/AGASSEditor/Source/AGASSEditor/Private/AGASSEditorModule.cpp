#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/AGASSMapDefinition.h"
#include "Data/AGASSModManifest.h"
#include "Data/AGASSSessionEventDefinition.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "IUATHelperModule.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "PluginDescriptor.h"
#include "Settings/AGASSModExportDeveloperSettings.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ToolMenu.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "AGASSEditor"

namespace AGASSEditor::Private
{
	constexpr TCHAR ExportPlatformName[] = TEXT("Windows");
	constexpr TCHAR PackagedGameProjectName[] = TEXT("AGASS");

	struct FResolvedReleaseInfo
	{
		FString Version;
		FString RootDirectory;
		FString MetadataDirectory;
		FString CommandLineRootDirectory;
	};

	struct FValidatedPluginInfo
	{
		FString PluginName;
		FString DescriptorFilePath;
		FString PluginBaseDir;
		FString MountedAssetPath;
		FPluginDescriptor Descriptor;
	};

	FString NormalizeDirectory(const FString& InPath)
	{
		FString Result = FPaths::ConvertRelativePathToFull(InPath);
		FPaths::NormalizeDirectoryName(Result);
		return Result;
	}

	FString NormalizeFilename(const FString& InPath)
	{
		FString Result = FPaths::ConvertRelativePathToFull(InPath);
		FPaths::NormalizeFilename(Result);
		return Result;
	}

	FString ResolveDirectoryPath(const FDirectoryPath& DirectoryPath)
	{
		if (DirectoryPath.Path.IsEmpty())
		{
			return FString();
		}

		return FPaths::IsRelative(DirectoryPath.Path)
			? NormalizeDirectory(FPaths::Combine(FPaths::ProjectDir(), DirectoryPath.Path))
			: NormalizeDirectory(DirectoryPath.Path);
	}

	FString GetDefaultReleaseRoot()
	{
		return NormalizeDirectory(FPaths::Combine(FPaths::ProjectDir(), TEXT("Releases")));
	}

	bool IsDefaultReleaseRoot(const FString& ReleaseRoot)
	{
		return NormalizeDirectory(ReleaseRoot).Equals(GetDefaultReleaseRoot(), ESearchCase::IgnoreCase);
	}

	bool IsPathUnderOrSame(const FString& CandidatePath, const FString& RootPath)
	{
		return CandidatePath.Equals(RootPath, ESearchCase::IgnoreCase) || FPaths::IsUnderDirectory(CandidatePath, RootPath);
	}

	FString NormalizeMountedAssetPath(const FString& MountedAssetPath)
	{
		FString Result = MountedAssetPath;
		while (Result.Len() > 1 && Result.EndsWith(TEXT("/")))
		{
			Result.LeftChopInline(1, EAllowShrinking::No);
		}
		return Result;
	}

	bool IsPackageInPlugin(const FString& PackageName, const FString& MountedAssetPath)
	{
		const FString NormalizedMountedAssetPath = NormalizeMountedAssetPath(MountedAssetPath);
		return PackageName.Equals(NormalizedMountedAssetPath, ESearchCase::IgnoreCase)
			|| PackageName.StartsWith(NormalizedMountedAssetPath + TEXT("/"), ESearchCase::IgnoreCase);
	}

	bool LoadJsonObject(const FString& FilePath, TSharedPtr<FJsonObject>& OutObject)
	{
		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
		{
			return false;
		}

		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		return FJsonSerializer::Deserialize(Reader, OutObject) && OutObject.IsValid();
	}

	bool SaveJsonObject(const FString& FilePath, const TSharedRef<FJsonObject>& JsonObject)
	{
		FString JsonText;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);
		if (!FJsonSerializer::Serialize(JsonObject, Writer))
		{
			return false;
		}

		return FFileHelper::SaveStringToFile(JsonText, *FilePath);
	}

	void ReplaceProjectUprojectPath(TArray<TSharedPtr<FJsonValue>>* Values, const FString& ProjectFileName)
	{
		if (Values == nullptr)
		{
			return;
		}

		const FString TargetPath = FString::Printf(TEXT("$(ProjectDir)/%s"), *ProjectFileName);
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			if (!Value.IsValid())
			{
				continue;
			}

			const TSharedPtr<FJsonObject> Object = Value->AsObject();
			if (!Object.IsValid())
			{
				continue;
			}

			FString PathValue;
			if (Object->TryGetStringField(TEXT("Path"), PathValue) && PathValue == TEXT("$(ProjectDir)/AGASS.uproject"))
			{
				Object->SetStringField(TEXT("Path"), TargetPath);
			}
		}
	}

	bool EnsureProjectNamedReceipt(const FString& SourceReceiptName, const FString& TargetReceiptName, const FString& TargetName, FString& OutFailureMessage)
	{
		const FString BinariesDir = NormalizeDirectory(FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries"), TEXT("Win64")));
		const FString SourceReceiptPath = FPaths::Combine(BinariesDir, SourceReceiptName);
		const FString TargetReceiptPath = FPaths::Combine(BinariesDir, TargetReceiptName);

		TSharedPtr<FJsonObject> JsonObject;
		if (!LoadJsonObject(SourceReceiptPath, JsonObject))
		{
			OutFailureMessage = FString::Printf(TEXT("Could not read source receipt '%s'."), *SourceReceiptPath);
			return false;
		}

		const FString ProjectFileName = FPaths::GetCleanFilename(FPaths::GetProjectFilePath());
		JsonObject->SetStringField(TEXT("TargetName"), TargetName);
		JsonObject->SetStringField(TEXT("Project"), FString::Printf(TEXT("../../%s"), *ProjectFileName));

		const TArray<TSharedPtr<FJsonValue>>* BuildProducts = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("BuildProducts"), BuildProducts))
		{
			ReplaceProjectUprojectPath(const_cast<TArray<TSharedPtr<FJsonValue>>*>(BuildProducts), ProjectFileName);
		}

		const TArray<TSharedPtr<FJsonValue>>* RuntimeDependencies = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("RuntimeDependencies"), RuntimeDependencies))
		{
			ReplaceProjectUprojectPath(const_cast<TArray<TSharedPtr<FJsonValue>>*>(RuntimeDependencies), ProjectFileName);
		}

		if (!SaveJsonObject(TargetReceiptPath, JsonObject.ToSharedRef()))
		{
			OutFailureMessage = FString::Printf(TEXT("Could not write project receipt '%s'."), *TargetReceiptPath);
			return false;
		}

		OutFailureMessage.Reset();
		return true;
	}

	bool EnsureProjectNamedReceipts(FString& OutFailureMessage)
	{
		const FString ProjectName = FApp::GetProjectName();
		if (ProjectName.IsEmpty() || ProjectName.Equals(PackagedGameProjectName, ESearchCase::IgnoreCase))
		{
			OutFailureMessage.Reset();
			return true;
		}

		if (!EnsureProjectNamedReceipt(TEXT("AGASSEditor.target"), FString::Printf(TEXT("%sEditor.target"), *ProjectName), FString::Printf(TEXT("%sEditor"), *ProjectName), OutFailureMessage))
		{
			return false;
		}

		return EnsureProjectNamedReceipt(TEXT("AGASS.target"), FString::Printf(TEXT("%s.target"), *ProjectName), ProjectName, OutFailureMessage);
	}

	bool TryPromptForPluginDescriptor(FString& OutDescriptorPath, FString& OutFailureMessage)
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::TryGet();
		if (DesktopPlatform == nullptr)
		{
			OutFailureMessage = TEXT("Desktop file dialogs are not available.");
			return false;
		}

		const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

		TArray<FString> SelectedFiles;
		const bool bPickedFile = DesktopPlatform->OpenFileDialog(
			ParentWindowHandle,
			LOCTEXT("SelectModPluginDialogTitle", "Select Mod Plugin").ToString(),
			FPaths::ProjectPluginsDir(),
			TEXT(""),
			TEXT("Unreal Plugin (*.uplugin)|*.uplugin"),
			EFileDialogFlags::None,
			SelectedFiles);

		if (!bPickedFile || SelectedFiles.Num() == 0)
		{
			OutFailureMessage.Reset();
			return false;
		}

		OutDescriptorPath = NormalizeFilename(SelectedFiles[0]);
		OutFailureMessage.Reset();
		return true;
	}

	bool TryResolveReleaseInfo(const UAGASSModExportDeveloperSettings& Settings, FResolvedReleaseInfo& OutReleaseInfo, FString& OutFailureMessage)
	{
		const FString ReleaseRoot = ResolveDirectoryPath(Settings.BaseReleaseRoot);
		if (ReleaseRoot.IsEmpty())
		{
			OutFailureMessage = TEXT("Base Release Root is empty. Point it at a folder that contains Releases/<Version>/Windows/Metadata.");
			return false;
		}

		if (!FPaths::DirectoryExists(ReleaseRoot))
		{
			OutFailureMessage = FString::Printf(TEXT("Base Release Root does not exist: %s"), *ReleaseRoot);
			return false;
		}

		const FString RequestedVersion = Settings.BaseReleaseVersion.TrimStartAndEnd();
		if (!RequestedVersion.IsEmpty())
		{
			const FString MetadataDirectory = FPaths::Combine(ReleaseRoot, RequestedVersion, ExportPlatformName, TEXT("Metadata"));
			if (!FPaths::DirectoryExists(MetadataDirectory))
			{
				OutFailureMessage = FString::Printf(TEXT("Could not find release metadata at '%s'."), *MetadataDirectory);
				return false;
			}

			OutReleaseInfo.Version = RequestedVersion;
			OutReleaseInfo.RootDirectory = ReleaseRoot;
			OutReleaseInfo.MetadataDirectory = NormalizeDirectory(MetadataDirectory);
			OutReleaseInfo.CommandLineRootDirectory = IsDefaultReleaseRoot(ReleaseRoot) ? FString() : ReleaseRoot;
			OutFailureMessage.Reset();
			return true;
		}

		TArray<FString> CandidateVersions;
		IFileManager::Get().FindFiles(CandidateVersions, *FPaths::Combine(ReleaseRoot, TEXT("*")), false, true);
		CandidateVersions.Sort();

		TArray<FString> ValidVersions;
		for (const FString& CandidateVersion : CandidateVersions)
		{
			const FString MetadataDirectory = FPaths::Combine(ReleaseRoot, CandidateVersion, ExportPlatformName, TEXT("Metadata"));
			if (FPaths::DirectoryExists(MetadataDirectory))
			{
				ValidVersions.Add(CandidateVersion);
			}
		}

		if (ValidVersions.Num() == 1)
		{
			OutReleaseInfo.Version = ValidVersions[0];
			OutReleaseInfo.RootDirectory = ReleaseRoot;
			OutReleaseInfo.MetadataDirectory = NormalizeDirectory(FPaths::Combine(ReleaseRoot, ValidVersions[0], ExportPlatformName, TEXT("Metadata")));
			OutReleaseInfo.CommandLineRootDirectory = IsDefaultReleaseRoot(ReleaseRoot) ? FString() : ReleaseRoot;
			OutFailureMessage.Reset();
			return true;
		}

		if (ValidVersions.IsEmpty())
		{
			OutFailureMessage = FString::Printf(TEXT("No release metadata was found under '%s'."), *ReleaseRoot);
		}
		else
		{
			OutFailureMessage = FString::Printf(
				TEXT("Multiple release versions were found under '%s'. Set Base Release Version explicitly. Found: %s"),
				*ReleaseRoot,
				*FString::Join(ValidVersions, TEXT(", ")));
		}

		return false;
	}

	bool TryLoadPluginDescriptor(const FString& DescriptorFilePath, FPluginDescriptor& OutDescriptor, FString& OutFailureMessage)
	{
		FText FailReason;
		if (!OutDescriptor.Load(DescriptorFilePath, &FailReason))
		{
			OutFailureMessage = FailReason.IsEmpty() ? TEXT("The plugin descriptor could not be loaded.") : FailReason.ToString();
			return false;
		}

		OutFailureMessage.Reset();
		return true;
	}

	bool TryValidatePlugin(const FString& DescriptorFilePath, FValidatedPluginInfo& OutPluginInfo, FString& OutFailureMessage)
	{
		const FString NormalizedDescriptorPath = NormalizeFilename(DescriptorFilePath);
		if (!FPaths::FileExists(NormalizedDescriptorPath))
		{
			OutFailureMessage = FString::Printf(TEXT("Plugin descriptor not found: %s"), *NormalizedDescriptorPath);
			return false;
		}

		const FString ProjectPluginsRoot = NormalizeDirectory(FPaths::ProjectPluginsDir());
		if (!IsPathUnderOrSame(NormalizedDescriptorPath, ProjectPluginsRoot))
		{
			OutFailureMessage = FString::Printf(TEXT("Mods must be authored as project plugins under '%s'."), *ProjectPluginsRoot);
			return false;
		}

		FPluginDescriptor PluginDescriptor;
		if (!TryLoadPluginDescriptor(NormalizedDescriptorPath, PluginDescriptor, OutFailureMessage))
		{
			return false;
		}

		if (!PluginDescriptor.bCanContainContent)
		{
			OutFailureMessage = TEXT("The selected plugin must be a content plugin.");
			return false;
		}

		if (PluginDescriptor.Modules.Num() > 0)
		{
			OutFailureMessage = TEXT("Native-code mods are not supported. Remove every module from the plugin descriptor.");
			return false;
		}

		const FString PluginName = FPaths::GetBaseFilename(NormalizedDescriptorPath);
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
		if (!Plugin.IsValid())
		{
			OutFailureMessage = FString::Printf(TEXT("Plugin '%s' is not registered. Enable it in Edit > Plugins and restart the editor if needed."), *PluginName);
			return false;
		}

		const FString MountedAssetPath = NormalizeMountedAssetPath(Plugin->GetMountedAssetPath());
		if (MountedAssetPath.IsEmpty())
		{
			OutFailureMessage = FString::Printf(TEXT("Plugin '%s' does not have a mounted asset path. Enable it and restart the editor."), *PluginName);
			return false;
		}

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		AssetRegistry.WaitForCompletion();

		TArray<FString> PathsToScan;
		PathsToScan.Add(MountedAssetPath);
		AssetRegistry.ScanPathsSynchronous(PathsToScan, true);

		FARFilter ManifestFilter;
		ManifestFilter.PackagePaths.Add(FName(*MountedAssetPath));
		ManifestFilter.ClassPaths.Add(UAGASSModManifest::StaticClass()->GetClassPathName());
		ManifestFilter.bRecursivePaths = true;
		ManifestFilter.bRecursiveClasses = true;

		TArray<FAssetData> ManifestAssets;
		AssetRegistry.GetAssets(ManifestFilter, ManifestAssets);
		ManifestAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
		{
			return Left.GetObjectPathString().Compare(Right.GetObjectPathString(), ESearchCase::IgnoreCase) < 0;
		});

		if (ManifestAssets.Num() != 1)
		{
			OutFailureMessage = TEXT("The mod plugin must contain exactly one UAGASSModManifest asset.");
			return false;
		}

		UAGASSModManifest* Manifest = Cast<UAGASSModManifest>(ManifestAssets[0].GetAsset());
		if (Manifest == nullptr)
		{
			OutFailureMessage = TEXT("The mod manifest asset could not be loaded.");
			return false;
		}

		if (Manifest->ModId.TrimStartAndEnd().IsEmpty())
		{
			OutFailureMessage = TEXT("The mod manifest must declare a stable ModId.");
			return false;
		}

		if (Manifest->GetEffectiveCompatibilityVersion().TrimStartAndEnd().IsEmpty())
		{
			OutFailureMessage = TEXT("The mod manifest must declare a Version or CompatibilityVersion.");
			return false;
		}

		if (Manifest->MapDefinitions.IsEmpty() && Manifest->SessionEventDefinitions.IsEmpty())
		{
			OutFailureMessage = TEXT("The mod manifest must reference at least one UAGASSMapDefinition asset or one session event definition.");
			return false;
		}

		TSet<FString> SeenMapIds;
		for (const TSoftObjectPtr<UAGASSMapDefinition>& MapDefinitionPtr : Manifest->MapDefinitions)
		{
			const FSoftObjectPath MapDefinitionPath = MapDefinitionPtr.ToSoftObjectPath();
			if (!IsPackageInPlugin(MapDefinitionPath.GetLongPackageName(), MountedAssetPath))
			{
				OutFailureMessage = FString::Printf(
					TEXT("Every referenced map definition must live inside the same plugin. Invalid asset: %s"),
					*MapDefinitionPath.ToString());
				return false;
			}

			UAGASSMapDefinition* MapDefinition = MapDefinitionPtr.LoadSynchronous();
			if (MapDefinition == nullptr)
			{
				OutFailureMessage = FString::Printf(TEXT("A referenced map definition could not be loaded: %s"), *MapDefinitionPath.ToString());
				return false;
			}

			const FString LoadedDefinitionPackage = MapDefinition->GetOutermost()->GetName();
			if (!IsPackageInPlugin(LoadedDefinitionPackage, MountedAssetPath))
			{
				OutFailureMessage = FString::Printf(
					TEXT("Every referenced map definition must live inside the same plugin. Invalid asset: %s"),
					*LoadedDefinitionPackage);
				return false;
			}

			const FString MapId = MapDefinition->MapId.TrimStartAndEnd();
			if (MapId.IsEmpty())
			{
				OutFailureMessage = TEXT("Every map definition must declare a stable MapId.");
				return false;
			}

			if (MapDefinition->GetEffectiveCompatibilityVersion().TrimStartAndEnd().IsEmpty())
			{
				OutFailureMessage = FString::Printf(TEXT("Map definition '%s' must declare a Version or CompatibilityVersion."), *LoadedDefinitionPackage);
				return false;
			}

			if (SeenMapIds.Contains(MapId))
			{
				OutFailureMessage = FString::Printf(TEXT("Duplicate MapId found in the manifest: %s"), *MapId);
				return false;
			}
			SeenMapIds.Add(MapId);

			const FString WorldPackageName = MapDefinition->WorldAsset.ToSoftObjectPath().GetLongPackageName();
			if (WorldPackageName.IsEmpty())
			{
				OutFailureMessage = FString::Printf(TEXT("Map definition '%s' must reference a WorldAsset."), *LoadedDefinitionPackage);
				return false;
			}

			if (!IsPackageInPlugin(WorldPackageName, MountedAssetPath))
			{
				OutFailureMessage = FString::Printf(
					TEXT("Every referenced map asset must live inside the same plugin. Invalid asset: %s"),
					*WorldPackageName);
				return false;
			}
		}

		TSet<FString> SeenEventIds;
		for (const FSoftObjectPath& SessionEventDefinitionPath : Manifest->SessionEventDefinitions)
		{
			if (!IsPackageInPlugin(SessionEventDefinitionPath.GetLongPackageName(), MountedAssetPath))
			{
				OutFailureMessage = FString::Printf(
					TEXT("Every referenced session event definition must live inside the same plugin. Invalid asset: %s"),
					*SessionEventDefinitionPath.ToString());
				return false;
			}

			UAGASSSessionEventDefinition* SessionEventDefinition = Cast<UAGASSSessionEventDefinition>(SessionEventDefinitionPath.TryLoad());
			if (SessionEventDefinition == nullptr)
			{
				OutFailureMessage = FString::Printf(
					TEXT("A referenced session event definition could not be loaded: %s"),
					*SessionEventDefinitionPath.ToString());
				return false;
			}

			const FString LoadedDefinitionPackage = SessionEventDefinition->GetOutermost()->GetName();
			if (!IsPackageInPlugin(LoadedDefinitionPackage, MountedAssetPath))
			{
				OutFailureMessage = FString::Printf(
					TEXT("Every referenced session event definition must live inside the same plugin. Invalid asset: %s"),
					*LoadedDefinitionPackage);
				return false;
			}

			const FString EventId = SessionEventDefinition->EventId.TrimStartAndEnd();
			if (EventId.IsEmpty())
			{
				OutFailureMessage = FString::Printf(
					TEXT("Session event definition '%s' must declare a stable EventId."),
					*LoadedDefinitionPackage);
				return false;
			}

			if (SeenEventIds.Contains(EventId))
			{
				OutFailureMessage = FString::Printf(TEXT("Duplicate EventId found in the manifest: %s"), *EventId);
				return false;
			}
			SeenEventIds.Add(EventId);
		}

		OutPluginInfo.PluginName = PluginName;
		OutPluginInfo.DescriptorFilePath = NormalizedDescriptorPath;
		OutPluginInfo.PluginBaseDir = NormalizeDirectory(Plugin->GetBaseDir());
		OutPluginInfo.MountedAssetPath = MountedAssetPath;
		OutPluginInfo.Descriptor = MoveTemp(PluginDescriptor);
		OutFailureMessage.Reset();
		return true;
	}

	bool TryCopyFile(const FString& SourcePath, const FString& DestinationPath, FString& OutFailureMessage)
	{
		if (IFileManager::Get().Copy(*DestinationPath, *SourcePath, true, true) != COPY_OK)
		{
			OutFailureMessage = FString::Printf(TEXT("Failed to copy '%s' to '%s'."), *SourcePath, *DestinationPath);
			return false;
		}

		OutFailureMessage.Reset();
		return true;
	}

	bool TryCopyCookedContainers(const FString& PluginName, const FString& StagingDirectory, const FString& ExportPakDirectory, FString& OutFailureMessage)
	{
		TArray<FString> CookedFiles;
		const TCHAR* Extensions[] = { TEXT("*.pak"), TEXT("*.utoc"), TEXT("*.ucas"), TEXT("*.sig") };
		for (const TCHAR* Extension : Extensions)
		{
			IFileManager::Get().FindFilesRecursive(CookedFiles, *StagingDirectory, Extension, true, false, false);
		}

		CookedFiles.Sort();

		bool bCopiedAnyFile = false;
		for (const FString& SourceFile : CookedFiles)
		{
			const FString Filename = FPaths::GetCleanFilename(SourceFile);
			if (!Filename.StartsWith(PluginName, ESearchCase::IgnoreCase))
			{
				continue;
			}

			const FString DestinationFile = FPaths::Combine(ExportPakDirectory, Filename);
			if (!TryCopyFile(SourceFile, DestinationFile, OutFailureMessage))
			{
				return false;
			}

			bCopiedAnyFile = true;
		}

		if (!bCopiedAnyFile)
		{
			OutFailureMessage = FString::Printf(TEXT("No cooked plugin containers for '%s' were found in '%s'."), *PluginName, *StagingDirectory);
			return false;
		}

		OutFailureMessage.Reset();
		return true;
	}

	bool TryFindCookedAssetRegistry(const FString& PluginBaseDir, FString& OutAssetRegistryPath, FString& OutFailureMessage)
	{
		const FString CookedRoot = FPaths::Combine(PluginBaseDir, TEXT("Saved"), TEXT("Cooked"));
		TArray<FString> AssetRegistryCandidates;
		IFileManager::Get().FindFilesRecursive(AssetRegistryCandidates, *CookedRoot, TEXT("AssetRegistry.bin"), true, false, false);

		if (AssetRegistryCandidates.IsEmpty())
		{
			OutFailureMessage = FString::Printf(TEXT("Could not find a cooked AssetRegistry.bin under '%s'."), *CookedRoot);
			return false;
		}

		AssetRegistryCandidates.Sort([](const FString& Left, const FString& Right)
		{
			return IFileManager::Get().GetTimeStamp(*Left) > IFileManager::Get().GetTimeStamp(*Right);
		});

		OutAssetRegistryPath = NormalizeFilename(AssetRegistryCandidates[0]);
		OutFailureMessage.Reset();
		return true;
	}

	bool FinalizeExport(
		const FValidatedPluginInfo& PluginInfo,
		const FString& StagingDirectory,
		const FString& ExportRootDirectory,
		FString& OutFailureMessage)
	{
		IFileManager& FileManager = IFileManager::Get();
		FileManager.DeleteDirectory(*ExportRootDirectory, false, true);

		const FString ModsRoot = FPaths::Combine(ExportRootDirectory, TEXT("Mods"));
		const FString ExportModRoot = FPaths::Combine(ModsRoot, PluginInfo.PluginName);
		const FString ExportPakDirectory = FPaths::Combine(ExportModRoot, TEXT("Content"), TEXT("Paks"), ExportPlatformName);
		if (!FileManager.MakeDirectory(*ExportPakDirectory, true))
		{
			OutFailureMessage = FString::Printf(TEXT("Could not create export directory '%s'."), *ExportPakDirectory);
			return false;
		}

		FPluginDescriptor ExportDescriptor = PluginInfo.Descriptor;
		ExportDescriptor.bExplicitlyLoaded = true;
		ExportDescriptor.EnabledByDefault = EPluginEnabledByDefault::Disabled;

		const FString ExportDescriptorPath = FPaths::Combine(ExportModRoot, FString::Printf(TEXT("%s.uplugin"), *PluginInfo.PluginName));
		FText DescriptorFailure;
		if (!ExportDescriptor.Save(ExportDescriptorPath, &DescriptorFailure))
		{
			OutFailureMessage = DescriptorFailure.IsEmpty()
				? FString::Printf(TEXT("Could not write exported descriptor '%s'."), *ExportDescriptorPath)
				: DescriptorFailure.ToString();
			return false;
		}

		if (!TryCopyCookedContainers(PluginInfo.PluginName, StagingDirectory, ExportPakDirectory, OutFailureMessage))
		{
			return false;
		}

		FString SourceAssetRegistryPath;
		if (!TryFindCookedAssetRegistry(PluginInfo.PluginBaseDir, SourceAssetRegistryPath, OutFailureMessage))
		{
			return false;
		}

		const FString ExportAssetRegistryPath = FPaths::Combine(ExportModRoot, TEXT("AssetRegistry.bin"));
		if (!TryCopyFile(SourceAssetRegistryPath, ExportAssetRegistryPath, OutFailureMessage))
		{
			return false;
		}

		const FString InstallNotes =
			TEXT("Copy the Mods folder from this export into the packaged AGASS game directory that contains Binaries, Content, and Mods.\r\n")
			TEXT("The final installed path should look like:\r\n")
			TEXT("AGASS/Mods/") + PluginInfo.PluginName + TEXT("/") + PluginInfo.PluginName + TEXT(".uplugin\r\n");
		if (!FFileHelper::SaveStringToFile(InstallNotes, *FPaths::Combine(ExportRootDirectory, TEXT("INSTALL.txt"))))
		{
			OutFailureMessage = FString::Printf(TEXT("Could not write install notes for '%s'."), *PluginInfo.PluginName);
			return false;
		}

		FileManager.DeleteDirectory(*StagingDirectory, false, true);
		OutFailureMessage.Reset();
		return true;
	}
}

class FAGASSEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAGASSEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

private:
	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);

		UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
		FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("AGASS"));
		Section.AddSubMenu(
			TEXT("AGASSMenu"),
			LOCTEXT("AGASSMenuLabel", "AGASS"),
			LOCTEXT("AGASSMenuTooltip", "AGASS editor and modding tools."),
			FNewToolMenuDelegate::CreateRaw(this, &FAGASSEditorModule::PopulateAGASSMenu),
			false,
			FSlateIcon());
	}

	void PopulateAGASSMenu(UToolMenu* Menu)
	{
		FToolMenuSection& Section = Menu->AddSection(TEXT("AGASSModding"), LOCTEXT("AGASSModdingSection", "Modding"));
		Section.AddMenuEntry(
			TEXT("AGASSExportMod"),
			LOCTEXT("ExportModLabel", "Export Mod..."),
			LOCTEXT("ExportModTooltip", "Package a content-only project plugin as a runtime AGASS mod."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FAGASSEditorModule::HandleExportMod)));

		Section.AddMenuEntry(
			TEXT("AGASSOpenExportSettings"),
			LOCTEXT("OpenExportSettingsLabel", "Open Export Settings"),
			LOCTEXT("OpenExportSettingsTooltip", "Open the AGASS mod export settings."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FAGASSEditorModule::HandleOpenExportSettings)));
	}

	void HandleOpenExportSettings() const
	{
		const UAGASSModExportDeveloperSettings* Settings = UAGASSModExportDeveloperSettings::Get();
		FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings")).ShowViewer(
			Settings->GetContainerName(),
			Settings->GetCategoryName(),
			Settings->GetSectionName());
	}

	void HandleExportMod() const
	{
		using namespace AGASSEditor::Private;

		FString DescriptorPath;
		FString FailureMessage;
		if (!TryPromptForPluginDescriptor(DescriptorPath, FailureMessage))
		{
			if (!FailureMessage.IsEmpty())
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FailureMessage));
			}
			return;
		}

		FValidatedPluginInfo PluginInfo;
		if (!TryValidatePlugin(DescriptorPath, PluginInfo, FailureMessage))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FailureMessage));
			return;
		}

		const UAGASSModExportDeveloperSettings* Settings = UAGASSModExportDeveloperSettings::Get();
		FResolvedReleaseInfo ReleaseInfo;
		if (!TryResolveReleaseInfo(*Settings, ReleaseInfo, FailureMessage))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FailureMessage));
			return;
		}

		if (!EnsureProjectNamedReceipts(FailureMessage))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FailureMessage));
			return;
		}

		const FString ExportOutputRoot = ResolveDirectoryPath(Settings->ExportOutputDirectory);
		const FString StagingRoot = ResolveDirectoryPath(Settings->TemporaryStagingDirectory);
		if (ExportOutputRoot.IsEmpty() || StagingRoot.IsEmpty())
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MissingExportDirectories", "Export output and staging directories must be configured before exporting a mod."));
			return;
		}

		IFileManager::Get().MakeDirectory(*ExportOutputRoot, true);
		IFileManager::Get().MakeDirectory(*StagingRoot, true);

		const FString ExportRootDirectory = FPaths::Combine(ExportOutputRoot, PluginInfo.PluginName);
		const FString StagingDirectory = FPaths::Combine(StagingRoot, PluginInfo.PluginName);
		IFileManager::Get().DeleteDirectory(*StagingDirectory, false, true);
		IFileManager::Get().MakeDirectory(*StagingDirectory, true);

		const FString ProjectFilePath = NormalizeFilename(FPaths::GetProjectFilePath());
		const FString ReleaseRootArgument = ReleaseInfo.CommandLineRootDirectory.IsEmpty()
			? FString()
			: FString::Printf(TEXT(" -basedonreleaseversionroot=\"%s\""), *ReleaseInfo.CommandLineRootDirectory);
		const FString UatCommand = FString::Printf(
			TEXT("-ScriptsForProject=\"%s\" BuildCookRun -project=\"%s\" -noP4 -clientconfig=Development -serverconfig=Development -platform=Win64 -cook -stage -pak -dlcname=%s -basedonreleaseversion=%s%s -DLCIncludeEngineContent -stagingdirectory=\"%s\" -utf8output -WaitForUATMutex -nocompile -nocompileeditor -unversionedcookedcontent -iostore -compressed -installed"),
			*ProjectFilePath,
			*ProjectFilePath,
			*PluginInfo.PluginName,
			*ReleaseInfo.Version,
			*ReleaseRootArgument,
			*StagingDirectory);

		IUATHelperModule::Get().CreateUatTask(
			UatCommand,
			LOCTEXT("ExportModPlatform", "Windows"),
			LOCTEXT("ExportModTaskName", "Export Mod"),
			FText::Format(LOCTEXT("ExportModTaskShortName", "Export {0}"), FText::FromString(PluginInfo.PluginName)),
			nullptr,
			nullptr,
			[PluginInfo, StagingDirectory, ExportRootDirectory](FString Result, double)
			{
				if (!Result.Equals(TEXT("Completed"), ESearchCase::IgnoreCase))
				{
					const FText FailureText = FText::Format(
						LOCTEXT("ExportFailedText", "Export Mod failed for '{0}'. Check the UAT log for details."),
						FText::FromString(PluginInfo.PluginName));
					FMessageDialog::Open(EAppMsgType::Ok, FailureText);
					return;
				}

				FString FinalizeFailure;
				if (!AGASSEditor::Private::FinalizeExport(PluginInfo, StagingDirectory, ExportRootDirectory, FinalizeFailure))
				{
					const FText FailureText = FText::Format(
						LOCTEXT("FinalizeExportFailedText", "The mod cook finished, but final export assembly failed.\n\n{0}"),
						FText::FromString(FinalizeFailure));
					FMessageDialog::Open(EAppMsgType::Ok, FailureText);
					return;
				}

				const FText SuccessText = FText::Format(
					LOCTEXT("ExportSucceededText", "Exported '{0}' to:\n{1}"),
					FText::FromString(PluginInfo.PluginName),
					FText::FromString(ExportRootDirectory));
				FMessageDialog::Open(EAppMsgType::Ok, SuccessText);
			},
			ExportRootDirectory);
	}
};

IMPLEMENT_MODULE(FAGASSEditorModule, AGASSEditor)

#undef LOCTEXT_NAMESPACE
