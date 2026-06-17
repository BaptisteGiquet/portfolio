#include "UI/AGASSFrontendButtonBase.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Styling/CoreStyle.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

UAGASSFrontendButtonBase::UAGASSFrontendButtonBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsSelectable(true);
	SetIsToggleable(false);
	SetIsInteractionEnabled(true);
	SetShouldSelectUponReceivingFocus(true);
	SetShouldUseFallbackDefaultInputAction(true);
}

#if WITH_EDITOR
void UAGASSFrontendButtonBase::ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);

	for (const FName WidgetName : {
		FName(TEXT("Border_Label")),
		FName(TEXT("Text_Label"))
	})
	{
		if (BlueprintWidgetTree.FindWidget(WidgetName) == nullptr)
		{
			CompileLog.Error(FText::Format(
				NSLOCTEXT("AGASSFrontend", "MissingRequiredFrontendButtonWidget", "{0} is missing required widget '{1}'."),
				FText::FromString(GetNameSafe(GetClass())),
				FText::FromName(WidgetName)));
		}
	}
}
#endif

void UAGASSFrontendButtonBase::SetButtonText(const FText& InText)
{
	ButtonText = InText;
	SynchronizeButtonText();
}

void UAGASSFrontendButtonBase::SetUseSelectedVisual(bool bInUseSelectedVisual)
{
	bUseSelectedVisual = bInUseSelectedVisual;
	UpdateVisualState();
}

TSharedRef<SWidget> UAGASSFrontendButtonBase::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		WidgetTree->RootWidget = nullptr;

		Border_Label = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Label"));
		Border_Label->SetPadding(FMargin(18.f, 12.f));

		Text_Label = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Label"));
		Text_Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 20));
		Text_Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text_Label->SetJustification(ETextJustify::Center);
		Text_Label->SetAutoWrapText(true);
		Border_Label->SetContent(Text_Label);
		WidgetTree->RootWidget = Border_Label;
		SynchronizeButtonText();
		UpdateVisualState();
	}

	return Super::RebuildWidget();
}

void UAGASSFrontendButtonBase::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();

	if (Border_Label == nullptr)
	{
		Border_Label = Cast<UBorder>(GetWidgetFromName(TEXT("Border_Label")));
	}

	if (Text_Label == nullptr)
	{
		Text_Label = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_Label")));
	}

	bHasFocusPath = HasAnyUserFocus();
	SynchronizeButtonText();
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Border_Label == nullptr)
	{
		Border_Label = Cast<UBorder>(GetWidgetFromName(TEXT("Border_Label")));
	}

	if (Text_Label == nullptr)
	{
		Text_Label = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_Label")));
	}

	SynchronizeButtonText();
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::NativeConstruct()
{
	Super::NativeConstruct();

	bHasFocusPath = HasAnyUserFocus();
	SynchronizeButtonText();
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	SynchronizeButtonText();
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::NativeOnSelected(bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::NativeOnDeselected(bool bBroadcast)
{
	Super::NativeOnDeselected(bBroadcast);
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bHasFocusPath = true;
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bHasFocusPath = false;
	UpdateVisualState();
}

void UAGASSFrontendButtonBase::SynchronizeButtonText()
{
	if (Text_Label != nullptr)
	{
		Text_Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 20));
		Text_Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text_Label->SetAutoWrapText(true);
		Text_Label->SetJustification(ETextJustify::Center);
		Text_Label->SetText(ButtonText);
	}
}

void UAGASSFrontendButtonBase::UpdateVisualState()
{
	if (Border_Label == nullptr)
	{
		return;
	}

	if (Text_Label == nullptr)
	{
		return;
	}

	FLinearColor FrameColor(0.74f, 0.67f, 0.49f, 1.0f);
	FLinearColor FillColor(0.67f, 0.59f, 0.38f, 1.0f);
	FLinearColor LabelColor = FLinearColor::White;
	const bool bShowSelectedVisual = bUseSelectedVisual && GetSelected();
	const bool bShowFocusVisual = IsHovered() || bHasFocusPath || HasAnyUserFocus();

	if (!GetIsEnabled())
	{
		FrameColor = FLinearColor(0.34f, 0.34f, 0.34f, 0.90f);
		FillColor = FLinearColor(0.27f, 0.27f, 0.27f, 0.90f);
		LabelColor = FLinearColor(0.74f, 0.74f, 0.74f, 1.0f);
	}
	else if (bShowSelectedVisual)
	{
		FrameColor = FLinearColor(0.95f, 0.90f, 0.79f, 1.0f);
		FillColor = FLinearColor(0.89f, 0.84f, 0.68f, 1.0f);
		LabelColor = FLinearColor(0.16f, 0.13f, 0.07f, 1.0f);
	}
	else if (bShowFocusVisual)
	{
		FrameColor = FLinearColor(0.82f, 0.75f, 0.56f, 1.0f);
		FillColor = FLinearColor(0.74f, 0.66f, 0.44f, 1.0f);
	}

	if (Border_Label != nullptr)
	{
		Border_Label->SetBrushColor(FillColor);
	}

	Text_Label->SetColorAndOpacity(FSlateColor(LabelColor));
}
