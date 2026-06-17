#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Interfaces/AGASSSharedTeamMoneyInterface.h"
#include "AGASSGameStateBase.generated.h"

class UAGASSTeamWalletComponent;
class UAGASSRunStateComponent;
class UAGASSSessionEventManagerComponent;
class UAGASSSessionInfoComponent;

UCLASS()
class AGASS_API AAGASSGameStateBase : public AGameStateBase, public IAGASSSharedTeamMoneyInterface
{
	GENERATED_BODY()

public:
	AAGASSGameStateBase();

	virtual int32 GetAGASSCurrentSharedMoney_Implementation() const override;
	virtual bool CanAGASSAffordSharedMoney_Implementation(int32 Amount) const override;
	virtual bool TrySpendAGASSSharedMoney_Implementation(int32 Amount) override;

	UAGASSTeamWalletComponent* GetTeamWalletComponent() const
	{
		return TeamWalletComponent;
	}

	UAGASSRunStateComponent* GetRunStateComponent() const
	{
		return RunStateComponent;
	}

	UAGASSSessionEventManagerComponent* GetSessionEventManagerComponent() const
	{
		return SessionEventManagerComponent;
	}

	UAGASSSessionInfoComponent* GetSessionInfoComponent() const
	{
		return SessionInfoComponent;
	}

private:
	UPROPERTY(VisibleAnywhere, Category = "AGASS|Economy")
	TObjectPtr<UAGASSTeamWalletComponent> TeamWalletComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Run")
	TObjectPtr<UAGASSRunStateComponent> RunStateComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Events")
	TObjectPtr<UAGASSSessionEventManagerComponent> SessionEventManagerComponent;

	UPROPERTY(VisibleAnywhere, Category = "AGASS|Online")
	TObjectPtr<UAGASSSessionInfoComponent> SessionInfoComponent;
};
