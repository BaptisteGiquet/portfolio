#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "AGASSSettingsLocal.generated.h"

USTRUCT()
struct AGASSCOMMON_API FAGASSTimedRunBestTimeRecord
{
	GENERATED_BODY()

	UPROPERTY(Config)
	FString LeaderboardKey;

	UPROPERTY(Config)
	FString MapId;

	UPROPERTY(Config)
	FString OwningModId;

	UPROPERTY(Config)
	int32 BestTimeMilliseconds = 0;
};

UENUM(BlueprintType)
enum class EAGASSAutoBenchmarkResult : uint8
{
	AppliedRecommendation,
	AppliedFallback,
	Unsupported
};

UCLASS()
class AGASSCOMMON_API UAGASSSettingsLocal : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UAGASSSettingsLocal();

	static UAGASSSettingsLocal* Get();

	UFUNCTION(BlueprintCallable, Category="Benchmark")
	bool CanRunAutoBenchmark() const;

	UFUNCTION(BlueprintCallable, Category="Benchmark")
	bool ShouldRunAutoBenchmarkAtStartup() const;

	UFUNCTION(BlueprintCallable, Category="Benchmark")
	EAGASSAutoBenchmarkResult RunAutoBenchmark(bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category="Benchmark")
	bool DidLastAutoBenchmarkUseFallback() const
	{
		return bAutoBenchmarkUsedFallback;
	}

	virtual void SetToDefaults() override;
	virtual void LoadSettings(bool bForceReload = false) override;
	virtual void ApplyNonResolutionSettings() override;

	UFUNCTION()
	float GetDisplayGamma() const
	{
		return DisplayGamma;
	}

	UFUNCTION()
	void SetDisplayGamma(float InGamma);

	UFUNCTION()
	float GetMasterVolume() const
	{
		return MasterVolume;
	}

	UFUNCTION()
	void SetMasterVolume(float InVolume);

	UFUNCTION()
	float GetSafeZone() const
	{
		return SafeZoneScale;
	}

	UFUNCTION()
	void SetSafeZone(float InValue);

	UFUNCTION()
	bool IsSubtitlesEnabled() const
	{
		return bSubtitlesEnabled;
	}

	UFUNCTION()
	void SetSubtitlesEnabled(bool bEnabled);

	UFUNCTION()
	bool ShouldShowFrontendInstanceRole() const
	{
		return bShowFrontendInstanceRole;
	}

	UFUNCTION()
	void SetShowFrontendInstanceRole(bool bEnabled);

	UFUNCTION()
	float GetGameplayFieldOfView() const
	{
		return GameplayFieldOfView;
	}

	UFUNCTION()
	void SetGameplayFieldOfView(float InValue);

	UFUNCTION()
	float GetPlacementDistanceSpeedScale() const
	{
		return PlacementDistanceSpeedScale;
	}

	UFUNCTION()
	void SetPlacementDistanceSpeedScale(float InValue);

	UFUNCTION()
	float GetPlacementRotationSensitivity() const
	{
		return PlacementRotationSensitivity;
	}

	UFUNCTION()
	void SetPlacementRotationSensitivity(float InValue);

	UFUNCTION()
	float GetMouseLookSensitivityHorizontal() const
	{
		return MouseLookSensitivityHorizontal;
	}

	UFUNCTION()
	void SetMouseLookSensitivityHorizontal(float InValue);

	UFUNCTION()
	float GetMouseLookSensitivityVertical() const
	{
		return MouseLookSensitivityVertical;
	}

	UFUNCTION()
	void SetMouseLookSensitivityVertical(float InValue);

	UFUNCTION()
	bool GetInvertMouseHorizontal() const
	{
		return bInvertMouseHorizontal;
	}

	UFUNCTION()
	void SetInvertMouseHorizontal(bool bEnabled);

	UFUNCTION()
	bool GetInvertMouseVertical() const
	{
		return bInvertMouseVertical;
	}

	UFUNCTION()
	void SetInvertMouseVertical(bool bEnabled);

	UFUNCTION()
	float GetGamepadLookSensitivityHorizontal() const
	{
		return GamepadLookSensitivityHorizontal;
	}

	UFUNCTION()
	void SetGamepadLookSensitivityHorizontal(float InValue);

	UFUNCTION()
	float GetGamepadLookSensitivityVertical() const
	{
		return GamepadLookSensitivityVertical;
	}

	UFUNCTION()
	void SetGamepadLookSensitivityVertical(float InValue);

	UFUNCTION()
	float GetGamepadLookDeadZone() const
	{
		return GamepadLookDeadZone;
	}

	UFUNCTION()
	void SetGamepadLookDeadZone(float InValue);

	UFUNCTION()
	bool GetInvertGamepadHorizontal() const
	{
		return bInvertGamepadHorizontal;
	}

	UFUNCTION()
	void SetInvertGamepadHorizontal(bool bEnabled);

	UFUNCTION()
	bool GetInvertGamepadVertical() const
	{
		return bInvertGamepadVertical;
	}

	UFUNCTION()
	void SetInvertGamepadVertical(bool bEnabled);

	UFUNCTION()
	FString GetScreenResolutionString() const;

	UFUNCTION()
	void SetScreenResolutionString(const FString& InResolutionString);

	int32 GetTimedRunBestMilliseconds(const FString& LeaderboardKey) const;
	bool UpdateTimedRunBestMilliseconds(const FString& LeaderboardKey, const FString& MapId, const FString& OwningModId, int32 NewTimeMilliseconds);

	FVector2D ApplyLookInputSettings(const FVector2D& RawInput, bool bGamepadInput) const;

private:
	void ApplyBenchmarkSettings(bool bSaveImmediately);
	void ApplyDisplayGamma();
	void ApplyMasterVolume();
	void ApplySafeZoneScale();
	void ApplySubtitlesSetting();
	bool HasValidBenchmarkRecommendation() const;
	static TOptional<FIntPoint> ParseResolutionString(const FString& InResolutionString);
	void RestoreBenchmarkProtectedVideoSettings(const FIntPoint& PreservedScreenResolution, EWindowMode::Type PreservedWindowMode);

	UPROPERTY(Config)
	float DisplayGamma = 2.2f;

	UPROPERTY(Config)
	float MasterVolume = 1.0f;

	UPROPERTY(Config)
	float SafeZoneScale = 1.0f;

	UPROPERTY(Config)
	bool bSubtitlesEnabled = true;

	UPROPERTY(Config)
	bool bShowFrontendInstanceRole = false;

	UPROPERTY(Config)
	float GameplayFieldOfView = 100.0f;

	UPROPERTY(Config)
	float PlacementDistanceSpeedScale = 1.0f;

	UPROPERTY(Config)
	float PlacementRotationSensitivity = 1.0f;

	UPROPERTY(Config)
	float MouseLookSensitivityHorizontal = 1.0f;

	UPROPERTY(Config)
	float MouseLookSensitivityVertical = 1.0f;

	UPROPERTY(Config)
	bool bInvertMouseHorizontal = false;

	UPROPERTY(Config)
	bool bInvertMouseVertical = false;

	UPROPERTY(Config)
	float GamepadLookSensitivityHorizontal = 1.0f;

	UPROPERTY(Config)
	float GamepadLookSensitivityVertical = 1.0f;

	UPROPERTY(Config)
	float GamepadLookDeadZone = 0.15f;

	UPROPERTY(Config)
	bool bInvertGamepadHorizontal = false;

	UPROPERTY(Config)
	bool bInvertGamepadVertical = false;

	UPROPERTY(Config)
	TArray<FAGASSTimedRunBestTimeRecord> TimedRunBestTimes;

	UPROPERTY(Config)
	int32 AppliedAutoBenchmarkSchemaVersion = 0;

	UPROPERTY(Config)
	bool bAutoBenchmarkUsedFallback = false;
};
