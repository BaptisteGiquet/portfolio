#include "Settings/AGASSSettingsLocal.h"

#include "AudioDevice.h"
#include "ComponentRecreateRenderStateContext.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Settings/AGASSBenchmarkDeveloperSettings.h"
#include "Widgets/Layout/SSafeZone.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSLocalSettings, Log, All);

namespace
{
	constexpr float MinDisplayGamma = 1.7f;
	constexpr float MaxDisplayGamma = 2.7f;
	constexpr float MinScalarSetting = 0.05f;
	constexpr float MinFallbackResolutionQuality = 25.0f;
	constexpr float MaxFallbackResolutionQuality = 100.0f;
}

UAGASSSettingsLocal::UAGASSSettingsLocal()
{
	SetToDefaults();
}

UAGASSSettingsLocal* UAGASSSettingsLocal::Get()
{
	return GEngine != nullptr ? Cast<UAGASSSettingsLocal>(GEngine->GetGameUserSettings()) : nullptr;
}

bool UAGASSSettingsLocal::CanRunAutoBenchmark() const
{
	return GEngine != nullptr && FApp::CanEverRender() && !GIsEditor && !IsRunningCommandlet();
}

bool UAGASSSettingsLocal::ShouldRunAutoBenchmarkAtStartup() const
{
	if (!CanRunAutoBenchmark())
	{
		return false;
	}

	const UAGASSBenchmarkDeveloperSettings* BenchmarkSettings = UAGASSBenchmarkDeveloperSettings::Get();
	const bool bShouldRun = BenchmarkSettings != nullptr && AppliedAutoBenchmarkSchemaVersion < BenchmarkSettings->BenchmarkSchemaVersion;
	UE_LOG(
		LogAGASSLocalSettings,
		Log,
		TEXT("ShouldRunAutoBenchmarkAtStartup=%s AppliedSchema=%d ConfigSchema=%d"),
		bShouldRun ? TEXT("true") : TEXT("false"),
		AppliedAutoBenchmarkSchemaVersion,
		BenchmarkSettings != nullptr ? BenchmarkSettings->BenchmarkSchemaVersion : -1);
	return bShouldRun;
}

EAGASSAutoBenchmarkResult UAGASSSettingsLocal::RunAutoBenchmark(const bool bSaveImmediately)
{
	if (!CanRunAutoBenchmark())
	{
		return EAGASSAutoBenchmarkResult::Unsupported;
	}

	const UAGASSBenchmarkDeveloperSettings* BenchmarkSettings = UAGASSBenchmarkDeveloperSettings::Get();
	if (BenchmarkSettings == nullptr)
	{
		return EAGASSAutoBenchmarkResult::Unsupported;
	}

	const FIntPoint PreservedScreenResolution = GetScreenResolution();
	const EWindowMode::Type PreservedWindowMode = GetFullscreenMode();

	RunHardwareBenchmark(
		FMath::Max(BenchmarkSettings->BenchmarkWorkScale, 1),
		FMath::Max(BenchmarkSettings->CPUMultiplier, 0.10f),
		FMath::Max(BenchmarkSettings->GPUMultiplier, 0.10f));

	UE_LOG(
		LogAGASSLocalSettings,
		Log,
		TEXT("RunAutoBenchmark raw results CPU=%.3f GPU=%.3f ResolutionQuality=%.2f WorkScale=%d CPUMultiplier=%.2f GPUMultiplier=%.2f"),
		GetLastCPUBenchmarkResult(),
		GetLastGPUBenchmarkResult(),
		ScalabilityQuality.ResolutionQuality,
		BenchmarkSettings->BenchmarkWorkScale,
		BenchmarkSettings->CPUMultiplier,
		BenchmarkSettings->GPUMultiplier);

	EAGASSAutoBenchmarkResult Result = EAGASSAutoBenchmarkResult::AppliedRecommendation;
	bAutoBenchmarkUsedFallback = false;

	if (!HasValidBenchmarkRecommendation())
	{
		ScalabilityQuality.SetFromSingleQualityLevel(FMath::Clamp(BenchmarkSettings->FallbackOverallQualityLevel, 0, 4));
		ScalabilityQuality.ResolutionQuality = FMath::Clamp(
			BenchmarkSettings->FallbackResolutionQuality,
			MinFallbackResolutionQuality,
			MaxFallbackResolutionQuality);
		bAutoBenchmarkUsedFallback = true;
		Result = EAGASSAutoBenchmarkResult::AppliedFallback;
	}

	FrameRateLimit = static_cast<float>(FMath::Clamp(BenchmarkSettings->TargetFrameRateLimit, 30, 240));
	AppliedAutoBenchmarkSchemaVersion = BenchmarkSettings->BenchmarkSchemaVersion;

	RestoreBenchmarkProtectedVideoSettings(PreservedScreenResolution, PreservedWindowMode);
	ApplyBenchmarkSettings(bSaveImmediately);

	UE_LOG(
		LogAGASSLocalSettings,
		Log,
		TEXT("RunAutoBenchmark applied Result=%s Fallback=%s FrameRateLimit=%.1f Resolution=%dx%d WindowMode=%d ResolutionQuality=%.2f Saved=%s"),
		Result == EAGASSAutoBenchmarkResult::AppliedFallback ? TEXT("Fallback") : TEXT("Recommendation"),
		bAutoBenchmarkUsedFallback ? TEXT("true") : TEXT("false"),
		FrameRateLimit,
		GetScreenResolution().X,
		GetScreenResolution().Y,
		static_cast<int32>(GetFullscreenMode()),
		ScalabilityQuality.ResolutionQuality,
		bSaveImmediately ? TEXT("true") : TEXT("false"));

	return Result;
}

void UAGASSSettingsLocal::SetToDefaults()
{
	Super::SetToDefaults();

	DisplayGamma = 2.2f;
	MasterVolume = 1.0f;
	SafeZoneScale = 1.0f;
	bSubtitlesEnabled = true;
	bShowFrontendInstanceRole = false;
	GameplayFieldOfView = 100.0f;
	PlacementDistanceSpeedScale = 1.0f;
	PlacementRotationSensitivity = 1.0f;
	MouseLookSensitivityHorizontal = 1.0f;
	MouseLookSensitivityVertical = 1.0f;
	bInvertMouseHorizontal = false;
	bInvertMouseVertical = false;
	GamepadLookSensitivityHorizontal = 1.0f;
	GamepadLookSensitivityVertical = 1.0f;
	GamepadLookDeadZone = 0.15f;
	bInvertGamepadHorizontal = false;
	bInvertGamepadVertical = false;
	AppliedAutoBenchmarkSchemaVersion = 0;
	bAutoBenchmarkUsedFallback = false;
}

void UAGASSSettingsLocal::LoadSettings(const bool bForceReload)
{
	Super::LoadSettings(bForceReload);

	ApplySubtitlesSetting();
	ApplyDisplayGamma();
	ApplyMasterVolume();
	ApplySafeZoneScale();
}

void UAGASSSettingsLocal::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();

	ApplySubtitlesSetting();
	ApplyDisplayGamma();
	ApplyMasterVolume();
	ApplySafeZoneScale();
}

void UAGASSSettingsLocal::SetDisplayGamma(const float InGamma)
{
	DisplayGamma = FMath::Clamp(InGamma, MinDisplayGamma, MaxDisplayGamma);
	ApplyDisplayGamma();
}

void UAGASSSettingsLocal::SetMasterVolume(const float InVolume)
{
	MasterVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
	ApplyMasterVolume();
}

void UAGASSSettingsLocal::SetSafeZone(const float InValue)
{
	SafeZoneScale = FMath::Clamp(InValue, 0.8f, 1.0f);
	ApplySafeZoneScale();
}

void UAGASSSettingsLocal::SetSubtitlesEnabled(const bool bEnabled)
{
	bSubtitlesEnabled = bEnabled;
	ApplySubtitlesSetting();
}

void UAGASSSettingsLocal::SetShowFrontendInstanceRole(const bool bEnabled)
{
	bShowFrontendInstanceRole = bEnabled;
}

void UAGASSSettingsLocal::SetGameplayFieldOfView(const float InValue)
{
	GameplayFieldOfView = FMath::Clamp(InValue, 80.0f, 110.0f);
}

void UAGASSSettingsLocal::SetPlacementDistanceSpeedScale(const float InValue)
{
	PlacementDistanceSpeedScale = FMath::Clamp(InValue, 0.5f, 2.0f);
}

void UAGASSSettingsLocal::SetPlacementRotationSensitivity(const float InValue)
{
	PlacementRotationSensitivity = FMath::Clamp(InValue, 0.5f, 2.0f);
}

void UAGASSSettingsLocal::SetMouseLookSensitivityHorizontal(const float InValue)
{
	MouseLookSensitivityHorizontal = FMath::Clamp(InValue, MinScalarSetting, 3.0f);
}

void UAGASSSettingsLocal::SetMouseLookSensitivityVertical(const float InValue)
{
	MouseLookSensitivityVertical = FMath::Clamp(InValue, MinScalarSetting, 3.0f);
}

void UAGASSSettingsLocal::SetInvertMouseHorizontal(const bool bEnabled)
{
	bInvertMouseHorizontal = bEnabled;
}

void UAGASSSettingsLocal::SetInvertMouseVertical(const bool bEnabled)
{
	bInvertMouseVertical = bEnabled;
}

void UAGASSSettingsLocal::SetGamepadLookSensitivityHorizontal(const float InValue)
{
	GamepadLookSensitivityHorizontal = FMath::Clamp(InValue, MinScalarSetting, 3.0f);
}

void UAGASSSettingsLocal::SetGamepadLookSensitivityVertical(const float InValue)
{
	GamepadLookSensitivityVertical = FMath::Clamp(InValue, MinScalarSetting, 3.0f);
}

void UAGASSSettingsLocal::SetGamepadLookDeadZone(const float InValue)
{
	GamepadLookDeadZone = FMath::Clamp(InValue, 0.0f, 0.35f);
}

void UAGASSSettingsLocal::SetInvertGamepadHorizontal(const bool bEnabled)
{
	bInvertGamepadHorizontal = bEnabled;
}

void UAGASSSettingsLocal::SetInvertGamepadVertical(const bool bEnabled)
{
	bInvertGamepadVertical = bEnabled;
}

FString UAGASSSettingsLocal::GetScreenResolutionString() const
{
	const FIntPoint Resolution = GetScreenResolution();
	return FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y);
}

void UAGASSSettingsLocal::SetScreenResolutionString(const FString& InResolutionString)
{
	if (const TOptional<FIntPoint> ParsedResolution = ParseResolutionString(InResolutionString))
	{
		SetScreenResolution(ParsedResolution.GetValue());
	}
}

int32 UAGASSSettingsLocal::GetTimedRunBestMilliseconds(const FString& LeaderboardKey) const
{
	if (LeaderboardKey.IsEmpty())
	{
		return 0;
	}

	for (const FAGASSTimedRunBestTimeRecord& Record : TimedRunBestTimes)
	{
		if (Record.LeaderboardKey.Equals(LeaderboardKey, ESearchCase::IgnoreCase))
		{
			return FMath::Max(Record.BestTimeMilliseconds, 0);
		}
	}

	return 0;
}

bool UAGASSSettingsLocal::UpdateTimedRunBestMilliseconds(
	const FString& LeaderboardKey,
	const FString& MapId,
	const FString& OwningModId,
	const int32 NewTimeMilliseconds)
{
	if (LeaderboardKey.IsEmpty() || NewTimeMilliseconds <= 0)
	{
		return false;
	}

	for (FAGASSTimedRunBestTimeRecord& Record : TimedRunBestTimes)
	{
		if (Record.LeaderboardKey.Equals(LeaderboardKey, ESearchCase::IgnoreCase))
		{
			if (Record.BestTimeMilliseconds > 0 && Record.BestTimeMilliseconds <= NewTimeMilliseconds)
			{
				return false;
			}

			Record.MapId = MapId;
			Record.OwningModId = OwningModId;
			Record.BestTimeMilliseconds = NewTimeMilliseconds;
			SaveSettings();
			return true;
		}
	}

	FAGASSTimedRunBestTimeRecord& NewRecord = TimedRunBestTimes.AddDefaulted_GetRef();
	NewRecord.LeaderboardKey = LeaderboardKey;
	NewRecord.MapId = MapId;
	NewRecord.OwningModId = OwningModId;
	NewRecord.BestTimeMilliseconds = NewTimeMilliseconds;
	SaveSettings();
	return true;
}

FVector2D UAGASSSettingsLocal::ApplyLookInputSettings(const FVector2D& RawInput, const bool bGamepadInput) const
{
	FVector2D AdjustedInput = RawInput;

	if (bGamepadInput)
	{
		auto ApplyDeadZone = [this](const float Value)
		{
			const float DeadZone = FMath::Clamp(GamepadLookDeadZone, 0.0f, 0.95f);
			const float Magnitude = FMath::Abs(Value);
			if (Magnitude <= DeadZone)
			{
				return 0.0f;
			}

			const float ScaledMagnitude = (Magnitude - DeadZone) / FMath::Max(1.0f - DeadZone, KINDA_SMALL_NUMBER);
			return FMath::Sign(Value) * ScaledMagnitude;
		};

		AdjustedInput.X = ApplyDeadZone(AdjustedInput.X) * GamepadLookSensitivityHorizontal * (bInvertGamepadHorizontal ? -1.0f : 1.0f);
		AdjustedInput.Y = ApplyDeadZone(AdjustedInput.Y) * GamepadLookSensitivityVertical * (bInvertGamepadVertical ? -1.0f : 1.0f);
		return AdjustedInput;
	}

	AdjustedInput.X *= MouseLookSensitivityHorizontal * (bInvertMouseHorizontal ? -1.0f : 1.0f);
	AdjustedInput.Y *= MouseLookSensitivityVertical * (bInvertMouseVertical ? -1.0f : 1.0f);
	return AdjustedInput;
}

void UAGASSSettingsLocal::ApplyBenchmarkSettings(const bool bSaveImmediately)
{
	{
		FGlobalComponentRecreateRenderStateContext Context;
		ApplyResolutionSettings(false);
		ApplyNonResolutionSettings();
	}

	RequestUIUpdate();

	if (bSaveImmediately)
	{
		SaveSettings();
	}
}

void UAGASSSettingsLocal::ApplyDisplayGamma()
{
	if (GEngine != nullptr && FApp::CanEverRender())
	{
		GEngine->DisplayGamma = FMath::Clamp(DisplayGamma, MinDisplayGamma, MaxDisplayGamma);
	}
}

void UAGASSSettingsLocal::ApplyMasterVolume()
{
	if (GEngine == nullptr)
	{
		return;
	}

	if (FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDevice())
	{
		AudioDevice->SetTransientPrimaryVolume(FMath::Clamp(MasterVolume, 0.0f, 1.0f));
	}
}

void UAGASSSettingsLocal::ApplySafeZoneScale()
{
	SSafeZone::SetGlobalSafeZoneScale(FMath::Clamp(SafeZoneScale, 0.8f, 1.0f));
}

void UAGASSSettingsLocal::ApplySubtitlesSetting()
{
	UGameplayStatics::SetSubtitlesEnabled(bSubtitlesEnabled);
}

bool UAGASSSettingsLocal::HasValidBenchmarkRecommendation() const
{
	return FMath::IsFinite(GetLastCPUBenchmarkResult())
		&& FMath::IsFinite(GetLastGPUBenchmarkResult())
		&& GetLastCPUBenchmarkResult() > 0.0f
		&& GetLastGPUBenchmarkResult() > 0.0f;
}

TOptional<FIntPoint> UAGASSSettingsLocal::ParseResolutionString(const FString& InResolutionString)
{
	FString LeftToken;
	FString RightToken;
	if (!InResolutionString.Split(TEXT("x"), &LeftToken, &RightToken))
	{
		return TOptional<FIntPoint>();
	}

	const int32 Width = FCString::Atoi(*LeftToken);
	const int32 Height = FCString::Atoi(*RightToken);
	if (Width <= 0 || Height <= 0)
	{
		return TOptional<FIntPoint>();
	}

	return FIntPoint(Width, Height);
}

void UAGASSSettingsLocal::RestoreBenchmarkProtectedVideoSettings(
	const FIntPoint& PreservedScreenResolution,
	const EWindowMode::Type PreservedWindowMode)
{
	SetScreenResolution(PreservedScreenResolution);
	SetFullscreenMode(PreservedWindowMode);
}
