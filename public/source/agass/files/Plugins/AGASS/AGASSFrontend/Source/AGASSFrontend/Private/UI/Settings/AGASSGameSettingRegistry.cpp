#include "UI/Settings/AGASSGameSettingRegistry.h"

#include "EditCondition/WhenCondition.h"
#include "Engine/GameInstance.h"
#include "EnhancedInputSubsystems.h"
#include "GameSettingCollection.h"
#include "GameSettingValueDiscreteDynamic.h"
#include "GameSettingValueScalarDynamic.h"
#include "Player/AGASSLocalPlayer.h"
#include "UI/Settings/AGASSGameSettingKeyBind.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#define LOCTEXT_NAMESPACE "AGASSSettings"

#define GET_LOCAL_SETTINGS_FUNCTION_PATH(FunctionOrPropertyName)						\
	MakeShared<FGameSettingDataSourceDynamic>(TArray<FString>({							\
		GET_FUNCTION_NAME_STRING_CHECKED(UAGASSLocalPlayer, GetLocalSettings),			\
		GET_FUNCTION_NAME_STRING_CHECKED(UAGASSSettingsLocal, FunctionOrPropertyName)	\
	}))

const FName UAGASSGameSettingRegistry::VideoCollection(TEXT("VideoCollection"));
const FName UAGASSGameSettingRegistry::AudioCollection(TEXT("AudioCollection"));
const FName UAGASSGameSettingRegistry::GameplayCollection(TEXT("GameplayCollection"));
const FName UAGASSGameSettingRegistry::ControlsCollection(TEXT("ControlsCollection"));
const FName UAGASSGameSettingRegistry::MouseKeyboardCollection(TEXT("MouseKeyboardCollection"));
const FName UAGASSGameSettingRegistry::GamepadCollection(TEXT("GamepadCollection"));
const FName UAGASSGameSettingRegistry::InterfaceCollection(TEXT("InterfaceCollection"));
const FName UAGASSGameSettingRegistry::KeyboardBindingsSection(TEXT("KeyboardBindingsSection"));
const FName UAGASSGameSettingRegistry::GamepadBindingsSection(TEXT("GamepadBindingsSection"));
const FName UAGASSGameSettingRegistry::RebindAdjustDistanceKeyboard(TEXT("KBM_AdjustDistanceItem"));
const FName UAGASSGameSettingRegistry::RebindInteractKeyboard(TEXT("KBM_Interact"));
const FName UAGASSGameSettingRegistry::RebindJumpKeyboard(TEXT("KBM_Jump"));
const FName UAGASSGameSettingRegistry::RebindMoveLeftKeyboard(TEXT("KBM_MoveLeft"));
const FName UAGASSGameSettingRegistry::RebindMoveRightKeyboard(TEXT("KBM_MoveRight"));
const FName UAGASSGameSettingRegistry::RebindMoveForwardKeyboard(TEXT("KBM_MoveForward"));
const FName UAGASSGameSettingRegistry::RebindMoveBackwardKeyboard(TEXT("KBM_MoveBackward"));
const FName UAGASSGameSettingRegistry::RebindAdjustDistanceDecreaseGamepad(TEXT("GP_AdjustDistanceDecrease"));
const FName UAGASSGameSettingRegistry::RebindAdjustDistanceIncreaseGamepad(TEXT("GP_AdjustDistanceIncrease"));
const FName UAGASSGameSettingRegistry::RebindInteractGamepad(TEXT("GP_Interact"));
const FName UAGASSGameSettingRegistry::RebindJumpGamepad(TEXT("GP_Jump"));

namespace
{
	UGameSettingCollectionPage* CreatePage(
		UObject* Outer,
		const FName& DevName,
		const FText& DisplayName,
		const FText& Description,
		const FText& NavigationText)
	{
		UGameSettingCollectionPage* const Page = NewObject<UGameSettingCollectionPage>(Outer);
		Page->SetDevName(DevName);
		Page->SetDisplayName(DisplayName);
		Page->SetDescriptionRichText(Description);
		Page->SetNavigationText(NavigationText);
		return Page;
	}

	UGameSettingCollection* CreateSection(
		UObject* Outer,
		const FName& DevName,
		const FText& DisplayName,
		const FText& Description)
	{
		UGameSettingCollection* const Section = NewObject<UGameSettingCollection>(Outer);
		Section->SetDevName(DevName);
		Section->SetDisplayName(DisplayName);
		Section->SetDescriptionRichText(Description);
		return Section;
	}

	void AddResolutionOption(UGameSettingValueDiscreteDynamic* Setting, TSet<FString>& AddedOptions, const FString& ResolutionString)
	{
		if (Setting == nullptr)
		{
			return;
		}

		const FString TrimmedResolution = ResolutionString.TrimStartAndEnd();
		if (TrimmedResolution.IsEmpty() || AddedOptions.Contains(TrimmedResolution))
		{
			return;
		}

		AddedOptions.Add(TrimmedResolution);
		Setting->AddDynamicOption(TrimmedResolution, FText::FromString(TrimmedResolution));
	}

	void AddCommonResolutionOptions(UGameSettingValueDiscreteDynamic* Setting, const UAGASSSettingsLocal* LocalSettings)
	{
		TSet<FString> AddedOptions;

		if (LocalSettings != nullptr)
		{
			const FIntPoint DesktopResolution = LocalSettings->GetDesktopResolution();
			if (DesktopResolution.X > 0 && DesktopResolution.Y > 0)
			{
				AddResolutionOption(Setting, AddedOptions, FString::Printf(TEXT("%dx%d"), DesktopResolution.X, DesktopResolution.Y));
			}

			AddResolutionOption(Setting, AddedOptions, LocalSettings->GetScreenResolutionString());
		}

		AddResolutionOption(Setting, AddedOptions, TEXT("1280x720"));
		AddResolutionOption(Setting, AddedOptions, TEXT("1600x900"));
		AddResolutionOption(Setting, AddedOptions, TEXT("1920x1080"));
		AddResolutionOption(Setting, AddedOptions, TEXT("2560x1440"));
		AddResolutionOption(Setting, AddedOptions, TEXT("3840x2160"));
	}

	void AddQualityOptions(UGameSettingValueDiscreteDynamic_Number* Setting)
	{
		if (Setting == nullptr)
		{
			return;
		}

		Setting->AddOption(0, LOCTEXT("QualityLow", "Low"));
		Setting->AddOption(1, LOCTEXT("QualityMedium", "Medium"));
		Setting->AddOption(2, LOCTEXT("QualityHigh", "High"));
		Setting->AddOption(3, LOCTEXT("QualityEpic", "Epic"));
	}

	void AddOverallQualityOptions(UGameSettingValueDiscreteDynamic_Number* Setting)
	{
		if (Setting == nullptr)
		{
			return;
		}

		Setting->AddOption(0, LOCTEXT("OverallQualityLow", "Low"));
		Setting->AddOption(1, LOCTEXT("OverallQualityMedium", "Medium"));
		Setting->AddOption(2, LOCTEXT("OverallQualityHigh", "High"));
		Setting->AddOption(3, LOCTEXT("OverallQualityEpic", "Epic"));
		Setting->AddOption(4, LOCTEXT("OverallQualityCinematic", "Cinematic"));
	}

	UEnhancedInputUserSettings* GetEnhancedInputUserSettings(ULocalPlayer* LocalPlayer)
	{
		if (LocalPlayer != nullptr)
		{
			if (UEnhancedInputLocalPlayerSubsystem* const InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				return InputSubsystem->GetUserSettings();
			}
		}

		return nullptr;
	}

	UAGASSGameSettingKeyBind* CreateKeyBindSetting(
		UAGASSGameSettingRegistry* Registry,
		ULocalPlayer* LocalPlayer,
		const FName DevName,
		const FText& DisplayName,
		const FText& Description,
		const EAGASSRebindInputType InputType,
		const bool bAllowAxisKeys = false,
		const bool bRequireMouseWheelAxis = false)
	{
		check(Registry != nullptr);
		check(LocalPlayer != nullptr);

		UAGASSGameSettingKeyBind* const Setting = NewObject<UAGASSGameSettingKeyBind>(Registry);
		Setting->SetDevName(DevName);
		Setting->SetDisplayName(DisplayName);
		Setting->SetDescriptionRichText(Description);
		Setting->InitializeInputData(LocalPlayer, DevName, InputType, bAllowAxisKeys, bRequireMouseWheelAxis);
		return Setting;
	}

	void BuildVideoSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage, ULocalPlayer* LocalPlayer, UAGASSSettingsLocal* LocalSettings);
	void BuildAudioSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage);
	void BuildGameplaySettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage);
	void BuildControlsSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage, ULocalPlayer* LocalPlayer);
	void BuildMouseKeyboardSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollection* RootCollection, ULocalPlayer* LocalPlayer);
	void BuildGamepadSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollection* RootCollection, ULocalPlayer* LocalPlayer);
	void BuildInterfaceSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage);

	void BuildVideoSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage, ULocalPlayer* LocalPlayer, UAGASSSettingsLocal* LocalSettings)
	{
		check(Registry != nullptr);
		check(RootPage != nullptr);
		check(LocalPlayer != nullptr);

		UGameSettingCollection* const DisplayPage = CreateSection(
			Registry,
			TEXT("VideoDisplaySection"),
			LOCTEXT("VideoDisplayPageName", "Display"),
			LOCTEXT("VideoDisplayPageDescription", "Configure resolution, frame pacing, and display behavior for AGASS."));
		RootPage->AddSetting(DisplayPage);

		UGameSettingCollection* const GraphicsPage = CreateSection(
			Registry,
			TEXT("GraphicsQualitySection"),
			LOCTEXT("GraphicsQualityPageName", "Graphics Quality"),
			LOCTEXT("GraphicsQualityPageDescription", "Tune rendering quality and performance-sensitive graphics settings."));
		RootPage->AddSetting(GraphicsPage);

		UGameSettingValueDiscreteDynamic_Enum* WindowModeSetting = NewObject<UGameSettingValueDiscreteDynamic_Enum>(Registry);
		WindowModeSetting->SetDevName(TEXT("WindowMode"));
		WindowModeSetting->SetDisplayName(LOCTEXT("WindowModeName", "Window Mode"));
		WindowModeSetting->SetDescriptionRichText(LOCTEXT("WindowModeDescription", "Choose how AGASS presents the game window on your display."));
		WindowModeSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetFullscreenMode));
		WindowModeSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetFullscreenMode));
		WindowModeSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetFullscreenMode());
		WindowModeSetting->AddEnumOption(EWindowMode::Fullscreen, LOCTEXT("WindowModeFullscreen", "Fullscreen"));
		WindowModeSetting->AddEnumOption(EWindowMode::WindowedFullscreen, LOCTEXT("WindowModeBorderless", "Borderless Window"));
		WindowModeSetting->AddEnumOption(EWindowMode::Windowed, LOCTEXT("WindowModeWindowed", "Windowed"));
		DisplayPage->AddSetting(WindowModeSetting);

		UGameSettingValueDiscreteDynamic* ResolutionSetting = NewObject<UGameSettingValueDiscreteDynamic>(Registry);
		ResolutionSetting->SetDevName(TEXT("Resolution"));
		ResolutionSetting->SetDisplayName(LOCTEXT("ResolutionName", "Resolution"));
		ResolutionSetting->SetDescriptionRichText(LOCTEXT("ResolutionDescription", "Set the render output resolution used by AGASS."));
		ResolutionSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetScreenResolutionString));
		ResolutionSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetScreenResolutionString));
		ResolutionSetting->SetDefaultValueFromString(GetDefault<UAGASSSettingsLocal>()->GetScreenResolutionString());
		AddCommonResolutionOptions(ResolutionSetting, LocalSettings);
		ResolutionSetting->AddEditDependency(WindowModeSetting);
		ResolutionSetting->AddEditCondition(MakeShared<FWhenCondition>(
			[WindowModeSetting](const ULocalPlayer*, FGameSettingEditableState& InOutEditState)
			{
				if (WindowModeSetting->GetValue<EWindowMode::Type>() == EWindowMode::WindowedFullscreen)
				{
					InOutEditState.Disable(LOCTEXT("ResolutionDisabledInBorderless", "Borderless Window uses the desktop resolution."));
				}
			}));
		DisplayPage->AddSetting(ResolutionSetting);

		UGameSettingValueDiscreteDynamic_Number* FrameRateSetting = NewObject<UGameSettingValueDiscreteDynamic_Number>(Registry);
		FrameRateSetting->SetDevName(TEXT("FrameRateLimit"));
		FrameRateSetting->SetDisplayName(LOCTEXT("FrameRateLimitName", "Frame Rate Limit"));
		FrameRateSetting->SetDescriptionRichText(LOCTEXT("FrameRateLimitDescription", "Limit how many frames per second AGASS renders."));
		FrameRateSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetFrameRateLimit));
		FrameRateSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetFrameRateLimit));
		FrameRateSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetFrameRateLimit());
		FrameRateSetting->AddOption(0.0f, LOCTEXT("FrameRateUnlimited", "Unlimited"));
		FrameRateSetting->AddOption(30.0f, LOCTEXT("FrameRate30", "30 FPS"));
		FrameRateSetting->AddOption(60.0f, LOCTEXT("FrameRate60", "60 FPS"));
		FrameRateSetting->AddOption(90.0f, LOCTEXT("FrameRate90", "90 FPS"));
		FrameRateSetting->AddOption(120.0f, LOCTEXT("FrameRate120", "120 FPS"));
		FrameRateSetting->AddOption(144.0f, LOCTEXT("FrameRate144", "144 FPS"));
		FrameRateSetting->AddOption(165.0f, LOCTEXT("FrameRate165", "165 FPS"));
		FrameRateSetting->AddOption(240.0f, LOCTEXT("FrameRate240", "240 FPS"));
		DisplayPage->AddSetting(FrameRateSetting);

		UGameSettingValueDiscreteDynamic_Bool* VSyncSetting = NewObject<UGameSettingValueDiscreteDynamic_Bool>(Registry);
		VSyncSetting->SetDevName(TEXT("VerticalSync"));
		VSyncSetting->SetDisplayName(LOCTEXT("VerticalSyncName", "Vertical Sync"));
		VSyncSetting->SetDescriptionRichText(LOCTEXT("VerticalSyncDescription", "Reduce tearing by synchronizing rendered frames to the display refresh."));
		VSyncSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(IsVSyncEnabled));
		VSyncSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetVSyncEnabled));
		VSyncSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->IsVSyncEnabled());
		DisplayPage->AddSetting(VSyncSetting);

		UGameSettingValueDiscreteDynamic_Bool* DynamicResolutionSetting = NewObject<UGameSettingValueDiscreteDynamic_Bool>(Registry);
		DynamicResolutionSetting->SetDevName(TEXT("DynamicResolution"));
		DynamicResolutionSetting->SetDisplayName(LOCTEXT("DynamicResolutionName", "Dynamic Resolution"));
		DynamicResolutionSetting->SetDescriptionRichText(LOCTEXT("DynamicResolutionDescription", "Allow AGASS to adjust resolution scaling at runtime to stabilize performance."));
		DynamicResolutionSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(IsDynamicResolutionEnabled));
		DynamicResolutionSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetDynamicResolutionEnabled));
		DynamicResolutionSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->IsDynamicResolutionEnabled());
		DisplayPage->AddSetting(DynamicResolutionSetting);

		UGameSettingValueScalarDynamic* BrightnessSetting = NewObject<UGameSettingValueScalarDynamic>(Registry);
		BrightnessSetting->SetDevName(TEXT("Brightness"));
		BrightnessSetting->SetDisplayName(LOCTEXT("BrightnessName", "Brightness"));
		BrightnessSetting->SetDescriptionRichText(LOCTEXT("BrightnessDescription", "Adjust the display gamma used while rendering AGASS."));
		BrightnessSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetDisplayGamma));
		BrightnessSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetDisplayGamma));
		BrightnessSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetDisplayGamma());
		BrightnessSetting->SetSourceRangeAndStep(TRange<double>(1.7, 2.7), 0.01);
		BrightnessSetting->SetDisplayFormat([](double SourceValue, double NormalizedValue)
		{
			return FText::Format(LOCTEXT("BrightnessFormat", "{0}%"), FText::AsNumber(FMath::RoundToInt(FMath::GetMappedRangeValueClamped(FVector2D(0.0, 1.0), FVector2D(50.0, 150.0), NormalizedValue))));
		});
		DisplayPage->AddSetting(BrightnessSetting);

		UGameSettingValueDiscreteDynamic_Number* OverallQualitySetting = NewObject<UGameSettingValueDiscreteDynamic_Number>(Registry);
		OverallQualitySetting->SetDevName(TEXT("OverallQuality"));
		OverallQualitySetting->SetDisplayName(LOCTEXT("OverallQualityName", "Quality Preset"));
		OverallQualitySetting->SetDescriptionRichText(LOCTEXT("OverallQualityDescription", "Set an overall rendering quality target for AGASS."));
		OverallQualitySetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetOverallScalabilityLevel));
		OverallQualitySetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetOverallScalabilityLevel));
		OverallQualitySetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetOverallScalabilityLevel());
		AddOverallQualityOptions(OverallQualitySetting);
		GraphicsPage->AddSetting(OverallQualitySetting);

		UGameSettingValueScalarDynamic* ResolutionScaleSetting = NewObject<UGameSettingValueScalarDynamic>(Registry);
		ResolutionScaleSetting->SetDevName(TEXT("ResolutionScale"));
		ResolutionScaleSetting->SetDisplayName(LOCTEXT("ResolutionScaleName", "3D Resolution"));
		ResolutionScaleSetting->SetDescriptionRichText(LOCTEXT("ResolutionScaleDescription", "Scale the 3D rendering resolution while keeping the UI at full resolution."));
		ResolutionScaleSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetResolutionScaleNormalized));
		ResolutionScaleSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetResolutionScaleNormalized));
		ResolutionScaleSetting->SetDisplayFormat(UGameSettingValueScalarDynamic::ZeroToOnePercent);
		GraphicsPage->AddSetting(ResolutionScaleSetting);

		auto AddQualitySetting = [Registry, GraphicsPage](const TCHAR* DevName, const FText& Name, const FText& Description, const TSharedRef<FGameSettingDataSourceDynamic>& Getter, const TSharedRef<FGameSettingDataSourceDynamic>& Setter)
		{
			UGameSettingValueDiscreteDynamic_Number* const Setting = NewObject<UGameSettingValueDiscreteDynamic_Number>(Registry);
			Setting->SetDevName(DevName);
			Setting->SetDisplayName(Name);
			Setting->SetDescriptionRichText(Description);
			Setting->SetDynamicGetter(Getter);
			Setting->SetDynamicSetter(Setter);
			AddQualityOptions(Setting);
			GraphicsPage->AddSetting(Setting);
		};

		AddQualitySetting(TEXT("ViewDistanceQuality"), LOCTEXT("ViewDistanceName", "View Distance"), LOCTEXT("ViewDistanceDescription", "Control how aggressively distant objects are culled."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetViewDistanceQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetViewDistanceQuality));
		AddQualitySetting(TEXT("GlobalIlluminationQuality"), LOCTEXT("GlobalIlluminationName", "Global Illumination"), LOCTEXT("GlobalIlluminationDescription", "Adjust indirect lighting quality and related performance cost."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetGlobalIlluminationQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetGlobalIlluminationQuality));
		AddQualitySetting(TEXT("ReflectionQuality"), LOCTEXT("ReflectionQualityName", "Reflections"), LOCTEXT("ReflectionQualityDescription", "Adjust reflection detail and accuracy."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetReflectionQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetReflectionQuality));
		AddQualitySetting(TEXT("AntiAliasingQuality"), LOCTEXT("AntiAliasingName", "Anti-Aliasing"), LOCTEXT("AntiAliasingDescription", "Smooth jagged edges at the cost of some rendering performance."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetAntiAliasingQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetAntiAliasingQuality));
		AddQualitySetting(TEXT("ShadowQuality"), LOCTEXT("ShadowQualityName", "Shadows"), LOCTEXT("ShadowQualityDescription", "Adjust shadow detail and range."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetShadowQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetShadowQuality));
		AddQualitySetting(TEXT("PostProcessingQuality"), LOCTEXT("PostProcessingName", "Post Processing"), LOCTEXT("PostProcessingDescription", "Adjust bloom, depth of field, and other post effects."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetPostProcessingQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetPostProcessingQuality));
		AddQualitySetting(TEXT("TextureQuality"), LOCTEXT("TextureQualityName", "Textures"), LOCTEXT("TextureQualityDescription", "Adjust the detail level of texture streaming and mip selection."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetTextureQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetTextureQuality));
		AddQualitySetting(TEXT("VisualEffectQuality"), LOCTEXT("EffectsQualityName", "Effects"), LOCTEXT("EffectsQualityDescription", "Adjust the fidelity of particle and transient visual effects."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetVisualEffectQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetVisualEffectQuality));
		AddQualitySetting(TEXT("FoliageQuality"), LOCTEXT("FoliageQualityName", "Foliage"), LOCTEXT("FoliageQualityDescription", "Adjust foliage density and rendering quality."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetFoliageQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetFoliageQuality));
		AddQualitySetting(TEXT("ShadingQuality"), LOCTEXT("ShadingQualityName", "Shading"), LOCTEXT("ShadingQualityDescription", "Adjust material and shading complexity."), GET_LOCAL_SETTINGS_FUNCTION_PATH(GetShadingQuality), GET_LOCAL_SETTINGS_FUNCTION_PATH(SetShadingQuality));
	}

	void BuildAudioSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage)
	{
		check(Registry != nullptr);
		check(RootPage != nullptr);

		UGameSettingValueScalarDynamic* MasterVolumeSetting = NewObject<UGameSettingValueScalarDynamic>(Registry);
		MasterVolumeSetting->SetDevName(TEXT("MasterVolume"));
		MasterVolumeSetting->SetDisplayName(LOCTEXT("MasterVolumeName", "Master Volume"));
		MasterVolumeSetting->SetDescriptionRichText(LOCTEXT("MasterVolumeDescription", "Set the overall audio output volume for AGASS."));
		MasterVolumeSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetMasterVolume));
		MasterVolumeSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetMasterVolume));
		MasterVolumeSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetMasterVolume());
		MasterVolumeSetting->SetDisplayFormat(UGameSettingValueScalarDynamic::ZeroToOnePercent);
		RootPage->AddSetting(MasterVolumeSetting);

		UGameSettingValueDiscreteDynamic_Number* AudioQualitySetting = NewObject<UGameSettingValueDiscreteDynamic_Number>(Registry);
		AudioQualitySetting->SetDevName(TEXT("AudioQuality"));
		AudioQualitySetting->SetDisplayName(LOCTEXT("AudioQualityName", "Audio Quality"));
		AudioQualitySetting->SetDescriptionRichText(LOCTEXT("AudioQualityDescription", "Adjust the audio mix quality level used by AGASS."));
		AudioQualitySetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetAudioQualityLevel));
		AudioQualitySetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetAudioQualityLevel));
		AudioQualitySetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetAudioQualityLevel());
		AddQualityOptions(AudioQualitySetting);
		RootPage->AddSetting(AudioQualitySetting);
	}

	void BuildGameplaySettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage)
	{
		check(Registry != nullptr);
		check(RootPage != nullptr);

		UGameSettingValueScalarDynamic* FieldOfViewSetting = NewObject<UGameSettingValueScalarDynamic>(Registry);
		FieldOfViewSetting->SetDevName(TEXT("GameplayFieldOfView"));
		FieldOfViewSetting->SetDisplayName(LOCTEXT("GameplayFieldOfViewName", "Field Of View"));
		FieldOfViewSetting->SetDescriptionRichText(LOCTEXT("GameplayFieldOfViewDescription", "Adjust the local first-person field of view."));
		FieldOfViewSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetGameplayFieldOfView));
		FieldOfViewSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetGameplayFieldOfView));
		FieldOfViewSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetGameplayFieldOfView());
		FieldOfViewSetting->SetSourceRangeAndStep(TRange<double>(80.0, 110.0), 1.0);
		FieldOfViewSetting->SetDisplayFormat(UGameSettingValueScalarDynamic::SourceAsInteger);
		RootPage->AddSetting(FieldOfViewSetting);

		UGameSettingValueScalarDynamic* PlacementDistanceSpeedSetting = NewObject<UGameSettingValueScalarDynamic>(Registry);
		PlacementDistanceSpeedSetting->SetDevName(TEXT("PlacementDistanceSpeedScale"));
		PlacementDistanceSpeedSetting->SetDisplayName(LOCTEXT("PlacementDistanceSpeedName", "Placement Distance Speed"));
		PlacementDistanceSpeedSetting->SetDescriptionRichText(LOCTEXT("PlacementDistanceSpeedDescription", "Scale how quickly placement preview distance changes in response to input."));
		PlacementDistanceSpeedSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetPlacementDistanceSpeedScale));
		PlacementDistanceSpeedSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetPlacementDistanceSpeedScale));
		PlacementDistanceSpeedSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetPlacementDistanceSpeedScale());
		PlacementDistanceSpeedSetting->SetSourceRangeAndStep(TRange<double>(0.5, 2.0), 0.05);
		PlacementDistanceSpeedSetting->SetDisplayFormat(UGameSettingValueScalarDynamic::RawTwoDecimals);
		RootPage->AddSetting(PlacementDistanceSpeedSetting);

		UGameSettingValueScalarDynamic* PlacementRotationSensitivitySetting = NewObject<UGameSettingValueScalarDynamic>(Registry);
		PlacementRotationSensitivitySetting->SetDevName(TEXT("PlacementRotationSensitivity"));
		PlacementRotationSensitivitySetting->SetDisplayName(LOCTEXT("PlacementRotationSensitivityName", "Placement Rotation Sensitivity"));
		PlacementRotationSensitivitySetting->SetDescriptionRichText(LOCTEXT("PlacementRotationSensitivityDescription", "Scale how quickly placement rotation mode reacts to look input."));
		PlacementRotationSensitivitySetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetPlacementRotationSensitivity));
		PlacementRotationSensitivitySetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetPlacementRotationSensitivity));
		PlacementRotationSensitivitySetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetPlacementRotationSensitivity());
		PlacementRotationSensitivitySetting->SetSourceRangeAndStep(TRange<double>(0.5, 2.0), 0.05);
		PlacementRotationSensitivitySetting->SetDisplayFormat(UGameSettingValueScalarDynamic::RawTwoDecimals);
		RootPage->AddSetting(PlacementRotationSensitivitySetting);
	}

	void BuildControlsSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage, ULocalPlayer* LocalPlayer)
	{
		check(Registry != nullptr);
		check(RootPage != nullptr);
		check(LocalPlayer != nullptr);

		UGameSettingCollection* const MouseKeyboardSection = CreateSection(
			Registry,
			UAGASSGameSettingRegistry::MouseKeyboardCollection,
			LOCTEXT("ControlsMouseKeyboardSectionName", "Mouse & Keyboard"),
			LOCTEXT("ControlsMouseKeyboardSectionDescription", "Configure mouse sensitivity, axis inversion, and keyboard-oriented control behavior."));
		RootPage->AddSetting(MouseKeyboardSection);
		BuildMouseKeyboardSettings(Registry, MouseKeyboardSection, LocalPlayer);

		UGameSettingCollection* const GamepadSection = CreateSection(
			Registry,
			UAGASSGameSettingRegistry::GamepadCollection,
			LOCTEXT("ControlsGamepadSectionName", "Gamepad"),
			LOCTEXT("ControlsGamepadSectionDescription", "Configure gamepad sensitivity, dead zones, axis inversion, and controller-oriented control behavior."));
		RootPage->AddSetting(GamepadSection);
		BuildGamepadSettings(Registry, GamepadSection, LocalPlayer);
	}

	void BuildMouseKeyboardSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollection* RootCollection, ULocalPlayer* LocalPlayer)
	{
		check(Registry != nullptr);
		check(RootCollection != nullptr);
		check(LocalPlayer != nullptr);

		UGameSettingValueScalarDynamic* HorizontalSensitivity = NewObject<UGameSettingValueScalarDynamic>(Registry);
		HorizontalSensitivity->SetDevName(TEXT("MouseLookSensitivityHorizontal"));
		HorizontalSensitivity->SetDisplayName(LOCTEXT("MouseLookHorizontalName", "Horizontal Look Sensitivity"));
		HorizontalSensitivity->SetDescriptionRichText(LOCTEXT("MouseLookHorizontalDescription", "Adjust horizontal mouse look sensitivity."));
		HorizontalSensitivity->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetMouseLookSensitivityHorizontal));
		HorizontalSensitivity->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetMouseLookSensitivityHorizontal));
		HorizontalSensitivity->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetMouseLookSensitivityHorizontal());
		HorizontalSensitivity->SetSourceRangeAndStep(TRange<double>(0.05, 3.0), 0.05);
		HorizontalSensitivity->SetDisplayFormat(UGameSettingValueScalarDynamic::RawTwoDecimals);
		RootCollection->AddSetting(HorizontalSensitivity);

		UGameSettingValueScalarDynamic* VerticalSensitivity = NewObject<UGameSettingValueScalarDynamic>(Registry);
		VerticalSensitivity->SetDevName(TEXT("MouseLookSensitivityVertical"));
		VerticalSensitivity->SetDisplayName(LOCTEXT("MouseLookVerticalName", "Vertical Look Sensitivity"));
		VerticalSensitivity->SetDescriptionRichText(LOCTEXT("MouseLookVerticalDescription", "Adjust vertical mouse look sensitivity."));
		VerticalSensitivity->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetMouseLookSensitivityVertical));
		VerticalSensitivity->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetMouseLookSensitivityVertical));
		VerticalSensitivity->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetMouseLookSensitivityVertical());
		VerticalSensitivity->SetSourceRangeAndStep(TRange<double>(0.05, 3.0), 0.05);
		VerticalSensitivity->SetDisplayFormat(UGameSettingValueScalarDynamic::RawTwoDecimals);
		RootCollection->AddSetting(VerticalSensitivity);

		UGameSettingValueDiscreteDynamic_Bool* InvertHorizontal = NewObject<UGameSettingValueDiscreteDynamic_Bool>(Registry);
		InvertHorizontal->SetDevName(TEXT("InvertMouseHorizontal"));
		InvertHorizontal->SetDisplayName(LOCTEXT("InvertMouseHorizontalName", "Invert Horizontal Axis"));
		InvertHorizontal->SetDescriptionRichText(LOCTEXT("InvertMouseHorizontalDescription", "Invert the horizontal mouse look direction."));
		InvertHorizontal->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetInvertMouseHorizontal));
		InvertHorizontal->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetInvertMouseHorizontal));
		InvertHorizontal->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetInvertMouseHorizontal());
		RootCollection->AddSetting(InvertHorizontal);

		UGameSettingValueDiscreteDynamic_Bool* InvertVertical = NewObject<UGameSettingValueDiscreteDynamic_Bool>(Registry);
		InvertVertical->SetDevName(TEXT("InvertMouseVertical"));
		InvertVertical->SetDisplayName(LOCTEXT("InvertMouseVerticalName", "Invert Vertical Axis"));
		InvertVertical->SetDescriptionRichText(LOCTEXT("InvertMouseVerticalDescription", "Invert the vertical mouse look direction."));
		InvertVertical->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetInvertMouseVertical));
		InvertVertical->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetInvertMouseVertical));
		InvertVertical->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetInvertMouseVertical());
		RootCollection->AddSetting(InvertVertical);

		if (GetEnhancedInputUserSettings(LocalPlayer) == nullptr)
		{
			return;
		}

		UGameSettingCollection* const KeyboardBindingsSection = CreateSection(
			Registry,
			UAGASSGameSettingRegistry::KeyboardBindingsSection,
			LOCTEXT("KeyboardBindingsSectionName", "Bindings"),
			LOCTEXT("KeyboardBindingsSectionDescription", "Customize keyboard and mouse bindings for gameplay actions."));
		RootCollection->AddSetting(KeyboardBindingsSection);

		KeyboardBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindAdjustDistanceKeyboard,
			LOCTEXT("RebindAdjustDistanceKeyboardName", "Adjust Distance"),
			LOCTEXT("RebindAdjustDistanceKeyboardDescription", "Change the keyboard or mouse-wheel binding used to adjust placement preview distance."),
			EAGASSRebindInputType::MouseAndKeyboard,
			true,
			true));

		KeyboardBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindInteractKeyboard,
			LOCTEXT("RebindInteractKeyboardName", "Interact"),
			LOCTEXT("RebindInteractKeyboardDescription", "Change the keyboard or mouse binding used to interact."),
			EAGASSRebindInputType::MouseAndKeyboard));

		KeyboardBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindJumpKeyboard,
			LOCTEXT("RebindJumpKeyboardName", "Jump"),
			LOCTEXT("RebindJumpKeyboardDescription", "Change the keyboard or mouse binding used to jump."),
			EAGASSRebindInputType::MouseAndKeyboard));

		KeyboardBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindMoveLeftKeyboard,
			LOCTEXT("RebindMoveLeftKeyboardName", "Move Left"),
			LOCTEXT("RebindMoveLeftKeyboardDescription", "Change the keyboard binding used to move left."),
			EAGASSRebindInputType::MouseAndKeyboard));

		KeyboardBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindMoveRightKeyboard,
			LOCTEXT("RebindMoveRightKeyboardName", "Move Right"),
			LOCTEXT("RebindMoveRightKeyboardDescription", "Change the keyboard binding used to move right."),
			EAGASSRebindInputType::MouseAndKeyboard));

		KeyboardBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindMoveForwardKeyboard,
			LOCTEXT("RebindMoveForwardKeyboardName", "Move Forward"),
			LOCTEXT("RebindMoveForwardKeyboardDescription", "Change the keyboard binding used to move forward."),
			EAGASSRebindInputType::MouseAndKeyboard));

		KeyboardBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindMoveBackwardKeyboard,
			LOCTEXT("RebindMoveBackwardKeyboardName", "Move Backward"),
			LOCTEXT("RebindMoveBackwardKeyboardDescription", "Change the keyboard binding used to move backward."),
			EAGASSRebindInputType::MouseAndKeyboard));
	}

	void BuildGamepadSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollection* RootCollection, ULocalPlayer* LocalPlayer)
	{
		check(Registry != nullptr);
		check(RootCollection != nullptr);
		check(LocalPlayer != nullptr);

		UGameSettingValueScalarDynamic* HorizontalSensitivity = NewObject<UGameSettingValueScalarDynamic>(Registry);
		HorizontalSensitivity->SetDevName(TEXT("GamepadLookSensitivityHorizontal"));
		HorizontalSensitivity->SetDisplayName(LOCTEXT("GamepadLookHorizontalName", "Horizontal Look Sensitivity"));
		HorizontalSensitivity->SetDescriptionRichText(LOCTEXT("GamepadLookHorizontalDescription", "Adjust horizontal gamepad look sensitivity."));
		HorizontalSensitivity->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetGamepadLookSensitivityHorizontal));
		HorizontalSensitivity->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetGamepadLookSensitivityHorizontal));
		HorizontalSensitivity->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetGamepadLookSensitivityHorizontal());
		HorizontalSensitivity->SetSourceRangeAndStep(TRange<double>(0.05, 3.0), 0.05);
		HorizontalSensitivity->SetDisplayFormat(UGameSettingValueScalarDynamic::RawTwoDecimals);
		RootCollection->AddSetting(HorizontalSensitivity);

		UGameSettingValueScalarDynamic* VerticalSensitivity = NewObject<UGameSettingValueScalarDynamic>(Registry);
		VerticalSensitivity->SetDevName(TEXT("GamepadLookSensitivityVertical"));
		VerticalSensitivity->SetDisplayName(LOCTEXT("GamepadLookVerticalName", "Vertical Look Sensitivity"));
		VerticalSensitivity->SetDescriptionRichText(LOCTEXT("GamepadLookVerticalDescription", "Adjust vertical gamepad look sensitivity."));
		VerticalSensitivity->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetGamepadLookSensitivityVertical));
		VerticalSensitivity->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetGamepadLookSensitivityVertical));
		VerticalSensitivity->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetGamepadLookSensitivityVertical());
		VerticalSensitivity->SetSourceRangeAndStep(TRange<double>(0.05, 3.0), 0.05);
		VerticalSensitivity->SetDisplayFormat(UGameSettingValueScalarDynamic::RawTwoDecimals);
		RootCollection->AddSetting(VerticalSensitivity);

		UGameSettingValueScalarDynamic* DeadZoneSetting = NewObject<UGameSettingValueScalarDynamic>(Registry);
		DeadZoneSetting->SetDevName(TEXT("GamepadLookDeadZone"));
		DeadZoneSetting->SetDisplayName(LOCTEXT("GamepadDeadZoneName", "Look Stick Dead Zone"));
		DeadZoneSetting->SetDescriptionRichText(LOCTEXT("GamepadDeadZoneDescription", "Set the dead zone used for right-stick look input."));
		DeadZoneSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetGamepadLookDeadZone));
		DeadZoneSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetGamepadLookDeadZone));
		DeadZoneSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetGamepadLookDeadZone());
		DeadZoneSetting->SetSourceRangeAndStep(TRange<double>(0.0, 0.35), 0.01);
		DeadZoneSetting->SetDisplayFormat([](double SourceValue, double NormalizedValue)
		{
			return FText::Format(LOCTEXT("DeadZonePercentFormat", "{0}%"), FText::AsNumber(FMath::RoundToInt(SourceValue * 100.0)));
		});
		RootCollection->AddSetting(DeadZoneSetting);

		UGameSettingValueDiscreteDynamic_Bool* InvertHorizontal = NewObject<UGameSettingValueDiscreteDynamic_Bool>(Registry);
		InvertHorizontal->SetDevName(TEXT("InvertGamepadHorizontal"));
		InvertHorizontal->SetDisplayName(LOCTEXT("InvertGamepadHorizontalName", "Invert Horizontal Axis"));
		InvertHorizontal->SetDescriptionRichText(LOCTEXT("InvertGamepadHorizontalDescription", "Invert the horizontal gamepad look direction."));
		InvertHorizontal->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetInvertGamepadHorizontal));
		InvertHorizontal->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetInvertGamepadHorizontal));
		InvertHorizontal->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetInvertGamepadHorizontal());
		RootCollection->AddSetting(InvertHorizontal);

		UGameSettingValueDiscreteDynamic_Bool* InvertVertical = NewObject<UGameSettingValueDiscreteDynamic_Bool>(Registry);
		InvertVertical->SetDevName(TEXT("InvertGamepadVertical"));
		InvertVertical->SetDisplayName(LOCTEXT("InvertGamepadVerticalName", "Invert Vertical Axis"));
		InvertVertical->SetDescriptionRichText(LOCTEXT("InvertGamepadVerticalDescription", "Invert the vertical gamepad look direction."));
		InvertVertical->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetInvertGamepadVertical));
		InvertVertical->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetInvertGamepadVertical));
		InvertVertical->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetInvertGamepadVertical());
		RootCollection->AddSetting(InvertVertical);

		if (GetEnhancedInputUserSettings(LocalPlayer) == nullptr)
		{
			return;
		}

		UGameSettingCollection* const GamepadBindingsSection = CreateSection(
			Registry,
			UAGASSGameSettingRegistry::GamepadBindingsSection,
			LOCTEXT("GamepadBindingsSectionName", "Bindings"),
			LOCTEXT("GamepadBindingsSectionDescription", "Customize controller bindings for gameplay actions."));
		RootCollection->AddSetting(GamepadBindingsSection);

		GamepadBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindAdjustDistanceDecreaseGamepad,
			LOCTEXT("RebindAdjustDistanceDecreaseGamepadName", "Adjust Distance Decrease"),
			LOCTEXT("RebindAdjustDistanceDecreaseGamepadDescription", "Change the controller trigger used to reduce placement preview distance."),
			EAGASSRebindInputType::Gamepad,
			true));

		GamepadBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindAdjustDistanceIncreaseGamepad,
			LOCTEXT("RebindAdjustDistanceIncreaseGamepadName", "Adjust Distance Increase"),
			LOCTEXT("RebindAdjustDistanceIncreaseGamepadDescription", "Change the controller trigger used to increase placement preview distance."),
			EAGASSRebindInputType::Gamepad,
			true));

		GamepadBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindInteractGamepad,
			LOCTEXT("RebindInteractGamepadName", "Interact"),
			LOCTEXT("RebindInteractGamepadDescription", "Change the controller button used to interact."),
			EAGASSRebindInputType::Gamepad));

		GamepadBindingsSection->AddSetting(CreateKeyBindSetting(
			Registry,
			LocalPlayer,
			UAGASSGameSettingRegistry::RebindJumpGamepad,
			LOCTEXT("RebindJumpGamepadName", "Jump"),
			LOCTEXT("RebindJumpGamepadDescription", "Change the controller button used to jump."),
			EAGASSRebindInputType::Gamepad));
	}

	void BuildInterfaceSettings(UAGASSGameSettingRegistry* Registry, UGameSettingCollectionPage* RootPage)
	{
		check(Registry != nullptr);
		check(RootPage != nullptr);

		UGameSettingValueDiscreteDynamic_Bool* SubtitlesSetting = NewObject<UGameSettingValueDiscreteDynamic_Bool>(Registry);
		SubtitlesSetting->SetDevName(TEXT("Subtitles"));
		SubtitlesSetting->SetDisplayName(LOCTEXT("SubtitlesName", "Subtitles"));
		SubtitlesSetting->SetDescriptionRichText(LOCTEXT("SubtitlesDescription", "Enable or disable subtitles."));
		SubtitlesSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(IsSubtitlesEnabled));
		SubtitlesSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetSubtitlesEnabled));
		SubtitlesSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->IsSubtitlesEnabled());
		RootPage->AddSetting(SubtitlesSetting);

		UGameSettingValueScalarDynamic* SafeZoneSetting = NewObject<UGameSettingValueScalarDynamic>(Registry);
		SafeZoneSetting->SetDevName(TEXT("SafeZone"));
		SafeZoneSetting->SetDisplayName(LOCTEXT("SafeZoneName", "Safe Zone"));
		SafeZoneSetting->SetDescriptionRichText(LOCTEXT("SafeZoneDescription", "Adjust the user interface safe zone scale."));
		SafeZoneSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(GetSafeZone));
		SafeZoneSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetSafeZone));
		SafeZoneSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->GetSafeZone());
		SafeZoneSetting->SetSourceRangeAndStep(TRange<double>(0.8, 1.0), 0.01);
		SafeZoneSetting->SetDisplayFormat([](double SourceValue, double NormalizedValue)
		{
			return FText::Format(LOCTEXT("SafeZonePercentFormat", "{0}%"), FText::AsNumber(FMath::RoundToInt(SourceValue * 100.0)));
		});
		RootPage->AddSetting(SafeZoneSetting);

		UGameSettingValueDiscreteDynamic_Bool* FrontendInstanceRoleSetting = NewObject<UGameSettingValueDiscreteDynamic_Bool>(Registry);
		FrontendInstanceRoleSetting->SetDevName(TEXT("ShowFrontendInstanceRole"));
		FrontendInstanceRoleSetting->SetDisplayName(LOCTEXT("ShowFrontendInstanceRoleName", "Show Frontend Network Role"));
		FrontendInstanceRoleSetting->SetDescriptionRichText(LOCTEXT("ShowFrontendInstanceRoleDescription", "Display the current frontend instance role label on menu screens."));
		FrontendInstanceRoleSetting->SetDynamicGetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(ShouldShowFrontendInstanceRole));
		FrontendInstanceRoleSetting->SetDynamicSetter(GET_LOCAL_SETTINGS_FUNCTION_PATH(SetShowFrontendInstanceRole));
		FrontendInstanceRoleSetting->SetDefaultValue(GetDefault<UAGASSSettingsLocal>()->ShouldShowFrontendInstanceRole());
		RootPage->AddSetting(FrontendInstanceRoleSetting);
	}
}

void UAGASSGameSettingRegistry::SaveChanges()
{
	Super::SaveChanges();

	if (UEnhancedInputUserSettings* const InputUserSettings = GetEnhancedInputUserSettings(OwningLocalPlayer))
	{
		InputUserSettings->ApplySettings();
		InputUserSettings->SaveSettings();
	}

	if (const UAGASSLocalPlayer* const AGASSLocalPlayer = Cast<UAGASSLocalPlayer>(OwningLocalPlayer))
	{
		if (UAGASSSettingsLocal* const LocalSettings = AGASSLocalPlayer->GetLocalSettings())
		{
			LocalSettings->ApplySettings(false);
			LocalSettings->SaveSettings();
		}
	}
}

void UAGASSGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	UAGASSLocalPlayer* const AGASSLocalPlayer = Cast<UAGASSLocalPlayer>(InLocalPlayer);
	UAGASSSettingsLocal* const LocalSettings = AGASSLocalPlayer != nullptr ? AGASSLocalPlayer->GetLocalSettings() : UAGASSSettingsLocal::Get();

	UGameSettingCollectionPage* const VideoPage = CreatePage(
		this,
		VideoCollection,
		LOCTEXT("VideoCollectionName", "Video"),
		LOCTEXT("VideoCollectionDescription", "Adjust display mode, rendering scale, and graphics quality."),
		LOCTEXT("VideoCollectionNav", "Video"));
	VideoPage->Initialize(InLocalPlayer);
	BuildVideoSettings(this, VideoPage, InLocalPlayer, LocalSettings);
	RegisterSetting(VideoPage);

	UGameSettingCollectionPage* const AudioPage = CreatePage(
		this,
		AudioCollection,
		LOCTEXT("AudioCollectionName", "Audio"),
		LOCTEXT("AudioCollectionDescription", "Adjust local AGASS audio output and mix quality."),
		LOCTEXT("AudioCollectionNav", "Audio"));
	AudioPage->Initialize(InLocalPlayer);
	BuildAudioSettings(this, AudioPage);
	RegisterSetting(AudioPage);

	UGameSettingCollectionPage* const GameplayPage = CreatePage(
		this,
		GameplayCollection,
		LOCTEXT("GameplayCollectionName", "Gameplay"),
		LOCTEXT("GameplayCollectionDescription", "Tune first-person camera and placement interaction behavior."),
		LOCTEXT("GameplayCollectionNav", "Gameplay"));
	GameplayPage->Initialize(InLocalPlayer);
	BuildGameplaySettings(this, GameplayPage);
	RegisterSetting(GameplayPage);

	UGameSettingCollectionPage* const ControlsPage = CreatePage(
		this,
		ControlsCollection,
		LOCTEXT("ControlsCollectionName", "Controls"),
		LOCTEXT("ControlsCollectionDescription", "Adjust mouse, keyboard, and gamepad behavior from a single controls screen."),
		LOCTEXT("ControlsCollectionNav", "Controls"));
	ControlsPage->Initialize(InLocalPlayer);
	BuildControlsSettings(this, ControlsPage, InLocalPlayer);
	RegisterSetting(ControlsPage);

	UGameSettingCollectionPage* const InterfacePage = CreatePage(
		this,
		InterfaceCollection,
		LOCTEXT("InterfaceCollectionName", "Interface"),
		LOCTEXT("InterfaceCollectionDescription", "Adjust subtitles, safe zone, and frontend visibility helpers."),
		LOCTEXT("InterfaceCollectionNav", "Interface"));
	InterfacePage->Initialize(InLocalPlayer);
	BuildInterfaceSettings(this, InterfacePage);
	RegisterSetting(InterfacePage);
}

#undef GET_LOCAL_SETTINGS_FUNCTION_PATH
#undef LOCTEXT_NAMESPACE
