#include "UI/AGASSSessionBrowserEntryWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CommonTextBlock.h"
#include "Engine/GameInstance.h"
#include "Misc/PackageName.h"
#include "UI/AGASSFrontendButtonBase.h"

namespace
{
	FText BuildCompactCsvSummary(const FString& Csv, const int32 FallbackCount, const FText& EmptyText)
	{
		TArray<FString> ParsedValues;
		Csv.ParseIntoArray(ParsedValues, TEXT(","), true);

		TArray<FString> FilteredValues;
		for (const FString& ParsedValue : ParsedValues)
		{
			const FString TrimmedValue = ParsedValue.TrimStartAndEnd();
			if (!TrimmedValue.IsEmpty())
			{
				FilteredValues.Add(TrimmedValue);
			}
		}

		if (FilteredValues.IsEmpty())
		{
			return FallbackCount > 0
				? FText::Format(NSLOCTEXT("AGASSFrontend", "SessionBrowserActiveModCountOnly", "{0} active"), FText::AsNumber(FallbackCount))
				: EmptyText;
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
			NSLOCTEXT("AGASSFrontend", "SessionBrowserCompactModsSummary", "{0}, {1}, +{2}"),
			FText::FromString(FilteredValues[0]),
			FText::FromString(FilteredValues[1]),
			FText::AsNumber(FilteredValues.Num() - 2));
	}
}

void UAGASSSessionBrowserEntryWidget::InitializeFromResult(const int32 InSearchResultIndex, const FAGASSSessionSearchResultData& InSessionData)
{
	SearchResultIndex = InSearchResultIndex;
	SessionData = InSessionData;
	RefreshView();
}

UWidget* UAGASSSessionBrowserEntryWidget::GetPrimaryFocusTarget() const
{
	return Button_Join;
}

TSharedRef<SWidget> UAGASSSessionBrowserEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		WidgetTree->RootWidget = nullptr;

		UBorder* EntryBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EntryBorder"));
		EntryBorder->SetBrushColor(FLinearColor(0.11f, 0.14f, 0.17f, 0.98f));
		EntryBorder->SetPadding(FMargin(18.f));
		WidgetTree->RootWidget = EntryBorder;

		UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EntryLayout"));
		EntryBorder->SetContent(Layout);

		Text_Details = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Details"));
		Text_Details->SetAutoWrapText(true);
		if (UVerticalBoxSlot* DetailsSlot = Layout->AddChildToVerticalBox(Text_Details))
		{
			DetailsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		}

		Text_Reason = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Reason"));
		Text_Reason->SetAutoWrapText(true);
		if (UVerticalBoxSlot* ReasonSlot = Layout->AddChildToVerticalBox(Text_Reason))
		{
			ReasonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		}

		Button_Join = WidgetTree->ConstructWidget<UAGASSFrontendButtonBase>(UAGASSFrontendButtonBase::StaticClass(), TEXT("Button_Join"));
		Button_Join->SetButtonText(NSLOCTEXT("AGASSFrontend", "JoinSessionEntryButton", "Join Session"));
		Layout->AddChildToVerticalBox(Button_Join);
		RefreshView();
	}

	return Super::RebuildWidget();
}

void UAGASSSessionBrowserEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Text_Details == nullptr)
	{
		Text_Details = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_Details")));
	}

	if (Text_Reason == nullptr)
	{
		Text_Reason = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_Reason")));
	}

	if (Button_Join == nullptr)
	{
		Button_Join = Cast<UAGASSFrontendButtonBase>(GetWidgetFromName(TEXT("Button_Join")));
	}

	if (Button_Join)
	{
		Button_Join->OnClicked().RemoveAll(this);
		Button_Join->OnClicked().AddUObject(this, &ThisClass::HandleJoinClicked);
	}

	RefreshView();
}

void UAGASSSessionBrowserEntryWidget::RefreshView()
{
	if (Text_Details)
	{
		const FString MapDisplayName = !SessionData.MapDisplayName.IsEmpty()
			? SessionData.MapDisplayName
			: (SessionData.MapPath.IsEmpty()
				? NSLOCTEXT("AGASSFrontend", "UnknownSessionMap", "Unknown Map").ToString()
				: FPackageName::GetShortName(SessionData.MapPath));
		const FText ActiveModsText = BuildCompactCsvSummary(
			SessionData.ActiveModIdsCsv,
			SessionData.ActiveModCount,
			NSLOCTEXT("AGASSFrontend", "SessionBrowserNoActiveMods", "None"));

		Text_Details->SetText(FText::Format(
			NSLOCTEXT(
				"AGASSFrontend",
				"SessionBrowserEntryDetails",
				"{0}\n{1}/{2} players | {3}\nInvite: {4} | {5}\nMods: {6}"),
			FText::FromString(SessionData.HostName.IsEmpty()
				? NSLOCTEXT("AGASSFrontend", "UnknownSessionHost", "Unknown Host").ToString()
				: SessionData.HostName),
			FText::AsNumber(SessionData.CurrentPlayers),
			FText::AsNumber(SessionData.MaxPlayers),
			FText::FromString(MapDisplayName),
			FText::FromString(SessionData.InviteCode.IsEmpty()
				? NSLOCTEXT("AGASSFrontend", "SessionBrowserNoInviteCode", "N/A").ToString()
				: SessionData.InviteCode),
			GetTransportText(),
			ActiveModsText));
	}

	if (Text_Reason)
	{
		Text_Reason->SetText(FText::FromString(SessionData.JoinDisabledReason));
		Text_Reason->SetVisibility(SessionData.JoinDisabledReason.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (Button_Join)
	{
		Button_Join->SetIsEnabled(SessionData.bIsJoinable);
		Button_Join->SetButtonText(SessionData.bIsJoinable
			? NSLOCTEXT("AGASSFrontend", "JoinSessionEntryButton", "Join Session")
			: NSLOCTEXT("AGASSFrontend", "JoinSessionEntryDisabledButton", "Join Unavailable"));
	}
}

void UAGASSSessionBrowserEntryWidget::HandleJoinClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UAGASSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UAGASSSessionSubsystem>())
		{
			SessionSubsystem->JoinSearchResult(SearchResultIndex);
		}
	}
}

FText UAGASSSessionBrowserEntryWidget::GetTransportText() const
{
	return SessionData.bIsLANMatch
		? NSLOCTEXT("AGASSFrontend", "SessionTransportLan", "LAN / Null")
		: NSLOCTEXT("AGASSFrontend", "SessionTransportOnline", "Advertised Online");
}
