#include "PixelizerEditorModule.h"

#include "PixelizerEditorProcessing.h"
#include "PixelizerToolSession.h"

#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Framework/Docking/TabManager.h"
#include "IDetailsView.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PixelizerEditor"

const FName FPixelizerEditorModule::PixelizerTabName(TEXT("PixelizerEditorTab"));

namespace
{
	static void SnapshotOriginalMaterialsForSelection(FPixelizerEditorModule& Module, const PixelizerEditorProcessing::FResolvedSelection& Selection)
	{
		if (!Selection.Actor || !Selection.Component)
		{
			Module.CurrentOriginalMaterialsBySlot.Reset();
			return;
		}

		const int32 NumMaterials = Selection.Component->GetNumMaterials();
		Module.CurrentOriginalMaterialsBySlot.Reset();
		Module.CurrentOriginalMaterialsBySlot.SetNum(NumMaterials);

		bool bLoadedFromRegistry = false;
		if (!Module.RegistryAsset)
		{
			const FString RegistryObjectPath = FString::Printf(
				TEXT("%s/%s.%s"),
				PixelizerEditorProcessing::RegistryFolder,
				PixelizerEditorProcessing::RegistryAssetName,
				PixelizerEditorProcessing::RegistryAssetName);
			if (UPixelizerGeneratedRegistryAsset* ExistingRegistry = LoadObject<UPixelizerGeneratedRegistryAsset>(nullptr, *RegistryObjectPath))
			{
				Module.RegistryAsset = ExistingRegistry;
			}
		}

		if (Module.RegistryAsset)
		{
			if (FPixelizerActorAssignmentRecord* Record = PixelizerEditorProcessing::FindActorRecord(Module.RegistryAsset, FSoftObjectPath(Selection.Actor)))
			{
				if (Record->OriginalMaterialsBySlot.Num() > 0)
				{
					for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
					{
						UMaterialInterface* FallbackCurrent = Selection.Component->GetMaterial(SlotIndex);
						UMaterialInterface* LoadedOriginal = nullptr;
						if (Record->OriginalMaterialsBySlot.IsValidIndex(SlotIndex))
						{
							LoadedOriginal = Cast<UMaterialInterface>(Record->OriginalMaterialsBySlot[SlotIndex].TryLoad());
						}
						Module.CurrentOriginalMaterialsBySlot[SlotIndex] = LoadedOriginal ? LoadedOriginal : FallbackCurrent;
					}
					bLoadedFromRegistry = true;
				}
			}
		}

		if (!bLoadedFromRegistry)
		{
			for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
			{
				Module.CurrentOriginalMaterialsBySlot[SlotIndex] = Selection.Component->GetMaterial(SlotIndex);
			}
		}
	}

	void BuildSelectionReports(FPixelizerEditorModule& Module, const PixelizerEditorProcessing::FResolvedSelection& Selection, const FString& SelectionWarnings)
	{
		Module.WarningReport = SelectionWarnings;
		Module.MaterialReport.Empty();
		Module.SelectionSummary.Empty();
		Module.CurrentSlotAnalyses.Reset();

		if (!Selection.Actor || !Selection.Component)
		{
			Module.SelectionSummary = TEXT("No supported StaticMeshActor/UStaticMeshComponent selected.");
			return;
		}

		Module.CurrentActor = Selection.Actor;
		Module.CurrentComponent = Selection.Component;
		Module.CurrentSelectionKey = Selection.Key;

		if (!Module.bPreviewActive)
		{
			SnapshotOriginalMaterialsForSelection(Module, Selection);
		}

		Module.SelectionSummary = FString::Printf(
			TEXT("Actor: %s\nComponent: %s\nMesh: %s"),
			*Selection.Actor->GetPathName(),
			*Selection.Component->GetPathName(),
			Selection.Component->GetStaticMesh() ? *Selection.Component->GetStaticMesh()->GetPathName() : TEXT("<None>"));

		PixelizerEditorProcessing::AnalyzeComponentMaterials(
			Selection.Component,
			Module.CurrentSlotAnalyses,
			Module.MaterialReport,
			Module.WarningReport,
			&Module.CurrentOriginalMaterialsBySlot);
		if (Module.WarningReport.IsEmpty())
		{
			Module.WarningReport = TEXT("No warnings.");
		}
	}

	bool BuildPreviewForComponent(
		FPixelizerEditorModule& Module,
		AActor* Actor,
		UStaticMeshComponent* Component,
		const TArray<FPixelizerMaterialSlotAnalysis>& Analyses,
		const TArray<TWeakObjectPtr<UMaterialInterface>>& OriginalMaterialsBySlot,
		const FPixelizerEffectSettings& Settings,
		const FPixelizerPaletteData& Palette,
		const FPixelizerOutlineSettings& Outline,
		FString& InOutWarnings,
		FString& OutStatus)
	{
		if (!Actor || !Component)
		{
			InOutWarnings += TEXT("No selected component for preview.\n");
			return false;
		}

		Module.PreviewKeepAliveObjects.Reset();
		PixelizerEditorProcessing::AssignMaterialsToComponent(Component, OriginalMaterialsBySlot);

		bool bAnyPreviewed = false;
		for (const FPixelizerMaterialSlotAnalysis& Analysis : Analyses)
		{
			if (!Analysis.bSupported)
			{
				continue;
			}

			UMaterialInterface* SourceMaterial = OriginalMaterialsBySlot.IsValidIndex(Analysis.SlotIndex) ? OriginalMaterialsBySlot[Analysis.SlotIndex].Get() : Analysis.AssignedMaterial.Get();
			UTexture2D* SourceTexture = Analysis.BaseColorTexture.Get();
			if (!SourceMaterial || !SourceTexture)
			{
				InOutWarnings += FString::Printf(TEXT("Slot %d missing source material/texture for preview.\n"), Analysis.SlotIndex);
				continue;
			}

			PixelizerEditorProcessing::FTextureProcessOutput ProcessOutput;
			if (!PixelizerEditorProcessing::ReadAndProcessTextureSource(SourceTexture, Settings, Palette.Colors, Outline, ProcessOutput))
			{
				InOutWarnings += FString::Printf(TEXT("Slot %d preview skipped: %s\n"), Analysis.SlotIndex, *ProcessOutput.Error);
				continue;
			}

			const FName UniqueTexName = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), TEXT("PixelizerPreviewTex"));
			UTexture2D* PreviewTexture = DuplicateObject<UTexture2D>(SourceTexture, GetTransientPackage(), UniqueTexName);
			if (!PreviewTexture || !PixelizerEditorProcessing::WriteTextureSource(PreviewTexture, ProcessOutput))
			{
				InOutWarnings += FString::Printf(TEXT("Slot %d preview texture creation failed.\n"), Analysis.SlotIndex);
				continue;
			}
			PreviewTexture->ClearFlags(RF_Public | RF_Standalone);

			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(SourceMaterial, Component);
			if (!MID)
			{
				InOutWarnings += FString::Printf(TEXT("Slot %d preview MID creation failed.\n"), Analysis.SlotIndex);
				continue;
			}
			MID->SetTextureParameterValue(Analysis.BaseColorParameterName, PreviewTexture);
			Component->SetMaterial(Analysis.SlotIndex, MID);

			Module.PreviewKeepAliveObjects.Add(TStrongObjectPtr<UObject>(PreviewTexture));
			Module.PreviewKeepAliveObjects.Add(TStrongObjectPtr<UObject>(MID));
			bAnyPreviewed = true;
		}

		Module.bPreviewActive = bAnyPreviewed;
		OutStatus = bAnyPreviewed
			? FString::Printf(TEXT("Preview applied to %s (transient MIDs/textures, not saved)."), *Actor->GetName())
			: TEXT("Preview: no supported slots were processed.");
		return bAnyPreviewed;
	}
}

class SPixelizerEditorPanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPixelizerEditorPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&, FPixelizerEditorModule* InModule, const TSharedRef<IDetailsView>& InDetailsView)
	{
		Module = InModule;
		DetailsView = InDetailsView;

		ChildSlot
		[
			SNew(SBorder)
			.Padding(8.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return Module ? Module->GetStatusText() : FText::GetEmpty(); })
					.AutoWrapText(true)
				]
				+ SScrollBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("SelectionTitle", "Selection / Actor Info"))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return Module ? Module->GetSelectionSummaryText() : FText::GetEmpty(); })
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(SButton)
								.Text(LOCTEXT("RefreshSelectionBtn", "Refresh Selection"))
								.OnClicked_Lambda([this]() { if (Module) Module->RefreshSelectionContext(true); return FReply::Handled(); })
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SButton)
								.Text(LOCTEXT("ApplyBuiltInPaletteBtn", "Apply Built-In Palette"))
								.OnClicked_Lambda([this]() { if (Module) Module->ApplyBuiltInPaletteToSession(); return FReply::Handled(); })
							]
						]
					]
				]
				+ SScrollBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("SettingsTitle", "Pixelation Settings / Palette / Presets"))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[DetailsView.ToSharedRef()]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(SButton).Text(LOCTEXT("LoadPaletteBtn", "Load Palette Asset"))
								.OnClicked_Lambda([this]() { if (Module) Module->LoadPaletteAssetIntoSession(); return FReply::Handled(); })
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(SButton).Text(LOCTEXT("SavePaletteBtn", "Save Palette Asset"))
								.OnClicked_Lambda([this]() { if (Module) Module->SavePaletteAssetFromSession(); return FReply::Handled(); })
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(SButton).Text(LOCTEXT("LoadPresetBtn", "Load Preset Asset"))
								.OnClicked_Lambda([this]() { if (Module) Module->LoadPresetAssetIntoSession(); return FReply::Handled(); })
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SButton).Text(LOCTEXT("SavePresetBtn", "Save Preset Asset"))
								.OnClicked_Lambda([this]() { if (Module) Module->SavePresetAssetFromSession(); return FReply::Handled(); })
							]
						]
					]
				]
				+ SScrollBox::Slot()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("ApplyTitle", "Preview / Apply / Revert / Batch"))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(SButton).Text(LOCTEXT("PreviewBtn", "Preview (Transient)"))
								.OnClicked_Lambda([this]() { if (Module) Module->PreviewCurrentSelection(); return FReply::Handled(); })
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(SButton).Text(LOCTEXT("ApplyBtn", "Apply"))
								.OnClicked_Lambda([this]() { if (Module) Module->ApplyCurrentSelection(); return FReply::Handled(); })
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SButton).Text(LOCTEXT("RevertBtn", "Revert Selected"))
								.OnClicked_Lambda([this]() { if (Module) Module->RevertCurrentSelection(); return FReply::Handled(); })
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
							[
								SNew(SButton).Text(LOCTEXT("DryRunBtn", "Dry Run Selected Actors"))
								.OnClicked_Lambda([this]() { if (Module) Module->DryRunSelectedActors(); return FReply::Handled(); })
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SButton).Text(LOCTEXT("BatchApplyBtn", "Batch Apply Selected"))
								.OnClicked_Lambda([this]() { if (Module) Module->BatchApplySelectedActors(); return FReply::Handled(); })
							]
						]
					]
				]
				+ SScrollBox::Slot().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("MaterialReportTitle", "Material Slots / Texture Targets"))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SMultiLineEditableTextBox)
						.IsReadOnly(true)
						.AutoWrapText(true)
						.Text_Lambda([this]() { return Module ? Module->GetMaterialReportText() : FText::GetEmpty(); })
					]
				]
				+ SScrollBox::Slot()
				[
					SNew(SExpandableArea)
					.AreaTitle(LOCTEXT("WarningsTitle", "Safety / Warnings / Dry-Run Results"))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SMultiLineEditableTextBox)
						.IsReadOnly(true)
						.AutoWrapText(true)
						.Text_Lambda([this]() { return Module ? Module->GetWarningReportText() : FText::GetEmpty(); })
					]
				]
			]
		];
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		if (Module)
		{
			Module->PollAutoSelection();
		}
	}

private:
	FPixelizerEditorModule* Module = nullptr;
	TSharedPtr<IDetailsView> DetailsView;
};

FPixelizerEditorModule& FPixelizerEditorModule::Get()
{
	return FModuleManager::LoadModuleChecked<FPixelizerEditorModule>("PixelizerEditor");
}

bool FPixelizerEditorModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("PixelizerEditor");
}

void FPixelizerEditorModule::StartupModule()
{
	SelectionSummary = TEXT("Select a StaticMeshActor to begin.");
	MaterialReport = TEXT("No selection yet.");
	WarningReport = TEXT("No warnings.");
	StatusReport = TEXT("Pixelizer loaded. Phase 1 supports BaseColor-like texture parameters on material instances/materials with exposed texture parameters.");

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PixelizerTabName, FOnSpawnTab::CreateRaw(this, &FPixelizerEditorModule::SpawnTab))
		.SetDisplayName(LOCTEXT("PixelizerTabDisplayName", "Pixelizer"))
		.SetTooltipText(LOCTEXT("PixelizerTabTooltip", "Non-destructive in-editor texture pixelization for selected static mesh actors"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);

	RefreshSelectionContext(true);
}

void FPixelizerEditorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PixelizerTabName);

	const bool bEngineExiting = IsEngineExitRequested();

	// Avoid touching UObject references during engine teardown; teardown order can invalidate
	// object handles before module shutdown runs.
	if (!bEngineExiting)
	{
		if (Session)
		{
			Session->RemoveFromRoot();
		}

		PreviewKeepAliveObjects.Reset();
	}

	Session = nullptr;
	RegistryAsset = nullptr;
	DetailsView.Reset();
}

TSharedRef<SDockTab> FPixelizerEditorModule::SpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	EnsureSession();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bAllowSearch = true;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	Args.bShowOptions = true;
	Args.bLockable = false;
	Args.bUpdatesFromSelection = false;

	DetailsView = PropertyEditorModule.CreateDetailView(Args);
	DetailsView->SetObject(Session);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("PixelizerTabLabel", "Pixelizer"))
		[
			SNew(SPixelizerEditorPanel, this, DetailsView.ToSharedRef())
		];
}

void FPixelizerEditorModule::RegisterMenus() {}
void FPixelizerEditorModule::UnregisterMenus() {}

void FPixelizerEditorModule::InvalidatePanel()
{
	if (DetailsView.IsValid())
	{
		DetailsView->ForceRefresh();
	}
}

UPixelizerToolSession* FPixelizerEditorModule::EnsureSession()
{
	if (!Session)
	{
		Session = NewObject<UPixelizerToolSession>(GetTransientPackage(), TEXT("PixelizerToolSession"), RF_Transactional);
		Session->AddToRoot();
	}
	return Session;
}

UPixelizerGeneratedRegistryAsset* FPixelizerEditorModule::EnsureRegistryAsset()
{
	if (!RegistryAsset)
	{
		RegistryAsset = PixelizerEditorProcessing::LoadOrCreateDataAsset<UPixelizerGeneratedRegistryAsset>(PixelizerEditorProcessing::RegistryFolder, PixelizerEditorProcessing::RegistryAssetName);
	}
	return RegistryAsset;
}

void FPixelizerEditorModule::RefreshSelectionContext(const bool bForce)
{
	FString SelectionWarnings;
	PixelizerEditorProcessing::FResolvedSelection Resolved;
	PixelizerEditorProcessing::ResolveSelectedStaticMeshComponent(Resolved, &SelectionWarnings);

	if (!bForce && Resolved.Key == CurrentSelectionKey && CurrentComponent.IsValid())
	{
		return;
	}

	if (bPreviewActive && bForce && CurrentComponent.IsValid() && Resolved.Component && Resolved.Key == CurrentSelectionKey)
	{
		PixelizerEditorProcessing::AssignMaterialsToComponent(CurrentComponent.Get(), CurrentOriginalMaterialsBySlot);
		PreviewKeepAliveObjects.Reset();
		bPreviewActive = false;
	}

	if (bPreviewActive && (!Resolved.Component || Resolved.Key != CurrentSelectionKey))
	{
		PreviewKeepAliveObjects.Reset();
		bPreviewActive = false;
	}

	CurrentActor = nullptr;
	CurrentComponent = nullptr;
	CurrentSelectionKey = Resolved.Key;

	BuildSelectionReports(*this, Resolved, SelectionWarnings);
	InvalidatePanel();
}

void FPixelizerEditorModule::ApplyBuiltInPaletteToSession()
{
	if (UPixelizerToolSession* ToolSession = EnsureSession())
	{
		ToolSession->ApplyBuiltInPalette();
		StatusReport = FString::Printf(TEXT("Applied built-in palette '%s' to working settings."), *ToolSession->WorkingPaletteName);
		InvalidatePanel();
	}
}

void FPixelizerEditorModule::LoadPaletteAssetIntoSession()
{
	if (UPixelizerToolSession* ToolSession = EnsureSession())
	{
		ToolSession->LoadFromPaletteAsset();
		StatusReport = ToolSession->SelectedPaletteAsset
			? FString::Printf(TEXT("Loaded palette asset '%s'."), *ToolSession->SelectedPaletteAsset->GetPathName())
			: TEXT("No palette asset selected.");
		InvalidatePanel();
	}
}

void FPixelizerEditorModule::LoadPresetAssetIntoSession()
{
	if (UPixelizerToolSession* ToolSession = EnsureSession())
	{
		ToolSession->LoadFromPresetAsset();
		StatusReport = ToolSession->SelectedPresetAsset
			? FString::Printf(TEXT("Loaded preset asset '%s'."), *ToolSession->SelectedPresetAsset->GetPathName())
			: TEXT("No preset asset selected.");
		InvalidatePanel();
	}
}

bool FPixelizerEditorModule::SavePaletteAssetFromSession()
{
	UPixelizerToolSession* ToolSession = EnsureSession();
	if (!ToolSession)
	{
		return false;
	}

	UPixelizerPaletteAsset* Asset = PixelizerEditorProcessing::LoadOrCreateDataAsset<UPixelizerPaletteAsset>(ToolSession->PaletteAssetFolder, ToolSession->PaletteAssetName);
	if (!Asset)
	{
		StatusReport = TEXT("Failed to create/load palette asset. Check folder/name path.");
		return false;
	}

	ToolSession->WriteToPaletteAsset(Asset);
	ToolSession->SelectedPaletteAsset = Asset;
	Asset->MarkPackageDirty();
	if (UPackage* Package = Asset->GetOutermost())
	{
		Package->MarkPackageDirty();
	}

	StatusReport = FString::Printf(TEXT("Palette asset updated: %s (save packages to persist on disk)."), *Asset->GetPathName());
	InvalidatePanel();
	return true;
}

bool FPixelizerEditorModule::SavePresetAssetFromSession()
{
	UPixelizerToolSession* ToolSession = EnsureSession();
	if (!ToolSession)
	{
		return false;
	}

	UPixelizerEffectPresetAsset* Asset = PixelizerEditorProcessing::LoadOrCreateDataAsset<UPixelizerEffectPresetAsset>(ToolSession->PresetAssetFolder, ToolSession->PresetAssetName);
	if (!Asset)
	{
		StatusReport = TEXT("Failed to create/load preset asset. Check folder/name path.");
		return false;
	}

	ToolSession->WriteToPresetAsset(Asset);
	ToolSession->SelectedPresetAsset = Asset;
	Asset->MarkPackageDirty();
	if (UPackage* Package = Asset->GetOutermost())
	{
		Package->MarkPackageDirty();
	}

	StatusReport = FString::Printf(TEXT("Preset asset updated: %s (save packages to persist on disk)."), *Asset->GetPathName());
	InvalidatePanel();
	return true;
}

bool FPixelizerEditorModule::PreviewCurrentSelection()
{
	EnsureSession();
	RefreshSelectionContext(true);
	if (!CurrentActor.IsValid() || !CurrentComponent.IsValid())
	{
		StatusReport = TEXT("Preview failed: no supported selection.");
		return false;
	}

	FString PreviewWarnings = WarningReport.Equals(TEXT("No warnings.")) ? FString() : WarningReport + TEXT("\n");
	FString PreviewStatus;
	const FPixelizerPaletteData Palette = Session->MakePaletteData();
	BuildPreviewForComponent(*this, CurrentActor.Get(), CurrentComponent.Get(), CurrentSlotAnalyses, CurrentOriginalMaterialsBySlot, Session->EffectSettings, Palette, Session->OutlineSettings, PreviewWarnings, PreviewStatus);

	StatusReport = PreviewStatus;
	WarningReport = PreviewWarnings.IsEmpty() ? TEXT("No warnings.") : PreviewWarnings;
	InvalidatePanel();
	return bPreviewActive;
}

bool FPixelizerEditorModule::ApplyCurrentSelection()
{
	EnsureSession();
	RefreshSelectionContext(true);
	if (!CurrentActor.IsValid() || !CurrentComponent.IsValid())
	{
		StatusReport = TEXT("Apply failed: no supported selection.");
		return false;
	}

	if (bPreviewActive)
	{
		PixelizerEditorProcessing::AssignMaterialsToComponent(CurrentComponent.Get(), CurrentOriginalMaterialsBySlot);
		PreviewKeepAliveObjects.Reset();
		bPreviewActive = false;
	}

	UPixelizerGeneratedRegistryAsset* Registry = EnsureRegistryAsset();
	if (!Registry)
	{
		StatusReport = TEXT("Apply failed: could not create/load Pixelizer registry asset.");
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("PixelizerApplyTransaction", "Apply Pixelizer Effect"));
	CurrentActor->Modify();
	CurrentComponent->Modify();

	const FPixelizerPaletteData Palette = Session->MakePaletteData();
	const FString PresetHash = PixelizerEditorProcessing::BuildEffectHash(Session->EffectSettings, Palette, Session->OutlineSettings);
	PixelizerEditorProcessing::FApplyComponentResult Result;
	PixelizerEditorProcessing::ApplyToComponentWithAssets(
		Registry,
		CurrentActor.Get(),
		CurrentComponent.Get(),
		CurrentSlotAnalyses,
		CurrentOriginalMaterialsBySlot,
		Session->EffectSettings,
		Palette,
		Session->OutlineSettings,
		PresetHash,
		Result);

	StatusReport = Result.Status;
	WarningReport = Result.Warnings.IsEmpty() ? TEXT("No warnings.") : Result.Warnings;
	InvalidatePanel();
	return Result.bAnyApplied;
}

bool FPixelizerEditorModule::RevertCurrentSelection()
{
	EnsureSession();
	RefreshSelectionContext(true);

	if (!CurrentActor.IsValid() || !CurrentComponent.IsValid())
	{
		StatusReport = TEXT("Revert failed: no supported selection.");
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("PixelizerRevertTransaction", "Revert Pixelizer Effect"));
	CurrentActor->Modify();
	CurrentComponent->Modify();

	TArray<TWeakObjectPtr<UMaterialInterface>> MaterialsToRestore = CurrentOriginalMaterialsBySlot;
	if (UPixelizerGeneratedRegistryAsset* Registry = EnsureRegistryAsset())
	{
		if (FPixelizerActorAssignmentRecord* Record = PixelizerEditorProcessing::FindActorRecord(Registry, FSoftObjectPath(CurrentActor.Get())))
		{
			MaterialsToRestore.SetNum(Record->OriginalMaterialsBySlot.Num());
			for (int32 Index = 0; Index < Record->OriginalMaterialsBySlot.Num(); ++Index)
			{
				MaterialsToRestore[Index] = Cast<UMaterialInterface>(Record->OriginalMaterialsBySlot[Index].TryLoad());
			}
		}
	}

	if (MaterialsToRestore.Num() == 0)
	{
		StatusReport = TEXT("Revert skipped: no stored original materials found for this actor.");
		return false;
	}

	PixelizerEditorProcessing::AssignMaterialsToComponent(CurrentComponent.Get(), MaterialsToRestore);
	PreviewKeepAliveObjects.Reset();
	bPreviewActive = false;
	StatusReport = FString::Printf(TEXT("Reverted material overrides on %s."), *CurrentActor->GetName());
	WarningReport = TEXT("No warnings.");
	InvalidatePanel();
	return true;
}

void FPixelizerEditorModule::DryRunSelectedActors()
{
	EnsureSession();
	if (!GEditor)
	{
		return;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		StatusReport = TEXT("Dry run: no selected actors.");
		return;
	}

	FString Report;
	int32 ActorCount = 0;
	int32 SupportedActorCount = 0;
	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		++ActorCount;
		AActor* Actor = Cast<AActor>(*It);
		UStaticMeshComponent* Component = nullptr;
		if (!PixelizerEditorProcessing::TryResolveActorToComponent(Actor, Component))
		{
			Report += FString::Printf(TEXT("%s: skipped (no static mesh component)\n"), Actor ? *Actor->GetName() : TEXT("<Invalid>"));
			continue;
		}

		TArray<FPixelizerMaterialSlotAnalysis> Analyses;
		FString DummyMaterialReport;
		FString ActorWarnings;
		PixelizerEditorProcessing::AnalyzeComponentMaterials(Component, Analyses, DummyMaterialReport, ActorWarnings);
		int32 SupportedSlots = 0;
		for (const FPixelizerMaterialSlotAnalysis& Analysis : Analyses)
		{
			SupportedSlots += Analysis.bSupported ? 1 : 0;
		}

		if (SupportedSlots > 0)
		{
			++SupportedActorCount;
		}

		Report += FString::Printf(TEXT("%s: %d supported slots / %d total slots\n"), *Actor->GetName(), SupportedSlots, Component->GetNumMaterials());
		if (!ActorWarnings.IsEmpty())
		{
			Report += ActorWarnings;
		}
		Report += TEXT("\n");
	}

	StatusReport = FString::Printf(TEXT("Dry run complete: %d/%d selected actors have at least one supported slot."), SupportedActorCount, ActorCount);
	WarningReport = Report.IsEmpty() ? TEXT("No warnings.") : Report;
	InvalidatePanel();
}

void FPixelizerEditorModule::BatchApplySelectedActors()
{
	EnsureSession();
	if (!GEditor)
	{
		return;
	}

	UPixelizerGeneratedRegistryAsset* Registry = EnsureRegistryAsset();
	if (!Registry)
	{
		StatusReport = TEXT("Batch apply failed: could not create/load Pixelizer registry asset.");
		return;
	}

	UPixelizerToolSession* ToolSession = EnsureSession();
	FPixelizerEffectSettings BatchSettings = ToolSession->EffectSettings;
	FPixelizerPaletteData BatchPalette = ToolSession->MakePaletteData();
	FPixelizerOutlineSettings BatchOutline = ToolSession->OutlineSettings;
	if (ToolSession->bBatchUseSelectedPresetAsset)
	{
		if (UPixelizerEffectPresetAsset* BatchPreset = ToolSession->GetEffectiveBatchPreset())
		{
			BatchSettings = BatchPreset->EffectSettings;
			BatchPalette = BatchPreset->Palette;
			BatchOutline = BatchPreset->OutlineSettings;
		}
	}

	if (ToolSession->bBatchDryRunBeforeApply)
	{
		DryRunSelectedActors();
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		StatusReport = TEXT("Batch apply: no selected actors.");
		return;
	}

	const FString PresetHash = PixelizerEditorProcessing::BuildEffectHash(BatchSettings, BatchPalette, BatchOutline);
	int32 ProcessedActors = 0;
	int32 ChangedActors = 0;
	FString BatchWarnings;

	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		AActor* Actor = Cast<AActor>(*It);
		UStaticMeshComponent* Component = nullptr;
		if (!PixelizerEditorProcessing::TryResolveActorToComponent(Actor, Component))
		{
			if (Actor)
			{
				BatchWarnings += FString::Printf(TEXT("%s: skipped (no static mesh component)\n"), *Actor->GetName());
			}
			continue;
		}

		++ProcessedActors;

		TArray<FPixelizerMaterialSlotAnalysis> Analyses;
		FString DummyMaterialReport;
		FString LocalWarnings;
		PixelizerEditorProcessing::AnalyzeComponentMaterials(Component, Analyses, DummyMaterialReport, LocalWarnings);

		TArray<TWeakObjectPtr<UMaterialInterface>> OriginalMaterials;
		OriginalMaterials.SetNum(Component->GetNumMaterials());
		for (int32 SlotIndex = 0; SlotIndex < Component->GetNumMaterials(); ++SlotIndex)
		{
			OriginalMaterials[SlotIndex] = Component->GetMaterial(SlotIndex);
		}

		Actor->Modify();
		Component->Modify();

		PixelizerEditorProcessing::FApplyComponentResult ApplyResult;
		PixelizerEditorProcessing::ApplyToComponentWithAssets(
			Registry,
			Actor,
			Component,
			Analyses,
			OriginalMaterials,
			BatchSettings,
			BatchPalette,
			BatchOutline,
			PresetHash,
			ApplyResult);

		if (ApplyResult.bAnyApplied)
		{
			++ChangedActors;
		}
		if (!LocalWarnings.IsEmpty())
		{
			BatchWarnings += FString::Printf(TEXT("%s analysis warnings:\n%s"), *Actor->GetName(), *LocalWarnings);
		}
		if (!ApplyResult.Warnings.IsEmpty())
		{
			BatchWarnings += FString::Printf(TEXT("%s apply warnings:\n%s"), *Actor->GetName(), *ApplyResult.Warnings);
		}
	}

	StatusReport = FString::Printf(TEXT("Batch apply complete: %d actors processed, %d actors changed."), ProcessedActors, ChangedActors);
	WarningReport = BatchWarnings.IsEmpty() ? TEXT("No warnings.") : BatchWarnings;
	RefreshSelectionContext(true);
	InvalidatePanel();
}

void FPixelizerEditorModule::PollAutoSelection()
{
	UPixelizerToolSession* ToolSession = EnsureSession();
	if (!ToolSession || !ToolSession->bAutoTrackSelection)
	{
		return;
	}

	FString SelectionWarnings;
	PixelizerEditorProcessing::FResolvedSelection Resolved;
	PixelizerEditorProcessing::ResolveSelectedStaticMeshComponent(Resolved, &SelectionWarnings);
	if (Resolved.Key != CurrentSelectionKey)
	{
		RefreshSelectionContext(true);
	}
}

FText FPixelizerEditorModule::GetSelectionSummaryText() const { return FText::FromString(SelectionSummary); }
FText FPixelizerEditorModule::GetMaterialReportText() const { return FText::FromString(MaterialReport); }
FText FPixelizerEditorModule::GetWarningReportText() const { return FText::FromString(WarningReport); }
FText FPixelizerEditorModule::GetStatusText() const { return FText::FromString(StatusReport); }

IMPLEMENT_MODULE(FPixelizerEditorModule, PixelizerEditor)

#undef LOCTEXT_NAMESPACE
