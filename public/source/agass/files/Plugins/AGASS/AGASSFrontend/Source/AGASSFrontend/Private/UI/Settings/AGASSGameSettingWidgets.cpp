#include "UI/Settings/AGASSGameSettingWidgets.h"

#include "AnalogSlider.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonActivatableWidget.h"
#include "CommonInputSubsystem.h"
#include "CommonInputTypeEnum.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameSettingAction.h"
#include "GameSettingCollection.h"
#include "Input/Events.h"
#include "Subsystems/AGASSUIManagerSubsystem.h"
#include "Settings/AGASSFrontendDeveloperSettings.h"
#include "Styling/CoreStyle.h"
#include "UI/AGASSFrontendButtonBase.h"
#include "UI/AGASSFrontendUITags.h"
#include "UI/Settings/AGASSGameSettingKeyBind.h"
#include "UI/Settings/AGASSGameSettingPressAnyKeyWidget.h"
#include "UI/Settings/AGASSKeyAlreadyBoundWarningWidget.h"
#include "UObject/UnrealType.h"

namespace
{
	FSlateFontInfo MakeSettingsFont(const int32 FontSize)
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FontSize);
	}

	template <typename TWidget>
	TWidget* PushModalWidgetForPlayer(ULocalPlayer* LocalPlayer, TSubclassOf<UCommonActivatableWidget> WidgetClass)
	{
		if (LocalPlayer == nullptr || WidgetClass == nullptr)
		{
			return nullptr;
		}

		if (UGameInstance* const GameInstance = LocalPlayer->GetGameInstance())
		{
			if (UAGASSUIManagerSubsystem* const UIManager = GameInstance->GetSubsystem<UAGASSUIManagerSubsystem>())
			{
				return Cast<TWidget>(UIManager->PushWidgetToLayer(LocalPlayer, AGASSFrontendTags::TAG_UI_LAYER_MODAL, WidgetClass));
			}
		}

		return nullptr;
	}

	bool ShouldRedirectFocusToChild(const UCommonUserWidget* Widget)
	{
		const ULocalPlayer* const LocalPlayer = Widget != nullptr ? Widget->GetOwningLocalPlayer() : nullptr;
		const UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(LocalPlayer);
		return InputSubsystem != nullptr && InputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad;
	}

	FReply FocusChildWidgetByName(UUserWidget* Owner, const FName& WidgetName, const FFocusEvent& InFocusEvent)
	{
		if (Owner != nullptr)
		{
			if (UWidget* const ChildWidget = Owner->GetWidgetFromName(WidgetName))
			{
				if (TSharedPtr<SWidget> CachedWidget = ChildWidget->GetCachedWidget())
				{
					return FReply::Handled().SetUserFocus(CachedWidget.ToSharedRef(), InFocusEvent.GetCause());
				}
			}
		}

		return FReply::Unhandled();
	}

	UBorder* CreateSettingsRowBorder(UWidgetTree* WidgetTree, const FName& WidgetName)
	{
		UBorder* const Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), WidgetName);
		Border->SetPadding(FMargin(12.f, 8.f));
		Border->SetBrushColor(FLinearColor(0.18f, 0.20f, 0.20f, 0.92f));
		return Border;
	}

	UCommonTextBlock* CreateSettingNameText(UWidgetTree* WidgetTree)
	{
		UCommonTextBlock* const TextBlock = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_SettingName"));
		TextBlock->SetFont(MakeSettingsFont(18));
		TextBlock->SetAutoWrapText(true);
		return TextBlock;
	}

	UCommonTextBlock* CreateValueText(UWidgetTree* WidgetTree, const FName& WidgetName)
	{
		UCommonTextBlock* const TextBlock = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), WidgetName);
		TextBlock->SetFont(MakeSettingsFont(16));
		TextBlock->SetJustification(ETextJustify::Right);
		TextBlock->SetAutoWrapText(false);
		return TextBlock;
	}

	void AddHorizontalChild(UHorizontalBox* Parent, UWidget* Child, const float FillWidth, const FMargin& Padding = FMargin())
	{
		if (Parent == nullptr || Child == nullptr)
		{
			return;
		}

		if (UHorizontalBoxSlot* const Slot = Parent->AddChildToHorizontalBox(Child))
		{
			Slot->SetPadding(Padding);
			Slot->SetVerticalAlignment(VAlign_Center);
			if (FillWidth > 0.f)
			{
				Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				Slot->SetHorizontalAlignment(HAlign_Fill);
			}
			else
			{
				Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				Slot->SetHorizontalAlignment(HAlign_Left);
			}
		}
	}

	void ConfigureEmbeddedSettingButton(UAGASSFrontendButtonBase* const Button)
	{
		if (Button == nullptr)
		{
			return;
		}

		// Setting rows live inside a selectable ListView. Trigger on mouse-down so the row does not
		// steal the release event before the embedded button can apply the change.
		Button->SetClickMethod(EButtonClickMethod::MouseDown);
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

	void AssignObjectWidgetProperty(UUserWidget* Owner, UClass* DeclaringClass, const TCHAR* PropertyName, UObject* Value)
	{
		if (Owner == nullptr || DeclaringClass == nullptr)
		{
			return;
		}

		if (FObjectPropertyBase* const Property = FindFProperty<FObjectPropertyBase>(DeclaringClass, PropertyName))
		{
			Property->SetObjectPropertyValue_InContainer(Owner, Value);
		}
	}

	void AssignCommonSettingBindings(UUserWidget* Owner)
	{
		if (Owner == nullptr)
		{
			return;
		}

		AssignObjectWidgetProperty(Owner, UGameSettingListEntry_Setting::StaticClass(), TEXT("Text_SettingName"), Owner->GetWidgetFromName(TEXT("Text_SettingName")));
	}

	UWidgetTree* EnsureWidgetTree(UUserWidget* Owner)
	{
		if (Owner == nullptr)
		{
			return nullptr;
		}

		if (Owner->WidgetTree == nullptr)
		{
			Owner->WidgetTree = NewObject<UWidgetTree>(Owner, TEXT("WidgetTree"), RF_Transient);
		}

		return Owner->WidgetTree;
	}

	void EnsureRotatorWidgetTree(UAGASSGameSettingRotatorWidget* Owner)
	{
		if (UWidgetTree* const WidgetTree = EnsureWidgetTree(Owner))
		{
			if (WidgetTree->RootWidget != nullptr)
			{
				return;
			}

			UBorder* const Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Rotator"));
			Border->SetPadding(FMargin(10.f, 6.f));
			Border->SetBrushColor(FLinearColor(0.26f, 0.28f, 0.29f, 0.96f));

			UCommonTextBlock* const RotatorText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("MyText"));
			RotatorText->SetFont(MakeSettingsFont(16));
			RotatorText->SetJustification(ETextJustify::Center);
			RotatorText->SetAutoWrapText(false);
			AssignObjectWidgetProperty(Owner, UCommonRotator::StaticClass(), TEXT("MyText"), RotatorText);
			Border->SetContent(RotatorText);

			WidgetTree->RootWidget = Border;
		}
	}

	void EnsureScalarEntryWidgetTree(UAGASSGameSettingScalarEntryWidget* Owner)
	{
		if (UWidgetTree* const WidgetTree = EnsureWidgetTree(Owner))
		{
			if (WidgetTree->RootWidget != nullptr)
			{
				return;
			}

			UBorder* const Border = CreateSettingsRowBorder(WidgetTree, TEXT("Border_Row"));
			WidgetTree->RootWidget = Border;

			UHorizontalBox* const LayoutRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row_Layout"));
			Border->SetContent(LayoutRow);

			UCommonTextBlock* const SettingNameText = CreateSettingNameText(WidgetTree);
			AddHorizontalChild(LayoutRow, SettingNameText, 1.f, FMargin(0.f, 0.f, 16.f, 0.f));

			UHorizontalBox* const ValuePanel = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Panel_Value"));
			AddHorizontalChild(LayoutRow, ValuePanel, 1.f);

			UAnalogSlider* const Slider = WidgetTree->ConstructWidget<UAnalogSlider>(UAnalogSlider::StaticClass(), TEXT("Slider_SettingValue"));
			AddHorizontalChild(ValuePanel, Slider, 1.f, FMargin(0.f, 0.f, 12.f, 0.f));

			USizeBox* const ValueTextSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Box_ValueText"));
			ValueTextSizeBox->SetMinDesiredWidth(88.f);
			ValueTextSizeBox->SetWidthOverride(96.f);
			UCommonTextBlock* const ValueText = CreateValueText(WidgetTree, TEXT("Text_SettingValue"));
			ValueTextSizeBox->SetContent(ValueText);
			AddHorizontalChild(ValuePanel, ValueTextSizeBox, 0.f);
		}
	}

	void EnsureDiscreteEntryWidgetTree(UAGASSGameSettingDiscreteEntryWidget* Owner)
	{
		if (UWidgetTree* const WidgetTree = EnsureWidgetTree(Owner))
		{
			if (WidgetTree->RootWidget != nullptr)
			{
				return;
			}

			UBorder* const Border = CreateSettingsRowBorder(WidgetTree, TEXT("Border_Row"));
			WidgetTree->RootWidget = Border;

			UHorizontalBox* const LayoutRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row_Layout"));
			Border->SetContent(LayoutRow);

			UCommonTextBlock* const SettingNameText = CreateSettingNameText(WidgetTree);
			AddHorizontalChild(LayoutRow, SettingNameText, 1.f, FMargin(0.f, 0.f, 16.f, 0.f));

			UHorizontalBox* const ValuePanel = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Panel_Value"));
			AddHorizontalChild(LayoutRow, ValuePanel, 1.f);

			UAGASSFrontendButtonBase* const DecreaseButton = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Decrease"));
			DecreaseButton->SetButtonText(FText::FromString(TEXT("<")));
			AddHorizontalChild(ValuePanel, DecreaseButton, 0.f, FMargin(0.f, 0.f, 8.f, 0.f));

			UAGASSGameSettingRotatorWidget* const Rotator = WidgetTree->ConstructWidget<UAGASSGameSettingRotatorWidget>(UAGASSGameSettingRotatorWidget::StaticClass(), TEXT("Rotator_SettingValue"));
			AddHorizontalChild(ValuePanel, Rotator, 1.f, FMargin(0.f, 0.f, 8.f, 0.f));

			UAGASSFrontendButtonBase* const IncreaseButton = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Increase"));
			IncreaseButton->SetButtonText(FText::FromString(TEXT(">")));
			AddHorizontalChild(ValuePanel, IncreaseButton, 0.f);
		}
	}

	void EnsureActionEntryWidgetTree(UAGASSGameSettingActionEntryWidget* Owner)
	{
		if (UWidgetTree* const WidgetTree = EnsureWidgetTree(Owner))
		{
			if (WidgetTree->RootWidget != nullptr)
			{
				return;
			}

			UBorder* const Border = CreateSettingsRowBorder(WidgetTree, TEXT("Border_Row"));
			WidgetTree->RootWidget = Border;

			UHorizontalBox* const LayoutRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row_Layout"));
			Border->SetContent(LayoutRow);

			UCommonTextBlock* const SettingNameText = CreateSettingNameText(WidgetTree);
			AddHorizontalChild(LayoutRow, SettingNameText, 1.f, FMargin(0.f, 0.f, 16.f, 0.f));

			UAGASSFrontendButtonBase* const ActionButton = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Action"));
			ActionButton->SetButtonText(NSLOCTEXT("AGASSFrontend", "SettingsActionFallback", "Open"));
			AddHorizontalChild(LayoutRow, ActionButton, 0.f);
		}
	}

	void EnsureKeyBindEntryWidgetTree(UAGASSGameSettingKeyBindEntryWidget* Owner)
	{
		if (UWidgetTree* const WidgetTree = EnsureWidgetTree(Owner))
		{
			if (WidgetTree->RootWidget != nullptr)
			{
				return;
			}

			UBorder* const Border = CreateSettingsRowBorder(WidgetTree, TEXT("Border_Row"));
			WidgetTree->RootWidget = Border;

			UHorizontalBox* const LayoutRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row_Layout"));
			Border->SetContent(LayoutRow);

			UCommonTextBlock* const SettingNameText = CreateSettingNameText(WidgetTree);
			AddHorizontalChild(LayoutRow, SettingNameText, 1.f, FMargin(0.f, 0.f, 16.f, 0.f));

			UHorizontalBox* const BindingPanel = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Panel_Binding"));
			AddHorizontalChild(LayoutRow, BindingPanel, 1.f, FMargin(0.f, 0.f, 12.f, 0.f));

			UImage* const BindingIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon_CurrentBinding"));
			BindingIcon->SetVisibility(ESlateVisibility::Collapsed);
			AddHorizontalChild(BindingPanel, BindingIcon, 0.f, FMargin(0.f, 0.f, 10.f, 0.f));

			UCommonTextBlock* const BindingText = CreateValueText(WidgetTree, TEXT("Text_CurrentBinding"));
			BindingText->SetJustification(ETextJustify::Left);
			AddHorizontalChild(BindingPanel, BindingText, 1.f);

			UAGASSFrontendButtonBase* const RebindButton = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Rebind"));
			RebindButton->SetButtonText(NSLOCTEXT("AGASSFrontend", "RebindButtonLabel", "Rebind"));
			AddHorizontalChild(LayoutRow, RebindButton, 0.f, FMargin(0.f, 0.f, 8.f, 0.f));

			UAGASSFrontendButtonBase* const ResetButton = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Reset"));
			ResetButton->SetButtonText(NSLOCTEXT("AGASSFrontend", "ResetBindingButtonLabel", "Reset"));
			AddHorizontalChild(LayoutRow, ResetButton, 0.f);
		}
	}

	void EnsureNavigationEntryWidgetTree(UAGASSGameSettingNavigationEntryWidget* Owner)
	{
		if (UWidgetTree* const WidgetTree = EnsureWidgetTree(Owner))
		{
			if (WidgetTree->RootWidget != nullptr)
			{
				return;
			}

			UBorder* const Border = CreateSettingsRowBorder(WidgetTree, TEXT("Border_Row"));
			WidgetTree->RootWidget = Border;

			UHorizontalBox* const LayoutRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row_Layout"));
			Border->SetContent(LayoutRow);

			UCommonTextBlock* const SettingNameText = CreateSettingNameText(WidgetTree);
			AddHorizontalChild(LayoutRow, SettingNameText, 1.f, FMargin(0.f, 0.f, 16.f, 0.f));

			UAGASSFrontendButtonBase* const NavigateButton = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Navigate"));
			NavigateButton->SetButtonText(NSLOCTEXT("AGASSFrontend", "SettingsNavigationFallback", "Open"));
			AddHorizontalChild(LayoutRow, NavigateButton, 0.f);
		}
	}

	void EnsureSectionHeaderEntryWidgetTree(UAGASSGameSettingSectionHeaderEntryWidget* Owner)
	{
		if (UWidgetTree* const WidgetTree = EnsureWidgetTree(Owner))
		{
			if (WidgetTree->RootWidget != nullptr)
			{
				return;
			}

			UBorder* const Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Row"));
			Border->SetPadding(FMargin(8.f, 12.f, 8.f, 6.f));
			Border->SetBrushColor(FLinearColor(0.10f, 0.11f, 0.12f, 0.96f));
			WidgetTree->RootWidget = Border;

			UCommonTextBlock* const SettingNameText = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_SettingName"));
			SettingNameText->SetFont(MakeSettingsFont(17));
			SettingNameText->SetAutoWrapText(true);
			Border->SetContent(SettingNameText);
		}
	}
}

UAGASSGameSettingRotatorWidget::UAGASSGameSettingRotatorWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAGASSGameSettingRotatorWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureRotatorWidgetTree(this);
	}
}

TSharedRef<SWidget> UAGASSGameSettingRotatorWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureRotatorWidgetTree(this);
	}

	return Super::RebuildWidget();
}

void UAGASSGameSettingScalarEntryWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureScalarEntryWidgetTree(this);
	}
}

TSharedRef<SWidget> UAGASSGameSettingScalarEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureScalarEntryWidgetTree(this);
	}

	return Super::RebuildWidget();
}

void UAGASSGameSettingScalarEntryWidget::NativeOnInitialized()
{
	AssignCommonSettingBindings(this);
	AssignObjectWidgetProperty(this, UGameSettingListEntrySetting_Scalar::StaticClass(), TEXT("Panel_Value"), GetWidgetFromName(TEXT("Panel_Value")));
	AssignObjectWidgetProperty(this, UGameSettingListEntrySetting_Scalar::StaticClass(), TEXT("Slider_SettingValue"), GetWidgetFromName(TEXT("Slider_SettingValue")));
	AssignObjectWidgetProperty(this, UGameSettingListEntrySetting_Scalar::StaticClass(), TEXT("Text_SettingValue"), GetWidgetFromName(TEXT("Text_SettingValue")));

	Super::NativeOnInitialized();
}

FReply UAGASSGameSettingScalarEntryWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	if (ShouldRedirectFocusToChild(this))
	{
		const FReply FocusReply = FocusChildWidgetByName(this, TEXT("Slider_SettingValue"), InFocusEvent);
		if (FocusReply.IsEventHandled())
		{
			return FocusReply;
		}
	}

	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

void UAGASSGameSettingDiscreteEntryWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureDiscreteEntryWidgetTree(this);
	}
}

TSharedRef<SWidget> UAGASSGameSettingDiscreteEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureDiscreteEntryWidgetTree(this);
	}

	return Super::RebuildWidget();
}

void UAGASSGameSettingDiscreteEntryWidget::NativeOnInitialized()
{
	AssignCommonSettingBindings(this);
	AssignObjectWidgetProperty(this, UGameSettingListEntrySetting_Discrete::StaticClass(), TEXT("Panel_Value"), GetWidgetFromName(TEXT("Panel_Value")));
	AssignObjectWidgetProperty(this, UGameSettingListEntrySetting_Discrete::StaticClass(), TEXT("Rotator_SettingValue"), GetWidgetFromName(TEXT("Rotator_SettingValue")));
	AssignObjectWidgetProperty(this, UGameSettingListEntrySetting_Discrete::StaticClass(), TEXT("Button_Decrease"), GetWidgetFromName(TEXT("Button_Decrease")));
	AssignObjectWidgetProperty(this, UGameSettingListEntrySetting_Discrete::StaticClass(), TEXT("Button_Increase"), GetWidgetFromName(TEXT("Button_Increase")));
	ConfigureEmbeddedSettingButton(Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Decrease"))));
	ConfigureEmbeddedSettingButton(Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Increase"))));

	Super::NativeOnInitialized();
}

FReply UAGASSGameSettingDiscreteEntryWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	if (ShouldRedirectFocusToChild(this))
	{
		const FReply FocusReply = FocusChildWidgetByName(this, TEXT("Rotator_SettingValue"), InFocusEvent);
		if (FocusReply.IsEventHandled())
		{
			return FocusReply;
		}
	}

	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

FReply UAGASSGameSettingDiscreteEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		if (WidgetContainsScreenPosition(GetWidgetFromName(TEXT("Button_Decrease")), ScreenPosition))
		{
			HandleOptionDecrease();
			return FReply::Handled();
		}

		if (WidgetContainsScreenPosition(GetWidgetFromName(TEXT("Button_Increase")), ScreenPosition))
		{
			HandleOptionIncrease();
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UAGASSGameSettingActionEntryWidget::SetSetting(UGameSetting* InSetting)
{
	Super::SetSetting(InSetting);

	if (const UGameSettingAction* const ResolvedActionSetting = Cast<UGameSettingAction>(InSetting))
	{
		if (UAGASSFrontendButtonBase* const ActionButton = Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Action"))))
		{
			ActionButton->SetButtonText(ResolvedActionSetting->GetActionText());
		}
	}
}

void UAGASSGameSettingActionEntryWidget::NativeOnInitialized()
{
	AssignCommonSettingBindings(this);
	AssignObjectWidgetProperty(this, UGameSettingListEntrySetting_Action::StaticClass(), TEXT("Button_Action"), GetWidgetFromName(TEXT("Button_Action")));
	ConfigureEmbeddedSettingButton(Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Action"))));

	Super::NativeOnInitialized();
}

void UAGASSGameSettingActionEntryWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureActionEntryWidgetTree(this);
	}
}

TSharedRef<SWidget> UAGASSGameSettingActionEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureActionEntryWidgetTree(this);
	}

	return Super::RebuildWidget();
}

FReply UAGASSGameSettingActionEntryWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	if (ShouldRedirectFocusToChild(this))
	{
		const FReply FocusReply = FocusChildWidgetByName(this, TEXT("Button_Action"), InFocusEvent);
		if (FocusReply.IsEventHandled())
		{
			return FocusReply;
		}
	}

	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

FReply UAGASSGameSettingActionEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& WidgetContainsScreenPosition(GetWidgetFromName(TEXT("Button_Action")), InMouseEvent.GetScreenSpacePosition()))
	{
		HandleActionButtonClicked();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UAGASSGameSettingKeyBindEntryWidget::SetSetting(UGameSetting* InSetting)
{
	KeyBindSetting = CastChecked<UAGASSGameSettingKeyBind>(InSetting);

	Super::SetSetting(InSetting);
	Refresh();
}

void UAGASSGameSettingKeyBindEntryWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureKeyBindEntryWidgetTree(this);
	}
}

void UAGASSGameSettingKeyBindEntryWidget::NativeOnInitialized()
{
	AssignCommonSettingBindings(this);

	if (UAGASSFrontendButtonBase* const RebindButton = Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Rebind"))))
	{
		RebindButton->OnClicked().RemoveAll(this);
		RebindButton->OnClicked().AddUObject(this, &ThisClass::HandleRebindClicked);
		ConfigureEmbeddedSettingButton(RebindButton);
	}

	if (UAGASSFrontendButtonBase* const ResetButton = Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Reset"))))
	{
		ResetButton->OnClicked().RemoveAll(this);
		ResetButton->OnClicked().AddUObject(this, &ThisClass::HandleResetClicked);
		ConfigureEmbeddedSettingButton(ResetButton);
	}

	Super::NativeOnInitialized();
}

void UAGASSGameSettingKeyBindEntryWidget::NativeOnEntryReleased()
{
	Super::NativeOnEntryReleased();

	KeyBindSetting = nullptr;
	PendingSelectedKey = EKeys::Invalid;
}

void UAGASSGameSettingKeyBindEntryWidget::OnSettingChanged()
{
	Super::OnSettingChanged();
	Refresh();
}

TSharedRef<SWidget> UAGASSGameSettingKeyBindEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureKeyBindEntryWidgetTree(this);
	}

	return Super::RebuildWidget();
}

FReply UAGASSGameSettingKeyBindEntryWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	if (ShouldRedirectFocusToChild(this))
	{
		const FReply FocusReply = FocusChildWidgetByName(this, TEXT("Button_Rebind"), InFocusEvent);
		if (FocusReply.IsEventHandled())
		{
			return FocusReply;
		}
	}

	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

FReply UAGASSGameSettingKeyBindEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		if (WidgetContainsScreenPosition(GetWidgetFromName(TEXT("Button_Rebind")), ScreenPosition))
		{
			HandleRebindClicked();
			return FReply::Handled();
		}

		if (WidgetContainsScreenPosition(GetWidgetFromName(TEXT("Button_Reset")), ScreenPosition))
		{
			HandleResetClicked();
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UAGASSGameSettingKeyBindEntryWidget::HandleRebindClicked()
{
	if (KeyBindSetting == nullptr)
	{
		return;
	}

	TSubclassOf<UAGASSGameSettingPressAnyKeyWidget> PressAnyKeyClass = UAGASSGameSettingPressAnyKeyWidget::StaticClass();
	if (const UAGASSFrontendDeveloperSettings* const FrontendSettings = UAGASSFrontendDeveloperSettings::Get())
	{
		if (UClass* const LoadedClass = FrontendSettings->GameSettingPressAnyKeyWidgetClass.LoadSynchronous())
		{
			PressAnyKeyClass = LoadedClass;
		}
	}

	UAGASSGameSettingPressAnyKeyWidget* const PressAnyKeyWidget = PushModalWidgetForPlayer<UAGASSGameSettingPressAnyKeyWidget>(
		GetOwningLocalPlayer(),
		PressAnyKeyClass);
	if (PressAnyKeyWidget == nullptr)
	{
		return;
	}

	PressAnyKeyWidget->Configure(KeyBindSetting->GetInputType(), KeyBindSetting->AllowsAxisKeys());
	PressAnyKeyWidget->OnKeySelected.AddUObject(this, &ThisClass::HandleKeySelected);
	PressAnyKeyWidget->OnKeySelectionCanceled.AddUObject(this, &ThisClass::HandleKeySelectionCanceled);
}

void UAGASSGameSettingKeyBindEntryWidget::HandleResetClicked()
{
	if (KeyBindSetting != nullptr)
	{
		KeyBindSetting->ResetToDefault();
	}
}

void UAGASSGameSettingKeyBindEntryWidget::HandleKeySelected(const FKey InKey)
{
	if (KeyBindSetting == nullptr)
	{
		return;
	}

	FText ConflictText;
	if (KeyBindSetting->HasDuplicateBindingForKey(InKey, ConflictText))
	{
		PendingSelectedKey = InKey;

		TSubclassOf<UAGASSKeyAlreadyBoundWarningWidget> WarningClass = UAGASSKeyAlreadyBoundWarningWidget::StaticClass();
		if (const UAGASSFrontendDeveloperSettings* const FrontendSettings = UAGASSFrontendDeveloperSettings::Get())
		{
			if (UClass* const LoadedClass = FrontendSettings->KeyAlreadyBoundWarningWidgetClass.LoadSynchronous())
			{
				WarningClass = LoadedClass;
			}
		}

		if (UAGASSKeyAlreadyBoundWarningWidget* const WarningWidget = PushModalWidgetForPlayer<UAGASSKeyAlreadyBoundWarningWidget>(
			GetOwningLocalPlayer(),
			WarningClass))
		{
			WarningWidget->SetWarningText(ConflictText);
			WarningWidget->OnConfirmed.AddUObject(this, &ThisClass::HandleDuplicateConfirmed);
			WarningWidget->OnCanceled.AddUObject(this, &ThisClass::HandleDuplicateCanceled);
		}

		return;
	}

	FText FailureReason;
	KeyBindSetting->ChangeBinding(InKey, FailureReason);
}

void UAGASSGameSettingKeyBindEntryWidget::HandleKeySelectionCanceled()
{
	PendingSelectedKey = EKeys::Invalid;
}

void UAGASSGameSettingKeyBindEntryWidget::HandleDuplicateConfirmed()
{
	if (KeyBindSetting == nullptr || !PendingSelectedKey.IsValid())
	{
		PendingSelectedKey = EKeys::Invalid;
		return;
	}

	FText FailureReason;
	KeyBindSetting->ChangeBinding(PendingSelectedKey, FailureReason);
	PendingSelectedKey = EKeys::Invalid;
}

void UAGASSGameSettingKeyBindEntryWidget::HandleDuplicateCanceled()
{
	PendingSelectedKey = EKeys::Invalid;
}

void UAGASSGameSettingKeyBindEntryWidget::Refresh()
{
	if (KeyBindSetting == nullptr)
	{
		return;
	}

	if (UCommonTextBlock* const BindingText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_CurrentBinding"))))
	{
		BindingText->SetText(KeyBindSetting->GetCurrentKeyText());
	}

	if (UImage* const BindingIcon = Cast<UImage>(GetWidgetFromName(TEXT("Icon_CurrentBinding"))))
	{
		FSlateBrush BindingBrush;
		if (KeyBindSetting->TryGetCurrentKeyBrush(BindingBrush))
		{
			BindingIcon->SetBrush(BindingBrush);
			BindingIcon->SetVisibility(ESlateVisibility::HitTestInvisible);

			if (UCommonTextBlock* const BindingText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_CurrentBinding"))))
			{
				BindingText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			BindingIcon->SetVisibility(ESlateVisibility::Collapsed);

			if (UCommonTextBlock* const BindingText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_CurrentBinding"))))
			{
				BindingText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}

	if (UAGASSFrontendButtonBase* const ResetButton = Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Reset"))))
	{
		ResetButton->SetVisibility(KeyBindSetting->IsCustomized() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UAGASSGameSettingSectionHeaderEntryWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureSectionHeaderEntryWidgetTree(this);
	}
}

void UAGASSGameSettingSectionHeaderEntryWidget::NativeOnInitialized()
{
	AssignCommonSettingBindings(this);

	Super::NativeOnInitialized();
}

TSharedRef<SWidget> UAGASSGameSettingSectionHeaderEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureSectionHeaderEntryWidgetTree(this);
	}

	return Super::RebuildWidget();
}

void UAGASSGameSettingNavigationEntryWidget::SetSetting(UGameSetting* InSetting)
{
	Super::SetSetting(InSetting);

	if (const UGameSettingCollectionPage* const NavigationSetting = Cast<UGameSettingCollectionPage>(InSetting))
	{
		if (UAGASSFrontendButtonBase* const NavigateButton = Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Navigate"))))
		{
			NavigateButton->SetButtonText(NavigationSetting->GetNavigationText());
		}
	}
}

void UAGASSGameSettingNavigationEntryWidget::NativeOnInitialized()
{
	AssignCommonSettingBindings(this);
	AssignObjectWidgetProperty(this, UGameSettingListEntrySetting_Navigation::StaticClass(), TEXT("Button_Navigate"), GetWidgetFromName(TEXT("Button_Navigate")));
	ConfigureEmbeddedSettingButton(Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Navigate"))));

	Super::NativeOnInitialized();
}

void UAGASSGameSettingNavigationEntryWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();

	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureNavigationEntryWidgetTree(this);
	}
}

TSharedRef<SWidget> UAGASSGameSettingNavigationEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		EnsureNavigationEntryWidgetTree(this);
	}

	return Super::RebuildWidget();
}

FReply UAGASSGameSettingNavigationEntryWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	if (ShouldRedirectFocusToChild(this))
	{
		const FReply FocusReply = FocusChildWidgetByName(this, TEXT("Button_Navigate"), InFocusEvent);
		if (FocusReply.IsEventHandled())
		{
			return FocusReply;
		}
	}

	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

FReply UAGASSGameSettingNavigationEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& WidgetContainsScreenPosition(GetWidgetFromName(TEXT("Button_Navigate")), InMouseEvent.GetScreenSpacePosition()))
	{
		HandleNavigationButtonClicked();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
