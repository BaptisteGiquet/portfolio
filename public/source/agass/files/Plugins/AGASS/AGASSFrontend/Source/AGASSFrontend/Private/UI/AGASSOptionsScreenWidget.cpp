#include "UI/AGASSOptionsScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/DataTable.h"
#include "GameSetting.h"
#include "GameSettingCollection.h"
#include "GameSettingValue.h"
#include "Input/CommonUIInputTypes.h"
#include "Settings/AGASSFrontendDeveloperSettings.h"
#include "Styling/CoreStyle.h"
#include "UI/AGASSFrontendButtonBase.h"
#include "UI/Settings/AGASSGameSettingPanelWidget.h"
#include "UI/Settings/AGASSGameSettingRegistry.h"

UAGASSOptionsScreenWidget::UAGASSOptionsScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
	OverrideBackActionDisplayName = NSLOCTEXT("AGASSFrontend", "FrontendBackActionName", "Back");
}

FText UAGASSOptionsScreenWidget::GetTitleText() const
{
	return NSLOCTEXT("AGASSFrontend", "OptionsTitle", "Options");
}

void UAGASSOptionsScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveWidgetByName(Button_TabVideo, TEXT("Button_TabVideo"));
	ResolveWidgetByName(Button_TabAudio, TEXT("Button_TabAudio"));
	ResolveWidgetByName(Button_TabGameplay, TEXT("Button_TabGameplay"));
	ResolveWidgetByName(Button_TabControls, TEXT("Button_TabControls"));
	ResolveWidgetByName(Button_TabInterface, TEXT("Button_TabInterface"));
	ResolveWidgetByName(Text_OptionsSummary, TEXT("Text_OptionsSummary"));
	ResolveWidgetByName(Settings_Panel, TEXT("Settings_Panel"));

	BindCategoryButton(Button_TabVideo, &ThisClass::HandleVideoTabClicked);
	BindCategoryButton(Button_TabAudio, &ThisClass::HandleAudioTabClicked);
	BindCategoryButton(Button_TabGameplay, &ThisClass::HandleGameplayTabClicked);
	BindCategoryButton(Button_TabControls, &ThisClass::HandleControlsTabClicked);
	BindCategoryButton(Button_TabInterface, &ThisClass::HandleInterfaceTabClicked);

	if (!NextTabActionHandle.IsValid())
	{
		if (UDataTable* const GenericInputActions = LoadObject<UDataTable>(nullptr, TEXT("/CommonUI/GenericInputActionDataTable.GenericInputActionDataTable")))
		{
			FDataTableRowHandle NextTabInputActionData;
			NextTabInputActionData.DataTable = GenericInputActions;
			NextTabInputActionData.RowName = TEXT("GenericRightShoulder");
			NextTabActionHandle = RegisterUIActionBinding(FBindUIActionArgs(NextTabInputActionData, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleNextTabAction)));

			FDataTableRowHandle PreviousTabInputActionData;
			PreviousTabInputActionData.DataTable = GenericInputActions;
			PreviousTabInputActionData.RowName = TEXT("GenericLeftShoulder");
			PreviousTabActionHandle = RegisterUIActionBinding(FBindUIActionArgs(PreviousTabInputActionData, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandlePreviousTabAction)));
		}
	}

	InitializeRegistry();
	RegisterTopLevelTabs();
	RefreshTopLevelTabs();
	RefreshSummaryText();
}

void UAGASSOptionsScreenWidget::NativeDestruct()
{
	if (Registry != nullptr && RegistrySettingChangedHandle.IsValid())
	{
		Registry->OnSettingChangedEvent.Remove(RegistrySettingChangedHandle);
		RegistrySettingChangedHandle.Reset();
	}

	if (NextTabActionHandle.IsValid())
	{
		NextTabActionHandle.Unregister();
		NextTabActionHandle = FUIActionBindingHandle();
	}

	if (PreviousTabActionHandle.IsValid())
	{
		PreviousTabActionHandle.Unregister();
		PreviousTabActionHandle = FUIActionBindingHandle();
	}

	ChangeTracker.StopWatchingRegistry();

	Super::NativeDestruct();
}

void UAGASSOptionsScreenWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	InitializeRegistry();

	RegisterTopLevelTabs();
	ShowCategory(ActiveCategoryId, false);
	RefreshSummaryText();
}

void UAGASSOptionsScreenWidget::NativeOnDeactivated()
{
	if (Registry != nullptr)
	{
		Registry->SaveChanges();
	}

	ChangeTracker.StopWatchingRegistry();

	Super::NativeOnDeactivated();
}

bool UAGASSOptionsScreenWidget::NativeOnHandleBackAction()
{
	if (Settings_Panel != nullptr && Settings_Panel->CanPopNavigationStack())
	{
		Settings_Panel->PopNavigationStack();
		return true;
	}

	DeactivateWidget();
	return true;
}

void UAGASSOptionsScreenWidget::BuildFallbackScreenContent(UVerticalBox* ContentBox)
{
	Text_OptionsSummary = AddBodyText(
		ContentBox,
		NSLOCTEXT("AGASSFrontend", "OptionsSummaryDefault", "Adjust local video, audio, controls, and interface settings. Changes apply immediately."),
		13,
		TEXT("Text_OptionsSummary"));
	AddSpacer(ContentBox, 10.f);

	UHorizontalBox* const TopTabsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row_TopTabs"));
	if (UVerticalBoxSlot* const TabsSlot = ContentBox->AddChildToVerticalBox(TopTabsRow))
	{
		TabsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	}

	auto AddTopTabButton = [this, TopTabsRow](const FName& WidgetName, const FText& Label, TObjectPtr<UAGASSFrontendButtonBase>& OutButton)
	{
		USizeBox* const ButtonSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Box_%s"), *WidgetName.ToString()));
		ButtonSizeBox->SetMinDesiredWidth(200.f);
		ButtonSizeBox->SetMinDesiredHeight(48.f);

		OutButton = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), WidgetName);
		OutButton->SetButtonText(Label);
		ButtonSizeBox->SetContent(OutButton);

		if (UHorizontalBoxSlot* const Slot = TopTabsRow->AddChildToHorizontalBox(ButtonSizeBox))
		{
			Slot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	};

	AddTopTabButton(TEXT("Button_TabVideo"), GetCategoryDisplayText(UAGASSGameSettingRegistry::VideoCollection), Button_TabVideo);
	AddTopTabButton(TEXT("Button_TabAudio"), GetCategoryDisplayText(UAGASSGameSettingRegistry::AudioCollection), Button_TabAudio);
	AddTopTabButton(TEXT("Button_TabGameplay"), GetCategoryDisplayText(UAGASSGameSettingRegistry::GameplayCollection), Button_TabGameplay);
	AddTopTabButton(TEXT("Button_TabControls"), GetCategoryDisplayText(UAGASSGameSettingRegistry::ControlsCollection), Button_TabControls);
	AddTopTabButton(TEXT("Button_TabInterface"), GetCategoryDisplayText(UAGASSGameSettingRegistry::InterfaceCollection), Button_TabInterface);

	USizeBox* const PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Box_SettingsPanel"));
	PanelSizeBox->SetMinDesiredHeight(620.f);
	if (UVerticalBoxSlot* const PanelContainerSlot = ContentBox->AddChildToVerticalBox(PanelSizeBox))
	{
		PanelContainerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	TSubclassOf<UAGASSGameSettingPanelWidget> SettingsPanelClass;
	if (const UAGASSFrontendDeveloperSettings* const FrontendSettings = UAGASSFrontendDeveloperSettings::Get())
	{
		SettingsPanelClass = FrontendSettings->GameSettingPanelWidgetClass.LoadSynchronous();
	}

	if (SettingsPanelClass != nullptr)
	{
		Settings_Panel = WidgetTree->ConstructWidget<UAGASSGameSettingPanelWidget>(SettingsPanelClass, TEXT("Settings_Panel"));
		PanelSizeBox->SetContent(Settings_Panel);
	}
	else
	{
		UTextBlock* const MissingPanelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_OptionsPanelMissing"));
		MissingPanelText->SetText(NSLOCTEXT("AGASSFrontend", "OptionsPanelMissingAsset", "Options panel asset is missing from AGASS frontend settings."));
		MissingPanelText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14));
		MissingPanelText->SetAutoWrapText(true);
		MissingPanelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.89f, 0.93f, 0.97f)));
		PanelSizeBox->SetContent(MissingPanelText);
	}
}

void UAGASSOptionsScreenWidget::RefreshFromSessionState()
{
	Super::RefreshFromSessionState();

	RefreshSummaryText();
}

UWidget* UAGASSOptionsScreenWidget::ResolveInitialFocusTarget() const
{
	if (Settings_Panel != nullptr)
	{
		return Settings_Panel.Get();
	}

	if (Button_TabVideo != nullptr)
	{
		return Button_TabVideo.Get();
	}

	return nullptr;
}

#if WITH_EDITOR
void UAGASSOptionsScreenWidget::GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const
{
	OutRequiredWidgets.Append({
		TEXT("Button_TabVideo"),
		TEXT("Button_TabAudio"),
		TEXT("Button_TabGameplay"),
		TEXT("Button_TabControls"),
		TEXT("Button_TabInterface"),
		TEXT("Text_OptionsSummary"),
		TEXT("Settings_Panel")
	});
}
#endif

void UAGASSOptionsScreenWidget::InitializeRegistry()
{
	if (Registry == nullptr && GetOwningLocalPlayer() != nullptr)
	{
		Registry = NewObject<UAGASSGameSettingRegistry>(this);
		Registry->Initialize(GetOwningLocalPlayer());
		RegistrySettingChangedHandle = Registry->OnSettingChangedEvent.AddUObject(this, &ThisClass::HandleRegistrySettingChanged);
	}

	if (Settings_Panel != nullptr && Registry != nullptr)
	{
		Settings_Panel->SetRegistry(Registry);
	}
}

void UAGASSOptionsScreenWidget::RegisterTopLevelTabs()
{
	if (Button_TabVideo != nullptr)
	{
		Button_TabVideo->SetUseSelectedVisual(true);
		Button_TabVideo->SetButtonText(GetCategoryDisplayText(UAGASSGameSettingRegistry::VideoCollection));
	}
	if (Button_TabAudio != nullptr)
	{
		Button_TabAudio->SetUseSelectedVisual(true);
		Button_TabAudio->SetButtonText(GetCategoryDisplayText(UAGASSGameSettingRegistry::AudioCollection));
	}
	if (Button_TabGameplay != nullptr)
	{
		Button_TabGameplay->SetUseSelectedVisual(true);
		Button_TabGameplay->SetButtonText(GetCategoryDisplayText(UAGASSGameSettingRegistry::GameplayCollection));
	}
	if (Button_TabControls != nullptr)
	{
		Button_TabControls->SetUseSelectedVisual(true);
		Button_TabControls->SetButtonText(GetCategoryDisplayText(UAGASSGameSettingRegistry::ControlsCollection));
	}
	if (Button_TabInterface != nullptr)
	{
		Button_TabInterface->SetUseSelectedVisual(true);
		Button_TabInterface->SetButtonText(GetCategoryDisplayText(UAGASSGameSettingRegistry::InterfaceCollection));
	}
}

void UAGASSOptionsScreenWidget::BindCategoryButton(UAGASSFrontendButtonBase* Button, void (UAGASSOptionsScreenWidget::*Handler)())
{
	if (Button != nullptr)
	{
		Button->OnClicked().RemoveAll(this);
		Button->OnClicked().AddUObject(this, Handler);
	}
}

void UAGASSOptionsScreenWidget::ShowCategory(const FName& InCategoryId, bool bFocusSettingsPanel)
{
	if (InCategoryId.IsNone())
	{
		return;
	}

	ActiveCategoryId = InCategoryId;

	if (Registry == nullptr || Settings_Panel == nullptr)
	{
		RefreshTopLevelTabs();
		return;
	}

	if (UGameSettingCollectionPage* const CategoryPage = Cast<UGameSettingCollectionPage>(Registry->FindSettingByDevName(InCategoryId)))
	{
		FGameSettingFilterState FilterState;
		FilterState.AddSettingToRootList(CategoryPage);
		Settings_Panel->SetFilterState(FilterState);
	}

	RefreshTopLevelTabs();

	if (bFocusSettingsPanel)
	{
		FocusSettingsPanelIfPossible();
	}
}

void UAGASSOptionsScreenWidget::RefreshTopLevelTabs()
{
	auto RefreshCategoryButton = [this](UAGASSFrontendButtonBase* Button, const FName& CategoryId)
	{
		if (Button != nullptr)
		{
			const bool bIsActiveCategory = CategoryId == ActiveCategoryId;
			Button->SetUseSelectedVisual(bIsActiveCategory);
			Button->SetIsSelected(bIsActiveCategory, false);
		}
	};

	RegisterTopLevelTabs();
	RefreshCategoryButton(Button_TabVideo, UAGASSGameSettingRegistry::VideoCollection);
	RefreshCategoryButton(Button_TabAudio, UAGASSGameSettingRegistry::AudioCollection);
	RefreshCategoryButton(Button_TabGameplay, UAGASSGameSettingRegistry::GameplayCollection);
	RefreshCategoryButton(Button_TabControls, UAGASSGameSettingRegistry::ControlsCollection);
	RefreshCategoryButton(Button_TabInterface, UAGASSGameSettingRegistry::InterfaceCollection);
}

void UAGASSOptionsScreenWidget::RefreshSummaryText()
{
	const FText SummaryText = NSLOCTEXT("AGASSFrontend", "OptionsSummaryImmediate", "Adjust local video, audio, controls, and interface settings. Changes apply immediately.");

	if (Text_Status != nullptr)
	{
		Text_Status->SetText(SummaryText);
		Text_Status->SetVisibility(ESlateVisibility::Visible);
	}

	if (Text_OptionsSummary != nullptr)
	{
		Text_OptionsSummary->SetText(SummaryText);
		Text_OptionsSummary->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FText UAGASSOptionsScreenWidget::GetCategoryDisplayText(const FName& CategoryId) const
{
	if (Registry != nullptr)
	{
		if (const UGameSetting* const Setting = Registry->FindSettingByDevName(CategoryId))
		{
			return Setting->GetDisplayName();
		}
	}

	if (CategoryId == UAGASSGameSettingRegistry::VideoCollection)
	{
		return NSLOCTEXT("AGASSFrontend", "OptionsVideoTab", "Video");
	}
	if (CategoryId == UAGASSGameSettingRegistry::AudioCollection)
	{
		return NSLOCTEXT("AGASSFrontend", "OptionsAudioTab", "Audio");
	}
	if (CategoryId == UAGASSGameSettingRegistry::GameplayCollection)
	{
		return NSLOCTEXT("AGASSFrontend", "OptionsGameplayTab", "Gameplay");
	}
	if (CategoryId == UAGASSGameSettingRegistry::ControlsCollection)
	{
		return NSLOCTEXT("AGASSFrontend", "OptionsControlsTab", "Controls");
	}

	return NSLOCTEXT("AGASSFrontend", "OptionsInterfaceTab", "Interface");
}

void UAGASSOptionsScreenWidget::HandleRegistrySettingChanged(UGameSetting* Setting, EGameSettingChangeReason Reason)
{
	if (Reason != EGameSettingChangeReason::DependencyChanged)
	{
		if (UGameSettingValue* const SettingValue = Cast<UGameSettingValue>(Setting))
		{
			SettingValue->Apply();
			SettingValue->StoreInitial();
		}
	}

	RefreshSummaryText();
}

TArray<FName> UAGASSOptionsScreenWidget::GetTopLevelCategoryOrder() const
{
	return {
		UAGASSGameSettingRegistry::VideoCollection,
		UAGASSGameSettingRegistry::AudioCollection,
		UAGASSGameSettingRegistry::GameplayCollection,
		UAGASSGameSettingRegistry::ControlsCollection,
		UAGASSGameSettingRegistry::InterfaceCollection
	};
}

int32 UAGASSOptionsScreenWidget::GetCategoryIndex(const FName& CategoryId) const
{
	return GetTopLevelCategoryOrder().IndexOfByKey(CategoryId);
}

bool UAGASSOptionsScreenWidget::IsGamepadInputActive() const
{
	if (const UCommonInputSubsystem* const InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		return InputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad;
	}

	return false;
}

void UAGASSOptionsScreenWidget::FocusSettingsPanelIfPossible() const
{
	if (Settings_Panel != nullptr && IsGamepadInputActive())
	{
		Settings_Panel->SetFocus();
	}
}

void UAGASSOptionsScreenWidget::ShowAdjacentCategory(int32 Direction)
{
	const TArray<FName> CategoryOrder = GetTopLevelCategoryOrder();
	const int32 CurrentIndex = FMath::Max(0, GetCategoryIndex(ActiveCategoryId));
	const int32 NextIndex = FMath::Clamp(CurrentIndex + Direction, 0, CategoryOrder.Num() - 1);
	if (CategoryOrder.IsValidIndex(NextIndex) && NextIndex != CurrentIndex)
	{
		ShowCategory(CategoryOrder[NextIndex], true);
	}
}

void UAGASSOptionsScreenWidget::HandleNextTabAction()
{
	ShowAdjacentCategory(+1);
}

void UAGASSOptionsScreenWidget::HandlePreviousTabAction()
{
	ShowAdjacentCategory(-1);
}

void UAGASSOptionsScreenWidget::HandleVideoTabClicked()
{
	ShowCategory(UAGASSGameSettingRegistry::VideoCollection, false);
}

void UAGASSOptionsScreenWidget::HandleAudioTabClicked()
{
	ShowCategory(UAGASSGameSettingRegistry::AudioCollection, false);
}

void UAGASSOptionsScreenWidget::HandleGameplayTabClicked()
{
	ShowCategory(UAGASSGameSettingRegistry::GameplayCollection, false);
}

void UAGASSOptionsScreenWidget::HandleControlsTabClicked()
{
	ShowCategory(UAGASSGameSettingRegistry::ControlsCollection, false);
}

void UAGASSOptionsScreenWidget::HandleInterfaceTabClicked()
{
	ShowCategory(UAGASSGameSettingRegistry::InterfaceCollection, false);
}
