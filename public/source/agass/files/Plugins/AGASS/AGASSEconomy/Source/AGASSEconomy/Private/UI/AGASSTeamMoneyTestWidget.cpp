#include "UI/AGASSTeamMoneyTestWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/AGASSTeamWalletComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

namespace
{
	FSlateFontInfo MakeFont(const int32 FontSize)
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FontSize);
	}
}

UAGASSTeamMoneyTestWidget::UAGASSTeamMoneyTestWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

TSharedRef<SWidget> UAGASSTeamMoneyTestWidget::RebuildWidget()
{
	WidgetTree->RootWidget = nullptr;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	BackgroundBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundBorder"));
	BackgroundBorder->SetBrushColor(FLinearColor(0.04f, 0.08f, 0.04f, 0.82f));
	BackgroundBorder->SetPadding(FMargin(12.f, 8.f));
	RootCanvas->AddChild(BackgroundBorder);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BackgroundBorder->Slot))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetPosition(FVector2D(20.f, 20.f));
	}

	BalanceTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BalanceTextBlock"));
	BalanceTextBlock->SetFont(MakeFont(20));
	BalanceTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 1.00f, 0.76f)));
	BalanceTextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
	BalanceTextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	BackgroundBorder->SetContent(BalanceTextBlock);

	UpdateBalanceDisplay(0);

	return Super::RebuildWidget();
}

void UAGASSTeamMoneyTestWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshWalletBinding();
}

void UAGASSTeamMoneyTestWidget::NativeDestruct()
{
	StopWalletRetryTimer();
	ClearWalletBinding();

	Super::NativeDestruct();
}

void UAGASSTeamMoneyTestWidget::RefreshWalletBinding()
{
	UAGASSTeamWalletComponent* const WalletComponent = ResolveWalletComponent();
	if (WalletComponent == BoundWalletComponent)
	{
		if (WalletComponent != nullptr)
		{
			UpdateBalanceDisplay(WalletComponent->GetCurrentBalance());
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			StopWalletRetryTimer();
			return;
		}

		SetVisibility(ESlateVisibility::Collapsed);
		StartWalletRetryTimer();
		return;
	}

	ClearWalletBinding();

	if (WalletComponent == nullptr)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		StartWalletRetryTimer();
		return;
	}

	BoundWalletComponent = WalletComponent;
	BoundWalletComponent->OnBalanceChanged.AddDynamic(this, &ThisClass::HandleWalletBalanceChanged);

	UpdateBalanceDisplay(BoundWalletComponent->GetCurrentBalance());
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	StopWalletRetryTimer();
}

void UAGASSTeamMoneyTestWidget::ClearWalletBinding()
{
	if (BoundWalletComponent != nullptr)
	{
		BoundWalletComponent->OnBalanceChanged.RemoveDynamic(this, &ThisClass::HandleWalletBalanceChanged);
		BoundWalletComponent = nullptr;
	}
}

void UAGASSTeamMoneyTestWidget::StartWalletRetryTimer()
{
	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetTimerManager().IsTimerActive(WalletRetryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(WalletRetryTimerHandle, this, &ThisClass::RefreshWalletBinding, 0.25f, true);
}

void UAGASSTeamMoneyTestWidget::StopWalletRetryTimer()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WalletRetryTimerHandle);
	}
}

void UAGASSTeamMoneyTestWidget::UpdateBalanceDisplay(const int32 CurrentBalance)
{
	if (BalanceTextBlock == nullptr)
	{
		return;
	}

	BalanceTextBlock->SetText(FText::Format(
		NSLOCTEXT("AGASSEconomy", "TeamMoneyTestFormat", "Team Money: {0}"),
		FText::AsNumber(CurrentBalance)));
}

UAGASSTeamWalletComponent* UAGASSTeamMoneyTestWidget::ResolveWalletComponent() const
{
	const UWorld* const World = GetWorld();
	const AGameStateBase* const GameState = World != nullptr ? World->GetGameState() : nullptr;
	return GameState != nullptr ? GameState->FindComponentByClass<UAGASSTeamWalletComponent>() : nullptr;
}

void UAGASSTeamMoneyTestWidget::HandleWalletBalanceChanged(const int32 PreviousBalance, const int32 NewBalance)
{
	UpdateBalanceDisplay(NewBalance);
}
