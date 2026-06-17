


#include "UI/Frontend/Data/EMROptionsDataRegistry.h"

#include "DLSSLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/Internationalization.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "LocalizationLibrary.h"
#include "Internationalization/Culture.h"
#include "Misc/App.h"
#include "ProximityVoiceLocalPlayerSubsystem.h"
#include "RenderUtils.h"
#include "StreamlineLibraryDLSSG.h"
#include "StreamlineLibraryReflex.h"
#include "UI/Frontend/EMRGameUserSettings.h"
#include "UI/Frontend/Data/EMRListDataObjectString.h"
#include "UI/Frontend/Data/EMRListDataObjectCollection.h"
#include "UI/Frontend/Data/EMRListDataObjectKeyRemap.h"
#include "UI/Frontend/Data/EMRListDataObjectScalar.h"
#include "UI/Frontend/Data/EMRListDataObjectStringResolution.h"
#include "UI/Frontend/Utils/DebugHelper.h"

#define MAKE_OPTIONS_DATA_CONTROL(SetterOrGetterFuncName) \
	MakeShared<FEMROptionsDataInteractionHelper>(GET_FUNCTION_NAME_STRING_CHECKED(UEMRGameUserSettings, SetterOrGetterFuncName))

namespace EMROptionsDataRegistryPrivate
{
	constexpr TCHAR UpscalerTSR[] = TEXT("TSR");
	constexpr TCHAR UpscalerDLSS[] = TEXT("DLSS");
	constexpr TCHAR UpscalerFSR[] = TEXT("FSR");

	constexpr TCHAR UpscalerQualityTSRDefault[] = TEXT("TSR_Default");
	constexpr TCHAR UpscalerQualityDLSSDLAA[] = TEXT("DLSS_DLAA");
	constexpr TCHAR UpscalerQualityDLSSUltraQuality[] = TEXT("DLSS_UltraQuality");
	constexpr TCHAR UpscalerQualityDLSSQuality[] = TEXT("DLSS_Quality");
	constexpr TCHAR UpscalerQualityDLSSBalanced[] = TEXT("DLSS_Balanced");
	constexpr TCHAR UpscalerQualityDLSSPerformance[] = TEXT("DLSS_Performance");
	constexpr TCHAR UpscalerQualityDLSSUltraPerformance[] = TEXT("DLSS_UltraPerformance");
	constexpr TCHAR UpscalerQualityFSRNativeAA[] = TEXT("FSR_NativeAA");
	constexpr TCHAR UpscalerQualityFSRQuality[] = TEXT("FSR_Quality");
	constexpr TCHAR UpscalerQualityFSRBalanced[] = TEXT("FSR_Balanced");
	constexpr TCHAR UpscalerQualityFSRPerformance[] = TEXT("FSR_Performance");
	constexpr TCHAR UpscalerQualityFSRUltraPerformance[] = TEXT("FSR_UltraPerformance");

	bool EqualsNoCase(const FString& Value, const TCHAR* Expected)
	{
		return Value.Equals(Expected, ESearchCase::IgnoreCase);
	}

	bool IsEngineFeatureQueryReady()
	{
		return GEngine && GEngine->IsInitialized();
	}

	bool HasConsoleVariable(const TCHAR* Name)
	{
		return IConsoleManager::Get().FindConsoleVariable(Name) != nullptr;
	}

	bool IsD3D12GraphicsRHI()
	{
		return FApp::GetGraphicsRHI().Contains(TEXT("D3D12"), ESearchCase::IgnoreCase);
	}

	bool IsFSRUpscalerRuntimeAvailable()
	{
		return IsD3D12GraphicsRHI() &&
			HasConsoleVariable(TEXT("r.FidelityFX.FSR.Enabled")) &&
			HasConsoleVariable(TEXT("r.FidelityFX.FSR.QualityMode"));
	}

	bool IsFSRFrameGenerationRuntimeAvailable()
	{
		return IsD3D12GraphicsRHI() && HasConsoleVariable(TEXT("r.FidelityFX.FI.Enabled"));
	}

	bool IsDLSSUpscalerRuntimeAvailable()
	{
		return IsEngineFeatureQueryReady() && UDLSSLibrary::QueryDLSSSupport() == UDLSSSupport::Supported;
	}

	bool IsDLSSFrameGenerationRuntimeAvailable()
	{
		return IsEngineFeatureQueryReady() &&
			UStreamlineLibraryDLSSG::QueryDLSSGSupport() == EStreamlineFeatureSupport::Supported &&
			UStreamlineLibraryDLSSG::IsDLSSGModeSupported(EStreamlineDLSSGMode::On2X);
	}

	bool IsNVIDIAReflexRuntimeAvailable()
	{
		return IsEngineFeatureQueryReady() && UStreamlineLibraryReflex::QueryReflexSupport() == EStreamlineFeatureSupport::Supported;
	}

	bool IsRayTracingRuntimeAvailable()
	{
		const IConsoleVariable* RuntimeRayTracingVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.Enable"));
		if (!IsEngineFeatureQueryReady() || RuntimeRayTracingVar == nullptr)
		{
			return false;
		}

		return IsRayTracingAllowed() && IsRayTracingEnableOnDemandSupported();
	}

	FString GetCurrentUpscalerSelection()
	{
		if (const UEMRGameUserSettings* Settings = UEMRGameUserSettings::Get())
		{
			return Settings->GetUpscaler();
		}

		return UpscalerTSR;
	}

	void PopulateDLSSQualityOptions(UEMRListDataObjectString& DataObject)
	{
		const auto AddDLSSModeIfSupported = [&DataObject](UDLSSMode Mode, const TCHAR* Token, const TCHAR* Label)
		{
			if (UDLSSLibrary::IsDLSSModeSupported(Mode))
			{
				DataObject.AddDynamicOption(Token, GetLocalizedText(UIStringTableId, Label));
			}
		};

		AddDLSSModeIfSupported(UDLSSMode::DLAA, UpscalerQualityDLSSDLAA, TEXT("UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.DLAA"));
		AddDLSSModeIfSupported(UDLSSMode::UltraQuality, UpscalerQualityDLSSUltraQuality, TEXT("UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.UltraQuality"));
		AddDLSSModeIfSupported(UDLSSMode::Quality, UpscalerQualityDLSSQuality, TEXT("UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.Quality"));
		AddDLSSModeIfSupported(UDLSSMode::Balanced, UpscalerQualityDLSSBalanced, TEXT("UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.Balanced"));
		AddDLSSModeIfSupported(UDLSSMode::Performance, UpscalerQualityDLSSPerformance, TEXT("UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.Performance"));
		AddDLSSModeIfSupported(UDLSSMode::UltraPerformance, UpscalerQualityDLSSUltraPerformance, TEXT("UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.UltraPerformance"));

		if (DataObject.GetAvailableOptionsTextArray().IsEmpty())
		{
			DataObject.AddDynamicOption(UpscalerQualityDLSSQuality, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.Quality"));
		}
	}

	void PopulateFSRQualityOptions(UEMRListDataObjectString& DataObject)
	{
		DataObject.AddDynamicOption(UpscalerQualityFSRNativeAA, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.NativeAA"));
		DataObject.AddDynamicOption(UpscalerQualityFSRQuality, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.Quality"));
		DataObject.AddDynamicOption(UpscalerQualityFSRBalanced, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.Balanced"));
		DataObject.AddDynamicOption(UpscalerQualityFSRPerformance, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.Performance"));
		DataObject.AddDynamicOption(UpscalerQualityFSRUltraPerformance, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.UltraPerformance"));
	}

	void PopulateUpscalerQualityOptions(UEMRListDataObjectString& DataObject)
	{
		const FString SelectedUpscaler = GetCurrentUpscalerSelection();

		if (EqualsNoCase(SelectedUpscaler, UpscalerDLSS) && IsDLSSUpscalerRuntimeAvailable())
		{
			PopulateDLSSQualityOptions(DataObject);
			return;
		}

		if (EqualsNoCase(SelectedUpscaler, UpscalerFSR) && IsFSRUpscalerRuntimeAvailable())
		{
			PopulateFSRQualityOptions(DataObject);
			return;
		}

		DataObject.AddDynamicOption(UpscalerQualityTSRDefault, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.TSRManaged"));
	}
}

#define UpscalerTSR EMROptionsDataRegistryPrivate::UpscalerTSR
#define UpscalerDLSS EMROptionsDataRegistryPrivate::UpscalerDLSS
#define UpscalerFSR EMROptionsDataRegistryPrivate::UpscalerFSR
#define UpscalerQualityTSRDefault EMROptionsDataRegistryPrivate::UpscalerQualityTSRDefault
#define UpscalerQualityDLSSDLAA EMROptionsDataRegistryPrivate::UpscalerQualityDLSSDLAA
#define UpscalerQualityDLSSUltraQuality EMROptionsDataRegistryPrivate::UpscalerQualityDLSSUltraQuality
#define UpscalerQualityDLSSQuality EMROptionsDataRegistryPrivate::UpscalerQualityDLSSQuality
#define UpscalerQualityDLSSBalanced EMROptionsDataRegistryPrivate::UpscalerQualityDLSSBalanced
#define UpscalerQualityDLSSPerformance EMROptionsDataRegistryPrivate::UpscalerQualityDLSSPerformance
#define UpscalerQualityDLSSUltraPerformance EMROptionsDataRegistryPrivate::UpscalerQualityDLSSUltraPerformance
#define UpscalerQualityFSRNativeAA EMROptionsDataRegistryPrivate::UpscalerQualityFSRNativeAA
#define UpscalerQualityFSRQuality EMROptionsDataRegistryPrivate::UpscalerQualityFSRQuality
#define UpscalerQualityFSRBalanced EMROptionsDataRegistryPrivate::UpscalerQualityFSRBalanced
#define UpscalerQualityFSRPerformance EMROptionsDataRegistryPrivate::UpscalerQualityFSRPerformance
#define UpscalerQualityFSRUltraPerformance EMROptionsDataRegistryPrivate::UpscalerQualityFSRUltraPerformance
#define EqualsNoCase EMROptionsDataRegistryPrivate::EqualsNoCase
#define IsEngineFeatureQueryReady EMROptionsDataRegistryPrivate::IsEngineFeatureQueryReady
#define HasConsoleVariable EMROptionsDataRegistryPrivate::HasConsoleVariable
#define IsFSRUpscalerRuntimeAvailable EMROptionsDataRegistryPrivate::IsFSRUpscalerRuntimeAvailable
#define IsFSRFrameGenerationRuntimeAvailable EMROptionsDataRegistryPrivate::IsFSRFrameGenerationRuntimeAvailable
#define IsDLSSUpscalerRuntimeAvailable EMROptionsDataRegistryPrivate::IsDLSSUpscalerRuntimeAvailable
#define IsDLSSFrameGenerationRuntimeAvailable EMROptionsDataRegistryPrivate::IsDLSSFrameGenerationRuntimeAvailable
#define IsNVIDIAReflexRuntimeAvailable EMROptionsDataRegistryPrivate::IsNVIDIAReflexRuntimeAvailable
#define IsRayTracingRuntimeAvailable EMROptionsDataRegistryPrivate::IsRayTracingRuntimeAvailable
#define GetCurrentUpscalerSelection EMROptionsDataRegistryPrivate::GetCurrentUpscalerSelection
#define PopulateDLSSQualityOptions EMROptionsDataRegistryPrivate::PopulateDLSSQualityOptions
#define PopulateFSRQualityOptions EMROptionsDataRegistryPrivate::PopulateFSRQualityOptions
#define PopulateUpscalerQualityOptions EMROptionsDataRegistryPrivate::PopulateUpscalerQualityOptions

void UEMROptionsDataRegistry::InitOptionsDataRegistry(ULocalPlayer* InOwningLocalPlayer)
{
	InitGameplayCollectionTab();
	InitAudioCollectionTab();
	InitVideoCollectionTab();
	InitControlCollectionTab(InOwningLocalPlayer);
}


TArray<UEMRListDataObjectBase*> UEMROptionsDataRegistry::GetListSourceItemsBySelectedTabID(const FName& InSelectedTabID) const
{
	UEMRListDataObjectCollection* const* FoundTabCollectionPointer = RegisteredOptionsTabCollections.FindByPredicate([InSelectedTabID](UEMRListDataObjectCollection* AvailableTabCollection)->bool
	{
		return AvailableTabCollection->GetDataID() == InSelectedTabID;
	});
	
	checkf(FoundTabCollectionPointer, TEXT("No valid tab found under the ID %s"), *InSelectedTabID.ToString());
	
	UEMRListDataObjectCollection* FoundTabCollection = *FoundTabCollectionPointer;
	
	TArray<UEMRListDataObjectBase*> AllChildListItems;
	
	for (UEMRListDataObjectBase* ChildListData : FoundTabCollection->GetAllChildListData())
	{
		if (!ChildListData) continue;
		
		AllChildListItems.Add(ChildListData);
		
		if (ChildListData->HasAnyChildListData())
		{
			FindChildListDataRecursively(ChildListData, AllChildListItems);
		}
	}
	
	return AllChildListItems;
}


void UEMROptionsDataRegistry::InitGameplayCollectionTab()
{
	UEMRListDataObjectCollection* GameplayTabCollection = NewObject<UEMRListDataObjectCollection>();
	if (!GameplayTabCollection) return;
	
	GameplayTabCollection->SetDataID(FName("GameplayTabCollection"));
	GameplayTabCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay"));
	
	/* This is the full code for constructor data interactor helper
	TSharedPtr<FEMROptionsDataInteractionHelper> ConstructedHelper = 
	MakeShared<FEMROptionsDataInteractionHelper>(GET_FUNCTION_NAME_STRING_CHECKED(UEMRGameUserSettings, GetCurrentGameDifficulty));
	*/
	
	// General Category
	{
		UEMRListDataObjectCollection* GeneralCategoryCollection = NewObject<UEMRListDataObjectCollection>();
		if (!GeneralCategoryCollection) return;
		
		GeneralCategoryCollection->SetDataID(FName("GameplayCategoryCollection"));
		GeneralCategoryCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General"));
		
		GameplayTabCollection->AddChildListData(GeneralCategoryCollection);
		
		// Language
		{
			UEMRListDataObjectString* Language = NewObject<UEMRListDataObjectString>();
			if (!Language) return;
			
			Language->SetDataID(FName("Language"));
			Language->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.Language.Name"));
			Language->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.Language.Description"));

			TArray<FString> AvailableCultures;
			FInternationalization::Get().GetAvailableCultures(AvailableCultures, true);

			for (const FString& CultureName : AvailableCultures)
			{
				if (CultureName.IsEmpty())
				{
					continue;
				}

				FCulturePtr Culture = FInternationalization::Get().GetCulture(CultureName);
				FText DisplayName = Culture.IsValid() ? FText::FromString(Culture->GetDisplayName()) : FText::FromString(CultureName);
				Language->AddDynamicOption(CultureName, DisplayName);
			}
			
			Language->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetCurrentLanguage));
			Language->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetCurrentLanguage));
			Language->SetDefaultValueFromString(FInternationalization::Get().GetCurrentCulture()->GetName());
			Language->SetShouldApplySettingsImmediately(true);
			
			GeneralCategoryCollection->AddChildListData(Language);
		}
		
		// Show FPS Counter
		{
			UEMRListDataObjectStringBool* ShowFPSCounter = NewObject<UEMRListDataObjectStringBool>();
			if (!ShowFPSCounter) return;
			
			ShowFPSCounter->SetDataID(FName("ShowFPSCounter"));
			ShowFPSCounter->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.ShowFPSCounter.Name"));
			ShowFPSCounter->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.ShowFPSCounter.Description"));
			
			ShowFPSCounter->SetFalseAsDefaultValue();
			ShowFPSCounter->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetShowFPSCounter));
			ShowFPSCounter->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetShowFPSCounter));
			ShowFPSCounter->SetShouldApplySettingsImmediately(true);
			
			GeneralCategoryCollection->AddChildListData(ShowFPSCounter);
		}

		// Show keybind helper
		{
			UEMRListDataObjectStringBool* ShowKeybindHelper = NewObject<UEMRListDataObjectStringBool>();
			if (!ShowKeybindHelper) return;

			ShowKeybindHelper->SetDataID(FName("ShowKeybindHelper"));
			ShowKeybindHelper->SetDataDisplayName(FText::FromString(TEXT("Show keybind helper")));
			ShowKeybindHelper->SetDescriptionRichText(FText::FromString(TEXT("Display the on-screen keybind helper during gameplay sessions.")));

			ShowKeybindHelper->SetTrueAsDefaultValue();
			ShowKeybindHelper->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetShowKeybindHelper));
			ShowKeybindHelper->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetShowKeybindHelper));
			ShowKeybindHelper->SetShouldApplySettingsImmediately(true);

			GeneralCategoryCollection->AddChildListData(ShowKeybindHelper);
		}
		
		// Color Blind Mode
		{
			UEMRListDataObjectString* ColorBlindMode = NewObject<UEMRListDataObjectString>();
			if (!ColorBlindMode) return;
			
			ColorBlindMode->SetDataID(FName("ColorBlindMode"));
			ColorBlindMode->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.ColorBlindMode.Name"));
			ColorBlindMode->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.ColorBlindMode.Description"));
			
			ColorBlindMode->AddDynamicOption(TEXT("Off"), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.ColorBlindMode.Off"));
			ColorBlindMode->AddDynamicOption(TEXT("Deuteranopia"), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.ColorBlindMode.Deuteranopia"));
			ColorBlindMode->AddDynamicOption(TEXT("Protanopia"), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.ColorBlindMode.Protanopia"));
			ColorBlindMode->AddDynamicOption(TEXT("Tritanopia"), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Gameplay.Category.General.ColorBlindMode.Tritanopia"));
			
			ColorBlindMode->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetColorBlindMode));
			ColorBlindMode->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetColorBlindMode));
			ColorBlindMode->SetDefaultValueFromString(TEXT("Off"));
			ColorBlindMode->SetShouldApplySettingsImmediately(true);
			
			GeneralCategoryCollection->AddChildListData(ColorBlindMode);
		}
	}
	
	RegisteredOptionsTabCollections.Add(GameplayTabCollection);
}



void UEMROptionsDataRegistry::InitVideoCollectionTab()
{
	UEMRListDataObjectCollection* VideoTabCollection = NewObject<UEMRListDataObjectCollection>();
	if (!VideoTabCollection) return;

	VideoTabCollection->SetDataID(FName("VideoTabCollection"));
	VideoTabCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video"));
	
	
	// Display Category 
	{
		UEMRListDataObjectCollection* DisplayCategoryCollection = NewObject<UEMRListDataObjectCollection>();
		if (!DisplayCategoryCollection) return;
		
		DisplayCategoryCollection->SetDataID(FName("DisplayCategoryCollection"));
		DisplayCategoryCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display"));
		
		VideoTabCollection->AddChildListData(DisplayCategoryCollection);
		
		FOptionsDataEditConditionDescriptor PackagedBuildOnlyCondition;
		PackagedBuildOnlyCondition.SetEditConditionFunction(
			[]()->bool
			{
				const bool bIsInEditor = GIsEditor || GIsPlayInEditorWorld;
				
				return !bIsInEditor;
			});
		
		
		PackagedBuildOnlyCondition.SetDisabledRichReason(FText::FromString(TEXT("<Disabled>This settings can only be adjusted in a packaged build</>")));
		
		
		UEMRListDataObjectStringEnum* CachedWindowMode = nullptr;
		// Window Mode
		{
			UEMRListDataObjectStringEnum* WindowMode = NewObject<UEMRListDataObjectStringEnum>();
			if (!WindowMode) return;
			
			WindowMode->SetDataID(FName("WindowMode"));
			WindowMode->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.WindowMode.Name"));
			WindowMode->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.WindowMode.Description"));
			WindowMode->AddEnumOption(EWindowMode::Fullscreen, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.WindowMode.Fullscreen"));
			WindowMode->AddEnumOption(EWindowMode::Windowed, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.WindowMode.Windowed"));
			WindowMode->AddEnumOption(EWindowMode::WindowedFullscreen, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.WindowMode.BorderlessFullscreen"));
			WindowMode->SetDefaultValueFromEnumOption(EWindowMode::WindowedFullscreen);
			
			WindowMode->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetFullscreenMode));
			WindowMode->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetFullscreenMode));
			WindowMode->SetShouldApplySettingsImmediately(true);

			WindowMode->AddEditCondition(PackagedBuildOnlyCondition);
			
			CachedWindowMode = WindowMode;
			
			DisplayCategoryCollection->AddChildListData(WindowMode);
		}
		
		// Screen Resolution
		{
			UEMRListDataObjectStringResolution* ScreenResolution = NewObject<UEMRListDataObjectStringResolution>();
			if (!ScreenResolution) return;
			
			ScreenResolution->SetDataID(FName("ScreenResolution"));
			ScreenResolution->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.ScreenResolution.Name"));
			ScreenResolution->SetDisabledRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.ScreenResolution.Description"));
			ScreenResolution->InitResolutionValues();
			
			ScreenResolution->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetScreenResolution));
			ScreenResolution->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetScreenResolution));
			ScreenResolution->SetShouldApplySettingsImmediately(true);
			
			ScreenResolution->AddEditCondition(PackagedBuildOnlyCondition);
			
			// Screen resolution is editable only when not in BorderlessFullScreen
			FOptionsDataEditConditionDescriptor ScreenResolutionEditCondition;
			ScreenResolutionEditCondition.SetEditConditionFunction(
				[CachedWindowMode]()->bool
				{
					const bool bIsBorderlessWindow = CachedWindowMode->GetCurrentValueAsEnum<EWindowMode::Type>() == EWindowMode::WindowedFullscreen;
					
					return !bIsBorderlessWindow;
				});
			
			ScreenResolutionEditCondition.SetDisabledRichReason(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.ScreenResolution.DisabledReason"));
			ScreenResolutionEditCondition.SetDisabledForcedStringValue(ScreenResolution->GetMaximumAllowedResolution());
			
			ScreenResolution->AddEditCondition(ScreenResolutionEditCondition);
			
			ScreenResolution->AddEditDependencyData(CachedWindowMode);
			
			DisplayCategoryCollection->AddChildListData(ScreenResolution);
		}
		
		
		// Vertical Sync
		{
			UEMRListDataObjectStringBool* VerticalSync = NewObject<UEMRListDataObjectStringBool>();
			if (!VerticalSync) return;
			
			VerticalSync->SetDataID(FName("VerticalSync"));
			VerticalSync->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.VerticalSync.Name"));
			VerticalSync->SetDisabledRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.VerticalSync.Description"));
			
			VerticalSync->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(IsVSyncEnabled));
			VerticalSync->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetVSyncEnabled));
			VerticalSync->SetFalseAsDefaultValue();
			
			VerticalSync->SetShouldApplySettingsImmediately(true);
			
			FOptionsDataEditConditionDescriptor VerticalSyncEditCondition;
			VerticalSyncEditCondition.SetEditConditionFunction(
				[CachedWindowMode]()->bool
				{
					return CachedWindowMode->GetCurrentValueAsEnum<EWindowMode::Type>() == EWindowMode::Fullscreen;
				});
			
			VerticalSyncEditCondition.SetDisabledRichReason(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.VerticalSync.DisabledReason"));
			VerticalSyncEditCondition.SetDisabledForcedStringValue(TEXT("False"));
			
			VerticalSync->AddEditCondition(VerticalSyncEditCondition);
			
			DisplayCategoryCollection->AddChildListData(VerticalSync);
		}
		
		
		// Frame Rate Limit
		{
			UEMRListDataObjectString* FrameRateLimit = NewObject<UEMRListDataObjectString>();
			if (!FrameRateLimit) return;
			
			FrameRateLimit->SetDataID(FName("FrameRateLimit"));
			FrameRateLimit->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.Name"));
			FrameRateLimit->SetDisabledRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.Description"));
			
			FrameRateLimit->AddDynamicOption(LexToString(30.f), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.30"));
			FrameRateLimit->AddDynamicOption(LexToString(60.f), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.60"));
			FrameRateLimit->AddDynamicOption(LexToString(90.f), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.90"));
			FrameRateLimit->AddDynamicOption(LexToString(120.f), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.120"));
			FrameRateLimit->AddDynamicOption(LexToString(144.f), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.144"));
			FrameRateLimit->AddDynamicOption(LexToString(165.f), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.165"));
			FrameRateLimit->AddDynamicOption(LexToString(240.f), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.240"));
			FrameRateLimit->AddDynamicOption(LexToString(360.f), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.360"));
			FrameRateLimit->AddDynamicOption(LexToString(0.f), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FrameRateLimit.NoLimit"));
			
			FrameRateLimit->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetFrameRateLimit));
			FrameRateLimit->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetFrameRateLimit));
			FrameRateLimit->SetDefaultValueFromString(LexToString(0.f));
			
			FrameRateLimit->SetShouldApplySettingsImmediately(true);
			
			DisplayCategoryCollection->AddChildListData(FrameRateLimit);
		}
		
		
		// Display Gamma
		{
			UEMRListDataObjectScalar* DisplayGamma = NewObject<UEMRListDataObjectScalar>();
			if (!DisplayGamma) return;
			
			DisplayGamma->SetDataID(FName("DisplayGamma"));
			DisplayGamma->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.DisplayGamma.Name"));
			DisplayGamma->SetDisabledRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.DisplayGamma.Description"));
			DisplayGamma->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			DisplayGamma->SetOutputValueRange(TRange<float>(1.7f, 2.7f)); // The default value Unreal has is 2.2f
			DisplayGamma->SetSliderStepSize(0.01f);
			DisplayGamma->SetDisplayNumericType(ECommonNumericType::Percentage);
			DisplayGamma->SetNumberFormattingOptions(UEMRListDataObjectScalar::NoDecimal());
			DisplayGamma->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetCurrentDisplayGamma));
			DisplayGamma->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetCurrentDisplayGamma));
			DisplayGamma->SetDefaultValueFromString(LexToString(2.2f));
		
			DisplayGamma->SetShouldApplySettingsImmediately(true);
		
			DisplayCategoryCollection->AddChildListData(DisplayGamma);
		}


		// Field of View
		{
			UEMRListDataObjectScalar* FieldOfView = NewObject<UEMRListDataObjectScalar>();
			if (!FieldOfView) return;
			
			FieldOfView->SetDataID(FName("FieldOfView"));
			FieldOfView->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FieldOfView.Name"));
			FieldOfView->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.FieldOfView.Description"));
			FieldOfView->SetDisplayValueRange(TRange<float>(60.f, 120.f));
			FieldOfView->SetOutputValueRange(TRange<float>(60.f, 120.f));
			FieldOfView->SetSliderStepSize(1.f);
			FieldOfView->SetDisplayNumericType(ECommonNumericType::Number);
			FieldOfView->SetNumberFormattingOptions(UEMRListDataObjectScalar::NoDecimal());
			FieldOfView->SetDefaultValueFromString(LexToString(90.f));
			
			FieldOfView->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetFieldOfView));
			FieldOfView->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetFieldOfView));
			FieldOfView->SetShouldApplySettingsImmediately(true);
			
			DisplayCategoryCollection->AddChildListData(FieldOfView);
		}


		// Motion Blur
		{
			UEMRListDataObjectStringBool* MotionBlur = NewObject<UEMRListDataObjectStringBool>();
			if (!MotionBlur) return;
			
			MotionBlur->SetDataID(FName("MotionBlur"));
			MotionBlur->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.MotionBlur.Name"));
			MotionBlur->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Display.MotionBlur.Description"));
			
			MotionBlur->SetTrueAsDefaultValue();
			MotionBlur->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMotionBlurEnabled));
			MotionBlur->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMotionBlurEnabled));
			
			MotionBlur->SetShouldApplySettingsImmediately(true);
			
			DisplayCategoryCollection->AddChildListData(MotionBlur);
		}
	}
	
	
	// Graphics Category
	{
		UEMRListDataObjectCollection* GraphicsCategoryCollection = NewObject<UEMRListDataObjectCollection>();
		if (!GraphicsCategoryCollection) return;
		
		GraphicsCategoryCollection->SetDataID(FName("GraphicsCategoryCollection"));
		GraphicsCategoryCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics"));
		
		VideoTabCollection->AddChildListData(GraphicsCategoryCollection);
		
		
		UEMRListDataObjectStringInteger* CachedOverallQuality = nullptr;
		UEMRListDataObjectString* CachedUpscaler = nullptr;
		// Overall Quality
		{
			UEMRListDataObjectStringInteger* OverallQuality = NewObject<UEMRListDataObjectStringInteger>();
			if (!OverallQuality) return;
			
			OverallQuality->SetDataID(FName("OverallQuality"));
			OverallQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Name"));
			OverallQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Description"));
			
			OverallQuality->AddIntegerOption(0, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Low"));
			OverallQuality->AddIntegerOption(1, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Medium"));
			OverallQuality->AddIntegerOption(2, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.High"));
			OverallQuality->AddIntegerOption(3, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Epic"));
			OverallQuality->AddIntegerOption(4, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Cinematic"));
			
			OverallQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetOverallScalabilityLevel));
			OverallQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetOverallScalabilityLevel));
			
			OverallQuality->SetShouldApplySettingsImmediately(true);
			
			CachedOverallQuality = OverallQuality;
			
			GraphicsCategoryCollection->AddChildListData(OverallQuality);
		}

		// Ray Tracing
		{
			UEMRListDataObjectStringBool* RayTracing = NewObject<UEMRListDataObjectStringBool>();
			if (!RayTracing) return;

			RayTracing->SetDataID(FName("RayTracing"));
			RayTracing->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.RayTracing.Name"));
			RayTracing->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.RayTracing.Description"));
			RayTracing->OverrideTrueDisplayText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.RayTracing.On"));
			RayTracing->OverrideFalseDisplayText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.RayTracing.Off"));
			RayTracing->SetFalseAsDefaultValue();
			RayTracing->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetRayTracingEnabled));
			RayTracing->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetRayTracingEnabled));
			RayTracing->SetShouldApplySettingsImmediately(true);

			FOptionsDataEditConditionDescriptor RayTracingSupportCondition;
			RayTracingSupportCondition.SetEditConditionFunction([]()->bool
			{
				return IsRayTracingRuntimeAvailable();
			});
			RayTracingSupportCondition.SetDisabledRichReason(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.RayTracing.DisabledReason"));
			RayTracingSupportCondition.SetDisabledForcedStringValue(TEXT("False"));
			RayTracing->AddEditCondition(RayTracingSupportCondition);

			GraphicsCategoryCollection->AddChildListData(RayTracing);
		}
		
		// Upscaler
		{
			UEMRListDataObjectString* Upscaler = NewObject<UEMRListDataObjectString>();
			if (!Upscaler) return;

			Upscaler->SetDataID(FName("Upscaler"));
			Upscaler->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.Upscaler.Name"));
			Upscaler->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.Upscaler.Description"));

			Upscaler->AddDynamicOption(UpscalerTSR, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.Upscaler.TSR"));
			if (IsDLSSUpscalerRuntimeAvailable())
			{
				Upscaler->AddDynamicOption(UpscalerDLSS, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.Upscaler.DLSS"));
			}
			if (IsFSRUpscalerRuntimeAvailable())
			{
				Upscaler->AddDynamicOption(UpscalerFSR, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.Upscaler.FSR"));
			}

			Upscaler->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetUpscaler));
			Upscaler->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetUpscaler));
			Upscaler->SetDefaultValueFromString(UpscalerTSR);
			Upscaler->SetShouldApplySettingsImmediately(true);

			CachedUpscaler = Upscaler;

			GraphicsCategoryCollection->AddChildListData(Upscaler);
		}

		// Upscaler Quality
		{
			UEMRListDataObjectString* UpscalerQuality = NewObject<UEMRListDataObjectString>();
			if (!UpscalerQuality) return;

			UpscalerQuality->SetDataID(FName("UpscalerQuality"));
			UpscalerQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.Name"));
			UpscalerQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.Description"));
			UpscalerQuality->SetDynamicOptionsRefreshFunction([](UEMRListDataObjectString& DataObject)
			{
				PopulateUpscalerQualityOptions(DataObject);
			});
			UpscalerQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetUpscalerQuality));
			UpscalerQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetUpscalerQuality));
			UpscalerQuality->SetDefaultValueFromString(UpscalerQualityTSRDefault);
			UpscalerQuality->SetShouldApplySettingsImmediately(true);

			FOptionsDataEditConditionDescriptor UpscalerQualityEditCondition;
			UpscalerQualityEditCondition.SetEditConditionFunction([]()->bool
			{
				return !GetCurrentUpscalerSelection().Equals(UpscalerTSR, ESearchCase::IgnoreCase);
			});
			UpscalerQualityEditCondition.SetDisabledRichReason(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.UpscalerQuality.DisabledReason"));
			UpscalerQualityEditCondition.SetDisabledForcedStringValue(UpscalerQualityTSRDefault);
			UpscalerQuality->AddEditCondition(UpscalerQualityEditCondition);

			if (CachedUpscaler)
			{
				UpscalerQuality->AddEditDependencyData(CachedUpscaler);
			}

			GraphicsCategoryCollection->AddChildListData(UpscalerQuality);
		}

		// Frame Generation
		{
			UEMRListDataObjectStringBool* FrameGeneration = NewObject<UEMRListDataObjectStringBool>();
			if (!FrameGeneration) return;

			FrameGeneration->SetDataID(FName("FrameGeneration"));
			FrameGeneration->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.FrameGeneration.Name"));
			FrameGeneration->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.FrameGeneration.Description"));
			FrameGeneration->SetFalseAsDefaultValue();
			FrameGeneration->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetFrameGenerationEnabled));
			FrameGeneration->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetFrameGenerationEnabled));
			FrameGeneration->SetShouldApplySettingsImmediately(true);

			FOptionsDataEditConditionDescriptor FrameGenerationUpscalerCondition;
			FrameGenerationUpscalerCondition.SetEditConditionFunction([]()->bool
			{
				return !GetCurrentUpscalerSelection().Equals(UpscalerTSR, ESearchCase::IgnoreCase);
			});
			FrameGenerationUpscalerCondition.SetDisabledRichReason(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.FrameGeneration.DisabledReason.UpscalerRequired"));
			FrameGenerationUpscalerCondition.SetDisabledForcedStringValue(TEXT("False"));
			FrameGeneration->AddEditCondition(FrameGenerationUpscalerCondition);

			FOptionsDataEditConditionDescriptor FrameGenerationEditorCondition;
			FrameGenerationEditorCondition.SetEditConditionFunction([]()->bool
			{
				return !GIsPlayInEditorWorld;
			});
			FrameGenerationEditorCondition.SetDisabledRichReason(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.FrameGeneration.DisabledReason.PIE"));
			FrameGenerationEditorCondition.SetDisabledForcedStringValue(TEXT("False"));
			FrameGeneration->AddEditCondition(FrameGenerationEditorCondition);

			FOptionsDataEditConditionDescriptor FrameGenerationSupportCondition;
			FrameGenerationSupportCondition.SetEditConditionFunction([]()->bool
			{
				const FString SelectedUpscaler = GetCurrentUpscalerSelection();
				if (SelectedUpscaler.Equals(UpscalerDLSS, ESearchCase::IgnoreCase))
				{
					return IsDLSSFrameGenerationRuntimeAvailable();
				}
				if (SelectedUpscaler.Equals(UpscalerFSR, ESearchCase::IgnoreCase))
				{
					return IsFSRFrameGenerationRuntimeAvailable();
				}
				return true;
			});
			FrameGenerationSupportCondition.SetDisabledRichReason(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.FrameGeneration.DisabledReason.Unsupported"));
			FrameGenerationSupportCondition.SetDisabledForcedStringValue(TEXT("False"));
			FrameGeneration->AddEditCondition(FrameGenerationSupportCondition);

			if (CachedUpscaler)
			{
				FrameGeneration->AddEditDependencyData(CachedUpscaler);
			}

			GraphicsCategoryCollection->AddChildListData(FrameGeneration);
		}

		// NVIDIA Reflex
		{
			UEMRListDataObjectString* NVIDIAReflex = NewObject<UEMRListDataObjectString>();
			if (!NVIDIAReflex) return;

			NVIDIAReflex->SetDataID(FName("NVIDIAReflex"));
			NVIDIAReflex->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.NVIDIAReflex.Name"));
			NVIDIAReflex->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.NVIDIAReflex.Description"));

			NVIDIAReflex->AddDynamicOption(TEXT("Off"), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.NVIDIAReflex.Off"));
			NVIDIAReflex->AddDynamicOption(TEXT("On"), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.NVIDIAReflex.On"));
			NVIDIAReflex->AddDynamicOption(TEXT("Boost"), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.NVIDIAReflex.Boost"));

			NVIDIAReflex->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetNVIDIAReflexMode));
			NVIDIAReflex->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetNVIDIAReflexMode));
			NVIDIAReflex->SetDefaultValueFromString(TEXT("Off"));
			NVIDIAReflex->SetShouldApplySettingsImmediately(true);

			FOptionsDataEditConditionDescriptor NVIDIAReflexSupportCondition;
			NVIDIAReflexSupportCondition.SetEditConditionFunction([]()->bool
			{
				return IsNVIDIAReflexRuntimeAvailable();
			});
			NVIDIAReflexSupportCondition.SetDisabledRichReason(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.NVIDIAReflex.DisabledReason"));
			NVIDIAReflexSupportCondition.SetDisabledForcedStringValue(TEXT("Off"));
			NVIDIAReflex->AddEditCondition(NVIDIAReflexSupportCondition);

			GraphicsCategoryCollection->AddChildListData(NVIDIAReflex);
		}
		
		
		// Global Illumination Quality
		{
			UEMRListDataObjectStringInteger* GlobalIlluminationQuality = NewObject<UEMRListDataObjectStringInteger>();
			if (!GlobalIlluminationQuality) return;
			
			GlobalIlluminationQuality->SetDataID(FName("GlobalIlluminationQuality"));
			GlobalIlluminationQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.GlobalIlluminationQuality.Name"));
			GlobalIlluminationQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.GlobalIlluminationQuality.Description"));
			
			GlobalIlluminationQuality->AddIntegerOption(0, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Low"));
			GlobalIlluminationQuality->AddIntegerOption(1, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Medium"));
			GlobalIlluminationQuality->AddIntegerOption(2, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.High"));
			GlobalIlluminationQuality->AddIntegerOption(3, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Epic"));
			GlobalIlluminationQuality->AddIntegerOption(4, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Cinematic"));
			
			GlobalIlluminationQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetGlobalIlluminationQuality));
			GlobalIlluminationQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetGlobalIlluminationQuality));
			
			GlobalIlluminationQuality->SetShouldApplySettingsImmediately(true);
			
			GlobalIlluminationQuality->AddEditDependencyData(CachedOverallQuality);
			CachedOverallQuality->AddEditDependencyData(GlobalIlluminationQuality);
			
			GraphicsCategoryCollection->AddChildListData(GlobalIlluminationQuality);
		}
		
		
		// Shadow Quality
		{
			UEMRListDataObjectStringInteger* ShadowQuality = NewObject<UEMRListDataObjectStringInteger>();
			if (!ShadowQuality) return;
			
			ShadowQuality->SetDataID(FName("ShadowQuality"));
			ShadowQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ShadowQuality.Name"));
			ShadowQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ShadowQuality.Description"));
			
			ShadowQuality->AddIntegerOption(0, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Low"));
			ShadowQuality->AddIntegerOption(1, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Medium"));
			ShadowQuality->AddIntegerOption(2, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.High"));
			ShadowQuality->AddIntegerOption(3, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Epic"));
			ShadowQuality->AddIntegerOption(4, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Cinematic"));
			
			ShadowQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetShadowQuality));
			ShadowQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetShadowQuality));
			
			ShadowQuality->SetShouldApplySettingsImmediately(true);
			
			ShadowQuality->AddEditDependencyData(CachedOverallQuality);
			CachedOverallQuality->AddEditDependencyData(ShadowQuality);
			
			GraphicsCategoryCollection->AddChildListData(ShadowQuality);
		}
		
		
		// AntiAliasing Quality
		{
			UEMRListDataObjectStringInteger* AntiAliasingQuality = NewObject<UEMRListDataObjectStringInteger>();
			if (!AntiAliasingQuality) return;
			
			AntiAliasingQuality->SetDataID(FName("AntiAliasingQuality"));
			AntiAliasingQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.AntiAliasingQuality.Name"));
			AntiAliasingQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.AntiAliasingQuality.Description"));
			
			AntiAliasingQuality->AddIntegerOption(0, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Low"));
			AntiAliasingQuality->AddIntegerOption(1, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Medium"));
			AntiAliasingQuality->AddIntegerOption(2, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.High"));
			AntiAliasingQuality->AddIntegerOption(3, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Epic"));
			AntiAliasingQuality->AddIntegerOption(4, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Cinematic"));
			
			AntiAliasingQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetAntiAliasingQuality));
			AntiAliasingQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetAntiAliasingQuality));
			
			AntiAliasingQuality->SetShouldApplySettingsImmediately(true);
			
			AntiAliasingQuality->AddEditDependencyData(CachedOverallQuality);
			CachedOverallQuality->AddEditDependencyData(AntiAliasingQuality);
			
			GraphicsCategoryCollection->AddChildListData(AntiAliasingQuality);
		}
		
		
		// View Distance
		{
			UEMRListDataObjectStringInteger* ViewDistanceQuality = NewObject<UEMRListDataObjectStringInteger>();
			if (!ViewDistanceQuality) return;
			
			ViewDistanceQuality->SetDataID(FName("ViewDistanceQuality"));
			ViewDistanceQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ViewDistanceQuality.Name"));
			ViewDistanceQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ViewDistanceQuality.Description"));
			
			ViewDistanceQuality->AddIntegerOption(0, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ViewDistanceQuality.Near"));
			ViewDistanceQuality->AddIntegerOption(1, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ViewDistanceQuality.Medium"));
			ViewDistanceQuality->AddIntegerOption(2, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ViewDistanceQuality.Far"));
			ViewDistanceQuality->AddIntegerOption(3, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ViewDistanceQuality.VeryFar"));
			ViewDistanceQuality->AddIntegerOption(4, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ViewDistanceQuality.VeryVeryFar"));
			
			ViewDistanceQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetViewDistanceQuality));
			ViewDistanceQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetViewDistanceQuality));
			
			ViewDistanceQuality->SetShouldApplySettingsImmediately(true);
			
			ViewDistanceQuality->AddEditDependencyData(CachedOverallQuality);
			CachedOverallQuality->AddEditDependencyData(ViewDistanceQuality);
			
			GraphicsCategoryCollection->AddChildListData(ViewDistanceQuality);
		}
		
		
		// Texture Quality
		{
			UEMRListDataObjectStringInteger* TextureQuality = NewObject<UEMRListDataObjectStringInteger>();
			if (!TextureQuality) return;
			
			TextureQuality->SetDataID(FName("TextureQuality"));
			TextureQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.TextureQuality.Name"));
			TextureQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.TextureQuality.Description"));
			
			TextureQuality->AddIntegerOption(0, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Low"));
			TextureQuality->AddIntegerOption(1, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Medium"));
			TextureQuality->AddIntegerOption(2, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.High"));
			TextureQuality->AddIntegerOption(3, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Epic"));
			TextureQuality->AddIntegerOption(4, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Cinematic"));
			
			TextureQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetTextureQuality));
			TextureQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetTextureQuality));
			
			TextureQuality->SetShouldApplySettingsImmediately(true);
			
			TextureQuality->AddEditDependencyData(CachedOverallQuality);
			CachedOverallQuality->AddEditDependencyData(TextureQuality);
			
			GraphicsCategoryCollection->AddChildListData(TextureQuality);
		}
		
		
		// Visual Effect Quality
		{
			UEMRListDataObjectStringInteger* VisualEffectQuality = NewObject<UEMRListDataObjectStringInteger>();
			if (!VisualEffectQuality) return;
			
			VisualEffectQuality->SetDataID(FName("VisualEffectQuality"));
			VisualEffectQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.VisualEffectQuality.Name"));
			VisualEffectQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.VisualEffectQuality.Description"));
			
			VisualEffectQuality->AddIntegerOption(0, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Low"));
			VisualEffectQuality->AddIntegerOption(1, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Medium"));
			VisualEffectQuality->AddIntegerOption(2, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.High"));
			VisualEffectQuality->AddIntegerOption(3, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Epic"));
			VisualEffectQuality->AddIntegerOption(4, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Cinematic"));
			
			VisualEffectQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetVisualEffectQuality));
			VisualEffectQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetVisualEffectQuality));
			
			VisualEffectQuality->SetShouldApplySettingsImmediately(true);
			
			VisualEffectQuality->AddEditDependencyData(CachedOverallQuality);
			CachedOverallQuality->AddEditDependencyData(VisualEffectQuality);
			
			GraphicsCategoryCollection->AddChildListData(VisualEffectQuality);
		}
		
		
		// Reflection Quality
		{
			UEMRListDataObjectStringInteger* ReflectionQuality = NewObject<UEMRListDataObjectStringInteger>();
			if (!ReflectionQuality) return;
			
			ReflectionQuality->SetDataID(FName("ReflectionQuality"));
			ReflectionQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ReflectionQuality.Name"));
			ReflectionQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.ReflectionQuality.Description"));
			
			ReflectionQuality->AddIntegerOption(0, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Low"));
			ReflectionQuality->AddIntegerOption(1, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Medium"));
			ReflectionQuality->AddIntegerOption(2, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.High"));
			ReflectionQuality->AddIntegerOption(3, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Epic"));
			ReflectionQuality->AddIntegerOption(4, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Cinematic"));
			
			ReflectionQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetReflectionQuality));
			ReflectionQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetReflectionQuality));
			
			ReflectionQuality->SetShouldApplySettingsImmediately(true);
			
			ReflectionQuality->AddEditDependencyData(CachedOverallQuality);
			CachedOverallQuality->AddEditDependencyData(ReflectionQuality);
			
			GraphicsCategoryCollection->AddChildListData(ReflectionQuality);
		}
		
		
		// Post Processing Quality
		{
			UEMRListDataObjectStringInteger* PostProcessingQuality = NewObject<UEMRListDataObjectStringInteger>();
			if (!PostProcessingQuality) return;
			
			PostProcessingQuality->SetDataID(FName("PostProcessingQuality"));
			PostProcessingQuality->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.PostProcessingQuality.Name"));
			PostProcessingQuality->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.PostProcessingQuality.Description"));
			
			PostProcessingQuality->AddIntegerOption(0, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Low"));
			PostProcessingQuality->AddIntegerOption(1, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Medium"));
			PostProcessingQuality->AddIntegerOption(2, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.High"));
			PostProcessingQuality->AddIntegerOption(3, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Epic"));
			PostProcessingQuality->AddIntegerOption(4, GetLocalizedText(UIStringTableId, "UI.Options.Tab.Video.Category.Graphics.OverallQuality.Cinematic"));
			
			PostProcessingQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetPostProcessingQuality));
			PostProcessingQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetPostProcessingQuality));
			
			PostProcessingQuality->SetShouldApplySettingsImmediately(true);
			
			PostProcessingQuality->AddEditDependencyData(CachedOverallQuality);
			CachedOverallQuality->AddEditDependencyData(PostProcessingQuality);
			
			GraphicsCategoryCollection->AddChildListData(PostProcessingQuality);
		}
	}
	
	
	RegisteredOptionsTabCollections.Add(VideoTabCollection);
}



void UEMROptionsDataRegistry::InitAudioCollectionTab()
{
	UEMRListDataObjectCollection* AudioTabCollection = NewObject<UEMRListDataObjectCollection>();
	if (!AudioTabCollection) return;

	AudioTabCollection->SetDataID(FName("AudioTabCollection"));
	AudioTabCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio"));
	
	// Volume Category
	{
		UEMRListDataObjectCollection* VolumeCategoryCollection = NewObject<UEMRListDataObjectCollection>();
		VolumeCategoryCollection->SetDataID(FName("VolumeCategoryCollection"));
		VolumeCategoryCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume"));
	
		AudioTabCollection->AddChildListData(VolumeCategoryCollection);
		
		// Overall Volume
		{
			UEMRListDataObjectScalar* OverallVolume = NewObject<UEMRListDataObjectScalar>();
			OverallVolume->SetDataID(FName("OverallVolume"));
			OverallVolume->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.OverallVolume.Name"));
			OverallVolume->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.OverallVolume.Description"));
			OverallVolume->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			OverallVolume->SetOutputValueRange(TRange<float>(0.f, 2.f));
			OverallVolume->SetSliderStepSize(0.01f);
			OverallVolume->SetDefaultValueFromString(LexToString(1.f));
			OverallVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			OverallVolume->SetNumberFormattingOptions(UEMRListDataObjectScalar::NoDecimal());
			
			OverallVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetOverallVolume));
			OverallVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetOverallVolume));
			OverallVolume->SetShouldApplySettingsImmediately(true);
			
			VolumeCategoryCollection->AddChildListData(OverallVolume);
		}
		
		// Music Volume
		{
			UEMRListDataObjectScalar* MusicVolume = NewObject<UEMRListDataObjectScalar>();
			MusicVolume->SetDataID(FName("MusicVolume"));
			MusicVolume->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.MusicVolume.Name"));
			MusicVolume->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.MusicVolume.Description"));
			MusicVolume->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			MusicVolume->SetOutputValueRange(TRange<float>(0.f, 2.f));
			MusicVolume->SetSliderStepSize(0.01f);
			MusicVolume->SetDefaultValueFromString(LexToString(1.f));
			MusicVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			MusicVolume->SetNumberFormattingOptions(UEMRListDataObjectScalar::NoDecimal());
			
			MusicVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMusicVolume));
			MusicVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMusicVolume));
			MusicVolume->SetShouldApplySettingsImmediately(true);
			
			VolumeCategoryCollection->AddChildListData(MusicVolume);
		}
		
		// SFX Volume
		{
			UEMRListDataObjectScalar* SFXVolume = NewObject<UEMRListDataObjectScalar>();
			SFXVolume->SetDataID(FName("SFXVolume"));
			SFXVolume->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.SFXVolume.Name"));
			SFXVolume->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.SFXVolume.Description"));
			SFXVolume->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			SFXVolume->SetOutputValueRange(TRange<float>(0.f, 2.f));
			SFXVolume->SetSliderStepSize(0.01f);
			SFXVolume->SetDefaultValueFromString(LexToString(1.f));
			SFXVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			SFXVolume->SetNumberFormattingOptions(UEMRListDataObjectScalar::NoDecimal());
			
			SFXVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetSFXVolume));
			SFXVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetSFXVolume));
			SFXVolume->SetShouldApplySettingsImmediately(true);
			
			VolumeCategoryCollection->AddChildListData(SFXVolume);
		}

		// Proximity Chat Volume
		{
			UEMRListDataObjectScalar* ProximityChatVolume = NewObject<UEMRListDataObjectScalar>();
			ProximityChatVolume->SetDataID(FName("ProximityChatVolume"));
			ProximityChatVolume->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.ProximityChatVolume.Name"));
			ProximityChatVolume->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.ProximityChatVolume.Description"));
			ProximityChatVolume->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			ProximityChatVolume->SetOutputValueRange(TRange<float>(0.f, 1.f));
			ProximityChatVolume->SetSliderStepSize(0.01f);
			ProximityChatVolume->SetDefaultValueFromString(LexToString(1.f));
			ProximityChatVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			ProximityChatVolume->SetNumberFormattingOptions(UEMRListDataObjectScalar::NoDecimal());
			
			ProximityChatVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetProximityChatVolume));
			ProximityChatVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetProximityChatVolume));
			ProximityChatVolume->SetShouldApplySettingsImmediately(true);
			
			VolumeCategoryCollection->AddChildListData(ProximityChatVolume);
		}

		// Microphone Volume
		{
			UEMRListDataObjectScalar* MicrophoneVolume = NewObject<UEMRListDataObjectScalar>();
			MicrophoneVolume->SetDataID(FName("MicrophoneVolume"));
			MicrophoneVolume->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.MicrophoneVolume.Name"));
			MicrophoneVolume->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Volume.MicrophoneVolume.Description"));
			MicrophoneVolume->SetDisplayValueRange(TRange<float>(0.f, 1.f));
			MicrophoneVolume->SetOutputValueRange(TRange<float>(0.f, 1.f));
			MicrophoneVolume->SetSliderStepSize(0.01f);
			MicrophoneVolume->SetDefaultValueFromString(LexToString(1.f));
			MicrophoneVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			MicrophoneVolume->SetNumberFormattingOptions(UEMRListDataObjectScalar::NoDecimal());
			
			MicrophoneVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMicrophoneVolume));
			MicrophoneVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMicrophoneVolume));
			MicrophoneVolume->SetShouldApplySettingsImmediately(true);
			
			VolumeCategoryCollection->AddChildListData(MicrophoneVolume);
		}
	}
	
	// Sound Category
	{
		UEMRListDataObjectCollection* SoundCategoryCollection = NewObject<UEMRListDataObjectCollection>();
		if (!SoundCategoryCollection) return;
		
		SoundCategoryCollection->SetDataID(FName("SoundCategoryCollection"));
		SoundCategoryCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Sound"));
		
		AudioTabCollection->AddChildListData(SoundCategoryCollection);

		// Microphone Mode
		{
			UEMRListDataObjectString* MicrophoneMode = NewObject<UEMRListDataObjectString>();
			if (!MicrophoneMode) return;
			
			MicrophoneMode->SetDataID(FName("MicrophoneMode"));
			MicrophoneMode->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Sound.MicrophoneMode.Name"));
			MicrophoneMode->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Sound.MicrophoneMode.Description"));
			
			MicrophoneMode->AddDynamicOption(TEXT("PushToTalk"), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Sound.MicrophoneMode.PushToTalk"));
			MicrophoneMode->AddDynamicOption(TEXT("OpenMic"), GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Sound.MicrophoneMode.OpenMic"));
			
			MicrophoneMode->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMicrophoneMode));
			MicrophoneMode->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMicrophoneMode));
			MicrophoneMode->SetDefaultValueFromString(TEXT("PushToTalk"));
			MicrophoneMode->SetShouldApplySettingsImmediately(true);
			
			SoundCategoryCollection->AddChildListData(MicrophoneMode);
		}

		// Microphone Input Device
		{
			UEMRListDataObjectString* MicrophoneInputDevice = NewObject<UEMRListDataObjectString>();
			if (!MicrophoneInputDevice) return;

			MicrophoneInputDevice->SetDataID(FName("MicrophoneInputDevice"));
			MicrophoneInputDevice->SetDataDisplayName(FText::FromString(TEXT("Microphone Input Device")));
			MicrophoneInputDevice->SetDescriptionRichText(FText::FromString(TEXT("Select which microphone is used for voice chat. System Default follows your OS default input device.")));

			const TArray<FProximityVoiceInputDevice> InputDevices = UProximityVoiceLocalPlayerSubsystem::QuerySystemInputDevices();
			for (const FProximityVoiceInputDevice& InputDevice : InputDevices)
			{
				MicrophoneInputDevice->AddDynamicOption(InputDevice.DeviceId, InputDevice.DisplayName);
			}

			MicrophoneInputDevice->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetPreferredMicrophoneInputDevice));
			MicrophoneInputDevice->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetPreferredMicrophoneInputDevice));
			MicrophoneInputDevice->SetDefaultValueFromString(TEXT(""));
			MicrophoneInputDevice->SetShouldApplySettingsImmediately(true);

			SoundCategoryCollection->AddChildListData(MicrophoneInputDevice);
		}

		// Mic Monitor Preview
		{
			UEMRListDataObjectStringBool* MicMonitorPreview = NewObject<UEMRListDataObjectStringBool>();
			if (!MicMonitorPreview) return;

			MicMonitorPreview->SetDataID(FName("MicMonitorPreview"));
			MicMonitorPreview->SetDataDisplayName(FText::FromString(TEXT("Mic Monitor Preview")));
			MicMonitorPreview->SetDescriptionRichText(FText::FromString(TEXT("Play your microphone back locally so you can monitor your mic signal.")));

			MicMonitorPreview->SetFalseAsDefaultValue();
			MicMonitorPreview->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMicMonitorEnabled));
			MicMonitorPreview->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMicMonitorEnabled));
			MicMonitorPreview->SetShouldApplySettingsImmediately(true);

			SoundCategoryCollection->AddChildListData(MicMonitorPreview);
		}
		
		// Allow Background Audio
		{
			UEMRListDataObjectStringBool* AllowBackgroundAudio = NewObject<UEMRListDataObjectStringBool>();
			if (!AllowBackgroundAudio) return;
			
			AllowBackgroundAudio->SetDataID(FName("AllowBackgroundAudio"));
			AllowBackgroundAudio->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Sound.AllowBackgroundAudio.Name"));
			AllowBackgroundAudio->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Audio.Category.Sound.AllowBackgroundAudio.Description"));
			
			AllowBackgroundAudio->SetFalseAsDefaultValue();
			AllowBackgroundAudio->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetAllowBackgroundAudio));
			AllowBackgroundAudio->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetAllowBackgroundAudio));
			
			AllowBackgroundAudio->SetShouldApplySettingsImmediately(true);
			
			SoundCategoryCollection->AddChildListData(AllowBackgroundAudio);
		}
		
	}
	
	RegisteredOptionsTabCollections.Add(AudioTabCollection);
}




void UEMROptionsDataRegistry::InitControlCollectionTab(ULocalPlayer* InOwningLocalPlayer)
{
	UEMRListDataObjectCollection* ControlsTabCollection = NewObject<UEMRListDataObjectCollection>();
	if (!ControlsTabCollection) return;

	ControlsTabCollection->SetDataID(FName("ControlTabCollection"));
	ControlsTabCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls"));
	
	UEnhancedInputLocalPlayerSubsystem* EISubsystem = InOwningLocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(EISubsystem);
	
	UEnhancedInputUserSettings* EIUserSettings = EISubsystem->GetUserSettings();
	check(EIUserSettings);
	
	
	// Mouse and Keyboard Category
	{
		UEMRListDataObjectCollection* MouseKeyboardCategoryCollection = NewObject<UEMRListDataObjectCollection>();
		if (!MouseKeyboardCategoryCollection) return;

		MouseKeyboardCategoryCollection->SetDataID(FName("MouseKeyboardCategoryCollection"));
		MouseKeyboardCategoryCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.MouseAndKeyboard"));
		
		ControlsTabCollection->AddChildListData(MouseKeyboardCategoryCollection);
		
		// Mouse Sensitivity
		{
			UEMRListDataObjectScalar* MouseSensitivity = NewObject<UEMRListDataObjectScalar>();
			if (!MouseSensitivity) return;
			
			MouseSensitivity->SetDataID(FName("MouseSensitivity"));
			MouseSensitivity->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.MouseAndKeyboard.MouseSensitivity.Name"));
			MouseSensitivity->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.MouseAndKeyboard.MouseSensitivity.Description"));
			MouseSensitivity->SetDisplayValueRange(TRange<float>(0.1f, 5.f));
			MouseSensitivity->SetOutputValueRange(TRange<float>(0.1f, 5.f));
			MouseSensitivity->SetSliderStepSize(0.1f);
			MouseSensitivity->SetDisplayNumericType(ECommonNumericType::Number);
			MouseSensitivity->SetNumberFormattingOptions(UEMRListDataObjectScalar::WithDecimal(2));
			MouseSensitivity->SetDefaultValueFromString(LexToString(1.f));
			
			MouseSensitivity->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMouseSensitivity));
			MouseSensitivity->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMouseSensitivity));
			MouseSensitivity->SetShouldApplySettingsImmediately(true);
			
			MouseKeyboardCategoryCollection->AddChildListData(MouseSensitivity);
		}
		
		// Mouse Invert X
		{
			UEMRListDataObjectStringBool* MouseInvertX = NewObject<UEMRListDataObjectStringBool>();
			if (!MouseInvertX) return;
			
			MouseInvertX->SetDataID(FName("MouseInvertX"));
			MouseInvertX->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.MouseAndKeyboard.MouseInvertX.Name"));
			MouseInvertX->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.MouseAndKeyboard.MouseInvertX.Description"));
			
			MouseInvertX->SetFalseAsDefaultValue();
			MouseInvertX->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMouseInvertX));
			MouseInvertX->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMouseInvertX));
			MouseInvertX->SetShouldApplySettingsImmediately(true);
			
			MouseKeyboardCategoryCollection->AddChildListData(MouseInvertX);
		}

		// Mouse Invert Y
		{
			UEMRListDataObjectStringBool* MouseInvertY = NewObject<UEMRListDataObjectStringBool>();
			if (!MouseInvertY) return;
			
			MouseInvertY->SetDataID(FName("MouseInvertY"));
			MouseInvertY->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.MouseAndKeyboard.MouseInvertY.Name"));
			MouseInvertY->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.MouseAndKeyboard.MouseInvertY.Description"));
			
			MouseInvertY->SetFalseAsDefaultValue();
			MouseInvertY->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMouseInvertY));
			MouseInvertY->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMouseInvertY));
			MouseInvertY->SetShouldApplySettingsImmediately(true);
			
			MouseKeyboardCategoryCollection->AddChildListData(MouseInvertY);
		}

		// Key remapping
		UEnhancedPlayerMappableKeyProfile* MappableKeyProfile = EIUserSettings->GetActiveKeyProfile();
		
		check(MappableKeyProfile);
		
		FPlayerMappableKeyQueryOptions MouseKeyboardOnly;
		MouseKeyboardOnly.KeyToMatch = EKeys::S;
		MouseKeyboardOnly.bMatchBasicKeyTypes = true;
		
		/*FPlayerMappableKeyQueryOptions GamepadOnly;
		GamepadOnly.KeyToMatch = EKeys::Gamepad_DPad_Up;
		GamepadOnly.bMatchBasicKeyTypes = true;
		*/
		
		for (const TPair<FName, FKeyMappingRow>& MappingRowPair : MappableKeyProfile->GetPlayerMappingRows())
		{
			for (const FPlayerKeyMapping& KeyMapping :MappingRowPair.Value.Mappings)
			{
				if (MappableKeyProfile->DoesMappingPassQueryOptions(KeyMapping, MouseKeyboardOnly))
				{
					UEMRListDataObjectKeyRemap* KeyRemapDataObject = NewObject<UEMRListDataObjectKeyRemap>();
					if (!KeyRemapDataObject) continue;
					
					KeyRemapDataObject->SetDataID(KeyMapping.GetMappingName());
					KeyRemapDataObject->SetDataDisplayName(KeyMapping.GetDisplayName());
					KeyRemapDataObject->InitKeyRemapData(EIUserSettings, MappableKeyProfile, ECommonInputType::MouseAndKeyboard, KeyMapping);
					
					MouseKeyboardCategoryCollection->AddChildListData(KeyRemapDataObject);
				}
			}
		}
	}
	
	// Gamepad Category
	{
		UEMRListDataObjectCollection* GamepadCategoryCollection = NewObject<UEMRListDataObjectCollection>();
		if (!GamepadCategoryCollection) return;

		GamepadCategoryCollection->SetDataID(FName("GamepadCategoryCollection"));
		GamepadCategoryCollection->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.Gamepad"));
		
		ControlsTabCollection->AddChildListData(GamepadCategoryCollection);

		// Gamepad Sensitivity
		{
			UEMRListDataObjectScalar* GamepadSensitivity = NewObject<UEMRListDataObjectScalar>();
			if (!GamepadSensitivity) return;
			
			GamepadSensitivity->SetDataID(FName("GamepadSensitivity"));
			GamepadSensitivity->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.Gamepad.GamepadSensitivity.Name"));
			GamepadSensitivity->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.Gamepad.GamepadSensitivity.Description"));
			GamepadSensitivity->SetDisplayValueRange(TRange<float>(0.1f, 5.f));
			GamepadSensitivity->SetOutputValueRange(TRange<float>(0.1f, 5.f));
			GamepadSensitivity->SetSliderStepSize(0.1f);
			GamepadSensitivity->SetDisplayNumericType(ECommonNumericType::Number);
			GamepadSensitivity->SetNumberFormattingOptions(UEMRListDataObjectScalar::WithDecimal(2));
			GamepadSensitivity->SetDefaultValueFromString(LexToString(1.f));
			
			GamepadSensitivity->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetGamepadSensitivity));
			GamepadSensitivity->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetGamepadSensitivity));
			GamepadSensitivity->SetShouldApplySettingsImmediately(true);
			
			GamepadCategoryCollection->AddChildListData(GamepadSensitivity);
		}

		// Gamepad Invert X
		{
			UEMRListDataObjectStringBool* GamepadInvertX = NewObject<UEMRListDataObjectStringBool>();
			if (!GamepadInvertX) return;
			
			GamepadInvertX->SetDataID(FName("GamepadInvertX"));
			GamepadInvertX->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.Gamepad.GamepadInvertX.Name"));
			GamepadInvertX->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.Gamepad.GamepadInvertX.Description"));
			
			GamepadInvertX->SetFalseAsDefaultValue();
			GamepadInvertX->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetGamepadInvertX));
			GamepadInvertX->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetGamepadInvertX));
			GamepadInvertX->SetShouldApplySettingsImmediately(true);
			
			GamepadCategoryCollection->AddChildListData(GamepadInvertX);
		}

		// Gamepad Invert Y
		{
			UEMRListDataObjectStringBool* GamepadInvertY = NewObject<UEMRListDataObjectStringBool>();
			if (!GamepadInvertY) return;
			
			GamepadInvertY->SetDataID(FName("GamepadInvertY"));
			GamepadInvertY->SetDataDisplayName(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.Gamepad.GamepadInvertY.Name"));
			GamepadInvertY->SetDescriptionRichText(GetLocalizedText(UIStringTableId, "UI.Options.Tab.Controls.Category.Gamepad.GamepadInvertY.Description"));
			
			GamepadInvertY->SetFalseAsDefaultValue();
			GamepadInvertY->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetGamepadInvertY));
			GamepadInvertY->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetGamepadInvertY));
			GamepadInvertY->SetShouldApplySettingsImmediately(true);
			
			GamepadCategoryCollection->AddChildListData(GamepadInvertY);
		}
		
		
		// Key remapping
		UEnhancedPlayerMappableKeyProfile* MappableKeyProfile = EIUserSettings->GetActiveKeyProfile();
		
		check(MappableKeyProfile);
		
		FPlayerMappableKeyQueryOptions GamepadOnly;
		GamepadOnly.KeyToMatch = EKeys::Gamepad_DPad_Up;
		GamepadOnly.bMatchBasicKeyTypes = true;
		
		for (const TPair<FName, FKeyMappingRow>& MappingRowPair : MappableKeyProfile->GetPlayerMappingRows())
		{
			for (const FPlayerKeyMapping& KeyMapping :MappingRowPair.Value.Mappings)
			{
				if (MappableKeyProfile->DoesMappingPassQueryOptions(KeyMapping, GamepadOnly))
				{
					UEMRListDataObjectKeyRemap* KeyRemapDataObject = NewObject<UEMRListDataObjectKeyRemap>();
					if (!KeyRemapDataObject) continue;
					
					KeyRemapDataObject->SetDataID(KeyMapping.GetMappingName());
					KeyRemapDataObject->SetDataDisplayName(KeyMapping.GetDisplayName());
					KeyRemapDataObject->InitKeyRemapData(EIUserSettings, MappableKeyProfile, ECommonInputType::Gamepad, KeyMapping);
					
					GamepadCategoryCollection->AddChildListData(KeyRemapDataObject);
				}
			}
		}
	}
	
	RegisteredOptionsTabCollections.Add(ControlsTabCollection);
}


void UEMROptionsDataRegistry::FindChildListDataRecursively(UEMRListDataObjectBase* InParentData, TArray<UEMRListDataObjectBase*>& OutFoundChildListData) const
{
	if (!InParentData || !InParentData->HasAnyChildListData()) return;
	
	for (UEMRListDataObjectBase* SubChildListData : InParentData->GetAllChildListData())
	{
		if (!SubChildListData) continue;
		
		OutFoundChildListData.Add(SubChildListData);
		
		if (SubChildListData->HasAnyChildListData())
		{
			FindChildListDataRecursively(SubChildListData, OutFoundChildListData);
		}
	}
}

#undef PopulateUpscalerQualityOptions
#undef PopulateFSRQualityOptions
#undef PopulateDLSSQualityOptions
#undef GetCurrentUpscalerSelection
#undef IsNVIDIAReflexRuntimeAvailable
#undef IsDLSSFrameGenerationRuntimeAvailable
#undef IsDLSSUpscalerRuntimeAvailable
#undef IsFSRFrameGenerationRuntimeAvailable
#undef IsFSRUpscalerRuntimeAvailable
#undef IsRayTracingRuntimeAvailable
#undef HasConsoleVariable
#undef IsEngineFeatureQueryReady
#undef EqualsNoCase
#undef UpscalerQualityFSRUltraPerformance
#undef UpscalerQualityFSRPerformance
#undef UpscalerQualityFSRBalanced
#undef UpscalerQualityFSRQuality
#undef UpscalerQualityFSRNativeAA
#undef UpscalerQualityDLSSUltraPerformance
#undef UpscalerQualityDLSSPerformance
#undef UpscalerQualityDLSSBalanced
#undef UpscalerQualityDLSSQuality
#undef UpscalerQualityDLSSUltraQuality
#undef UpscalerQualityDLSSDLAA
#undef UpscalerQualityTSRDefault
#undef UpscalerFSR
#undef UpscalerDLSS
#undef UpscalerTSR
