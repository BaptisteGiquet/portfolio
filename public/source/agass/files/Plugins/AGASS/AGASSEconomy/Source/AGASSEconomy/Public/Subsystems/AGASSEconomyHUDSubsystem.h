#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TimerManager.h"
#include "AGASSEconomyHUDSubsystem.generated.h"

class UUserWidget;

UCLASS()
class AGASSECONOMY_API UAGASSEconomyHUDSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool ShouldManageThisLocalPlayer() const;
	bool ShouldDisplayInWorld(const UWorld* World) const;
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleRetryCreateWidget();
	void TryCreateTeamMoneyWidget();
	void RemoveTeamMoneyWidget();
	void StartCreateRetryTimer();
	void StopCreateRetryTimer();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TeamMoneyWidget;

	FDelegateHandle PostLoadMapHandle;
	FTimerHandle CreateRetryTimerHandle;
};
