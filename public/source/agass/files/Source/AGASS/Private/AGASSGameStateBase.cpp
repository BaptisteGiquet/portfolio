#include "AGASSGameStateBase.h"

#include "Components/AGASSSessionInfoComponent.h"
#include "Components/AGASSSessionEventManagerComponent.h"
#include "Components/AGASSRunStateComponent.h"
#include "Components/AGASSTeamWalletComponent.h"

AAGASSGameStateBase::AAGASSGameStateBase()
{
	TeamWalletComponent = CreateDefaultSubobject<UAGASSTeamWalletComponent>(TEXT("TeamWalletComponent"));
	RunStateComponent = CreateDefaultSubobject<UAGASSRunStateComponent>(TEXT("RunStateComponent"));
	SessionEventManagerComponent = CreateDefaultSubobject<UAGASSSessionEventManagerComponent>(TEXT("SessionEventManagerComponent"));
	SessionInfoComponent = CreateDefaultSubobject<UAGASSSessionInfoComponent>(TEXT("SessionInfoComponent"));
}

int32 AAGASSGameStateBase::GetAGASSCurrentSharedMoney_Implementation() const
{
	return TeamWalletComponent != nullptr ? TeamWalletComponent->GetCurrentBalance() : 0;
}

bool AAGASSGameStateBase::CanAGASSAffordSharedMoney_Implementation(const int32 Amount) const
{
	return TeamWalletComponent != nullptr && TeamWalletComponent->CanAfford(Amount);
}

bool AAGASSGameStateBase::TrySpendAGASSSharedMoney_Implementation(const int32 Amount)
{
	return TeamWalletComponent != nullptr && TeamWalletComponent->TrySpendMoney(Amount);
}
