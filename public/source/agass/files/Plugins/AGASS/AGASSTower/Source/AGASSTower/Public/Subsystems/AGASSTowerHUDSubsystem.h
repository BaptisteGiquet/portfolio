#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TimerManager.h"
#include "AGASSTowerHUDSubsystem.generated.h"

class UUserWidget;

UCLASS()
class AGASSTOWER_API UAGASSTowerHUDSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

private:
	bool ShouldManageThisLocalPlayer() const;
	bool ShouldDisplayForWorld(const UWorld* World) const;
	APlayerController* ResolveRuntimePlayerController(UWorld* World) const;
	bool IsRuntimePlayerControllerReady(const APlayerController* PlayerController) const;
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void HandleRetryCreateWidget();
	void TryCreateEndOfRunWidget();
	void TryCreateTimedRunWidget();
	void RemoveEndOfRunWidget();
	void RemoveTimedRunWidget();
	void StartRetryTimer();
	void StopRetryTimer();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TimedRunWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> EndOfRunWidget;

	FDelegateHandle PostLoadMapHandle;
	FTimerHandle RetryTimerHandle;
};
