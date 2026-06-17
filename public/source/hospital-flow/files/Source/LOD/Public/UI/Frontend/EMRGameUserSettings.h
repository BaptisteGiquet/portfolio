

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "EMRGameUserSettings.generated.h"

class UEMRFPSCounterWidget;
class UEMRGameplayKeybindHelperWidget;

/**
 * 
 */
UCLASS()
class LOD_API UEMRGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	UEMRGameUserSettings();
	
	static UEMRGameUserSettings* Get();
	void ApplyImmediateSettingByDataID(const FName& DataID);

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
	virtual void ApplyNonResolutionSettings() override;
	virtual void ApplyHardwareBenchmarkResults() override;
	virtual void SetOverallScalabilityLevel(int32 Value) override;
	virtual int32 GetOverallScalabilityLevel() const override;

	bool IsStartupShaderOptimizationCompleteForKey(const FString& RuntimeKey) const;
	void MarkStartupShaderOptimizationComplete(const FString& RuntimeKey);
	
	//***** Gameplay Collection Tab *****//
	UFUNCTION()
	FString GetCurrentGameDifficulty() const { return CurrentGameDifficulty; }
	
	UFUNCTION()
	void SetCurrentGameDifficulty(const FString& InNewDifficulty) { CurrentGameDifficulty = InNewDifficulty; }

	UFUNCTION()
	FString GetCurrentLanguage() const { return CurrentLanguage; }

	UFUNCTION()
	void SetCurrentLanguage(const FString& InNewLanguage);

	UFUNCTION()
	bool GetShowFPSCounter() const { return bShowFPSCounter; }

	UFUNCTION()
	void SetShowFPSCounter(bool bInShowFPSCounter);

	UFUNCTION()
	bool GetShowKeybindHelper() const { return bShowKeybindHelper; }

	UFUNCTION()
	void SetShowKeybindHelper(bool bInShowKeybindHelper);

	void ApplyShowKeybindHelper();

	UFUNCTION()
	FString GetColorBlindMode() const { return ColorBlindMode; }

	UFUNCTION()
	void SetColorBlindMode(const FString& InNewColorBlindMode) { ColorBlindMode = InNewColorBlindMode; }
	//***** Gameplay Collection Tab *****//
	
	
	//***** Audio Collection Tab *****//
	UFUNCTION()
	float GetOverallVolume() const { return OverallVolume; }
	
	UFUNCTION()
	void SetOverallVolume(float InVolume);
	
	UFUNCTION()
	float GetMusicVolume() const { return MusicVolume; }
	
	UFUNCTION()
	void SetMusicVolume(float InVolume);
	
		
	UFUNCTION()
	float GetSFXVolume() const { return SFXVolume; }
	
	UFUNCTION()
	void SetSFXVolume(float InVolume);

	UFUNCTION()
	float GetProximityChatVolume() const { return ProximityChatVolume; }

	UFUNCTION()
	void SetProximityChatVolume(float InVolume);

	UFUNCTION()
	float GetMicrophoneVolume() const { return MicrophoneVolume; }

	UFUNCTION()
	void SetMicrophoneVolume(float InVolume);

	UFUNCTION()
	FString GetMicrophoneMode() const { return MicrophoneMode; }

	UFUNCTION()
	void SetMicrophoneMode(const FString& InNewMicrophoneMode) { MicrophoneMode = InNewMicrophoneMode; }

	UFUNCTION()
	FString GetPreferredMicrophoneInputDevice() const { return PreferredMicrophoneInputDevice; }

	UFUNCTION()
	void SetPreferredMicrophoneInputDevice(const FString& InDeviceId) { PreferredMicrophoneInputDevice = InDeviceId; }

	UFUNCTION()
	bool GetMicMonitorEnabled() const { return bMicMonitorEnabled; }

	UFUNCTION()
	void SetMicMonitorEnabled(bool bInMicMonitorEnabled) { bMicMonitorEnabled = bInMicMonitorEnabled; }
	
	UFUNCTION()
	bool GetAllowBackgroundAudio() const { return bAllowBackgroundAudio; }
	
	UFUNCTION()
	void SetAllowBackgroundAudio(bool ShouldAllowBackgroundAudio);
	
	UFUNCTION()
	bool GetUseHDRAudio() const { return bUseHDRAudio; }
	
	UFUNCTION()
	void SetUseHDRAudio(bool ShouldUseHDRAudio);
	//***** Audio Collection Tab *****//
	
	
	//***** Video Collection Tab *****//
	UFUNCTION()
	float GetCurrentDisplayGamma();
	
	UFUNCTION()
	void SetCurrentDisplayGamma(float InNewDisplayGamma);

	UFUNCTION()
	float GetFieldOfView() const { return FieldOfView; }

	UFUNCTION()
	void SetFieldOfView(float InFieldOfView);

	UFUNCTION()
	bool GetMotionBlurEnabled() const { return bMotionBlurEnabled; }

	UFUNCTION()
	void SetMotionBlurEnabled(bool bInMotionBlurEnabled) { bMotionBlurEnabled = bInMotionBlurEnabled; }

	UFUNCTION()
	bool GetRayTracingEnabled() const;

	UFUNCTION()
	void SetRayTracingEnabled(bool bInRayTracingEnabled);

	UFUNCTION()
	FString GetUpscaler() const;

	UFUNCTION()
	void SetUpscaler(const FString& InUpscaler);

	UFUNCTION()
	FString GetUpscalerQuality() const;

	UFUNCTION()
	void SetUpscalerQuality(const FString& InUpscalerQuality);

	UFUNCTION()
	bool GetFrameGenerationEnabled() const { return bFrameGenerationEnabled; }

	UFUNCTION()
	void SetFrameGenerationEnabled(bool bInFrameGenerationEnabled) { bFrameGenerationEnabled = bInFrameGenerationEnabled; }

	UFUNCTION()
	FString GetNVIDIAReflexMode() const;

	UFUNCTION()
	void SetNVIDIAReflexMode(const FString& InNVIDIAReflexMode);
	//***** Video Collection Tab *****//


	//***** Controls Collection Tab *****//
	UFUNCTION()
	float GetMouseSensitivity() const { return MouseSensitivity; }

	UFUNCTION()
	void SetMouseSensitivity(float InMouseSensitivity) { MouseSensitivity = FMath::Clamp(InMouseSensitivity, 0.1f, 5.0f); }

	UFUNCTION()
	bool GetMouseInvertX() const { return bMouseInvertX; }

	UFUNCTION()
	void SetMouseInvertX(bool bInMouseInvertX) { bMouseInvertX = bInMouseInvertX; }

	UFUNCTION()
	bool GetMouseInvertY() const { return bMouseInvertY; }

	UFUNCTION()
	void SetMouseInvertY(bool bInMouseInvertY) { bMouseInvertY = bInMouseInvertY; }

	UFUNCTION()
	float GetGamepadSensitivity() const { return GamepadSensitivity; }

	UFUNCTION()
	void SetGamepadSensitivity(float InGamepadSensitivity) { GamepadSensitivity = FMath::Clamp(InGamepadSensitivity, 0.1f, 5.0f); }

	UFUNCTION()
	bool GetGamepadInvertX() const { return bGamepadInvertX; }

	UFUNCTION()
	void SetGamepadInvertX(bool bInGamepadInvertX) { bGamepadInvertX = bInGamepadInvertX; }

	UFUNCTION()
	bool GetGamepadInvertY() const { return bGamepadInvertY; }

	UFUNCTION()
	void SetGamepadInvertY(bool bInGamepadInvertY) { bGamepadInvertY = bInGamepadInvertY; }
	//***** Controls Collection Tab *****//
	
	
private:
	void ApplyCustomScalabilityPreset(int32 PresetLevel);
	bool IsCustomScalabilityPreset(int32 PresetLevel) const;
	bool AreCoreScalabilityQualitiesUniform(int32 QualityLevel) const;

	void ApplyGameplaySettings();
	void ApplyVideoSettings();
	void ApplyAudioSettings();
	void ApplyControlSettings();
	void ApplyShowFPSCounter();
	void ApplyColorBlindMode();
	void ApplyFieldOfView();
	void ApplyMotionBlur();
	void ApplyRayTracingSettings();
	void ApplyRayTracingBenchmarkDefaultIfNeeded();
	void ApplyUpscalerSettings();
	void ApplyNVIDIAReflexSettings();
	void ApplySoundMix();
	bool ApplySoundClassVolume(const TSoftObjectPtr<class USoundClass>& SoundClass, float InVolume);

	//***** Gameplay Collection Tab *****//
	UPROPERTY(Config)
	FString CurrentGameDifficulty;

	UPROPERTY(Config)
	FString CurrentLanguage;

	UPROPERTY(Config)
	bool bShowFPSCounter;

	UPROPERTY(Config)
	bool bShowKeybindHelper;

	UPROPERTY(Config)
	FString ColorBlindMode;
	//***** Gameplay Collection Tab *****//
	
	//***** Audio Collection Tab *****//
	UPROPERTY(Config)
	float OverallVolume;
	
	UPROPERTY(Config)
	float MusicVolume;
	
	UPROPERTY(Config)
	float SFXVolume;

	UPROPERTY(Config)
	float ProximityChatVolume;

	UPROPERTY(Config)
	float MicrophoneVolume;

	UPROPERTY(Config)
	FString MicrophoneMode;

	UPROPERTY(Config)
	FString PreferredMicrophoneInputDevice;

	UPROPERTY(Config)
	bool bMicMonitorEnabled;
	
	UPROPERTY(Config)
	bool bAllowBackgroundAudio;
	
	UPROPERTY(Config)
	bool bUseHDRAudio;

	UPROPERTY(Config)
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(Config)
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(Config)
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(Config)
	TSoftObjectPtr<USoundClass> ProximityChatSoundClass;

	UPROPERTY(Config)
	TSoftObjectPtr<USoundClass> MicrophoneSoundClass;
	//***** Audio Collection Tab *****//

	//***** Video Collection Tab *****//
	UPROPERTY(Config)
	float FieldOfView;

	UPROPERTY(Config)
	bool bMotionBlurEnabled;

	UPROPERTY(Config)
	bool bRayTracingEnabled = false;

	UPROPERTY(Config)
	bool bRayTracingBenchmarkDefaultInitialized = false;

	UPROPERTY(Config)
	FString Upscaler;

	UPROPERTY(Config)
	FString UpscalerQuality;

	UPROPERTY(Config)
	bool bFrameGenerationEnabled;

	UPROPERTY(Config)
	FString NVIDIAReflexMode;
	//***** Video Collection Tab *****//

	//***** Controls Collection Tab *****//
	UPROPERTY(Config)
	float MouseSensitivity;

	UPROPERTY(Config)
	bool bMouseInvertX;

	UPROPERTY(Config)
	bool bMouseInvertY;

	UPROPERTY(Config)
	float GamepadSensitivity;

	UPROPERTY(Config)
	bool bGamepadInvertX;

	UPROPERTY(Config)
	bool bGamepadInvertY;
	//***** Controls Collection Tab *****//

	UPROPERTY(Transient)
	TObjectPtr<class USoundMix> SettingsSoundMix = nullptr;

	UPROPERTY(Transient)
	bool bHasAppliedShowFPSCounterState = false;

	UPROPERTY(Transient)
	bool bAppliedShowFPSCounter = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UEMRFPSCounterWidget>> ActiveFPSCounterWidgets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UEMRGameplayKeybindHelperWidget>> ActiveKeybindHelperWidgets;

	UPROPERTY(Config)
	FString StartupShaderOptimizationKey;

	UPROPERTY(Config)
	bool bStartupShaderOptimizationCompleted = false;
	
};
