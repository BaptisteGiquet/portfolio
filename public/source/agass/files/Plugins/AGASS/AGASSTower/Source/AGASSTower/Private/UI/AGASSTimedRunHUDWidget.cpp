#include "UI/AGASSTimedRunHUDWidget.h"

#include "Components/TextBlock.h"
#include "Components/AGASSRunStateComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Types/AGASSTimedRunTypes.h"

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

UAGASSTimedRunHUDWidget::UAGASSTimedRunHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
	SetIsFocusable(false);
}

EAGASSTimedRunState UAGASSTimedRunHUDWidget::GetDisplayedTimedRunState() const
{
	return CachedTimedRunState;
}

FText UAGASSTimedRunHUDWidget::GetDisplayedTimedRunLabelText() const
{
	return CachedTimedRunLabelText;
}

FText UAGASSTimedRunHUDWidget::GetDisplayedTimedRunValueText() const
{
	return CachedTimedRunValueText;
}

int32 UAGASSTimedRunHUDWidget::GetDisplayedTimedRunMilliseconds() const
{
	return CachedTimedRunMilliseconds;
}

void UAGASSTimedRunHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	StartRetryTimer();
	RefreshRunStateBinding();
	UpdateDisplay();
}

void UAGASSTimedRunHUDWidget::NativeDestruct()
{
	StopRetryTimer();
	ClearRunStateBinding();
	Super::NativeDestruct();
}

void UAGASSTimedRunHUDWidget::RefreshRunStateBinding()
{
	UAGASSRunStateComponent* const RunStateComponent = ResolveRunStateComponent();
	if (RunStateComponent == BoundRunStateComponent)
	{
		return;
	}

	ClearRunStateBinding();
	if (RunStateComponent == nullptr)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		StartRetryTimer();
		return;
	}

	BoundRunStateComponent = RunStateComponent;
	BoundRunStateComponent->OnRunStateChanged().AddUObject(this, &ThisClass::HandleRunStateChanged);
	BoundRunStateComponent->OnRunSummaryChanged().AddUObject(this, &ThisClass::HandleRunSummaryChanged);
	UpdateDisplay();
}

void UAGASSTimedRunHUDWidget::ClearRunStateBinding()
{
	if (BoundRunStateComponent == nullptr)
	{
		return;
	}

	BoundRunStateComponent->OnRunStateChanged().RemoveAll(this);
	BoundRunStateComponent->OnRunSummaryChanged().RemoveAll(this);
	BoundRunStateComponent = nullptr;
}

void UAGASSTimedRunHUDWidget::StartRetryTimer()
{
	UWorld* const World = GetWorld();
	if (World == nullptr || World->GetTimerManager().IsTimerActive(RetryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		RetryTimerHandle,
		this,
		&ThisClass::HandleRetryTimerElapsed,
		FMath::Max(DisplayRefreshIntervalSeconds, 0.01f),
		true);
}

void UAGASSTimedRunHUDWidget::StopRetryTimer()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTimerHandle);
	}
}

void UAGASSTimedRunHUDWidget::HandleRetryTimerElapsed()
{
	RefreshRunStateBinding();
	UpdateDisplay();
}

void UAGASSTimedRunHUDWidget::UpdateDisplay()
{
	if (BoundRunStateComponent == nullptr)
	{
		CachedTimedRunState = EAGASSTimedRunState::WaitingToStart;
		CachedTimedRunMilliseconds = 0;
		CachedTimedRunLabelText = NSLOCTEXT("AGASSTower", "TimedRunWaitingLabel", "Time");
		CachedTimedRunValueText = FormatTimedRunMilliseconds(0);
		if (Text_TimerLabel != nullptr)
		{
			Text_TimerLabel->SetText(CachedTimedRunLabelText);
		}
		if (Text_TimerValue != nullptr)
		{
			Text_TimerValue->SetText(CachedTimedRunValueText);
		}
		SetVisibility(ESlateVisibility::Collapsed);
		ReceiveTimedRunDisplayUpdated(CachedTimedRunState, CachedTimedRunLabelText, CachedTimedRunValueText, CachedTimedRunMilliseconds);
		return;
	}

	CachedTimedRunState = BoundRunStateComponent->GetTimedRunState();
	CachedTimedRunMilliseconds = BoundRunStateComponent->GetCurrentElapsedTimedRunMilliseconds();
	switch (CachedTimedRunState)
	{
	case EAGASSTimedRunState::WaitingToStart:
		CachedTimedRunLabelText = NSLOCTEXT("AGASSTower", "TimedRunWaitingLabel", "Time");
		CachedTimedRunValueText = FormatTimedRunMilliseconds(0);
		break;

	case EAGASSTimedRunState::Active:
		CachedTimedRunLabelText = NSLOCTEXT("AGASSTower", "TimedRunActiveLabel", "Time");
		CachedTimedRunValueText = FormatTimedRunMilliseconds(CachedTimedRunMilliseconds);
		break;

	case EAGASSTimedRunState::Completed:
	default:
		CachedTimedRunLabelText = NSLOCTEXT("AGASSTower", "TimedRunCompletedLabel", "Final Time");
		CachedTimedRunValueText = FormatTimedRunMilliseconds(CachedTimedRunMilliseconds);
		break;
	}

	if (Text_TimerLabel != nullptr)
	{
		Text_TimerLabel->SetText(CachedTimedRunLabelText);
	}
	if (Text_TimerValue != nullptr)
	{
		Text_TimerValue->SetText(CachedTimedRunValueText);
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ReceiveTimedRunDisplayUpdated(CachedTimedRunState, CachedTimedRunLabelText, CachedTimedRunValueText, CachedTimedRunMilliseconds);
}

UAGASSRunStateComponent* UAGASSTimedRunHUDWidget::ResolveRunStateComponent() const
{
	const UWorld* const World = GetWorld();
	const AGameStateBase* const GameState = World != nullptr ? World->GetGameState() : nullptr;
	return GameState != nullptr ? GameState->FindComponentByClass<UAGASSRunStateComponent>() : nullptr;
}

void UAGASSTimedRunHUDWidget::HandleRunStateChanged()
{
	UpdateDisplay();
}

void UAGASSTimedRunHUDWidget::HandleRunSummaryChanged()
{
	UpdateDisplay();
}
