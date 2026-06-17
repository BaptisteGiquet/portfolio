#include "UI/Common/Screens/GameHud/EMRGameplayKeybindHelperWidget.h"

#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "CommonUITypes.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"

#define LOCTEXT_NAMESPACE "EMRGameplayKeybindHelperWidget"

void UEMRGameplayKeybindHelperWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDisplayedBindings();
}

void UEMRGameplayKeybindHelperWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(false);
	SetVisibility(ESlateVisibility::HitTestInvisible);

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();

	if (UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(LocalPlayer))
	{
		BoundCommonInputSubsystem = CommonInputSubsystem;
		InputMethodChangedHandle = CommonInputSubsystem->OnInputMethodChangedNative.AddUObject(this, &ThisClass::HandleInputMethodChanged);
	}

	if (UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		EnhancedInputSubsystem->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &ThisClass::HandleControlMappingsRebuilt);
	}

	RefreshDisplayedBindings();
}

void UEMRGameplayKeybindHelperWidget::NativeDestruct()
{
	if (UCommonInputSubsystem* CommonInputSubsystem = BoundCommonInputSubsystem.Get())
	{
		if (InputMethodChangedHandle.IsValid())
		{
			CommonInputSubsystem->OnInputMethodChangedNative.Remove(InputMethodChangedHandle);
		}
	}

	InputMethodChangedHandle.Reset();
	BoundCommonInputSubsystem.Reset();

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			EnhancedInputSubsystem->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &ThisClass::HandleControlMappingsRebuilt);
		}
	}

	Super::NativeDestruct();
}

void UEMRGameplayKeybindHelperWidget::HandleControlMappingsRebuilt()
{
	RefreshDisplayedBindings();
}

void UEMRGameplayKeybindHelperWidget::HandleInputMethodChanged(ECommonInputType /*NewInputType*/)
{
	RefreshDisplayedBindings();
}

void UEMRGameplayKeybindHelperWidget::RefreshDisplayedBindings()
{
	if (CommonTextBlock_KeybindSummary)
	{
		CommonTextBlock_KeybindSummary->SetVisibility(ESlateVisibility::HitTestInvisible);
		CommonTextBlock_KeybindSummary->SetText(BuildSummaryText());
	}
}

FKey UEMRGameplayKeybindHelperWidget::ResolveFirstKeyForCurrentInput(const UInputAction* InInputAction) const
{
	if (!InInputAction)
	{
		return FKey();
	}

	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (!LocalPlayer)
	{
		return FKey();
	}

	const UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!EnhancedInputSubsystem)
	{
		return FKey();
	}

	const TArray<FKey> MappedKeys = EnhancedInputSubsystem->QueryKeysMappedToAction(InInputAction);
	if (MappedKeys.IsEmpty())
	{
		return FKey();
	}

	ECommonInputType CurrentInputType = ECommonInputType::MouseAndKeyboard;
	if (const UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(LocalPlayer))
	{
		CurrentInputType = CommonInputSubsystem->GetCurrentInputType();
	}

	for (const FKey& MappedKey : MappedKeys)
	{
		if (MappedKey.IsValid() && CommonUI::IsKeyValidForInputType(MappedKey, CurrentInputType))
		{
			return MappedKey;
		}
	}

	for (const FKey& MappedKey : MappedKeys)
	{
		if (MappedKey.IsValid())
		{
			return MappedKey;
		}
	}

	return FKey();
}

FText UEMRGameplayKeybindHelperWidget::BuildSummaryText() const
{
	static const FText UnboundText = LOCTEXT("Unbound", "Unbound");
	static const FText DefaultLabelText = LOCTEXT("DefaultLabel", "Action");
	static const FText NewLineSeparator = FText::FromString(TEXT("\n"));

	TArray<FText> SummaryLines;
	SummaryLines.Reserve(KeybindEntries.Num());

	for (const FEMRGameplayKeybindEntry& KeybindEntry : KeybindEntries)
	{
		const FText LabelText = KeybindEntry.DisplayLabel.IsEmpty() ? DefaultLabelText : KeybindEntry.DisplayLabel;
		const FKey ResolvedKey = ResolveFirstKeyForCurrentInput(KeybindEntry.InputAction);
		const FText KeyText = ResolvedKey.IsValid() ? ResolvedKey.GetDisplayName() : UnboundText;

		SummaryLines.Add(FText::Format(LOCTEXT("LineFormat", "{0}: {1}"), LabelText, KeyText));
	}

	return SummaryLines.IsEmpty() ? FText::GetEmpty() : FText::Join(NewLineSeparator, SummaryLines);
}

#undef LOCTEXT_NAMESPACE
