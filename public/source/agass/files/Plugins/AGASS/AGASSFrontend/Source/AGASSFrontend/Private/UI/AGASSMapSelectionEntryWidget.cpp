#include "UI/AGASSMapSelectionEntryWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Misc/PackageName.h"
#include "Subsystems/AGASSModsSubsystem.h"
#include "UI/AGASSFrontendButtonBase.h"

namespace
{
	FString ResolveOwningModLabel(const UAGASSModsSubsystem* ModsSubsystem, const FString& ModId)
	{
		if (ModsSubsystem != nullptr)
		{
			for (const FAGASSDiscoveredModInfo& ModInfo : ModsSubsystem->GetDiscoveredMods())
			{
				if (ModInfo.ModId.Equals(ModId, ESearchCase::IgnoreCase))
				{
					return ModInfo.GetDisplayLabel();
				}
			}
		}

		return ModId;
	}
}

void UAGASSMapSelectionEntryWidget::InitializeFromMapInfo(const FAGASSAvailableMapInfo& InMapInfo)
{
	MapInfo = InMapInfo;
	RefreshView();
}

UWidget* UAGASSMapSelectionEntryWidget::GetPrimaryFocusTarget() const
{
	return Button_Select;
}

TSharedRef<SWidget> UAGASSMapSelectionEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		WidgetTree->RootWidget = nullptr;

		UBorder* EntryBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MapEntryBorder"));
		EntryBorder->SetBrushColor(FLinearColor(0.10f, 0.12f, 0.16f, 0.98f));
		EntryBorder->SetPadding(FMargin(16.f));
		WidgetTree->RootWidget = EntryBorder;

		UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MapEntryLayout"));
		EntryBorder->SetContent(Layout);

		Text_Details = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Details"));
		Text_Details->SetAutoWrapText(true);
		if (UVerticalBoxSlot* DetailsSlot = Layout->AddChildToVerticalBox(Text_Details))
		{
			DetailsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		}

		Button_Select = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Select"));
		Layout->AddChildToVerticalBox(Button_Select);
		RefreshView();
	}

	return Super::RebuildWidget();
}

void UAGASSMapSelectionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Text_Details == nullptr)
	{
		Text_Details = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_Details")));
	}

	if (Button_Select == nullptr)
	{
		Button_Select = Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Select")));
	}

	if (Button_Select != nullptr)
	{
		Button_Select->OnClicked().RemoveAll(this);
		Button_Select->OnClicked().AddUObject(this, &ThisClass::HandleSelectClicked);
	}

	RefreshView();
}

void UAGASSMapSelectionEntryWidget::RefreshView()
{
	const UAGASSModsSubsystem* ModsSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSModsSubsystem>() : nullptr;
	const FString OwningModLabel = ResolveOwningModLabel(ModsSubsystem, MapInfo.OwningModId);
	const FString WorldLabel = MapInfo.WorldPackagePath.IsEmpty()
		? NSLOCTEXT("AGASSFrontend", "MapEntryUnknownWorld", "Unknown").ToString()
		: FPackageName::GetShortName(MapInfo.WorldPackagePath);

	if (Text_Details != nullptr)
	{
		Text_Details->SetText(MapInfo.bIsBaseGameMap
			? FText::Format(
				NSLOCTEXT("AGASSFrontend", "MapEntryDetailsBaseGame", "{0}\nId: {1} | World: {2}\nSource: Base Game"),
				FText::FromString(MapInfo.GetDisplayLabel()),
				FText::FromString(MapInfo.MapId),
				FText::FromString(WorldLabel))
			: FText::Format(
				NSLOCTEXT("AGASSFrontend", "MapEntryDetailsModded", "{0}\nId: {1} | World: {2}\nMod: {3}"),
				FText::FromString(MapInfo.GetDisplayLabel()),
				FText::FromString(MapInfo.MapId),
				FText::FromString(WorldLabel),
				FText::FromString(OwningModLabel)));
	}

	if (Button_Select != nullptr)
	{
		Button_Select->SetButtonText(MapInfo.bIsCurrentlySelected
			? (MapInfo.bIsBaseGameMap
				? NSLOCTEXT("AGASSFrontend", "MapAlreadySelectedBaseButton", "Selected")
				: NSLOCTEXT("AGASSFrontend", "MapAlreadySelectedModdedButton", "Selected Map"))
			: (MapInfo.bIsBaseGameMap
				? NSLOCTEXT("AGASSFrontend", "MapSelectBaseButton", "Use Base Map")
				: NSLOCTEXT("AGASSFrontend", "MapSelectModdedButton", "Use Map + Mod")));
		Button_Select->SetIsEnabled(!MapInfo.bIsCurrentlySelected);
	}
}

void UAGASSMapSelectionEntryWidget::HandleSelectClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UAGASSModsSubsystem* ModsSubsystem = GameInstance->GetSubsystem<UAGASSModsSubsystem>())
		{
			FString FailureMessage;
			ModsSubsystem->SetSelectedMapId(MapInfo.MapId, FailureMessage);
		}
	}
}
