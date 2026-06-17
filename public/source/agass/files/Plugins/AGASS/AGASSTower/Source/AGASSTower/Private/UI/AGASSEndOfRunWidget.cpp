#include "UI/AGASSEndOfRunWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/AGASSRunStateComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAGASSEndOfRunWidget, Log, All);

namespace
{
	FSlateFontInfo MakeFont(const int32 FontSize, const TCHAR* Weight = TEXT("Regular"))
	{
		return FCoreStyle::GetDefaultFontStyle(Weight, FontSize);
	}

	FText FormatRunDuration(const float RunDurationSeconds)
	{
		const int32 TotalSeconds = FMath::Max(FMath::RoundToInt(RunDurationSeconds), 0);
		const int32 Minutes = TotalSeconds / 60;
		const int32 Seconds = TotalSeconds % 60;
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), Minutes, Seconds));
	}
}

UAGASSEndOfRunWidget::UAGASSEndOfRunWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
	SetIsFocusable(false);
}

TSharedRef<SWidget> UAGASSEndOfRunWidget::RebuildWidget()
{
	WidgetTree->RootWidget = nullptr;

	UCanvasPanel* const RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
	BackgroundBorder->SetBrushColor(FLinearColor(0.05f, 0.07f, 0.09f, 0.92f));
	BackgroundBorder->SetPadding(FMargin(22.f));
	RootCanvas->AddChild(BackgroundBorder);
	if (UCanvasPanelSlot* const BorderSlot = Cast<UCanvasPanelSlot>(BackgroundBorder->Slot))
	{
		BorderSlot->SetAutoSize(true);
		BorderSlot->SetAnchors(FAnchors(0.5f, 0.08f));
		BorderSlot->SetAlignment(FVector2D(0.5f, 0.f));
	}

	UVerticalBox* const Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Layout"));
	BackgroundBorder->SetContent(Layout);

	TitleTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleTextBlock"));
	TitleTextBlock->SetFont(MakeFont(28, TEXT("Bold")));
	TitleTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.97f, 0.98f, 1.f)));
	if (UVerticalBoxSlot* const TitleSlot = Layout->AddChildToVerticalBox(TitleTextBlock))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	BodyTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BodyTextBlock"));
	BodyTextBlock->SetFont(MakeFont(16));
	BodyTextBlock->SetAutoWrapText(true);
	BodyTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.92f, 0.97f)));
	if (UVerticalBoxSlot* const BodySlot = Layout->AddChildToVerticalBox(BodyTextBlock))
	{
		BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	StatsTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatsTextBlock"));
	StatsTextBlock->SetFont(MakeFont(15));
	StatsTextBlock->SetAutoWrapText(true);
	StatsTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.88f, 0.72f)));
	if (UVerticalBoxSlot* const StatsSlot = Layout->AddChildToVerticalBox(StatsTextBlock))
	{
		StatsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	CountdownTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountdownTextBlock"));
	CountdownTextBlock->SetFont(MakeFont(15, TEXT("Bold")));
	CountdownTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.84f, 0.45f)));
	Layout->AddChildToVerticalBox(CountdownTextBlock);

	return Super::RebuildWidget();
}

void UAGASSEndOfRunWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(
		LogAGASSEndOfRunWidget,
		Display,
		TEXT("NativeConstruct widget=%s owningPlayer=%s world=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetOwningPlayer()),
		*GetNameSafe(GetWorld()));

	StartRetryTimer();
	RefreshRunStateBinding();
	UpdateDisplay();
}

void UAGASSEndOfRunWidget::NativeDestruct()
{
	StopRetryTimer();
	ClearRunStateBinding();

	Super::NativeDestruct();
}

void UAGASSEndOfRunWidget::RefreshRunStateBinding()
{
	UAGASSRunStateComponent* const RunStateComponent = ResolveRunStateComponent();
	if (RunStateComponent == BoundRunStateComponent)
	{
		UE_LOG(
			LogAGASSEndOfRunWidget,
			Verbose,
			TEXT("RefreshRunStateBinding kept existing binding. widget=%s runState=%s"),
			*GetNameSafe(this),
			*GetNameSafe(RunStateComponent));
		return;
	}

	ClearRunStateBinding();

	if (RunStateComponent == nullptr)
	{
		UE_LOG(
			LogAGASSEndOfRunWidget,
			Display,
			TEXT("RefreshRunStateBinding did not find run-state component yet. widget=%s world=%s gameState=%s"),
			*GetNameSafe(this),
			*GetNameSafe(GetWorld()),
			*GetNameSafe(GetWorld() != nullptr ? GetWorld()->GetGameState() : nullptr));
		SetVisibility(ESlateVisibility::Collapsed);
		StartRetryTimer();
		return;
	}

	BoundRunStateComponent = RunStateComponent;
	UE_LOG(
		LogAGASSEndOfRunWidget,
		Display,
		TEXT("RefreshRunStateBinding bound widget=%s to runState=%s phase=%d hasSummary=%d"),
		*GetNameSafe(this),
		*GetNameSafe(BoundRunStateComponent),
		static_cast<int32>(BoundRunStateComponent->GetRunPhase()),
		BoundRunStateComponent->GetRunSummary().bHasRunSummary ? 1 : 0);
	BoundRunStateComponent->OnRunStateChanged().AddUObject(this, &ThisClass::HandleRunStateChanged);
	BoundRunStateComponent->OnRunSummaryChanged().AddUObject(this, &ThisClass::HandleRunSummaryChanged);
	UpdateDisplay();
}

void UAGASSEndOfRunWidget::ClearRunStateBinding()
{
	if (BoundRunStateComponent == nullptr)
	{
		return;
	}

	BoundRunStateComponent->OnRunStateChanged().RemoveAll(this);
	BoundRunStateComponent->OnRunSummaryChanged().RemoveAll(this);
	BoundRunStateComponent = nullptr;
}

void UAGASSEndOfRunWidget::StartRetryTimer()
{
	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetTimerManager().IsTimerActive(RetryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(RetryTimerHandle, this, &ThisClass::HandleRetryTimerElapsed, 0.25f, true);
}

void UAGASSEndOfRunWidget::StopRetryTimer()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTimerHandle);
	}
}

void UAGASSEndOfRunWidget::HandleRetryTimerElapsed()
{
	RefreshRunStateBinding();

	if (BoundRunStateComponent != nullptr
		&& (BoundRunStateComponent->GetRunPhase() != EAGASSRunPhase::Active || BoundRunStateComponent->GetRunSummary().bHasRunSummary))
	{
		UE_LOG(
			LogAGASSEndOfRunWidget,
			Display,
			TEXT("HandleRetryTimerElapsed forcing display refresh widget=%s phase=%d hasSummary=%d"),
			*GetNameSafe(this),
			static_cast<int32>(BoundRunStateComponent->GetRunPhase()),
			BoundRunStateComponent->GetRunSummary().bHasRunSummary ? 1 : 0);
		UpdateDisplay();
	}
}

void UAGASSEndOfRunWidget::UpdateDisplay()
{
	if (BoundRunStateComponent == nullptr)
	{
		UE_LOG(LogAGASSEndOfRunWidget, Verbose, TEXT("UpdateDisplay collapsed because no run-state is bound. widget=%s"), *GetNameSafe(this));
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FAGASSRunSummaryData& RunSummary = BoundRunStateComponent->GetRunSummary();
	if (!RunSummary.bHasRunSummary || BoundRunStateComponent->GetRunPhase() == EAGASSRunPhase::Active)
	{
		UE_LOG(
			LogAGASSEndOfRunWidget,
			Display,
			TEXT("UpdateDisplay collapsed widget=%s phase=%d hasSummary=%d"),
			*GetNameSafe(this),
			static_cast<int32>(BoundRunStateComponent->GetRunPhase()),
			RunSummary.bHasRunSummary ? 1 : 0);
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (TitleTextBlock != nullptr)
	{
		TitleTextBlock->SetText(
			!RunSummary.CompletionTitle.IsEmpty()
				? RunSummary.CompletionTitle
				: NSLOCTEXT("AGASSTower", "RunCompleteTitleFallback", "Objective Complete"));
	}

	if (BodyTextBlock != nullptr)
	{
		BodyTextBlock->SetText(
			!RunSummary.CompletionBody.IsEmpty()
				? RunSummary.CompletionBody
				: NSLOCTEXT("AGASSTower", "RunCompleteBodyFallback", "The tower objective was reached. The session will return to FrontendMap shortly."));
	}

	if (StatsTextBlock != nullptr)
	{
		StatsTextBlock->SetText(FText::Format(
			NSLOCTEXT("AGASSTower", "RunStatsFormat", "Objective: {0}\nTeam Money: {1}\nPlayers Present: {2}\nPlaceables In World: {3}\nRun Time: {4}"),
			RunSummary.ObjectiveName.IsEmpty() ? NSLOCTEXT("AGASSTower", "ObjectiveUnknown", "Unknown Objective") : RunSummary.ObjectiveName,
			FText::AsNumber(RunSummary.FinalTeamMoney),
			FText::AsNumber(RunSummary.ConnectedPlayerCount),
			FText::AsNumber(RunSummary.RemainingPlaceableCount),
			FormatRunDuration(RunSummary.RunDurationSeconds)));
	}

	if (CountdownTextBlock != nullptr)
	{
		CountdownTextBlock->SetText(FText::Format(
			NSLOCTEXT("AGASSTower", "ReturnCountdownFormat", "Returning to FrontendMap in {0} second(s)."),
			FText::AsNumber(FMath::CeilToInt(BoundRunStateComponent->GetRemainingReturnToFrontendSeconds()))));
	}

	UE_LOG(
		LogAGASSEndOfRunWidget,
		Display,
		TEXT("UpdateDisplay showing widget=%s title=\"%s\" remaining=%.2f"),
		*GetNameSafe(this),
		*RunSummary.CompletionTitle.ToString(),
		BoundRunStateComponent->GetRemainingReturnToFrontendSeconds());
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

UAGASSRunStateComponent* UAGASSEndOfRunWidget::ResolveRunStateComponent() const
{
	const UWorld* const World = GetWorld();
	const AGameStateBase* const GameState = World != nullptr ? World->GetGameState() : nullptr;
	UE_LOG(
		LogAGASSEndOfRunWidget,
		Verbose,
		TEXT("ResolveRunStateComponent widget=%s world=%s gameState=%s"),
		*GetNameSafe(this),
		*GetNameSafe(World),
		*GetNameSafe(GameState));
	return GameState != nullptr ? GameState->FindComponentByClass<UAGASSRunStateComponent>() : nullptr;
}

void UAGASSEndOfRunWidget::HandleRunStateChanged()
{
	UE_LOG(LogAGASSEndOfRunWidget, Display, TEXT("HandleRunStateChanged widget=%s"), *GetNameSafe(this));
	UpdateDisplay();
}

void UAGASSEndOfRunWidget::HandleRunSummaryChanged()
{
	UE_LOG(LogAGASSEndOfRunWidget, Display, TEXT("HandleRunSummaryChanged widget=%s"), *GetNameSafe(this));
	UpdateDisplay();
}
