#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "Types/AGASSTimedRunTypes.h"
#include "AGASSTimedRunHUDWidget.generated.h"

class UAGASSRunStateComponent;
class UTextBlock;

UCLASS()
class AGASSTOWER_API UAGASSTimedRunHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UAGASSTimedRunHUDWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintPure, Category = "AGASS|Timed Run")
	EAGASSTimedRunState GetDisplayedTimedRunState() const;

	UFUNCTION(BlueprintPure, Category = "AGASS|Timed Run")
	FText GetDisplayedTimedRunLabelText() const;

	UFUNCTION(BlueprintPure, Category = "AGASS|Timed Run")
	FText GetDisplayedTimedRunValueText() const;

	UFUNCTION(BlueprintPure, Category = "AGASS|Timed Run")
	int32 GetDisplayedTimedRunMilliseconds() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "AGASS|Timed Run", meta = (DisplayName = "On Timed Run Display Updated"))
	void ReceiveTimedRunDisplayUpdated(EAGASSTimedRunState TimedRunState, const FText& LabelText, const FText& ValueText, int32 Milliseconds);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AGASS|Timed Run", meta = (ClampMin = "0.01"))
	float DisplayRefreshIntervalSeconds = 0.25f;

private:
	void RefreshRunStateBinding();
	void ClearRunStateBinding();
	void StartRetryTimer();
	void StopRetryTimer();
	void HandleRetryTimerElapsed();
	void UpdateDisplay();
	UAGASSRunStateComponent* ResolveRunStateComponent() const;

	void HandleRunStateChanged();
	void HandleRunSummaryChanged();

	UPROPERTY(BlueprintReadOnly, Category = "AGASS|Timed Run", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> Text_TimerLabel;

	UPROPERTY(BlueprintReadOnly, Category = "AGASS|Timed Run", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> Text_TimerValue;

	UPROPERTY(Transient)
	TObjectPtr<UAGASSRunStateComponent> BoundRunStateComponent;

	UPROPERTY(Transient)
	EAGASSTimedRunState CachedTimedRunState = EAGASSTimedRunState::WaitingToStart;

	UPROPERTY(Transient)
	FText CachedTimedRunLabelText;

	UPROPERTY(Transient)
	FText CachedTimedRunValueText;

	UPROPERTY(Transient)
	int32 CachedTimedRunMilliseconds = 0;

	FTimerHandle RetryTimerHandle;
};
