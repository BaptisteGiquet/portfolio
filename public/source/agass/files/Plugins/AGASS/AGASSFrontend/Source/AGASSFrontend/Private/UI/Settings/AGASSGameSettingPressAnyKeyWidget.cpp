#include "UI/Settings/AGASSGameSettingPressAnyKeyWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Containers/Ticker.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

namespace
{
	FSlateFontInfo MakeModalFont(const int32 FontSize, const TCHAR* Typeface = TEXT("Regular"))
	{
		return FCoreStyle::GetDefaultFontStyle(Typeface, FontSize);
	}
}

class FAGASSPressAnyKeyInputProcessor final : public IInputProcessor
{
public:
	explicit FAGASSPressAnyKeyInputProcessor(UAGASSGameSettingPressAnyKeyWidget* InOwner)
		: Owner(InOwner)
	{
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
	}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		return HandleKey(InKeyEvent.GetKey(), true);
	}

	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		return HandleKey(InKeyEvent.GetKey(), false);
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		return HandleKey(MouseEvent.GetEffectingButton(), true);
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		return HandleKey(MouseEvent.GetEffectingButton(), false);
	}

	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) override
	{
		if (UAGASSGameSettingPressAnyKeyWidget* const PinnedOwner = Owner.Get())
		{
			if (PinnedOwner->InputType == EAGASSRebindInputType::MouseAndKeyboard && InWheelEvent.GetWheelDelta() != 0.f)
			{
				PinnedOwner->HandleCapturedKey(EKeys::MouseWheelAxis);
				return true;
			}
		}

		return false;
	}

	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override
	{
		if (UAGASSGameSettingPressAnyKeyWidget* const PinnedOwner = Owner.Get())
		{
			const FKey Key = InAnalogInputEvent.GetKey();
			if (Key.IsGamepadKey() && FMath::Abs(InAnalogInputEvent.GetAnalogValue()) >= 0.5f)
			{
				PinnedOwner->HandleCapturedKey(Key);
				return true;
			}
		}

		return false;
	}

private:
	bool HandleKey(const FKey Key, const bool bPressed) const
	{
		if (UAGASSGameSettingPressAnyKeyWidget* const PinnedOwner = Owner.Get())
		{
			const bool bIsCancelKey =
				Key == EKeys::Escape
				|| Key == EKeys::Gamepad_Special_Left
				|| Key == EKeys::Gamepad_Special_Right
				|| (PinnedOwner->InputType != EAGASSRebindInputType::Gamepad && Key == EKeys::Virtual_Gamepad_Back.GetVirtualKey());
			if (bIsCancelKey)
			{
				if (!bPressed)
				{
					PinnedOwner->HandleSelectionCanceled();
				}
				return true;
			}

			if (Key.IsTouch())
			{
				return true;
			}

			const bool bExpectGamepad = PinnedOwner->InputType == EAGASSRebindInputType::Gamepad;
			if (Key.IsGamepadKey() != bExpectGamepad)
			{
				return true;
			}

			if (!PinnedOwner->bAllowAxisKeys && (Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D()))
			{
				return true;
			}

			if (!bPressed && !Key.IsGamepadKey() && !Key.IsMouseButton())
			{
				PinnedOwner->HandleCapturedKey(Key);
			}
			else if (bPressed && Key.IsGamepadKey() && !Key.IsAxis1D() && !Key.IsAxis2D() && !Key.IsAxis3D())
			{
				PinnedOwner->HandleCapturedKey(Key);
			}

			return true;
		}

		return false;
	}

	TWeakObjectPtr<UAGASSGameSettingPressAnyKeyWidget> Owner;
};

#define LOCTEXT_NAMESPACE "AGASSSettings"

UAGASSGameSettingPressAnyKeyWidget::UAGASSGameSettingPressAnyKeyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// This widget captures raw input for rebinding. Let the input processor own cancel behavior
	// so CommonUI does not steal rebindable gamepad buttons such as Face Button Right.
	bIsBackHandler = false;
	bIsBackActionDisplayedInActionBar = false;
}

void UAGASSGameSettingPressAnyKeyWidget::Configure(const EAGASSRebindInputType InInputType, const bool bInAllowAxisKeys)
{
	InputType = InInputType;
	bAllowAxisKeys = bInAllowAxisKeys;
	RefreshPromptText();
}

void UAGASSGameSettingPressAnyKeyWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	bSelectionComplete = false;
	RefreshPromptText();

	InputProcessor = MakeShared<FAGASSPressAnyKeyInputProcessor>(this);
	FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
}

void UAGASSGameSettingPressAnyKeyWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	if (FSlateApplication::IsInitialized() && InputProcessor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}

	InputProcessor.Reset();
}

bool UAGASSGameSettingPressAnyKeyWidget::NativeOnHandleBackAction()
{
	HandleSelectionCanceled();
	return true;
}

TSharedRef<SWidget> UAGASSGameSettingPressAnyKeyWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureWidgetTree();
	}

	return Super::RebuildWidget();
}

void UAGASSGameSettingPressAnyKeyWidget::EnsureWidgetTree()
{
	if (WidgetTree == nullptr)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (WidgetTree->RootWidget != nullptr)
	{
		return;
	}

	UOverlay* const RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Root"));
	WidgetTree->RootWidget = RootOverlay;

	UBorder* const Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.03f, 0.04f, 0.05f, 0.70f));
	if (UOverlaySlot* const BackdropSlot = RootOverlay->AddChildToOverlay(Backdrop))
	{
		BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
		BackdropSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* const PanelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Box_Panel"));
	PanelBox->SetWidthOverride(620.f);
	PanelBox->SetMaxDesiredWidth(620.f);
	if (UOverlaySlot* const PanelSlot = RootOverlay->AddChildToOverlay(PanelBox))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
		PanelSlot->SetPadding(FMargin(24.f));
	}

	UBorder* const PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Panel"));
	PanelBorder->SetPadding(FMargin(28.f));
	PanelBorder->SetBrushColor(FLinearColor(0.07f, 0.09f, 0.11f, 0.98f));
	PanelBox->SetContent(PanelBorder);

	UVerticalBox* const Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox_Layout"));
	PanelBorder->SetContent(Layout);

	Text_Prompt = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Prompt"));
	Text_Prompt->SetFont(MakeModalFont(24, TEXT("Bold")));
	Text_Prompt->SetAutoWrapText(true);
	if (UVerticalBoxSlot* const PromptSlot = Layout->AddChildToVerticalBox(Text_Prompt))
	{
		PromptSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	Text_Details = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Details"));
	Text_Details->SetFont(MakeModalFont(16));
	Text_Details->SetAutoWrapText(true);
	Layout->AddChildToVerticalBox(Text_Details);

	RefreshPromptText();
}

void UAGASSGameSettingPressAnyKeyWidget::RefreshPromptText()
{
	if (Text_Prompt == nullptr || Text_Details == nullptr)
	{
		return;
	}

	if (InputType == EAGASSRebindInputType::Gamepad)
	{
		Text_Prompt->SetText(LOCTEXT("PressAnyKeyPromptGamepad", "Press a gamepad button"));
		Text_Details->SetText(
			bAllowAxisKeys
				? LOCTEXT("PressAnyKeyDetailsGamepadAxis", "Buttons and triggers are accepted. Press Escape or the controller menu button to cancel.")
				: LOCTEXT("PressAnyKeyDetailsGamepad", "Press Escape or the controller menu button to cancel."));
	}
	else
	{
		Text_Prompt->SetText(LOCTEXT("PressAnyKeyPromptKeyboard", "Press a keyboard or mouse input"));
		Text_Details->SetText(
			bAllowAxisKeys
				? LOCTEXT("PressAnyKeyDetailsKeyboardAxis", "The mouse wheel is accepted for this binding. Press Escape to cancel.")
				: LOCTEXT("PressAnyKeyDetailsKeyboard", "Press Escape to cancel."));
	}
}

void UAGASSGameSettingPressAnyKeyWidget::HandleCapturedKey(const FKey InKey)
{
	if (bSelectionComplete)
	{
		return;
	}

	bSelectionComplete = true;
	Dismiss([this, InKey]()
	{
		OnKeySelected.Broadcast(InKey);
	});
}

void UAGASSGameSettingPressAnyKeyWidget::HandleSelectionCanceled()
{
	if (bSelectionComplete)
	{
		return;
	}

	bSelectionComplete = true;
	Dismiss([this]()
	{
		OnKeySelectionCanceled.Broadcast();
	});
}

void UAGASSGameSettingPressAnyKeyWidget::Dismiss(TFunction<void()> PostDismissCallback)
{
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, PostDismissCallback](float)
	{
		if (FSlateApplication::IsInitialized() && InputProcessor.IsValid())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
		}

		InputProcessor.Reset();
		DeactivateWidget();
		PostDismissCallback();
		return false;
	}));
}

#undef LOCTEXT_NAMESPACE
