#include "UI/AGASSFrontendScreenWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonInputModeTypes.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "Styling/CoreStyle.h"
#include "Subsystems/AGASSPlatformMenuSubsystem.h"
#include "Subsystems/AGASSInviteCodeSubsystem.h"
#include "Subsystems/AGASSModsSubsystem.h"
#include "Subsystems/AGASSSessionSubsystem.h"
#include "UI/AGASSFrontendButtonBase.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

namespace
{
	FSlateFontInfo MakeFont(const int32 FontSize)
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FontSize);
	}

}

UAGASSFrontendScreenWidget::UAGASSFrontendScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UAGASSFrontendScreenWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		WidgetTree->RootWidget = nullptr;

		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
		WidgetTree->RootWidget = RootOverlay;

		UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Background"));
		Background->SetBrushColor(FLinearColor(0.04f, 0.07f, 0.08f, 0.98f));
		RootOverlay->AddChild(Background);
		if (UOverlaySlot* BackgroundSlot = Cast<UOverlaySlot>(Background->Slot))
		{
			BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
			BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
		}

		USizeBox* PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizeBox"));
		PanelSizeBox->SetWidthOverride(1380.f);
		RootOverlay->AddChild(PanelSizeBox);
		if (UOverlaySlot* PanelSlot = Cast<UOverlaySlot>(PanelSizeBox->Slot))
		{
			PanelSlot->SetHorizontalAlignment(HAlign_Center);
			PanelSlot->SetVerticalAlignment(VAlign_Center);
			PanelSlot->SetPadding(FMargin(24.f, 18.f));
		}

		UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Panel"));
		PanelBorder->SetBrushColor(FLinearColor(0.07f, 0.12f, 0.14f, 0.98f));
		PanelBorder->SetPadding(FMargin(34.f, 28.f));
		PanelSizeBox->AddChild(PanelBorder);

		UVerticalBox* PanelLayout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelLayout"));
		PanelBorder->SetContent(PanelLayout);

		Text_Title = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Title"));
		Text_Title->SetText(GetTitleText());
		Text_Title->SetAutoWrapText(true);
		if (UVerticalBoxSlot* TitleSlot = PanelLayout->AddChildToVerticalBox(Text_Title))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		Text_Status = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Status"));
		Text_Status->SetAutoWrapText(true);
		if (UVerticalBoxSlot* StatusSlot = PanelLayout->AddChildToVerticalBox(Text_Status))
		{
			StatusSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		UScrollBox* ContentScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ContentScrollBox"));
		ContentScrollBox->SetAnimateWheelScrolling(true);
		ContentScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
		if (UVerticalBoxSlot* ContentSlot = PanelLayout->AddChildToVerticalBox(ContentScrollBox))
		{
			ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
		ContentScrollBox->AddChild(ContentBox);
		BuildFallbackScreenContent(ContentBox);
	}

	return Super::RebuildWidget();
}

void UAGASSFrontendScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveWidgetByName(Text_Title, TEXT("Text_Title"));
	ResolveWidgetByName(Text_Status, TEXT("Text_Status"));

	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		SessionStateChangedHandle = SessionSubsystem->OnSessionStateChanged().AddUObject(this, &ThisClass::HandleSessionStateChanged);
	}

	RefreshFromSessionState();
}

void UAGASSFrontendScreenWidget::NativeDestruct()
{
	if (UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		if (SessionStateChangedHandle.IsValid())
		{
			SessionSubsystem->OnSessionStateChanged().Remove(SessionStateChangedHandle);
			SessionStateChangedHandle.Reset();
		}
	}

	Super::NativeDestruct();
}

void UAGASSFrontendScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshFromSessionState();
}

TOptional<FUIInputConfig> UAGASSFrontendScreenWidget::GetDesiredInputConfig() const
{
	FUIInputConfig InputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
	InputConfig.bIgnoreLookInput = true;
	InputConfig.bIgnoreMoveInput = true;
	return InputConfig;
}

UWidget* UAGASSFrontendScreenWidget::NativeGetDesiredFocusTarget() const
{
	return ResolveInitialFocusTarget();
}

UWidget* UAGASSFrontendScreenWidget::ResolveInitialFocusTarget() const
{
	return nullptr;
}

#if WITH_EDITOR
void UAGASSFrontendScreenWidget::ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);

	TArray<FName> RequiredWidgetNames;
	GetRequiredNamedWidgets(RequiredWidgetNames);
	ValidateRequiredNamedWidgets(BlueprintWidgetTree, CompileLog, RequiredWidgetNames);
}

void UAGASSFrontendScreenWidget::GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const
{
}

void UAGASSFrontendScreenWidget::ValidateRequiredNamedWidgets(
	const UWidgetTree& BlueprintWidgetTree,
	IWidgetCompilerLog& CompileLog,
	TConstArrayView<FName> RequiredWidgetNames) const
{
	for (const FName WidgetName : RequiredWidgetNames)
	{
		if (BlueprintWidgetTree.FindWidget(WidgetName) == nullptr)
		{
			CompileLog.Error(FText::Format(
				NSLOCTEXT("AGASSFrontend", "MissingRequiredFrontendWidget", "{0} is missing required widget '{1}'."),
				FText::FromString(GetNameSafe(GetClass())),
				FText::FromName(WidgetName)));
		}
	}
}
#endif

void UAGASSFrontendScreenWidget::RefreshFromSessionState()
{
	if (Text_Title != nullptr)
	{
		Text_Title->SetText(GetTitleText());
	}

	if (Text_Status == nullptr)
	{
		return;
	}

	if (const UAGASSSessionSubsystem* SessionSubsystem = GetSessionSubsystem())
	{
		const FString StatusMessage = SessionSubsystem->GetStatusMessage();
		Text_Status->SetText(FText::FromString(StatusMessage));
		Text_Status->SetVisibility(StatusMessage.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		return;
	}

	Text_Status->SetVisibility(ESlateVisibility::Collapsed);
}

UAGASSSessionSubsystem* UAGASSFrontendScreenWidget::GetSessionSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UAGASSSessionSubsystem>();
	}

	return nullptr;
}

UAGASSInviteCodeSubsystem* UAGASSFrontendScreenWidget::GetInviteCodeSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UAGASSInviteCodeSubsystem>();
	}

	return nullptr;
}

UAGASSModsSubsystem* UAGASSFrontendScreenWidget::GetModsSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UAGASSModsSubsystem>();
	}

	return nullptr;
}

UAGASSPlatformMenuSubsystem* UAGASSFrontendScreenWidget::GetPlatformMenuSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UAGASSPlatformMenuSubsystem>();
	}

	return nullptr;
}

UAGASSFrontendButtonBase* UAGASSFrontendScreenWidget::CreateMenuButton(const FText& Label, const FName& WidgetName)
{
	UAGASSFrontendButtonBase* Button = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), WidgetName);
	Button->SetButtonText(Label);
	return Button;
}

UAGASSFrontendButtonBase* UAGASSFrontendScreenWidget::AddMenuButton(UVerticalBox* Parent, const FText& Label, const FName& WidgetName)
{
	UAGASSFrontendButtonBase* Button = CreateMenuButton(Label, WidgetName);
	if (UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	return Button;
}

UEditableTextBox* UAGASSFrontendScreenWidget::AddEditableTextBox(UVerticalBox* Parent, const FText& HintText, const FName& WidgetName)
{
	UEditableTextBox* EditableTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), WidgetName);
	EditableTextBox->SetHintText(HintText);

	if (UVerticalBoxSlot* TextSlot = Parent->AddChildToVerticalBox(EditableTextBox))
	{
		TextSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	return EditableTextBox;
}

UTextBlock* UAGASSFrontendScreenWidget::AddBodyText(UVerticalBox* Parent, const FText& Text, const int32 FontSize, const FName& WidgetName)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), WidgetName);
	TextBlock->SetText(Text);
	TextBlock->SetFont(MakeFont(FontSize));
	TextBlock->SetAutoWrapText(true);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.89f, 0.93f, 0.97f)));

	if (UVerticalBoxSlot* TextSlot = Parent->AddChildToVerticalBox(TextBlock))
	{
		TextSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	return TextBlock;
}

UHorizontalBox* UAGASSFrontendScreenWidget::AddHorizontalBox(UVerticalBox* Parent, const FName& WidgetName)
{
	UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), WidgetName);
	if (UVerticalBoxSlot* HorizontalSlot = Parent->AddChildToVerticalBox(HorizontalBox))
	{
		HorizontalSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	return HorizontalBox;
}

UScrollBox* UAGASSFrontendScreenWidget::AddScrollBox(UVerticalBox* Parent, const float HeightOverride, const FName& WidgetName)
{
	UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), WidgetName);
	ScrollBox->SetAnimateWheelScrolling(true);

	UWidget* WidgetToAdd = ScrollBox;
	if (HeightOverride > 0.f)
	{
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SizeBox->SetHeightOverride(HeightOverride);
		SizeBox->AddChild(ScrollBox);
		WidgetToAdd = SizeBox;
	}

	if (UVerticalBoxSlot* ScrollSlot = Parent->AddChildToVerticalBox(WidgetToAdd))
	{
		ScrollSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	return ScrollBox;
}

void UAGASSFrontendScreenWidget::AddSpacer(UVerticalBox* Parent, const float Height)
{
	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	Spacer->SetSize(FVector2D(1.f, Height));
	Parent->AddChildToVerticalBox(Spacer);
}

void UAGASSFrontendScreenWidget::HandleSessionStateChanged()
{
	RefreshFromSessionState();
}
