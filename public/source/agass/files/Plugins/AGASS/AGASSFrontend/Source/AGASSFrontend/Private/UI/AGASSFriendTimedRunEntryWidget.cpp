#include "UI/AGASSFriendTimedRunEntryWidget.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"

namespace
{
	FText FormatTimedRunMilliseconds(const int32 Milliseconds)
	{
		const int32 ClampedMilliseconds = FMath::Max(Milliseconds, 0);
		const int32 Minutes = ClampedMilliseconds / 60000;
		const int32 Seconds = (ClampedMilliseconds / 1000) % 60;
		const int32 RemainingMilliseconds = ClampedMilliseconds % 1000;
		return FText::FromString(FString::Printf(TEXT("%d:%02d.%03d"), Minutes, Seconds, RemainingMilliseconds));
	}
}

void UAGASSFriendTimedRunEntryWidget::InitializeEntry(const int32 InRank, const FString& InFriendName, const int32 InTimeMilliseconds)
{
	Rank = InRank;
	FriendName = InFriendName;
	TimeMilliseconds = InTimeMilliseconds;
	RefreshView();
}

TSharedRef<SWidget> UAGASSFriendTimedRunEntryWidget::RebuildWidget()
{
	if (Cast<UWidgetBlueprintGeneratedClass>(GetClass()) == nullptr)
	{
		WidgetTree->RootWidget = nullptr;

		UBorder* const RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
		RootBorder->SetBrushColor(FLinearColor(0.10f, 0.12f, 0.16f, 0.98f));
		RootBorder->SetPadding(FMargin(10.f, 8.f));
		WidgetTree->RootWidget = RootBorder;

		Text_Label = WidgetTree->ConstructWidget<UCommonTextBlock>(UCommonTextBlock::StaticClass(), TEXT("Text_Label"));
		Text_Label->SetAutoWrapText(true);
		RootBorder->SetContent(Text_Label);
		RefreshView();
	}

	return Super::RebuildWidget();
}

void UAGASSFriendTimedRunEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Text_Label == nullptr)
	{
		Text_Label = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("Text_Label")));
	}

	RefreshView();
}

void UAGASSFriendTimedRunEntryWidget::RefreshView()
{
	if (Text_Label == nullptr)
	{
		return;
	}

	Text_Label->SetText(FText::Format(
		NSLOCTEXT("AGASSFrontend", "FriendTimedRunEntryFormat", "#{0}  {1}  {2}"),
		FText::AsNumber(FMath::Max(Rank, 1)),
		FText::FromString(!FriendName.IsEmpty() ? FriendName : NSLOCTEXT("AGASSFrontend", "FriendTimedRunUnknownFriend", "Unknown Friend").ToString()),
		FormatTimedRunMilliseconds(TimeMilliseconds)));
}
