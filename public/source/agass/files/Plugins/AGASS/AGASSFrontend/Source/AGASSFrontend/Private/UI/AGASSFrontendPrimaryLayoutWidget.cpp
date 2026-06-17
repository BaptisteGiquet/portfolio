#include "UI/AGASSFrontendPrimaryLayoutWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/CommonBoundActionBar.h"
#include "UI/AGASSFrontendUITags.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogAGASSFrontendLayout, Log, All);

UAGASSFrontendPrimaryLayoutWidget::UAGASSFrontendPrimaryLayoutWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAGASSFrontendPrimaryLayoutWidget::SetFrontendShellVisible(const bool bVisible)
{
	const ESlateVisibility ShellVisibility = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;

	if (Image_Backdrop != nullptr)
	{
		Image_Backdrop->SetVisibility(ShellVisibility);
	}

	if (Image_TopBar != nullptr)
	{
		Image_TopBar->SetVisibility(ShellVisibility);
	}

	if (Text_Wordmark != nullptr)
	{
		Text_Wordmark->SetVisibility(ShellVisibility);
	}
}

#if WITH_EDITOR
void UAGASSFrontendPrimaryLayoutWidget::ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);

	for (const FName WidgetName : {
		FName(TEXT("GameLayer_Stack")),
		FName(TEXT("GameMenu_Stack")),
		FName(TEXT("Menu_Stack")),
		FName(TEXT("Modal_Stack")),
		FName(TEXT("BottomActionBar"))
	})
	{
		if (BlueprintWidgetTree.FindWidget(WidgetName) == nullptr)
		{
			CompileLog.Error(FText::Format(
				NSLOCTEXT("AGASSFrontend", "MissingRequiredFrontendLayoutWidget", "{0} is missing required widget '{1}'."),
				FText::FromString(GetNameSafe(GetClass())),
				FText::FromName(WidgetName)));
		}
	}
}
#endif

void UAGASSFrontendPrimaryLayoutWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (GameLayer_Stack == nullptr)
	{
		GameLayer_Stack = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("GameLayer_Stack")));
	}

	if (GameMenu_Stack == nullptr)
	{
		GameMenu_Stack = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("GameMenu_Stack")));
	}

	if (Menu_Stack == nullptr)
	{
		Menu_Stack = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("Menu_Stack")));
	}

	if (Modal_Stack == nullptr)
	{
		Modal_Stack = Cast<UCommonActivatableWidgetStack>(GetWidgetFromName(TEXT("Modal_Stack")));
	}

	if (BottomActionBar == nullptr)
	{
		BottomActionBar = Cast<UCommonBoundActionBar>(GetWidgetFromName(TEXT("BottomActionBar")));
	}

	if (Image_Backdrop == nullptr)
	{
		Image_Backdrop = Cast<UImage>(GetWidgetFromName(TEXT("Image_Backdrop")));
	}

	if (Image_TopBar == nullptr)
	{
		Image_TopBar = Cast<UImage>(GetWidgetFromName(TEXT("Image_TopBar")));
	}

	if (Text_Wordmark == nullptr)
	{
		Text_Wordmark = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_Wordmark")));
	}

	UE_LOG(LogAGASSFrontendLayout, Log, TEXT("Primary layout NativeConstruct on %s. Game=%s GameMenu=%s Menu=%s Modal=%s BottomActionBar=%s Backdrop=%s TopBar=%s Wordmark=%s Class=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GameLayer_Stack),
		*GetNameSafe(GameMenu_Stack),
		*GetNameSafe(Menu_Stack),
		*GetNameSafe(Modal_Stack),
		*GetNameSafe(BottomActionBar),
		*GetNameSafe(Image_Backdrop),
		*GetNameSafe(Image_TopBar),
		*GetNameSafe(Text_Wordmark),
		*GetNameSafe(GetClass()));

	ResetRegisteredLayers();
	RegisterLayer(AGASSFrontendTags::TAG_UI_LAYER_GAME, GameLayer_Stack);
	RegisterLayer(AGASSFrontendTags::TAG_UI_LAYER_GAMEMENU, GameMenu_Stack);
	RegisterLayer(AGASSFrontendTags::TAG_UI_LAYER_MENU, Menu_Stack);
	RegisterLayer(AGASSFrontendTags::TAG_UI_LAYER_MODAL, Modal_Stack);
}

TSharedRef<SWidget> UAGASSFrontendPrimaryLayoutWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		UE_LOG(LogAGASSFrontendLayout, Log, TEXT("RebuildWidget using native fallback for %s."), *GetNameSafe(this));
		WidgetTree->RootWidget = nullptr;

		UVerticalBox* RootLayout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootLayout"));
		WidgetTree->RootWidget = RootLayout;

		UOverlay* LayerOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LayerOverlay"));
		if (UVerticalBoxSlot* OverlaySlot = RootLayout->AddChildToVerticalBox(LayerOverlay))
		{
			OverlaySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		auto AddLayerStack = [this, LayerOverlay](const TCHAR* StackName, TObjectPtr<UCommonActivatableWidgetStack>& StackWidget)
		{
			StackWidget = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass(), StackName);
			LayerOverlay->AddChild(StackWidget);
			if (UOverlaySlot* LayerSlot = Cast<UOverlaySlot>(StackWidget->Slot))
			{
				LayerSlot->SetHorizontalAlignment(HAlign_Fill);
				LayerSlot->SetVerticalAlignment(VAlign_Fill);
			}
		};

		AddLayerStack(TEXT("GameLayer_Stack"), GameLayer_Stack);
		AddLayerStack(TEXT("GameMenu_Stack"), GameMenu_Stack);
		AddLayerStack(TEXT("Menu_Stack"), Menu_Stack);
		AddLayerStack(TEXT("Modal_Stack"), Modal_Stack);

		BottomActionBar = WidgetTree->ConstructWidget<UCommonBoundActionBar>(UCommonBoundActionBar::StaticClass(), TEXT("BottomActionBar"));
		if (UVerticalBoxSlot* ActionBarSlot = RootLayout->AddChildToVerticalBox(BottomActionBar))
		{
			ActionBarSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 18.f));
		}
	}
	else
	{
		UE_LOG(LogAGASSFrontendLayout, Log, TEXT("RebuildWidget using Widget Blueprint tree for %s (%s)."),
			*GetNameSafe(this),
			*GetNameSafe(GetClass()));
	}

	return Super::RebuildWidget();
}
