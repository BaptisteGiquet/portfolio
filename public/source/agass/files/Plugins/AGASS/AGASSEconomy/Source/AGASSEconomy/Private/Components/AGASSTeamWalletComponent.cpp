#include "Components/AGASSTeamWalletComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Settings/AGASSEconomyDeveloperSettings.h"
#include "Subsystems/AGASSGameplayEventSubsystem.h"

UAGASSTeamWalletComponent::UAGASSTeamWalletComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAGASSTeamWalletComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() != nullptr && GetOwner()->HasAuthority() && !bInitializedFromSettings)
	{
		const UAGASSEconomyDeveloperSettings* const EconomySettings = UAGASSEconomyDeveloperSettings::Get();
		SetCurrentBalance(EconomySettings != nullptr ? FMath::Max(EconomySettings->InitialSharedMoney, 0) : 0);
		bInitializedFromSettings = true;
		bHasProcessedInitialBalanceSnapshot = true;
	}
}

void UAGASSTeamWalletComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentBalance);
}

int32 UAGASSTeamWalletComponent::GetCurrentBalance() const
{
	return CurrentBalance;
}

bool UAGASSTeamWalletComponent::CanAfford(const int32 Amount) const
{
	return Amount <= 0 || CurrentBalance >= Amount;
}

bool UAGASSTeamWalletComponent::TrySpendMoney(const int32 Amount)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || Amount < 0 || !CanAfford(Amount))
	{
		return false;
	}

	SetCurrentBalance(CurrentBalance - Amount);
	return true;
}

void UAGASSTeamWalletComponent::AddMoney(const int32 Amount)
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority() || Amount <= 0)
	{
		return;
	}

	SetCurrentBalance(CurrentBalance + Amount);
}

void UAGASSTeamWalletComponent::OnRep_CurrentBalance(const int32 PreviousBalance)
{
	if (!bHasProcessedInitialBalanceSnapshot)
	{
		bHasProcessedInitialBalanceSnapshot = true;
		return;
	}

	EmitBalanceDeltaEvents(PreviousBalance, CurrentBalance);
	OnBalanceChanged.Broadcast(PreviousBalance, CurrentBalance);
}

void UAGASSTeamWalletComponent::SetCurrentBalance(const int32 NewBalance)
{
	const int32 PreviousBalance = CurrentBalance;
	CurrentBalance = FMath::Max(NewBalance, 0);

	if (PreviousBalance != CurrentBalance)
	{
		if (bHasProcessedInitialBalanceSnapshot)
		{
			EmitBalanceDeltaEvents(PreviousBalance, CurrentBalance);
		}

		OnBalanceChanged.Broadcast(PreviousBalance, CurrentBalance);
	}
}

void UAGASSTeamWalletComponent::EmitBalanceDeltaEvents(const int32 PreviousBalance, const int32 NewBalance)
{
	const int32 Delta = NewBalance - PreviousBalance;
	if (Delta > 0)
	{
		UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(this, AGASSGameplayEventNames::MoneyEarned(), Delta, static_cast<float>(Delta), true);
	}
	else if (Delta < 0)
	{
		const int32 SpentAmount = FMath::Abs(Delta);
		UAGASSGameplayEventSubsystem::BroadcastGameplayEvent(this, AGASSGameplayEventNames::MoneySpent(), SpentAmount, static_cast<float>(SpentAmount), true);
	}
}
