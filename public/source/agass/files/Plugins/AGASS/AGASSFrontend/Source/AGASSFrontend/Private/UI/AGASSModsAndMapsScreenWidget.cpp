#include "UI/AGASSModsAndMapsScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Settings/AGASSFrontendDeveloperSettings.h"
#include "Subsystems/AGASSModsSubsystem.h"
#include "UI/AGASSFrontendButtonBase.h"
#include "UI/AGASSMapSelectionEntryWidget.h"
#include "UI/AGASSModToggleEntryWidget.h"

namespace
{
	FText BuildCompactListSummary(const TArray<FString>& Values, const FText& EmptyText)
	{
		TArray<FString> FilteredValues;
		for (const FString& Value : Values)
		{
			const FString TrimmedValue = Value.TrimStartAndEnd();
			if (!TrimmedValue.IsEmpty())
			{
				FilteredValues.Add(TrimmedValue);
			}
		}

		if (FilteredValues.IsEmpty())
		{
			return EmptyText;
		}

		if (FilteredValues.Num() == 1)
		{
			return FText::FromString(FilteredValues[0]);
		}

		if (FilteredValues.Num() == 2)
		{
			return FText::FromString(FString::Printf(TEXT("%s, %s"), *FilteredValues[0], *FilteredValues[1]));
		}

		return FText::Format(
			NSLOCTEXT("AGASSFrontend", "ModsAndMapsCompactList", "{0}, {1}, +{2}"),
			FText::FromString(FilteredValues[0]),
			FText::FromString(FilteredValues[1]),
			FText::AsNumber(FilteredValues.Num() - 2));
	}

	FString ResolveSelectedMapOwningModLabel(const UAGASSModsSubsystem* ModsSubsystem, const FAGASSAvailableMapInfo* SelectedMap)
	{
		if (ModsSubsystem == nullptr || SelectedMap == nullptr || SelectedMap->OwningModId.IsEmpty())
		{
			return FString();
		}

		for (const FAGASSDiscoveredModInfo& ModInfo : ModsSubsystem->GetDiscoveredMods())
		{
			if (ModInfo.ModId.Equals(SelectedMap->OwningModId, ESearchCase::IgnoreCase))
			{
				return ModInfo.GetDisplayLabel();
			}
		}

		return SelectedMap->OwningModId;
	}

	FText BuildSelectedMapDisplayText(const FAGASSAvailableMapInfo* SelectedMap)
	{
		return FText::FromString(SelectedMap != nullptr
			? SelectedMap->GetDisplayLabel()
			: NSLOCTEXT("AGASSFrontend", "ModsAndMapsSelectionUnconfigured", "Unconfigured").ToString());
	}

	FText BuildSelectedMapSourceText(const UAGASSModsSubsystem* ModsSubsystem, const FAGASSAvailableMapInfo* SelectedMap)
	{
		if (SelectedMap == nullptr)
		{
			return NSLOCTEXT("AGASSFrontend", "ModsAndMapsSourceUnknown", "Unknown");
		}

		if (SelectedMap->bIsBaseGameMap)
		{
			return NSLOCTEXT("AGASSFrontend", "ModsAndMapsSourceBaseGame", "Base Game");
		}

		const FString OwningModLabel = ResolveSelectedMapOwningModLabel(ModsSubsystem, SelectedMap);
		return OwningModLabel.IsEmpty()
			? NSLOCTEXT("AGASSFrontend", "ModsAndMapsSourceModded", "Modded")
			: FText::FromString(OwningModLabel);
	}

	FText BuildActiveModsSummaryText(const UAGASSModsSubsystem* ModsSubsystem)
	{
		return BuildCompactListSummary(
			ModsSubsystem != nullptr ? ModsSubsystem->GetActiveModIds() : TArray<FString>{},
			NSLOCTEXT("AGASSFrontend", "ModsAndMapsBaseGameOnly", "Base Game Only"));
	}
}

UAGASSModsAndMapsScreenWidget::UAGASSModsAndMapsScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bIsBackActionDisplayedInActionBar = true;
	OverrideBackActionDisplayName = NSLOCTEXT("AGASSFrontend", "FrontendBackActionName", "Back");
}

void UAGASSModsAndMapsScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ResolveWidgetByName(Button_RefreshRegistry, TEXT("Button_RefreshRegistry"));
	ResolveWidgetByName(Text_CurrentSelection, TEXT("Text_CurrentSelection"));
	ResolveWidgetByName(Text_CurrentMapValue, TEXT("Text_CurrentMapValue"));
	ResolveWidgetByName(Text_CurrentSourceValue, TEXT("Text_CurrentSourceValue"));
	ResolveWidgetByName(Text_CurrentModsValue, TEXT("Text_CurrentModsValue"));
	ResolveWidgetByName(Scroll_Maps, TEXT("Scroll_Maps"));
	ResolveWidgetByName(Scroll_Mods, TEXT("Scroll_Mods"));

	if (Button_RefreshRegistry != nullptr)
	{
		Button_RefreshRegistry->OnClicked().RemoveAll(this);
		Button_RefreshRegistry->OnClicked().AddUObject(this, &ThisClass::HandleRefreshClicked);
	}

	if (UAGASSModsSubsystem* ModsSubsystem = GetModsSubsystem())
	{
		ModsRegistryChangedHandle = ModsSubsystem->OnRegistryChanged().AddUObject(this, &ThisClass::HandleModsRegistryChanged);
		ModsSelectionChangedHandle = ModsSubsystem->OnSelectionChanged().AddUObject(this, &ThisClass::HandleModsSelectionChanged);
	}

	RebuildLists();
}

void UAGASSModsAndMapsScreenWidget::NativeDestruct()
{
	if (UAGASSModsSubsystem* ModsSubsystem = GetModsSubsystem())
	{
		if (ModsRegistryChangedHandle.IsValid())
		{
			ModsSubsystem->OnRegistryChanged().Remove(ModsRegistryChangedHandle);
			ModsRegistryChangedHandle.Reset();
		}

		if (ModsSelectionChangedHandle.IsValid())
		{
			ModsSubsystem->OnSelectionChanged().Remove(ModsSelectionChangedHandle);
			ModsSelectionChangedHandle.Reset();
		}
	}

	Super::NativeDestruct();
}

FText UAGASSModsAndMapsScreenWidget::GetTitleText() const
{
	return NSLOCTEXT("AGASSFrontend", "ModsAndMapsTitle", "Mods");
}

	void UAGASSModsAndMapsScreenWidget::BuildFallbackScreenContent(UVerticalBox* ContentBox)
	{
		AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "ModsAndMapsBody", "Pick the map you want to host first. Selecting a modded map automatically enables the mod that owns it. Use the mod list to review what each mod provides, or disable extra mods after switching back to a base-game map."), 16);

		AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "CurrentHostSetupHeading", "Current Host Setup"), 18);
		Text_CurrentMapValue = AddBodyText(ContentBox, FText::GetEmpty(), 15, TEXT("Text_CurrentMapValue"));
		Text_CurrentSourceValue = AddBodyText(ContentBox, FText::GetEmpty(), 15, TEXT("Text_CurrentSourceValue"));
		Text_CurrentModsValue = AddBodyText(ContentBox, FText::GetEmpty(), 15, TEXT("Text_CurrentModsValue"));
		Button_RefreshRegistry = AddMenuButton(ContentBox, NSLOCTEXT("AGASSFrontend", "RefreshModsRegistryButton", "Refresh Mod Registry"), TEXT("Button_RefreshRegistry"));

		AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "AvailableMapsHeading", "Available Maps"), 18);
	Scroll_Maps = AddScrollBox(ContentBox, 220.f, TEXT("Scroll_Maps"));

	AddBodyText(ContentBox, NSLOCTEXT("AGASSFrontend", "DiscoveredModsHeading", "Discovered Mods"), 18);
	Scroll_Mods = AddScrollBox(ContentBox, 220.f, TEXT("Scroll_Mods"));
}

void UAGASSModsAndMapsScreenWidget::RefreshFromSessionState()
{
	Super::RefreshFromSessionState();
	RebuildLists();
}

void UAGASSModsAndMapsScreenWidget::RebuildLists()
{
	UAGASSModsSubsystem* ModsSubsystem = GetModsSubsystem();
	if (ModsSubsystem == nullptr)
	{
		return;
	}

	if (Text_CurrentSelection)
	{
		Text_CurrentSelection->SetVisibility(ESlateVisibility::Collapsed);
	}

	const FAGASSAvailableMapInfo* SelectedMap = ModsSubsystem->GetSelectedMapInfo();
	const FText MapDisplayText = BuildSelectedMapDisplayText(SelectedMap);
	const FText SourceDisplayText = BuildSelectedMapSourceText(ModsSubsystem, SelectedMap);
	const FText ActiveModsDisplayText = BuildActiveModsSummaryText(ModsSubsystem);

	if (Text_CurrentMapValue != nullptr)
	{
		Text_CurrentMapValue->SetText(MapDisplayText);
	}

	if (Text_CurrentSourceValue != nullptr)
	{
		Text_CurrentSourceValue->SetText(SourceDisplayText);
	}

	if (Text_CurrentModsValue != nullptr)
	{
		Text_CurrentModsValue->SetText(ActiveModsDisplayText);
	}

	if (Text_CurrentSelection != nullptr
		&& Text_CurrentMapValue == nullptr
		&& Text_CurrentSourceValue == nullptr
		&& Text_CurrentModsValue == nullptr)
	{
		Text_CurrentSelection->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Text_CurrentSelection->SetText(FText::Format(
			NSLOCTEXT("AGASSFrontend", "CurrentHostSelectionSummaryFallback", "Map: {0}\nSource: {1}\nMods: {2}"),
			MapDisplayText,
			SourceDisplayText,
			ActiveModsDisplayText));
	}

	const UAGASSFrontendDeveloperSettings* FrontendSettings = UAGASSFrontendDeveloperSettings::Get();
	TSubclassOf<UAGASSMapSelectionEntryWidget> MapEntryWidgetClass = FrontendSettings ? FrontendSettings->MapSelectionEntryWidgetClass.LoadSynchronous() : nullptr;
	if (MapEntryWidgetClass == nullptr)
	{
		MapEntryWidgetClass = UAGASSMapSelectionEntryWidget::StaticClass();
	}

	TSubclassOf<UAGASSModToggleEntryWidget> ModEntryWidgetClass = FrontendSettings ? FrontendSettings->ModToggleEntryWidgetClass.LoadSynchronous() : nullptr;
	if (ModEntryWidgetClass == nullptr)
	{
		ModEntryWidgetClass = UAGASSModToggleEntryWidget::StaticClass();
	}

	if (Scroll_Maps)
	{
		Scroll_Maps->ClearChildren();
		for (const FAGASSAvailableMapInfo& MapInfo : ModsSubsystem->GetAvailableMaps())
		{
			UAGASSMapSelectionEntryWidget* EntryWidget = CreateWidget<UAGASSMapSelectionEntryWidget>(GetOwningPlayer(), MapEntryWidgetClass);
			if (EntryWidget == nullptr)
			{
				continue;
			}

			EntryWidget->InitializeFromMapInfo(MapInfo);
			Scroll_Maps->AddChild(EntryWidget);
		}
	}

	if (Scroll_Mods)
	{
		Scroll_Mods->ClearChildren();
		for (const FAGASSDiscoveredModInfo& ModInfo : ModsSubsystem->GetDiscoveredMods())
		{
			UAGASSModToggleEntryWidget* EntryWidget = CreateWidget<UAGASSModToggleEntryWidget>(GetOwningPlayer(), ModEntryWidgetClass);
			if (EntryWidget == nullptr)
			{
				continue;
			}

			EntryWidget->InitializeFromModInfo(ModInfo);
			Scroll_Mods->AddChild(EntryWidget);
		}
	}
}

UWidget* UAGASSModsAndMapsScreenWidget::ResolveInitialFocusTarget() const
{
	if (Scroll_Maps != nullptr)
	{
		if (const UAGASSMapSelectionEntryWidget* FirstMapEntry = Cast<UAGASSMapSelectionEntryWidget>(Scroll_Maps->GetChildAt(0)))
		{
			return FirstMapEntry->GetPrimaryFocusTarget();
		}
	}

	return Button_RefreshRegistry;
}

#if WITH_EDITOR
void UAGASSModsAndMapsScreenWidget::GetRequiredNamedWidgets(TArray<FName>& OutRequiredWidgets) const
{
	OutRequiredWidgets.Append({
		TEXT("Button_RefreshRegistry"),
		TEXT("Text_CurrentMapValue"),
		TEXT("Text_CurrentSourceValue"),
		TEXT("Text_CurrentModsValue"),
		TEXT("Scroll_Maps"),
		TEXT("Scroll_Mods")
	});
}
#endif

void UAGASSModsAndMapsScreenWidget::HandleModsRegistryChanged()
{
	RebuildLists();
}

void UAGASSModsAndMapsScreenWidget::HandleModsSelectionChanged()
{
	RebuildLists();
}

void UAGASSModsAndMapsScreenWidget::HandleRefreshClicked()
{
	if (UAGASSModsSubsystem* ModsSubsystem = GetModsSubsystem())
	{
		ModsSubsystem->RefreshRegistry();
	}
}
