#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "AGASSTeamMoneyTestWidget.generated.h"

class UAGASSTeamWalletComponent;
class UBorder;
class UTextBlock;

UCLASS()
class AGASSECONOMY_API UAGASSTeamMoneyTestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UAGASSTeamMoneyTestWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void RefreshWalletBinding();
	void ClearWalletBinding();
	void StartWalletRetryTimer();
	void StopWalletRetryTimer();
	void UpdateBalanceDisplay(int32 CurrentBalance);
	UAGASSTeamWalletComponent* ResolveWalletComponent() const;

	UFUNCTION()
	void HandleWalletBalanceChanged(int32 PreviousBalance, int32 NewBalance);

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BalanceTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UAGASSTeamWalletComponent> BoundWalletComponent;

	FTimerHandle WalletRetryTimerHandle;
};
