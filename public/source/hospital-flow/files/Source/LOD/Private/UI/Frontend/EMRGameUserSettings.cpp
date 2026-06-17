


#include "UI/Frontend/EMRGameUserSettings.h"
#include "Characters/Player/EMRPlayerController.h"
#include "Framework/EMRNightShiftGameState.h"
#include "UI/Common/Screens/GameHud/EMRGameplayKeybindHelperWidget.h"
#include "UI/Frontend/EMRFPSCounterWidget.h"
#include "UI/Frontend/EMRFrontendDeveloperSettings.h"

#include "AudioDevice.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "DLSSLibrary.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "ProximityVoiceLocalPlayerSubsystem.h"
#include "RenderUtils.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "StreamlineLibraryDLSSG.h"
#include "StreamlineLibraryReflex.h"

namespace
{
	DEFINE_LOG_CATEGORY_STATIC(LogEMRVideoSettings, Log, All);

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

	constexpr TCHAR NVIDIAReflexModeOff[] = TEXT("Off");
	constexpr TCHAR NVIDIAReflexModeOn[] = TEXT("On");
	constexpr TCHAR NVIDIAReflexModeBoost[] = TEXT("Boost");

	bool EqualsNoCase(const FString& Value, const TCHAR* Expected)
	{
		return Value.Equals(Expected, ESearchCase::IgnoreCase);
	}

	TSubclassOf<UEMRFPSCounterWidget> ResolveFPSCounterWidgetClass()
	{
		if (const UEMRFrontendDeveloperSettings* FrontendSettings = GetDefault<UEMRFrontendDeveloperSettings>())
		{
			if (!FrontendSettings->FPSStatsWidgetClass.IsNull())
			{
				if (TSubclassOf<UEMRFPSCounterWidget> LoadedClass = FrontendSettings->FPSStatsWidgetClass.LoadSynchronous())
				{
					return LoadedClass;
				}
			}
		}

		return UEMRFPSCounterWidget::StaticClass();
	}

	TSubclassOf<UEMRGameplayKeybindHelperWidget> ResolveGameplayKeybindHelperWidgetClass()
	{
		if (const UEMRFrontendDeveloperSettings* FrontendSettings = GetDefault<UEMRFrontendDeveloperSettings>())
		{
			if (!FrontendSettings->GameplayKeybindHelperWidgetClass.IsNull())
			{
				if (TSubclassOf<UEMRGameplayKeybindHelperWidget> LoadedClass = FrontendSettings->GameplayKeybindHelperWidgetClass.LoadSynchronous())
				{
					return LoadedClass;
				}
			}
		}

		return nullptr;
	}

	int32 ResolveGameplayKeybindHelperWidgetZOrder()
	{
		if (const UEMRFrontendDeveloperSettings* FrontendSettings = GetDefault<UEMRFrontendDeveloperSettings>())
		{
			return FrontendSettings->GameplayKeybindHelperZOrder;
		}

		return 4200;
	}

	FString NormalizeUpscalerString(const FString& InUpscaler)
	{
		if (EqualsNoCase(InUpscaler, UpscalerDLSS))
		{
			return UpscalerDLSS;
		}
		if (EqualsNoCase(InUpscaler, UpscalerFSR))
		{
			return UpscalerFSR;
		}
		return UpscalerTSR;
	}

	bool IsExternalUpscalerSelection(const FString& Upscaler)
	{
		return EqualsNoCase(Upscaler, UpscalerDLSS) || EqualsNoCase(Upscaler, UpscalerFSR);
	}

	FString NormalizeUpscalerQualityString(const FString& InUpscalerQuality)
	{
		static const TCHAR* KnownQualities[] =
		{
			UpscalerQualityTSRDefault,
			UpscalerQualityDLSSDLAA,
			UpscalerQualityDLSSUltraQuality,
			UpscalerQualityDLSSQuality,
			UpscalerQualityDLSSBalanced,
			UpscalerQualityDLSSPerformance,
			UpscalerQualityDLSSUltraPerformance,
			UpscalerQualityFSRNativeAA,
			UpscalerQualityFSRQuality,
			UpscalerQualityFSRBalanced,
			UpscalerQualityFSRPerformance,
			UpscalerQualityFSRUltraPerformance
		};

		for (const TCHAR* KnownQuality : KnownQualities)
		{
			if (EqualsNoCase(InUpscalerQuality, KnownQuality))
			{
				return KnownQuality;
			}
		}

		return UpscalerQualityTSRDefault;
	}

	FString NormalizeNVIDIAReflexModeString(const FString& InMode)
	{
		if (EqualsNoCase(InMode, NVIDIAReflexModeBoost))
		{
			return NVIDIAReflexModeBoost;
		}
		if (EqualsNoCase(InMode, NVIDIAReflexModeOn) || EqualsNoCase(InMode, TEXT("Enabled")))
		{
			return NVIDIAReflexModeOn;
		}
		return NVIDIAReflexModeOff;
	}

	bool IsEngineFeatureQueryReady()
	{
		return GEngine && GEngine->IsInitialized();
	}

	bool HasConsoleVariable(const TCHAR* Name)
	{
		return IConsoleManager::Get().FindConsoleVariable(Name) != nullptr;
	}

	FString GetGraphicsRHIName()
	{
		const FString RHIName = FApp::GetGraphicsRHI();
		return RHIName.IsEmpty() ? TEXT("<unknown>") : RHIName;
	}

	bool IsD3D12GraphicsRHI()
	{
		return GetGraphicsRHIName().Contains(TEXT("D3D12"), ESearchCase::IgnoreCase);
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

	bool TryUpscalerQualityTokenToDLSSMode(const FString& Token, UDLSSMode& OutMode)
	{
		if (EqualsNoCase(Token, UpscalerQualityDLSSDLAA))
		{
			OutMode = UDLSSMode::DLAA;
			return true;
		}
		if (EqualsNoCase(Token, UpscalerQualityDLSSUltraQuality))
		{
			OutMode = UDLSSMode::UltraQuality;
			return true;
		}
		if (EqualsNoCase(Token, UpscalerQualityDLSSQuality))
		{
			OutMode = UDLSSMode::Quality;
			return true;
		}
		if (EqualsNoCase(Token, UpscalerQualityDLSSBalanced))
		{
			OutMode = UDLSSMode::Balanced;
			return true;
		}
		if (EqualsNoCase(Token, UpscalerQualityDLSSPerformance))
		{
			OutMode = UDLSSMode::Performance;
			return true;
		}
		if (EqualsNoCase(Token, UpscalerQualityDLSSUltraPerformance))
		{
			OutMode = UDLSSMode::UltraPerformance;
			return true;
		}
		return false;
	}

	FString DLSSModeToUpscalerQualityToken(UDLSSMode Mode)
	{
		switch (Mode)
		{
		case UDLSSMode::DLAA:
			return UpscalerQualityDLSSDLAA;
		case UDLSSMode::UltraQuality:
			return UpscalerQualityDLSSUltraQuality;
		case UDLSSMode::Quality:
			return UpscalerQualityDLSSQuality;
		case UDLSSMode::Balanced:
			return UpscalerQualityDLSSBalanced;
		case UDLSSMode::Performance:
			return UpscalerQualityDLSSPerformance;
		case UDLSSMode::UltraPerformance:
			return UpscalerQualityDLSSUltraPerformance;
		default:
			return UpscalerQualityDLSSQuality;
		}
	}

	bool TryUpscalerQualityTokenToFSRQualityMode(const FString& Token, int32& OutQualityMode)
	{
		if (EqualsNoCase(Token, UpscalerQualityFSRNativeAA))
		{
			OutQualityMode = 0;
			return true;
		}
		if (EqualsNoCase(Token, UpscalerQualityFSRQuality))
		{
			OutQualityMode = 1;
			return true;
		}
		if (EqualsNoCase(Token, UpscalerQualityFSRBalanced))
		{
			OutQualityMode = 2;
			return true;
		}
		if (EqualsNoCase(Token, UpscalerQualityFSRPerformance))
		{
			OutQualityMode = 3;
			return true;
		}
		if (EqualsNoCase(Token, UpscalerQualityFSRUltraPerformance))
		{
			OutQualityMode = 4;
			return true;
		}
		return false;
	}

	int32 ResolveEffectiveScalabilityLevel(const UEMRGameUserSettings& Settings);

	const TCHAR* BoolToYesNo(bool bValue)
	{
		return bValue ? TEXT("Yes") : TEXT("No");
	}

	FString GetConsoleVarAsString(const TCHAR* Name)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			return CVar->GetString();
		}

		return TEXT("<missing>");
	}

	template <typename TEnum>
	FString EnumValueToNameString(TEnum Value)
	{
		if (const UEnum* Enum = StaticEnum<TEnum>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(Value));
		}

		return FString::Printf(TEXT("%d"), static_cast<int32>(Value));
	}

	void LogUpscalerDiagnostics(const UEMRGameUserSettings& Settings, const TCHAR* ContextLabel)
	{
		const FString SelectedUpscaler = Settings.GetUpscaler();
		const FString SelectedQuality = Settings.GetUpscalerQuality();
		const FString SelectedReflex = Settings.GetNVIDIAReflexMode();
		const bool bEngineReady = IsEngineFeatureQueryReady();
		const int32 EffectiveScalabilityLevel = ResolveEffectiveScalabilityLevel(Settings);

		UE_LOG(LogEMRVideoSettings, Warning,
			TEXT("[UpscalerDiag][%s] Requested: Upscaler=%s Quality=%s FrameGenRequested=%s Reflex=%s | PIE=%s Editor=%s EngineReady=%s | DynResEnabled=%s DynResOp=%s | Scalability=%d"),
			ContextLabel,
			*SelectedUpscaler,
			*SelectedQuality,
			BoolToYesNo(Settings.GetFrameGenerationEnabled()),
			*SelectedReflex,
			BoolToYesNo(GIsPlayInEditorWorld),
			BoolToYesNo(GIsEditor),
			BoolToYesNo(bEngineReady),
			BoolToYesNo(Settings.IsDynamicResolutionEnabled()),
			*GetConsoleVarAsString(TEXT("r.DynamicRes.OperationMode")),
			EffectiveScalabilityLevel);

		UE_LOG(LogEMRVideoSettings, Warning,
			TEXT("[UpscalerDiag][%s] RuntimeRHI: FApp::GetGraphicsRHI=%s"),
			ContextLabel,
			*GetGraphicsRHIName());

		UE_LOG(LogEMRVideoSettings, Warning,
			TEXT("[UpscalerDiag][%s] CVars: r.AntiAliasingMethod=%s r.TemporalAA.Upscaler=%s r.ScreenPercentage=%s r.NGX.DLSS.Enable=%s r.FidelityFX.FSR.Enabled=%s r.FidelityFX.FSR.QualityMode=%s r.FidelityFX.FI.Enabled=%s r.FidelityFX.FSR.EnabledInEditorViewport=%s"),
			ContextLabel,
			*GetConsoleVarAsString(TEXT("r.AntiAliasingMethod")),
			*GetConsoleVarAsString(TEXT("r.TemporalAA.Upscaler")),
			*GetConsoleVarAsString(TEXT("r.ScreenPercentage")),
			*GetConsoleVarAsString(TEXT("r.NGX.DLSS.Enable")),
			*GetConsoleVarAsString(TEXT("r.FidelityFX.FSR.Enabled")),
			*GetConsoleVarAsString(TEXT("r.FidelityFX.FSR.QualityMode")),
			*GetConsoleVarAsString(TEXT("r.FidelityFX.FI.Enabled")),
			*GetConsoleVarAsString(TEXT("r.FidelityFX.FSR.EnabledInEditorViewport")));

		UE_LOG(LogEMRVideoSettings, Warning,
			TEXT("[UpscalerDiag][%s] Streamline CVars: r.Streamline.DLSSG.Enable=%s r.Streamline.DLSSG.FramesToGenerate=%s t.Streamline.Reflex.Mode=%s t.Streamline.Reflex.Auto=%s"),
			ContextLabel,
			*GetConsoleVarAsString(TEXT("r.Streamline.DLSSG.Enable")),
			*GetConsoleVarAsString(TEXT("r.Streamline.DLSSG.FramesToGenerate")),
			*GetConsoleVarAsString(TEXT("t.Streamline.Reflex.Mode")),
			*GetConsoleVarAsString(TEXT("t.Streamline.Reflex.Auto")));

		if (GIsPlayInEditorWorld && Settings.GetFrameGenerationEnabled())
		{
			UE_LOG(LogEMRVideoSettings, Warning,
				TEXT("[UpscalerDiag][%s] PIE NOTE: Frame Generation is intentionally gated off in regular PIE viewport. Compare FG performance only in Standalone/separate game window."),
				ContextLabel);
		}

		if (!bEngineReady)
		{
			return;
		}

		const UDLSSSupport DLSSSupport = UDLSSLibrary::QueryDLSSSupport();
		const bool bDLSSEnabled = UDLSSLibrary::IsDLSSEnabled();
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UDLSSMode CurrentDLSSMode = UDLSSLibrary::GetDLSSMode();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		const EStreamlineFeatureSupport DLSSGSupport = UStreamlineLibraryDLSSG::QueryDLSSGSupport();
		const EStreamlineDLSSGMode CurrentDLSSGMode = UStreamlineLibraryDLSSG::GetDLSSGMode();
		const EStreamlineFeatureSupport ReflexSupport = UStreamlineLibraryReflex::QueryReflexSupport();
		const EStreamlineReflexMode CurrentReflexMode = UStreamlineLibraryReflex::GetReflexMode();

		UE_LOG(LogEMRVideoSettings, Warning,
			TEXT("[UpscalerDiag][%s] Runtime: DLSSSupport=%s DLSSEnabled=%s DLSSMode=%s | DLSSGSupport=%s DLSSGMode=%s | ReflexSupport=%s ReflexMode=%s | FSRUpscalerRuntime=%s FSRFGRuntime=%s"),
			ContextLabel,
			*EnumValueToNameString(DLSSSupport),
			BoolToYesNo(bDLSSEnabled),
			*EnumValueToNameString(CurrentDLSSMode),
			*EnumValueToNameString(DLSSGSupport),
			*EnumValueToNameString(CurrentDLSSGMode),
			*EnumValueToNameString(ReflexSupport),
			*EnumValueToNameString(CurrentReflexMode),
			BoolToYesNo(IsFSRUpscalerRuntimeAvailable()),
			BoolToYesNo(IsFSRFrameGenerationRuntimeAvailable()));

		if (DLSSGSupport == EStreamlineFeatureSupport::Supported)
		{
			float DLSSGFrameRateHz = 0.0f;
			int32 DLSSGFramesPresented = 0;
			UStreamlineLibraryDLSSG::GetDLSSGFrameTiming(DLSSGFrameRateHz, DLSSGFramesPresented);
			UE_LOG(LogEMRVideoSettings, Warning,
				TEXT("[UpscalerDiag][%s] DLSSG Timing: FrameRateHz=%.2f FramesPresented=%d"),
				ContextLabel,
				DLSSGFrameRateHz,
				DLSSGFramesPresented);
		}
	}

	struct FEMRScalabilityPresetProfile
	{
		int32 ViewDistanceQuality = 2;
		int32 ShadowQuality = 2;
		int32 GlobalIlluminationQuality = 2;
		int32 ReflectionQuality = 2;
		int32 AntiAliasingQuality = 2;
		int32 TextureQuality = 2;
		int32 VisualEffectQuality = 2;
		int32 PostProcessQuality = 2;
		int32 FoliageQuality = 2;
		int32 ShadingQuality = 2;
		float ResolutionScaleNormalized = 1.0f;
	};

	void SetIntCVar(const TCHAR* Name, int32 Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByGameSetting);
		}
	}

	void SetFloatCVar(const TCHAR* Name, float Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			CVar->Set(Value, ECVF_SetByGameSetting);
		}
	}

	int32 ResolveEffectiveScalabilityLevel(const UEMRGameUserSettings& Settings)
	{
		const int32 OverallLevel = Settings.GetOverallScalabilityLevel();
		if (OverallLevel >= 0 && OverallLevel <= 4)
		{
			return OverallLevel;
		}

		// Benchmark/autodetect can produce mixed group levels. Derive a conservative
		// tier so runtime policies (TSR/DR ranges) still apply predictably.
		int32 InferredLevel = 4;
		InferredLevel = FMath::Min(InferredLevel, Settings.GetViewDistanceQuality());
		InferredLevel = FMath::Min(InferredLevel, Settings.GetShadowQuality());
		InferredLevel = FMath::Min(InferredLevel, Settings.GetGlobalIlluminationQuality());
		InferredLevel = FMath::Min(InferredLevel, Settings.GetReflectionQuality());
		InferredLevel = FMath::Min(InferredLevel, Settings.GetAntiAliasingQuality());
		InferredLevel = FMath::Min(InferredLevel, Settings.GetTextureQuality());
		InferredLevel = FMath::Min(InferredLevel, Settings.GetVisualEffectQuality());
		InferredLevel = FMath::Min(InferredLevel, Settings.GetPostProcessingQuality());
		InferredLevel = FMath::Min(InferredLevel, Settings.GetFoliageQuality());
		InferredLevel = FMath::Min(InferredLevel, Settings.GetShadingQuality());
		return FMath::Clamp(InferredLevel, 0, 4);
	}

	void ApplyScalabilityRuntimePolicy(UEMRGameUserSettings& Settings, int32 ScalabilityLevel)
	{
		if (ScalabilityLevel < 0 || ScalabilityLevel > 4)
		{
			return;
		}

		if (IsExternalUpscalerSelection(Settings.GetUpscaler()))
		{
			Settings.SetDynamicResolutionEnabled(false);
			if (GEngine)
			{
				GEngine->SetDynamicResolutionUserSetting(false);
			}
			SetIntCVar(TEXT("r.DynamicRes.OperationMode"), 0);
			return;
		}

		// Make dynamic resolution follow user settings, then apply project tier policy.
		SetIntCVar(TEXT("r.DynamicRes.OperationMode"), 1);

		if (ScalabilityLevel <= 1)
		{
			// Minimum hardware path: TSR + dynamic resolution safety net.
			Settings.SetDynamicResolutionEnabled(true);
			if (GEngine)
			{
				GEngine->SetDynamicResolutionUserSetting(Settings.IsDynamicResolutionEnabled());
			}
			SetIntCVar(TEXT("r.AntiAliasingMethod"), 4);
			SetFloatCVar(TEXT("r.DynamicRes.MinScreenPercentage"), ScalabilityLevel == 0 ? 50.0f : 70.0f);
			SetFloatCVar(TEXT("r.DynamicRes.MaxScreenPercentage"), 100.0f);
			SetFloatCVar(TEXT("r.DynamicRes.FrameTimeBudget"), 16.67f);
			return;
		}

		// Recommended / high-end defaults: native rendering and no dynamic resolution.
		Settings.SetDynamicResolutionEnabled(false);
		if (GEngine)
		{
			GEngine->SetDynamicResolutionUserSetting(Settings.IsDynamicResolutionEnabled());
		}
		SetIntCVar(TEXT("r.AntiAliasingMethod"), 2);
		SetFloatCVar(TEXT("r.DynamicRes.MinScreenPercentage"), 85.0f);
		SetFloatCVar(TEXT("r.DynamicRes.MaxScreenPercentage"), 100.0f);
		SetFloatCVar(TEXT("r.DynamicRes.FrameTimeBudget"), ScalabilityLevel >= 3 ? 8.33f : 11.11f);
	}

	EColorVisionDeficiency ToColorVisionDeficiency(const FString& Mode)
	{
		if (Mode.Equals(TEXT("Deuteranopia"), ESearchCase::IgnoreCase))
		{
			return EColorVisionDeficiency::Deuteranope;
		}

		if (Mode.Equals(TEXT("Protanopia"), ESearchCase::IgnoreCase))
		{
			return EColorVisionDeficiency::Protanope;
		}

		if (Mode.Equals(TEXT("Tritanopia"), ESearchCase::IgnoreCase))
		{
			return EColorVisionDeficiency::Tritanope;
		}

		return EColorVisionDeficiency::NormalVision;
	}

	template <typename FuncType>
	void ForEachGameWorld(FuncType&& Func)
	{
		if (!GEngine)
		{
			return;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld())
			{
				continue;
			}

			Func(World);
		}
	}

	FEMRScalabilityPresetProfile GetCustomPresetProfile(int32 PresetLevel)
	{
		// Presets 0/1 intentionally map to custom low/medium tiers.
		// Lumen stays enabled at all levels through project DefaultScalability.ini overrides.
		switch (PresetLevel)
		{
		case 0:
			return FEMRScalabilityPresetProfile
			{
				0, // ViewDistanceQuality
				0, // ShadowQuality
				0, // GlobalIlluminationQuality
				0, // ReflectionQuality
				1, // AntiAliasingQuality
				0, // TextureQuality
				0, // VisualEffectQuality
				1, // PostProcessQuality
				0, // FoliageQuality
				0, // ShadingQuality
				1.0f // ResolutionScaleNormalized
			};
		case 1:
			return FEMRScalabilityPresetProfile
			{
				1, // ViewDistanceQuality
				1, // ShadowQuality
				1, // GlobalIlluminationQuality
				1, // ReflectionQuality
				1, // AntiAliasingQuality
				1, // TextureQuality
				1, // VisualEffectQuality
				1, // PostProcessQuality
				1, // FoliageQuality
				1, // ShadingQuality
				1.0f // ResolutionScaleNormalized
			};
		default:
			return FEMRScalabilityPresetProfile{};
		}
	}

	TAutoConsoleVariable<FString> CVarEMRGameDifficulty(
		TEXT("emr.GameDifficulty"),
		TEXT("Standard"),
		TEXT("Preferred gameplay difficulty from the options menu."),
		ECVF_Default);

	TAutoConsoleVariable<FString> CVarEMRMicrophoneMode(
		TEXT("emr.MicrophoneMode"),
		TEXT("PushToTalk"),
		TEXT("Preferred microphone mode from the options menu."),
		ECVF_Default);
}


UEMRGameUserSettings::UEMRGameUserSettings()
	: OverallVolume(1.f)
	,MusicVolume(1.f)
	,SFXVolume(1.f)
	,ProximityChatVolume(1.f)
	,MicrophoneVolume(1.f)
	,bMicMonitorEnabled(false)
	,bAllowBackgroundAudio(false)
	,bUseHDRAudio(false)
	,FieldOfView(90.f)
	,bMotionBlurEnabled(true)
	,bRayTracingEnabled(false)
	,bRayTracingBenchmarkDefaultInitialized(false)
	,bFrameGenerationEnabled(false)
	,MouseSensitivity(1.f)
	,bMouseInvertX(false)
	,bMouseInvertY(false)
	,GamepadSensitivity(1.f)
	,bGamepadInvertX(false)
	,bGamepadInvertY(false)
{
	CurrentGameDifficulty = TEXT("Standard");
	CurrentLanguage = FInternationalization::Get().GetCurrentCulture()->GetName();
	bShowFPSCounter = false;
	bShowKeybindHelper = true;
	ColorBlindMode = TEXT("Off");
	MicrophoneMode = TEXT("PushToTalk");
	PreferredMicrophoneInputDevice = TEXT("");
	Upscaler = UpscalerTSR;
	UpscalerQuality = UpscalerQualityTSRDefault;
	NVIDIAReflexMode = NVIDIAReflexModeOff;
	StartupShaderOptimizationKey = TEXT("");
	bStartupShaderOptimizationCompleted = false;
}


UEMRGameUserSettings* UEMRGameUserSettings::Get()
{
	if (GEngine)
	{
		return CastChecked<UEMRGameUserSettings>(GEngine->GetGameUserSettings());
	}
	
	return nullptr;
}

void UEMRGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);
}

void UEMRGameUserSettings::ApplyHardwareBenchmarkResults()
{
	Super::ApplyHardwareBenchmarkResults();
	ApplyScalabilityRuntimePolicy(*this, ResolveEffectiveScalabilityLevel(*this));
	ApplyRayTracingBenchmarkDefaultIfNeeded();
	ApplyRayTracingSettings();
	SaveSettings();
}

void UEMRGameUserSettings::ApplyImmediateSettingByDataID(const FName& DataID)
{
	if (DataID.IsNone())
	{
		return;
	}

	const auto IsID = [&DataID](const TCHAR* IDText)
	{
		return DataID == FName(IDText);
	};

	if (IsID(TEXT("Language")) || IsID(TEXT("DisplayGamma")))
	{
		// Applied directly by their setters.
		return;
	}

	if (IsID(TEXT("ShowFPSCounter")))
	{
		ApplyShowFPSCounter();
		return;
	}

	if (IsID(TEXT("ShowKeybindHelper")))
	{
		ApplyShowKeybindHelper();
		return;
	}

	if (IsID(TEXT("ColorBlindMode")))
	{
		ApplyColorBlindMode();
		return;
	}

	if (IsID(TEXT("WindowMode")) || IsID(TEXT("ScreenResolution")))
	{
		ApplyResolutionSettings(true);
		return;
	}

	if (IsID(TEXT("VerticalSync")))
	{
		if (IConsoleVariable* VSyncVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))
		{
			VSyncVar->Set(IsVSyncEnabled() ? 1 : 0, ECVF_SetByGameSetting);
		}
		return;
	}

	if (IsID(TEXT("FrameRateLimit")))
	{
		SetFrameRateLimitCVar(GetEffectiveFrameRateLimit());
		return;
	}

	if (IsID(TEXT("FieldOfView")))
	{
		ApplyFieldOfView();
		return;
	}

	if (IsID(TEXT("MotionBlur")))
	{
		ApplyMotionBlur();
		return;
	}

	if (IsID(TEXT("Upscaler")) ||
		IsID(TEXT("UpscalerQuality")) ||
		IsID(TEXT("RayTracing")) ||
		IsID(TEXT("FrameGeneration")) ||
		IsID(TEXT("NVIDIAReflex")))
	{
		ApplyScalabilityRuntimePolicy(*this, ResolveEffectiveScalabilityLevel(*this));
		ApplyVideoSettings();
		return;
	}

	if (IsID(TEXT("OverallQuality")) ||
		IsID(TEXT("GlobalIlluminationQuality")) ||
		IsID(TEXT("ShadowQuality")) ||
		IsID(TEXT("AntiAliasingQuality")) ||
		IsID(TEXT("ViewDistanceQuality")) ||
		IsID(TEXT("TextureQuality")) ||
		IsID(TEXT("VisualEffectQuality")) ||
		IsID(TEXT("ReflectionQuality")) ||
		IsID(TEXT("PostProcessingQuality")))
	{
		if (GEngine && GEngine->IsInitialized())
		{
			Scalability::SetQualityLevels(ScalabilityQuality);
			IConsoleManager::Get().CallAllConsoleVariableSinks();
		}

		if (IsID(TEXT("OverallQuality")))
		{
			ApplyScalabilityRuntimePolicy(*this, ResolveEffectiveScalabilityLevel(*this));
		}

		ApplyVideoSettings();
		return;
	}

	if (IsID(TEXT("OverallVolume")) ||
		IsID(TEXT("MusicVolume")) ||
		IsID(TEXT("SFXVolume")) ||
		IsID(TEXT("ProximityChatVolume")) ||
		IsID(TEXT("MicrophoneVolume")) ||
		IsID(TEXT("MicrophoneMode")) ||
		IsID(TEXT("MicrophoneInputDevice")) ||
		IsID(TEXT("MicMonitorPreview")) ||
		IsID(TEXT("AllowBackgroundAudio")))
	{
		ApplyAudioSettings();
		return;
	}

	if (IsID(TEXT("MouseSensitivity")) ||
		IsID(TEXT("MouseInvertX")) ||
		IsID(TEXT("MouseInvertY")) ||
		IsID(TEXT("GamepadSensitivity")) ||
		IsID(TEXT("GamepadInvertX")) ||
		IsID(TEXT("GamepadInvertY")))
	{
		ApplyControlSettings();
	}
}

void UEMRGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	ApplyScalabilityRuntimePolicy(*this, ResolveEffectiveScalabilityLevel(*this));

	ApplyGameplaySettings();
	ApplyAudioSettings();
	const bool bWasRayTracingBenchmarkDefaultInitialized = bRayTracingBenchmarkDefaultInitialized;
	ApplyRayTracingBenchmarkDefaultIfNeeded();
	if (!bWasRayTracingBenchmarkDefaultInitialized && bRayTracingBenchmarkDefaultInitialized)
	{
		SaveSettings();
	}
	ApplyVideoSettings();
	ApplyControlSettings();
}


void UEMRGameUserSettings::SetOverallVolume(float InVolume)
{
	OverallVolume = FMath::Clamp(InVolume, 0.0f, 2.0f);
	
	// The actual logic for controlling the volume goes here
}


void UEMRGameUserSettings::SetMusicVolume(float InVolume)
{
	MusicVolume = FMath::Clamp(InVolume, 0.0f, 2.0f);
}


void UEMRGameUserSettings::SetSFXVolume(float InVolume)
{
	SFXVolume = FMath::Clamp(InVolume, 0.0f, 2.0f);
}


void UEMRGameUserSettings::SetProximityChatVolume(float InVolume)
{
	ProximityChatVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
}


void UEMRGameUserSettings::SetMicrophoneVolume(float InVolume)
{
	MicrophoneVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
}


void UEMRGameUserSettings::SetAllowBackgroundAudio(bool ShouldAllowBackgroundAudio)
{
	bAllowBackgroundAudio = ShouldAllowBackgroundAudio;
}


void UEMRGameUserSettings::SetUseHDRAudio(bool ShouldUseHDRAudio)
{
	bUseHDRAudio = ShouldUseHDRAudio;
}


void UEMRGameUserSettings::SetCurrentLanguage(const FString& InNewLanguage)
{
	if (InNewLanguage.IsEmpty())
	{
		return;
	}

	const bool bSucceeded = FInternationalization::Get().SetCurrentCulture(InNewLanguage);
	if (bSucceeded)
	{
		CurrentLanguage = InNewLanguage;
	}
	else
	{
		CurrentLanguage = FInternationalization::Get().GetCurrentCulture()->GetName();
	}
}


void UEMRGameUserSettings::SetShowFPSCounter(bool bInShowFPSCounter)
{
	bShowFPSCounter = bInShowFPSCounter;
}

void UEMRGameUserSettings::SetShowKeybindHelper(bool bInShowKeybindHelper)
{
	bShowKeybindHelper = bInShowKeybindHelper;
}


float UEMRGameUserSettings::GetCurrentDisplayGamma()
{
	if (GEngine)
	{
		return GEngine->GetDisplayGamma();
	}
	
	return 2.2f;
}


void UEMRGameUserSettings::SetCurrentDisplayGamma(float InNewDisplayGamma)
{
	if (GEngine)
	{
		GEngine->DisplayGamma = FMath::Clamp(InNewDisplayGamma, 0.5f, 5.0f);
	}
}


void UEMRGameUserSettings::SetFieldOfView(float InFieldOfView)
{
	FieldOfView = FMath::Clamp(InFieldOfView, 60.0f, 120.0f);
}

bool UEMRGameUserSettings::GetRayTracingEnabled() const
{
	return bRayTracingEnabled && IsRayTracingRuntimeAvailable();
}

void UEMRGameUserSettings::SetRayTracingEnabled(bool bInRayTracingEnabled)
{
	bRayTracingEnabled = bInRayTracingEnabled && IsRayTracingRuntimeAvailable();
}

FString UEMRGameUserSettings::GetUpscaler() const
{
	const FString NormalizedUpscaler = NormalizeUpscalerString(Upscaler);

	if (!IsEngineFeatureQueryReady())
	{
		return NormalizedUpscaler;
	}

	if (EqualsNoCase(NormalizedUpscaler, UpscalerDLSS) && !IsDLSSUpscalerRuntimeAvailable())
	{
		return UpscalerTSR;
	}

	if (EqualsNoCase(NormalizedUpscaler, UpscalerFSR) && !IsFSRUpscalerRuntimeAvailable())
	{
		return UpscalerTSR;
	}

	return NormalizedUpscaler;
}

void UEMRGameUserSettings::SetUpscaler(const FString& InUpscaler)
{
	Upscaler = NormalizeUpscalerString(InUpscaler);
}

FString UEMRGameUserSettings::GetUpscalerQuality() const
{
	const FString SelectedUpscaler = GetUpscaler();
	const FString NormalizedQuality = NormalizeUpscalerQualityString(UpscalerQuality);

	if (EqualsNoCase(SelectedUpscaler, UpscalerTSR))
	{
		return UpscalerQualityTSRDefault;
	}

	if (EqualsNoCase(SelectedUpscaler, UpscalerFSR))
	{
		int32 IgnoredFSRQuality = 1;
		return TryUpscalerQualityTokenToFSRQualityMode(NormalizedQuality, IgnoredFSRQuality)
			? NormalizedQuality
			: FString(UpscalerQualityFSRQuality);
	}

	if (EqualsNoCase(SelectedUpscaler, UpscalerDLSS))
	{
		UDLSSMode ParsedMode = UDLSSMode::Quality;
		if (!TryUpscalerQualityTokenToDLSSMode(NormalizedQuality, ParsedMode))
		{
			return UpscalerQualityDLSSQuality;
		}

		if (!IsEngineFeatureQueryReady() || !IsDLSSUpscalerRuntimeAvailable())
		{
			return NormalizedQuality;
		}

		if (UDLSSLibrary::IsDLSSModeSupported(ParsedMode))
		{
			return NormalizedQuality;
		}

		const TArray<UDLSSMode> FallbackModes =
		{
			UDLSSMode::Quality,
			UDLSSMode::Balanced,
			UDLSSMode::Performance,
			UDLSSMode::UltraPerformance,
			UDLSSMode::UltraQuality,
			UDLSSMode::DLAA
		};

		for (const UDLSSMode FallbackMode : FallbackModes)
		{
			if (UDLSSLibrary::IsDLSSModeSupported(FallbackMode))
			{
				return DLSSModeToUpscalerQualityToken(FallbackMode);
			}
		}

		return UpscalerQualityDLSSQuality;
	}

	return UpscalerQualityTSRDefault;
}

void UEMRGameUserSettings::SetUpscalerQuality(const FString& InUpscalerQuality)
{
	UpscalerQuality = NormalizeUpscalerQualityString(InUpscalerQuality);
}

FString UEMRGameUserSettings::GetNVIDIAReflexMode() const
{
	const FString NormalizedMode = NormalizeNVIDIAReflexModeString(NVIDIAReflexMode);

	if (!IsEngineFeatureQueryReady() || !IsNVIDIAReflexRuntimeAvailable())
	{
		return NVIDIAReflexModeOff;
	}

	const EStreamlineReflexMode DesiredMode = EqualsNoCase(NormalizedMode, NVIDIAReflexModeBoost)
	? EStreamlineReflexMode::Boost
	: (EqualsNoCase(NormalizedMode, NVIDIAReflexModeOn) ? EStreamlineReflexMode::Enabled : EStreamlineReflexMode::Off);

	if (UStreamlineLibraryReflex::IsReflexModeSupported(DesiredMode))
	{
		return NormalizedMode;
	}

	return UStreamlineLibraryReflex::IsReflexModeSupported(EStreamlineReflexMode::Enabled)
	? FString(NVIDIAReflexModeOn)
	: FString(NVIDIAReflexModeOff);
}

void UEMRGameUserSettings::SetNVIDIAReflexMode(const FString& InNVIDIAReflexMode)
{
	NVIDIAReflexMode = NormalizeNVIDIAReflexModeString(InNVIDIAReflexMode);
}

void UEMRGameUserSettings::SetOverallScalabilityLevel(int32 Value)
{
	const int32 ClampedValue = FMath::Clamp(Value, 0, 4);

	if (ClampedValue >= 2)
	{
		Super::SetOverallScalabilityLevel(ClampedValue);
		SetResolutionScaleNormalized(1.0f);
		ApplyScalabilityRuntimePolicy(*this, ClampedValue);
		return;
	}

	ApplyCustomScalabilityPreset(ClampedValue);
	ApplyScalabilityRuntimePolicy(*this, ClampedValue);
}

int32 UEMRGameUserSettings::GetOverallScalabilityLevel() const
{
	if (IsCustomScalabilityPreset(0))
	{
		return 0;
	}

	if (IsCustomScalabilityPreset(1))
	{
		return 1;
	}

	for (int32 QualityLevel = 4; QualityLevel >= 0; --QualityLevel)
	{
		if (AreCoreScalabilityQualitiesUniform(QualityLevel))
		{
			return QualityLevel;
		}
	}

	return -1;
}

void UEMRGameUserSettings::ApplyCustomScalabilityPreset(int32 PresetLevel)
{
	const FEMRScalabilityPresetProfile Profile = GetCustomPresetProfile(PresetLevel);

	SetViewDistanceQuality(Profile.ViewDistanceQuality);
	SetShadowQuality(Profile.ShadowQuality);
	SetGlobalIlluminationQuality(Profile.GlobalIlluminationQuality);
	SetReflectionQuality(Profile.ReflectionQuality);
	SetAntiAliasingQuality(Profile.AntiAliasingQuality);
	SetTextureQuality(Profile.TextureQuality);
	SetVisualEffectQuality(Profile.VisualEffectQuality);
	SetPostProcessingQuality(Profile.PostProcessQuality);
	SetFoliageQuality(Profile.FoliageQuality);
	SetShadingQuality(Profile.ShadingQuality);
	SetResolutionScaleNormalized(Profile.ResolutionScaleNormalized);
}

bool UEMRGameUserSettings::IsCustomScalabilityPreset(int32 PresetLevel) const
{
	if (PresetLevel < 0 || PresetLevel > 1)
	{
		return false;
	}

	const FEMRScalabilityPresetProfile Profile = GetCustomPresetProfile(PresetLevel);

	return
	GetViewDistanceQuality() == Profile.ViewDistanceQuality &&
	GetShadowQuality() == Profile.ShadowQuality &&
	GetGlobalIlluminationQuality() == Profile.GlobalIlluminationQuality &&
	GetReflectionQuality() == Profile.ReflectionQuality &&
	GetAntiAliasingQuality() == Profile.AntiAliasingQuality &&
	GetTextureQuality() == Profile.TextureQuality &&
	GetVisualEffectQuality() == Profile.VisualEffectQuality &&
	GetPostProcessingQuality() == Profile.PostProcessQuality &&
	GetFoliageQuality() == Profile.FoliageQuality &&
	GetShadingQuality() == Profile.ShadingQuality;
}

bool UEMRGameUserSettings::AreCoreScalabilityQualitiesUniform(int32 QualityLevel) const
{
	return
	GetViewDistanceQuality() == QualityLevel &&
	GetShadowQuality() == QualityLevel &&
	GetGlobalIlluminationQuality() == QualityLevel &&
	GetReflectionQuality() == QualityLevel &&
	GetAntiAliasingQuality() == QualityLevel &&
	GetTextureQuality() == QualityLevel &&
	GetVisualEffectQuality() == QualityLevel &&
	GetPostProcessingQuality() == QualityLevel &&
	GetFoliageQuality() == QualityLevel &&
	GetShadingQuality() == QualityLevel;
}

void UEMRGameUserSettings::ApplyGameplaySettings()
{
	CVarEMRGameDifficulty->Set(*CurrentGameDifficulty, ECVF_SetByGameSetting);
	CVarEMRMicrophoneMode->Set(*MicrophoneMode, ECVF_SetByGameSetting);

	ApplyShowFPSCounter();
	ApplyShowKeybindHelper();
	ApplyColorBlindMode();
}

void UEMRGameUserSettings::ApplyVideoSettings()
{
	ApplyFieldOfView();
	ApplyMotionBlur();
	ApplyRayTracingSettings();
	ApplyUpscalerSettings();
	ApplyNVIDIAReflexSettings();
	LogUpscalerDiagnostics(*this, TEXT("ApplyVideoSettings"));
}

void UEMRGameUserSettings::ApplyAudioSettings()
{
	ProximityChatVolume = FMath::Clamp(ProximityChatVolume, 0.0f, 1.0f);
	MicrophoneVolume = FMath::Clamp(MicrophoneVolume, 0.0f, 1.0f);

	if (IConsoleVariable* MuteWhenUnfocusedVar = IConsoleManager::Get().FindConsoleVariable(TEXT("au.MuteAudioWhenNotInFocus")))
	{
		MuteWhenUnfocusedVar->Set(bAllowBackgroundAudio ? 0 : 1, ECVF_SetByGameSetting);
	}

	if (IConsoleVariable* HDRAudioVar = IConsoleManager::Get().FindConsoleVariable(TEXT("au.EnableHDR")))
	{
		HDRAudioVar->Set(bUseHDRAudio ? 1 : 0, ECVF_SetByGameSetting);
	}

	if (IConsoleVariable* MicInputGainVar = IConsoleManager::Get().FindConsoleVariable(TEXT("voice.MicInputGain")))
	{
		MicInputGainVar->Set(MicrophoneVolume, ECVF_SetByGameSetting);
	}

	ForEachGameWorld([this](UWorld* World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (!PlayerController || !PlayerController->IsLocalController())
			{
				continue;
			}

			ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
			if (!LocalPlayer)
			{
				continue;
			}

			if (UProximityVoiceLocalPlayerSubsystem* VoiceSubsystem = LocalPlayer->GetSubsystem<UProximityVoiceLocalPlayerSubsystem>())
			{
				VoiceSubsystem->ApplyVoiceSettings(MicrophoneMode, MicrophoneVolume, ProximityChatVolume, bMicMonitorEnabled, PreferredMicrophoneInputDevice);
			}
		}
	});

	if (MasterSoundClass.IsNull() && GEngine)
	{
		if (FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
		{
			AudioDevice->SetTransientPrimaryVolume(OverallVolume);
		}
	}

	ApplySoundMix();
}

void UEMRGameUserSettings::ApplyControlSettings()
{
	// Control settings are consumed by input handlers (e.g., player look input).
}

void UEMRGameUserSettings::ApplyShowFPSCounter()
{
	ActiveFPSCounterWidgets.RemoveAll([](const TObjectPtr<UEMRFPSCounterWidget>& Widget)
	{
		return !IsValid(Widget);
	});

	if (!bShowFPSCounter)
	{
		for (UEMRFPSCounterWidget* Widget : ActiveFPSCounterWidgets)
		{
			if (Widget)
			{
				Widget->RemoveFromParent();
			}
		}

		ActiveFPSCounterWidgets.Reset();
		bAppliedShowFPSCounter = false;
		bHasAppliedShowFPSCounterState = true;
		return;
	}

	constexpr int32 FPSCounterWidgetZOrder = 5000;
	const TSubclassOf<UEMRFPSCounterWidget> FPSCounterWidgetClass = ResolveFPSCounterWidgetClass();

	ForEachGameWorld([this, FPSCounterWidgetClass](UWorld* World)
	{
		if (!World)
		{
			return;
		}

		UEMRFPSCounterWidget* ExistingWidgetForWorld = nullptr;
		for (UEMRFPSCounterWidget* ExistingWidget : ActiveFPSCounterWidgets)
		{
			if (ExistingWidget && ExistingWidget->GetWorld() == World)
			{
				ExistingWidgetForWorld = ExistingWidget;
				break;
			}
		}

		if (ExistingWidgetForWorld)
		{
			if (!ExistingWidgetForWorld->IsInViewport())
			{
				ExistingWidgetForWorld->AddToViewport(FPSCounterWidgetZOrder);
			}
			return;
		}

		APlayerController* LocalPlayerController = nullptr;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get(); PC && PC->IsLocalController())
			{
				LocalPlayerController = PC;
				break;
			}
		}

		if (!LocalPlayerController)
		{
			return;
		}

		if (UEMRFPSCounterWidget* NewWidget = CreateWidget<UEMRFPSCounterWidget>(LocalPlayerController, FPSCounterWidgetClass))
		{
			NewWidget->AddToViewport(FPSCounterWidgetZOrder);
			ActiveFPSCounterWidgets.Add(NewWidget);
		}
	});

	bAppliedShowFPSCounter = true;
	bHasAppliedShowFPSCounterState = true;
}

void UEMRGameUserSettings::ApplyShowKeybindHelper()
{
	ActiveKeybindHelperWidgets.RemoveAll([](const TObjectPtr<UEMRGameplayKeybindHelperWidget>& Widget)
	{
		return !IsValid(Widget);
	});

	const TSubclassOf<UEMRGameplayKeybindHelperWidget> KeybindHelperWidgetClass = ResolveGameplayKeybindHelperWidgetClass();

	if (!bShowKeybindHelper || !KeybindHelperWidgetClass)
	{
		for (UEMRGameplayKeybindHelperWidget* Widget : ActiveKeybindHelperWidgets)
		{
			if (Widget)
			{
				Widget->RemoveFromParent();
			}
		}

		ActiveKeybindHelperWidgets.Reset();
		return;
	}

	const int32 KeybindHelperWidgetZOrder = ResolveGameplayKeybindHelperWidgetZOrder();

	ForEachGameWorld([this, KeybindHelperWidgetClass, KeybindHelperWidgetZOrder](UWorld* World)
	{
		if (!World)
		{
			return;
		}

		UEMRGameplayKeybindHelperWidget* ExistingWidgetForWorld = nullptr;
		for (UEMRGameplayKeybindHelperWidget* ExistingWidget : ActiveKeybindHelperWidgets)
		{
			if (ExistingWidget && ExistingWidget->GetWorld() == World)
			{
				ExistingWidgetForWorld = ExistingWidget;
				break;
			}
		}

		const AEMRNightShiftGameState* RunGameState = World->GetGameState<AEMRNightShiftGameState>();
		const bool bShouldDisplayInWorld = RunGameState && RunGameState->GetRunPhase() == EER_RunPhase::InNightShift;

		if (!bShouldDisplayInWorld)
		{
			if (ExistingWidgetForWorld)
			{
				ExistingWidgetForWorld->RemoveFromParent();
				ActiveKeybindHelperWidgets.RemoveAll([ExistingWidgetForWorld](const TObjectPtr<UEMRGameplayKeybindHelperWidget>& Widget)
				{
					return Widget == ExistingWidgetForWorld;
				});
			}

			return;
		}

		if (ExistingWidgetForWorld)
		{
			if (!ExistingWidgetForWorld->IsInViewport())
			{
				ExistingWidgetForWorld->AddToViewport(KeybindHelperWidgetZOrder);
			}
			return;
		}

		AEMRPlayerController* LocalGameplayController = nullptr;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (AEMRPlayerController* PlayerController = Cast<AEMRPlayerController>(It->Get()); PlayerController && PlayerController->IsLocalController())
			{
				LocalGameplayController = PlayerController;
				break;
			}
		}

		if (!LocalGameplayController)
		{
			return;
		}

		if (UEMRGameplayKeybindHelperWidget* NewWidget = CreateWidget<UEMRGameplayKeybindHelperWidget>(LocalGameplayController, KeybindHelperWidgetClass))
		{
			NewWidget->AddToViewport(KeybindHelperWidgetZOrder);
			ActiveKeybindHelperWidgets.Add(NewWidget);
		}
	});
}

void UEMRGameUserSettings::ApplyColorBlindMode()
{
	const EColorVisionDeficiency Deficiency = ToColorVisionDeficiency(ColorBlindMode);
	const bool bCorrectDeficiency = Deficiency != EColorVisionDeficiency::NormalVision;
	constexpr int32 Severity = 5;

	UWidgetBlueprintLibrary::SetColorVisionDeficiencyType(Deficiency, Severity, bCorrectDeficiency, false);
}

void UEMRGameUserSettings::ApplyFieldOfView()
{
	ForEachGameWorld([this](UWorld* World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PlayerController = It->Get())
			{
				if (PlayerController->PlayerCameraManager)
				{
					PlayerController->PlayerCameraManager->SetFOV(FieldOfView);
				}
			}
		}
	});
}

void UEMRGameUserSettings::ApplyMotionBlur()
{
	if (IConsoleVariable* MotionBlurVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.MotionBlur")))
	{
		MotionBlurVar->Set(bMotionBlurEnabled ? 1 : 0, ECVF_SetByGameSetting);
	}
}

void UEMRGameUserSettings::ApplyRayTracingSettings()
{
	IConsoleVariable* RuntimeRayTracingVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.Enable"));
	if (!RuntimeRayTracingVar)
	{
		bRayTracingEnabled = false;
		return;
	}

	if (!IsRayTracingRuntimeAvailable())
	{
		bRayTracingEnabled = false;
		RuntimeRayTracingVar->Set(0, ECVF_SetByGameSetting);
		return;
	}

	RuntimeRayTracingVar->Set(bRayTracingEnabled ? 1 : 0, ECVF_SetByGameSetting);
}

void UEMRGameUserSettings::ApplyRayTracingBenchmarkDefaultIfNeeded()
{
	if (bRayTracingBenchmarkDefaultInitialized)
	{
		return;
	}

	const bool bHasBenchmarkResults = GetLastCPUBenchmarkResult() >= 0.0f && GetLastGPUBenchmarkResult() >= 0.0f;
	if (!bHasBenchmarkResults)
	{
		return;
	}

	if (ResolveEffectiveScalabilityLevel(*this) <= 1)
	{
		bRayTracingEnabled = false;
	}

	bRayTracingBenchmarkDefaultInitialized = true;
}

void UEMRGameUserSettings::ApplyUpscalerSettings()
{
	const FString SelectedUpscaler = GetUpscaler();
	if (!Upscaler.Equals(SelectedUpscaler, ESearchCase::IgnoreCase))
	{
		Upscaler = SelectedUpscaler;
	}

	const FString SelectedUpscalerQuality = GetUpscalerQuality();
	if (!UpscalerQuality.Equals(SelectedUpscalerQuality, ESearchCase::IgnoreCase))
	{
		UpscalerQuality = SelectedUpscalerQuality;
	}

	// Keep editor startup/viewports safe even if the user enables FSR for gameplay windows.
	SetIntCVar(TEXT("r.FidelityFX.FSR.EnabledInEditorViewport"), 0);

	// Disable both frame generation paths first, then re-enable the selected one if allowed.
	if (IsEngineFeatureQueryReady())
	{
		UStreamlineLibraryDLSSG::SetDLSSGMode(EStreamlineDLSSGMode::Off);
	}
	SetIntCVar(TEXT("r.FidelityFX.FI.Enabled"), 0);

	// Disable both external temporal upscalers before enabling the selected path.
	if (IsEngineFeatureQueryReady())
	{
		UDLSSLibrary::EnableDLSS(false);
	}
	SetIntCVar(TEXT("r.FidelityFX.FSR.Enabled"), 0);

	if (IsExternalUpscalerSelection(SelectedUpscaler))
	{
		SetDynamicResolutionEnabled(false);
		if (GEngine)
		{
			GEngine->SetDynamicResolutionUserSetting(false);
		}
		SetIntCVar(TEXT("r.DynamicRes.OperationMode"), 0);
	}

	// Keep the project on a temporal AA path for TSR/DLSS/FSR selection.
	SetIntCVar(TEXT("r.AntiAliasingMethod"), 4);

	const bool bAllowFrameGenerationInThisContext = !GIsPlayInEditorWorld;

	if (EqualsNoCase(SelectedUpscaler, UpscalerDLSS))
	{
		UDLSSMode DesiredDLSSMode = UDLSSMode::Quality;
		bool bHasValidDLSSModeToken = TryUpscalerQualityTokenToDLSSMode(SelectedUpscalerQuality, DesiredDLSSMode);
		bool bDLSSModeSupported = bHasValidDLSSModeToken && IsDLSSUpscalerRuntimeAvailable() && UDLSSLibrary::IsDLSSModeSupported(DesiredDLSSMode);

		if (!bDLSSModeSupported)
		{
			const TArray<UDLSSMode> FallbackModes =
			{
				UDLSSMode::Quality,
				UDLSSMode::Balanced,
				UDLSSMode::Performance,
				UDLSSMode::UltraPerformance,
				UDLSSMode::UltraQuality,
				UDLSSMode::DLAA
			};

			for (const UDLSSMode CandidateMode : FallbackModes)
			{
				if (UDLSSLibrary::IsDLSSModeSupported(CandidateMode))
				{
					DesiredDLSSMode = CandidateMode;
					bDLSSModeSupported = true;
					const FString FallbackToken = DLSSModeToUpscalerQualityToken(CandidateMode);
					if (!UpscalerQuality.Equals(FallbackToken, ESearchCase::IgnoreCase))
					{
						UE_LOG(LogEMRVideoSettings, Warning, TEXT("Saved DLSS quality '%s' is unsupported on this system. Falling back to '%s'."), *SelectedUpscalerQuality, *FallbackToken);
						UpscalerQuality = FallbackToken;
					}
					break;
				}
			}
		}

		if (bDLSSModeSupported)
		{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
			UDLSSLibrary::SetDLSSMode(nullptr, DesiredDLSSMode);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
		else
		{
			UE_LOG(LogEMRVideoSettings, Warning, TEXT("DLSS was selected but no supported DLSS quality modes were found. Falling back to TSR."));
			Upscaler = UpscalerTSR;
			UpscalerQuality = UpscalerQualityTSRDefault;
			ApplyScalabilityRuntimePolicy(*this, ResolveEffectiveScalabilityLevel(*this));
			return;
		}

		if (bFrameGenerationEnabled && bAllowFrameGenerationInThisContext && IsDLSSFrameGenerationRuntimeAvailable())
		{
			UStreamlineLibraryDLSSG::SetDLSSGMode(EStreamlineDLSSGMode::On2X);
		}

		return;
	}

	if (EqualsNoCase(SelectedUpscaler, UpscalerFSR))
	{
		int32 FSRQualityMode = 1;
		if (!TryUpscalerQualityTokenToFSRQualityMode(SelectedUpscalerQuality, FSRQualityMode))
		{
			UE_LOG(LogEMRVideoSettings, Warning, TEXT("Saved FSR quality '%s' is invalid. Falling back to FSR Quality."), *SelectedUpscalerQuality);
			FSRQualityMode = 1;
			UpscalerQuality = UpscalerQualityFSRQuality;
		}

		const bool bFSRUpscalerSupported = IsFSRUpscalerRuntimeAvailable();
		if (!bFSRUpscalerSupported)
		{
			UE_LOG(LogEMRVideoSettings, Warning,
				TEXT("FSR was selected but is unavailable on the active runtime RHI (%s). FSR plugin backend requires D3D12 for this project build. Falling back to TSR behavior."),
				*GetGraphicsRHIName());
		}

		SetIntCVar(TEXT("r.FidelityFX.FSR.QualityMode"), FSRQualityMode);
		SetIntCVar(TEXT("r.FidelityFX.FSR.Enabled"), bFSRUpscalerSupported ? 1 : 0);

		const bool bFSRFGRequested = bFrameGenerationEnabled && bAllowFrameGenerationInThisContext;
		const bool bFSRFGSupported = IsFSRFrameGenerationRuntimeAvailable();
		if (bFSRFGRequested && !bFSRFGSupported)
		{
			UE_LOG(LogEMRVideoSettings, Warning,
				TEXT("FSR Frame Generation was requested but is unavailable on the active runtime RHI (%s). AMD FI path in this project build requires D3D12."),
				*GetGraphicsRHIName());
		}

		if (bFSRFGRequested && bFSRFGSupported)
		{
			SetIntCVar(TEXT("r.FidelityFX.FI.Enabled"), 1);
		}

		return;
	}

	// TSR path: external upscalers and frame generation already disabled above.
	UpscalerQuality = UpscalerQualityTSRDefault;
}

void UEMRGameUserSettings::ApplyNVIDIAReflexSettings()
{
	const FString SelectedReflexMode = GetNVIDIAReflexMode();
	if (!NVIDIAReflexMode.Equals(SelectedReflexMode, ESearchCase::IgnoreCase))
	{
		NVIDIAReflexMode = SelectedReflexMode;
	}

	if (!IsEngineFeatureQueryReady() || !IsNVIDIAReflexRuntimeAvailable())
	{
		return;
	}

	EStreamlineReflexMode ReflexMode = EStreamlineReflexMode::Off;
	if (EqualsNoCase(SelectedReflexMode, NVIDIAReflexModeBoost))
	{
		ReflexMode = EStreamlineReflexMode::Boost;
	}
	else if (EqualsNoCase(SelectedReflexMode, NVIDIAReflexModeOn))
	{
		ReflexMode = EStreamlineReflexMode::Enabled;
	}

	if (!UStreamlineLibraryReflex::IsReflexModeSupported(ReflexMode))
	{
		UE_LOG(LogEMRVideoSettings, Warning, TEXT("Saved NVIDIA Reflex mode '%s' is unsupported on this system. Falling back to Off."), *SelectedReflexMode);
		ReflexMode = EStreamlineReflexMode::Off;
		NVIDIAReflexMode = NVIDIAReflexModeOff;
	}

	UStreamlineLibraryReflex::SetReflexMode(ReflexMode);
}

void UEMRGameUserSettings::ApplySoundMix()
{
	if (!SettingsSoundMix)
	{
		SettingsSoundMix = NewObject<USoundMix>(this, TEXT("EMR_SettingsSoundMix"));
		SettingsSoundMix->bApplyEQ = false;
	}

	const bool bAnyOverrideApplied =
	ApplySoundClassVolume(MasterSoundClass, OverallVolume) |
	ApplySoundClassVolume(MusicSoundClass, MusicVolume) |
	ApplySoundClassVolume(SFXSoundClass, SFXVolume) |
	ApplySoundClassVolume(ProximityChatSoundClass, ProximityChatVolume) |
	ApplySoundClassVolume(MicrophoneSoundClass, MicrophoneVolume);

	if (bAnyOverrideApplied)
	{
		ForEachGameWorld([this](UWorld* World)
		{
			UGameplayStatics::PushSoundMixModifier(World, SettingsSoundMix);
		});
	}
}

bool UEMRGameUserSettings::ApplySoundClassVolume(const TSoftObjectPtr<USoundClass>& SoundClass, float InVolume)
{
	if (!SettingsSoundMix)
	{
		return false;
	}

	USoundClass* LoadedSoundClass = SoundClass.LoadSynchronous();
	if (!LoadedSoundClass)
	{
		return false;
	}

	ForEachGameWorld([this, LoadedSoundClass, InVolume](UWorld* World)
	{
		UGameplayStatics::SetSoundMixClassOverride(World, SettingsSoundMix, LoadedSoundClass, InVolume, 1.0f, 0.0f, true);
	});

	return true;
}

bool UEMRGameUserSettings::IsStartupShaderOptimizationCompleteForKey(const FString& RuntimeKey) const
{
	return bStartupShaderOptimizationCompleted && !RuntimeKey.IsEmpty() && StartupShaderOptimizationKey == RuntimeKey;
}

void UEMRGameUserSettings::MarkStartupShaderOptimizationComplete(const FString& RuntimeKey)
{
	if (RuntimeKey.IsEmpty())
	{
		return;
	}

	StartupShaderOptimizationKey = RuntimeKey;
	bStartupShaderOptimizationCompleted = true;
	SaveSettings();
}
