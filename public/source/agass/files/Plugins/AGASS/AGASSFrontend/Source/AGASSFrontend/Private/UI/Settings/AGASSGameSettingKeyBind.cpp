#include "UI/Settings/AGASSGameSettingKeyBind.h"

#include "CommonInputBaseTypes.h"
#include "CommonInputSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputCoreTypes.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#define LOCTEXT_NAMESPACE "AGASSSettings"

void UAGASSGameSettingKeyBind::InitializeInputData(
	ULocalPlayer* InLocalPlayer,
	const FName InMappingName,
	const EAGASSRebindInputType InInputType,
	const bool bInAllowAxisKeys,
	const bool bInRequireMouseWheelAxis)
{
	RebindLocalPlayer = InLocalPlayer;
	MappingName = InMappingName;
	InputType = InInputType;
	bAllowAxisKeys = bInAllowAxisKeys;
	bRequireMouseWheelAxis = bInRequireMouseWheelAxis;

	Initialize(InLocalPlayer);
}

FText UAGASSGameSettingKeyBind::GetCurrentKeyText() const
{
	const FKey CurrentKey = GetCurrentKey();
	return CurrentKey.IsValid()
		? CurrentKey.GetDisplayName()
		: LOCTEXT("RebindUnbound", "Unbound");
}

bool UAGASSGameSettingKeyBind::TryGetCurrentKeyBrush(FSlateBrush& OutBrush) const
{
	if (InputType != EAGASSRebindInputType::Gamepad)
	{
		return false;
	}

	const FKey CurrentKey = GetCurrentKey();
	if (!CurrentKey.IsValid())
	{
		return false;
	}

	const UCommonInputPlatformSettings* const PlatformSettings = UCommonInputPlatformSettings::Get();
	if (PlatformSettings == nullptr)
	{
		return false;
	}

	FName GamepadName = FCommonInputDefaults::GamepadGeneric;
	if (const UCommonInputSubsystem* const InputSubsystem = UCommonInputSubsystem::Get(RebindLocalPlayer.Get()))
	{
		const FName CurrentGamepadName = InputSubsystem->GetCurrentGamepadName();
		if (!CurrentGamepadName.IsNone())
		{
			GamepadName = CurrentGamepadName;
		}
	}

	return PlatformSettings->TryGetInputBrush(OutBrush, CurrentKey, ECommonInputType::Gamepad, GamepadName);
}

bool UAGASSGameSettingKeyBind::IsCustomized() const
{
	if (const FPlayerKeyMapping* const CurrentMapping = FindCurrentMapping())
	{
		return CurrentMapping->IsCustomized();
	}

	return false;
}

bool UAGASSGameSettingKeyBind::HasDuplicateBindingForKey(const FKey InKey, FText& OutConflictText) const
{
	OutConflictText = FText::GetEmpty();

	UEnhancedInputUserSettings* const UserSettings = GetUserSettings();
	if (UserSettings == nullptr || !InKey.IsValid())
	{
		return false;
	}

	UEnhancedPlayerMappableKeyProfile* const KeyProfile = UserSettings->GetActiveKeyProfile();
	if (KeyProfile == nullptr)
	{
		return false;
	}

	TArray<FName> MappingNames;
	KeyProfile->GetMappingNamesForKey(InKey, MappingNames);

	TArray<FText> Conflicts;
	for (const FName& ExistingMappingName : MappingNames)
	{
		if (ExistingMappingName == MappingName)
		{
			continue;
		}

		if (const FKeyMappingRow* const MappingRow = KeyProfile->FindKeyMappingRow(ExistingMappingName))
		{
			for (const FPlayerKeyMapping& Mapping : MappingRow->Mappings)
			{
				if (Mapping.GetCurrentKey() == InKey)
				{
					Conflicts.Add(GetDisplayNameForMapping(ExistingMappingName));
					break;
				}
			}
		}
	}

	if (Conflicts.Num() == 0)
	{
		return false;
	}

	TArray<FString> ConflictStrings;
	ConflictStrings.Reserve(Conflicts.Num());
	for (const FText& Conflict : Conflicts)
	{
		ConflictStrings.Add(Conflict.ToString());
	}

	OutConflictText = FText::Format(
		LOCTEXT("RebindDuplicateWarning", "{0} is already bound to {1}."),
		InKey.GetDisplayName(),
		FText::FromString(FString::Join(ConflictStrings, TEXT(", "))));
	return true;
}

bool UAGASSGameSettingKeyBind::ChangeBinding(const FKey InKey, FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();

	if (!ValidateKeyForRow(InKey, OutFailureReason))
	{
		return false;
	}

	UEnhancedInputUserSettings* const UserSettings = GetUserSettings();
	if (UserSettings == nullptr)
	{
		OutFailureReason = LOCTEXT("RebindNoUserSettings", "Input rebinding is unavailable for this local player.");
		return false;
	}

	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingName;
	Args.Slot = EPlayerMappableKeySlot::First;
	Args.NewKey = InKey;

	FGameplayTagContainer FailureReason;
	UserSettings->MapPlayerKey(Args, FailureReason);

	if (!FailureReason.IsEmpty())
	{
		OutFailureReason = LOCTEXT("RebindFailed", "Unable to change this binding.");
		return false;
	}

	NotifySettingChanged(EGameSettingChangeReason::Change);
	return true;
}

void UAGASSGameSettingKeyBind::StoreInitial()
{
	InitialKey = GetCurrentKey();
}

void UAGASSGameSettingKeyBind::ResetToDefault()
{
	if (UEnhancedInputUserSettings* const UserSettings = GetUserSettings())
	{
		FMapPlayerKeyArgs Args;
		Args.MappingName = MappingName;
		Args.Slot = EPlayerMappableKeySlot::First;

		FGameplayTagContainer FailureReason;
		UserSettings->ResetAllPlayerKeysInRow(Args, FailureReason);
		if (FailureReason.IsEmpty())
		{
			NotifySettingChanged(EGameSettingChangeReason::Change);
		}
	}
}

void UAGASSGameSettingKeyBind::RestoreToInitial()
{
	FText FailureReason;
	ChangeBinding(InitialKey, FailureReason);
}

void UAGASSGameSettingKeyBind::OnApply()
{
	Super::OnApply();

	if (UEnhancedInputUserSettings* const UserSettings = GetUserSettings())
	{
		UserSettings->ApplySettings();
		UserSettings->SaveSettings();
	}
}

UEnhancedInputUserSettings* UAGASSGameSettingKeyBind::GetUserSettings() const
{
	if (ULocalPlayer* const ResolvedLocalPlayer = RebindLocalPlayer.Get())
	{
		if (UEnhancedInputLocalPlayerSubsystem* const InputSubsystem = ResolvedLocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			return InputSubsystem->GetUserSettings();
		}
	}

	return nullptr;
}

const FPlayerKeyMapping* UAGASSGameSettingKeyBind::FindCurrentMapping() const
{
	if (UEnhancedInputUserSettings* const UserSettings = GetUserSettings())
	{
		return UserSettings->FindCurrentMappingForSlot(MappingName, EPlayerMappableKeySlot::First);
	}

	return nullptr;
}

FKey UAGASSGameSettingKeyBind::GetCurrentKey() const
{
	if (const FPlayerKeyMapping* const CurrentMapping = FindCurrentMapping())
	{
		return CurrentMapping->GetCurrentKey();
	}

	return EKeys::Invalid;
}

bool UAGASSGameSettingKeyBind::ValidateKeyForRow(const FKey InKey, FText& OutFailureReason) const
{
	OutFailureReason = FText::GetEmpty();

	if (!InKey.IsValid())
	{
		OutFailureReason = LOCTEXT("RebindInvalidKey", "Choose a valid key or button.");
		return false;
	}

	if (InputType == EAGASSRebindInputType::Gamepad)
	{
		if (!InKey.IsGamepadKey())
		{
			OutFailureReason = LOCTEXT("RebindRequiresGamepad", "Choose a gamepad button or trigger for this binding.");
			return false;
		}

		if (!bAllowAxisKeys && (InKey.IsAxis1D() || InKey.IsAxis2D() || InKey.IsAxis3D()))
		{
			OutFailureReason = LOCTEXT("RebindNoGamepadAxis", "This binding does not accept analog axes.");
			return false;
		}

		return true;
	}

	if (InKey.IsGamepadKey())
	{
		OutFailureReason = LOCTEXT("RebindRequiresKeyboard", "Choose a keyboard or mouse input for this binding.");
		return false;
	}

	if (bRequireMouseWheelAxis && InKey != EKeys::MouseWheelAxis)
	{
		OutFailureReason = LOCTEXT("RebindRequiresMouseWheel", "This binding currently requires the mouse wheel.");
		return false;
	}

	if (!bAllowAxisKeys && (InKey.IsAxis1D() || InKey.IsAxis2D() || InKey.IsAxis3D()))
	{
		OutFailureReason = LOCTEXT("RebindNoAxis", "This binding does not accept analog axis inputs.");
		return false;
	}

	return true;
}

FText UAGASSGameSettingKeyBind::GetDisplayNameForMapping(const FName InMappingName) const
{
	if (UEnhancedInputUserSettings* const UserSettings = GetUserSettings())
	{
		if (UEnhancedPlayerMappableKeyProfile* const KeyProfile = UserSettings->GetActiveKeyProfile())
		{
			if (const FKeyMappingRow* const MappingRow = KeyProfile->FindKeyMappingRow(InMappingName))
			{
				for (const FPlayerKeyMapping& Mapping : MappingRow->Mappings)
				{
					if (!Mapping.GetDisplayName().IsEmpty())
					{
						return Mapping.GetDisplayName();
					}
				}
			}
		}
	}

	return FText::FromName(InMappingName);
}

#undef LOCTEXT_NAMESPACE
