#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "AGASSEndOfRunWidget.generated.h"

class UAGASSRunStateComponent;
class UBorder;
class UTextBlock;

UCLASS()
class AGASSTOWER_API UAGASSEndOfRunWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UAGASSEndOfRunWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

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

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatsTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountdownTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UAGASSRunStateComponent> BoundRunStateComponent;

	FTimerHandle RetryTimerHandle;
};
