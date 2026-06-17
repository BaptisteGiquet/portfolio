#include "UI/Settings/AGASSKeyAlreadyBoundWarningWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Containers/Ticker.h"
#include "Input/Events.h"
#include "Styling/CoreStyle.h"
#include "UI/AGASSFrontendButtonBase.h"

namespace
{
	FSlateFontInfo MakeWarningFont(const int32 FontSize, const TCHAR* Typeface = TEXT("Regular"))
	{
		return FCoreStyle::GetDefaultFontStyle(Typeface, FontSize);
	}

	bool WidgetContainsScreenPosition(const UWidget* Widget, const FVector2D& ScreenPosition)
	{
		if (Widget == nullptr || !Widget->GetIsEnabled())
		{
			return false;
		}

		const FGeometry& Geometry = Widget->GetCachedGeometry();
		const FVector2D AbsolutePosition = Geometry.GetAbsolutePosition();
		const FVector2D AbsoluteSize = Geometry.GetAbsoluteSize();
		if (AbsoluteSize.IsNearlyZero())
		{
			return false;
		}

		const FSlateRect Rect(AbsolutePosition.X, AbsolutePosition.Y, AbsolutePosition.X + AbsoluteSize.X, AbsolutePosition.Y + AbsoluteSize.Y);
		return Rect.ContainsPoint(ScreenPosition);
	}
}

#define LOCTEXT_NAMESPACE "AGASSSettings"

UAGASSKeyAlreadyBoundWarningWidget::UAGASSKeyAlreadyBoundWarningWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
}

void UAGASSKeyAlreadyBoundWarningWidget::SetWarningText(const FText& InWarningText)
{
	WarningText = InWarningText;

	if (Text_Warning != nullptr)
	{
		Text_Warning->SetText(WarningText);
	}
}

void UAGASSKeyAlreadyBoundWarningWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	bDismissed = false;
	if (Text_Warning != nullptr)
	{
		Text_Warning->SetText(WarningText);
	}

	if (Button_Confirm != nullptr)
	{
		Button_Confirm->SetFocus();
	}
}

bool UAGASSKeyAlreadyBoundWarningWidget::NativeOnHandleBackAction()
{
	HandleCancelClicked();
	return true;
}

TSharedRef<SWidget> UAGASSKeyAlreadyBoundWarningWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureWidgetTree();
	}

	return Super::RebuildWidget();
}

FReply UAGASSKeyAlreadyBoundWarningWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		if (WidgetContainsScreenPosition(GetWidgetFromName(TEXT("Button_Confirm")), ScreenPosition))
		{
			HandleConfirmClicked();
			return FReply::Handled();
		}

		if (WidgetContainsScreenPosition(GetWidgetFromName(TEXT("Button_Cancel")), ScreenPosition))
		{
			HandleCancelClicked();
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UAGASSKeyAlreadyBoundWarningWidget::EnsureWidgetTree()
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
	PanelBox->SetWidthOverride(760.f);
	PanelBox->SetMaxDesiredWidth(760.f);
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

	Text_Warning = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Warning"));
	Text_Warning->SetFont(MakeWarningFont(20, TEXT("Bold")));
	Text_Warning->SetAutoWrapText(true);
	if (UVerticalBoxSlot* const WarningSlot = Layout->AddChildToVerticalBox(Text_Warning))
	{
		WarningSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	}

	UHorizontalBox* const ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HBox_Buttons"));
	Layout->AddChildToVerticalBox(ButtonRow);

	Button_Confirm = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Confirm"));
	Button_Confirm->SetButtonText(LOCTEXT("RebindReplace", "Replace"));
	Button_Confirm->SetClickMethod(EButtonClickMethod::MouseDown);
	Button_Confirm->OnClicked().AddUObject(this, &ThisClass::HandleConfirmClicked);
	if (UHorizontalBoxSlot* const ConfirmSlot = ButtonRow->AddChildToHorizontalBox(Button_Confirm))
	{
		ConfirmSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		ConfirmSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	Button_Cancel = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Cancel"));
	Button_Cancel->SetButtonText(LOCTEXT("RebindCancel", "Cancel"));
	Button_Cancel->SetClickMethod(EButtonClickMethod::MouseDown);
	Button_Cancel->OnClicked().AddUObject(this, &ThisClass::HandleCancelClicked);
	if (UHorizontalBoxSlot* const CancelSlot = ButtonRow->AddChildToHorizontalBox(Button_Cancel))
	{
		CancelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	if (Text_Warning != nullptr)
	{
		Text_Warning->SetText(WarningText);
	}
}

void UAGASSKeyAlreadyBoundWarningWidget::HandleConfirmClicked()
{
	if (bDismissed)
	{
		return;
	}

	bDismissed = true;
	Dismiss([this]()
	{
		OnConfirmed.Broadcast();
	});
}

void UAGASSKeyAlreadyBoundWarningWidget::HandleCancelClicked()
{
	if (bDismissed)
	{
		return;
	}

	bDismissed = true;
	Dismiss([this]()
	{
		OnCanceled.Broadcast();
	});
}

void UAGASSKeyAlreadyBoundWarningWidget::Dismiss(TFunction<void()> PostDismissCallback)
{
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, PostDismissCallback](float)
	{
		DeactivateWidget();
		PostDismissCallback();
		return false;
	}));
}

#undef LOCTEXT_NAMESPACE
