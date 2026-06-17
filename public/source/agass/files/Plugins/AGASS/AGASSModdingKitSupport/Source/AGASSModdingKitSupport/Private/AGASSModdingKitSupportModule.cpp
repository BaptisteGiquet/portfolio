#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSModdingKitSupport, Log, All);

namespace AGASSModdingKitSupport
{
	FString GetProjectName()
	{
		return FPaths::GetBaseFilename(FPaths::GetProjectFilePath());
	}

	FString GetProjectFileName()
	{
		return FPaths::GetCleanFilename(FPaths::GetProjectFilePath());
	}

	FString GetProjectRoot()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
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

			TSharedPtr<FJsonObject> Object = Value->AsObject();
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

	void EnsureProjectNamedReceipt(
		const FString& SourceReceiptName,
		const FString& TargetReceiptName,
		const FString& TargetName)
	{
		const FString BinariesDir = FPaths::Combine(GetProjectRoot(), TEXT("Binaries"), TEXT("Win64"));
		const FString SourceReceiptPath = FPaths::Combine(BinariesDir, SourceReceiptName);
		const FString TargetReceiptPath = FPaths::Combine(BinariesDir, TargetReceiptName);

		TSharedPtr<FJsonObject> JsonObject;
		if (!LoadJsonObject(SourceReceiptPath, JsonObject))
		{
			UE_LOG(LogAGASSModdingKitSupport, Warning, TEXT("Failed to read source receipt %s"), *SourceReceiptPath);
			return;
		}

		const FString ProjectFileName = GetProjectFileName();
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

		if (SaveJsonObject(TargetReceiptPath, JsonObject.ToSharedRef()))
		{
			UE_LOG(LogAGASSModdingKitSupport, Verbose, TEXT("Wrote project receipt %s"), *TargetReceiptPath);
		}
	}

	void EnsureNoSourceReceipts()
	{
		const FString ProjectName = GetProjectName();
		if (ProjectName.IsEmpty() || ProjectName.Equals(TEXT("AGASS"), ESearchCase::IgnoreCase))
		{
			return;
		}

		EnsureProjectNamedReceipt(TEXT("AGASSEditor.target"), FString::Printf(TEXT("%sEditor.target"), *ProjectName), FString::Printf(TEXT("%sEditor"), *ProjectName));
		EnsureProjectNamedReceipt(TEXT("AGASS.target"), FString::Printf(TEXT("%s.target"), *ProjectName), ProjectName);
	}

	void PatchExportedAssetRegistries()
	{
		const FString ProjectRoot = GetProjectRoot();
		const FString ProjectName = GetProjectName();
		const FString ModExportsRoot = FPaths::Combine(ProjectRoot, TEXT("Saved"), TEXT("ModExports"));
		const FString PluginsRoot = FPaths::Combine(ProjectRoot, TEXT("Plugins"));

		TArray<FString> ExportDirs;
		IFileManager::Get().FindFiles(ExportDirs, *(ModExportsRoot / TEXT("*")), false, true);

		for (const FString& ExportDirName : ExportDirs)
		{
			const FString ModsRoot = FPaths::Combine(ModExportsRoot, ExportDirName, TEXT("Mods"));

			TArray<FString> ModDirs;
			IFileManager::Get().FindFiles(ModDirs, *(ModsRoot / TEXT("*")), false, true);

			for (const FString& ModDirName : ModDirs)
			{
				const FString ModRoot = FPaths::Combine(ModsRoot, ModDirName);
				const FString DestAssetRegistry = FPaths::Combine(ModRoot, TEXT("AssetRegistry.bin"));

				if (!IFileManager::Get().FileExists(*FPaths::Combine(ModRoot, FString::Printf(TEXT("%s.uplugin"), *ModDirName))))
				{
					continue;
				}

				const FString SourceAssetRegistry = FPaths::Combine(
					PluginsRoot,
					ModDirName,
					TEXT("Saved"),
					TEXT("Cooked"),
					TEXT("Windows"),
					ProjectName,
					TEXT("Plugins"),
					ModDirName,
					TEXT("AssetRegistry.bin"));

				if (!IFileManager::Get().FileExists(*SourceAssetRegistry))
				{
					continue;
				}

				if (IFileManager::Get().FileExists(*DestAssetRegistry))
				{
					const FDateTime DestTime = IFileManager::Get().GetTimeStamp(*DestAssetRegistry);
					const FDateTime SourceTime = IFileManager::Get().GetTimeStamp(*SourceAssetRegistry);
					if (DestTime >= SourceTime)
					{
						continue;
					}
				}

				if (IFileManager::Get().Copy(*DestAssetRegistry, *SourceAssetRegistry, true, true) == COPY_OK)
				{
					UE_LOG(LogAGASSModdingKitSupport, Log, TEXT("Copied AssetRegistry.bin into exported mod %s"), *ModDirName);
				}
			}
		}
	}

	void EnsureReleaseMetadataAssetRegistries()
	{
		const FString ReleasesRoot = FPaths::Combine(GetProjectRoot(), TEXT("Releases"));
		if (!IFileManager::Get().DirectoryExists(*ReleasesRoot))
		{
			return;
		}

		TArray<FString> ReleaseVersions;
		IFileManager::Get().FindFiles(ReleaseVersions, *(ReleasesRoot / TEXT("*")), false, true);

		for (const FString& ReleaseVersion : ReleaseVersions)
		{
			const FString ReleaseVersionRoot = FPaths::Combine(ReleasesRoot, ReleaseVersion);
			TArray<FString> PlatformDirectories;
			IFileManager::Get().FindFiles(PlatformDirectories, *(ReleaseVersionRoot / TEXT("*")), false, true);

			for (const FString& PlatformDirectory : PlatformDirectories)
			{
				const FString PlatformRoot = FPaths::Combine(ReleaseVersionRoot, PlatformDirectory);
				const FString SourceAssetRegistry = FPaths::Combine(PlatformRoot, TEXT("AssetRegistry.bin"));
				if (!IFileManager::Get().FileExists(*SourceAssetRegistry))
				{
					continue;
				}

				const FString MetadataRoot = FPaths::Combine(PlatformRoot, TEXT("Metadata"));
				const FString MetadataAssetRegistry = FPaths::Combine(MetadataRoot, TEXT("AssetRegistry.bin"));
				IFileManager::Get().MakeDirectory(*MetadataRoot, true);

				if (IFileManager::Get().FileExists(*MetadataAssetRegistry))
				{
					const FDateTime DestTime = IFileManager::Get().GetTimeStamp(*MetadataAssetRegistry);
					const FDateTime SourceTime = IFileManager::Get().GetTimeStamp(*SourceAssetRegistry);
					if (DestTime >= SourceTime)
					{
						continue;
					}
				}

				if (IFileManager::Get().Copy(*MetadataAssetRegistry, *SourceAssetRegistry, true, true) == COPY_OK)
				{
					UE_LOG(LogAGASSModdingKitSupport, Verbose, TEXT("Ensured release metadata asset registry for %s/%s"), *ReleaseVersion, *PlatformDirectory);
				}
			}
		}
	}
}

class FAGASSModdingKitSupportModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		AGASSModdingKitSupport::EnsureNoSourceReceipts();
		AGASSModdingKitSupport::PatchExportedAssetRegistries();
		AGASSModdingKitSupport::EnsureReleaseMetadataAssetRegistries();
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FAGASSModdingKitSupportModule::HandleTicker),
			2.0f);
	}

	virtual void ShutdownModule() override
	{
		if (TickHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
			TickHandle.Reset();
		}
	}

private:
	bool HandleTicker(float)
	{
		AGASSModdingKitSupport::EnsureNoSourceReceipts();
		AGASSModdingKitSupport::PatchExportedAssetRegistries();
		AGASSModdingKitSupport::EnsureReleaseMetadataAssetRegistries();
		return true;
	}

private:
	FTSTicker::FDelegateHandle TickHandle;
};

IMPLEMENT_MODULE(FAGASSModdingKitSupportModule, AGASSModdingKitSupport)
