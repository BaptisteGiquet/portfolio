#include "UI/AGASSModToggleEntryWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Subsystems/AGASSModsSubsystem.h"
#include "UI/AGASSFrontendButtonBase.h"

namespace
{
	FText BuildProvidedMapsLabel(const UAGASSModsSubsystem* ModsSubsystem, const FAGASSDiscoveredModInfo& ModInfo)
	{
		if (ModsSubsystem == nullptr || ModInfo.ModId.IsEmpty())
		{
			return NSLOCTEXT("AGASSFrontend", "ModEntryNoProvidedMapsFallback", "None");
		}

		TArray<FString> ProvidedMapLabels;
		for (const FAGASSAvailableMapInfo& MapInfo : ModsSubsystem->GetAvailableMaps())
		{
			if (MapInfo.OwningModId.Equals(ModInfo.ModId, ESearchCase::IgnoreCase))
			{
				ProvidedMapLabels.Add(MapInfo.GetDisplayLabel());
			}
		}

		if (ProvidedMapLabels.IsEmpty())
		{
			return NSLOCTEXT("AGASSFrontend", "ModEntryNoProvidedMaps", "None");
		}

		if (ProvidedMapLabels.Num() == 1)
		{
			return FText::FromString(ProvidedMapLabels[0]);
		}

		if (ProvidedMapLabels.Num() == 2)
		{
			return FText::FromString(FString::Printf(TEXT("%s, %s"), *ProvidedMapLabels[0], *ProvidedMapLabels[1]));
		}

		return FText::Format(
			NSLOCTEXT("AGASSFrontend", "ModEntryProvidedMapsCompact", "{0}, {1}, +{2}"),
			FText::FromString(ProvidedMapLabels[0]),
			FText::FromString(ProvidedMapLabels[1]),
			FText::AsNumber(ProvidedMapLabels.Num() - 2));
	}
}

void UAGASSModToggleEntryWidget::InitializeFromModInfo(const FAGASSDiscoveredModInfo& InModInfo)
{
	ModInfo = InModInfo;
	RefreshView();
}

UWidget* UAGASSModToggleEntryWidget::GetPrimaryFocusTarget() const
{
	return Button_Toggle;
}

TSharedRef<SWidget> UAGASSModToggleEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		WidgetTree->RootWidget = nullptr;

		UBorder* EntryBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModEntryBorder"));
		EntryBorder->SetBrushColor(FLinearColor(0.11f, 0.10f, 0.14f, 0.98f));
		EntryBorder->SetPadding(FMargin(16.f));
		WidgetTree->RootWidget = EntryBorder;

		UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModEntryLayout"));
		EntryBorder->SetContent(Layout);

		Text_Details = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Details"));
		Text_Details->SetAutoWrapText(true);
		if (UVerticalBoxSlot* DetailsSlot = Layout->AddChildToVerticalBox(Text_Details))
		{
			DetailsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		}

		Button_Toggle = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Toggle"));
		Layout->AddChildToVerticalBox(Button_Toggle);
		RefreshView();
	}

	return Super::RebuildWidget();
}

void UAGASSModToggleEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Text_Details == nullptr)
	{
		Text_Details = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_Details")));
	}

	if (Button_Toggle == nullptr)
	{
		Button_Toggle = Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Toggle")));
	}

	if (Button_Toggle != nullptr)
	{
		Button_Toggle->OnClicked().RemoveAll(this);
		Button_Toggle->OnClicked().AddUObject(this, &ThisClass::HandleToggleClicked);
	}

	RefreshView();
}

void UAGASSModToggleEntryWidget::RefreshView()
{
	const UAGASSModsSubsystem* ModsSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAGASSModsSubsystem>() : nullptr;
	const FAGASSAvailableMapInfo* SelectedMapInfo = ModsSubsystem ? ModsSubsystem->GetSelectedMapInfo() : nullptr;
	const bool bRequiredBySelectedMap =
		SelectedMapInfo != nullptr
		&& !SelectedMapInfo->OwningModId.IsEmpty()
		&& SelectedMapInfo->OwningModId.Equals(ModInfo.ModId, ESearchCase::IgnoreCase);
	const FText ProvidedMapsLabel = BuildProvidedMapsLabel(ModsSubsystem, ModInfo);

	if (Text_Details != nullptr)
	{
		if (!ModInfo.ValidationError.IsEmpty())
		{
			Text_Details->SetText(FText::Format(
				NSLOCTEXT("AGASSFrontend", "ModEntryDetailsInvalid", "{0}\nPlugin: {1} | Version: {2}\nMaps: {3}\nIssue: {4}"),
				FText::FromString(ModInfo.GetDisplayLabel()),
				FText::FromString(ModInfo.PluginName),
				FText::FromString(ModInfo.GetEffectiveCompatibilityVersion()),
				ProvidedMapsLabel,
				FText::FromString(ModInfo.ValidationError)));
		}
		else if (bRequiredBySelectedMap && SelectedMapInfo != nullptr)
		{
			Text_Details->SetText(FText::Format(
				NSLOCTEXT("AGASSFrontend", "ModEntryDetailsRequired", "{0}\nPlugin: {1} | Version: {2}\nMaps: {3}\nRequired by map: {4}"),
				FText::FromString(ModInfo.GetDisplayLabel()),
				FText::FromString(ModInfo.PluginName),
				FText::FromString(ModInfo.GetEffectiveCompatibilityVersion()),
				ProvidedMapsLabel,
				FText::FromString(SelectedMapInfo->GetDisplayLabel())));
		}
		else
		{
			Text_Details->SetText(FText::Format(
				NSLOCTEXT("AGASSFrontend", "ModEntryDetailsNormal", "{0}\nPlugin: {1} | Version: {2}\nMaps: {3}\nStatus: {4}"),
				FText::FromString(ModInfo.GetDisplayLabel()),
				FText::FromString(ModInfo.PluginName),
				FText::FromString(ModInfo.GetEffectiveCompatibilityVersion()),
				ProvidedMapsLabel,
				ModInfo.bIsActive
					? NSLOCTEXT("AGASSFrontend", "ModEntryStatusActive", "Active")
					: NSLOCTEXT("AGASSFrontend", "ModEntryStatusInactive", "Inactive")));
		}
	}

	if (Button_Toggle != nullptr)
	{
		Button_Toggle->SetButtonText(!ModInfo.bIsValid
			? NSLOCTEXT("AGASSFrontend", "InvalidModButton", "Unavailable")
			: (bRequiredBySelectedMap
				? NSLOCTEXT("AGASSFrontend", "RequiredModButton", "Required")
				: (ModInfo.bIsActive
					? NSLOCTEXT("AGASSFrontend", "DisableModButton", "Disable Mod")
					: NSLOCTEXT("AGASSFrontend", "EnableModButton", "Enable Mod"))));
		Button_Toggle->SetIsEnabled(ModInfo.bIsValid && !bRequiredBySelectedMap);
	}
}

void UAGASSModToggleEntryWidget::HandleToggleClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UAGASSModsSubsystem* ModsSubsystem = GameInstance->GetSubsystem<UAGASSModsSubsystem>())
		{
			FString FailureMessage;
			ModsSubsystem->SetModActive(ModInfo.ModId, !ModInfo.bIsActive, FailureMessage);
		}
	}
}
