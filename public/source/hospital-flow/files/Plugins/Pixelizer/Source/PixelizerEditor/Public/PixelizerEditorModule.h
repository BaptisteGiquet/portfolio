#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Materials/MaterialParameters.h"
#include "UObject/StrongObjectPtr.h"

class AActor;
class IDetailsView;
class SDockTab;
class UMaterialInterface;
class UStaticMeshComponent;
class UTexture2D;
class UPixelizerGeneratedRegistryAsset;
class UPixelizerToolSession;

struct FPixelizerMaterialSlotAnalysis
{
	int32 SlotIndex = INDEX_NONE;
	FString SlotName;
	TWeakObjectPtr<UMaterialInterface> AssignedMaterial;
	TWeakObjectPtr<UTexture2D> BaseColorTexture;
	FName BaseColorParameterName = NAME_None;
	FMaterialParameterInfo BaseColorParameterInfo;
	bool bSupported = false;
	FString Status;
	TArray<FString> Notes;
};

class FPixelizerEditorModule : public IModuleInterface
{
public:
	static FPixelizerEditorModule& Get();
	static bool IsAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	UPixelizerToolSession* GetSession() const { return Session; }

	void RefreshSelectionContext(bool bForce = false);
	void ApplyBuiltInPaletteToSession();
	void LoadPaletteAssetIntoSession();
	void LoadPresetAssetIntoSession();
	bool SavePaletteAssetFromSession();
	bool SavePresetAssetFromSession();
	bool PreviewCurrentSelection();
	bool ApplyCurrentSelection();
	bool RevertCurrentSelection();
	void DryRunSelectedActors();
	void BatchApplySelectedActors();
	void PollAutoSelection();

	FText GetSelectionSummaryText() const;
	FText GetMaterialReportText() const;
	FText GetWarningReportText() const;
	FText GetStatusText() const;

private:
	TSharedRef<SDockTab> SpawnTab(const class FSpawnTabArgs& SpawnTabArgs);
	void RegisterMenus();
	void UnregisterMenus();
	void InvalidatePanel();

	UPixelizerToolSession* EnsureSession();
	UPixelizerGeneratedRegistryAsset* EnsureRegistryAsset();

private:
	static const FName PixelizerTabName;

public:
	// Exposed for internal helper functions in the module implementation file.
	UPixelizerToolSession* Session = nullptr;
	UPixelizerGeneratedRegistryAsset* RegistryAsset = nullptr;

	TWeakObjectPtr<AActor> CurrentActor;
	TWeakObjectPtr<UStaticMeshComponent> CurrentComponent;
	TArray<TWeakObjectPtr<UMaterialInterface>> CurrentOriginalMaterialsBySlot;
	TArray<FPixelizerMaterialSlotAnalysis> CurrentSlotAnalyses;
	FString CurrentSelectionKey;
	bool bPreviewActive = false;

	FString SelectionSummary;
	FString MaterialReport;
	FString WarningReport;
	FString StatusReport;

	TArray<TStrongObjectPtr<UObject>> PreviewKeepAliveObjects;
	TSharedPtr<IDetailsView> DetailsView;
};
