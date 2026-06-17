#include "GAS/Attributes/EMRTeamAttributeSet.h"

#include "Framework/EMRNightShiftGameState.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


void UEMRTeamAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, TotalRevenue, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Reputation, COND_None, REPNOTIFY_Always)
}


void UEMRTeamAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetReputationAttribute())
	{
		if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			if (const AEMRNightShiftGameState* RunGS = ASC->GetOwner<AEMRNightShiftGameState>())
			{
				if (RunGS->IsReputationFrozen())
				{
					NewValue = RunGS->GetFrozenReputation();
					return;
				}

				if (RunGS->IsReputationLockedAtZero())
				{
					NewValue = 0.f;
					return;
				}
			}
		}

		NewValue = FMath::Clamp(NewValue, 0.f, 100.f);
	}
}


void UEMRTeamAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetReputationAttribute())
	{
		float ClampedValue = FMath::Clamp(GetReputation(), 0.f, 100.f);
		if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			if (const AEMRNightShiftGameState* RunGS = ASC->GetOwner<AEMRNightShiftGameState>())
			{
				if (RunGS->IsReputationFrozen())
				{
					ClampedValue = RunGS->GetFrozenReputation();
				}
				else
				if (RunGS->IsReputationLockedAtZero())
				{
					ClampedValue = 0.f;
				}
			}
		}
		SetReputation(ClampedValue);
	}
}


void UEMRTeamAttributeSet::OnRep_TotalRevenue(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, TotalRevenue, OldValue)
}


void UEMRTeamAttributeSet::OnRep_Reputation(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Reputation, OldValue)
}
