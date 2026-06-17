

#include "MobaAttributeSet_Hero.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

void UMobaAttributeSet_Hero::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, Experience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, PreviousLevelExperience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, NextLevelExperience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, MaxLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, UpgradePoint, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, Gold, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, StrengthGrowthRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAttributeSet_Hero, IntelligenceGrowthRate, COND_None, REPNOTIFY_Always);
}


void UMobaAttributeSet_Hero::OnRep_Strength(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, Strength, OldValue);
}

void UMobaAttributeSet_Hero::OnRep_Intelligence(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, Intelligence, OldValue);
}

void UMobaAttributeSet_Hero::OnRep_Experience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, Experience, OldValue);
}



void UMobaAttributeSet_Hero::OnRep_PrevLevelExperience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, PreviousLevelExperience, OldValue);
}



void UMobaAttributeSet_Hero::OnRep_NextLevelExperience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, NextLevelExperience, OldValue);
}



void UMobaAttributeSet_Hero::OnRep_Level(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, Level, OldValue);
}



void UMobaAttributeSet_Hero::OnRep_MaxLevel(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, MaxLevel, OldValue);
}



void UMobaAttributeSet_Hero::OnRep_UpgradePoint(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, UpgradePoint, OldValue);
}



void UMobaAttributeSet_Hero::OnRep_Gold(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, Gold, OldValue);
}



void UMobaAttributeSet_Hero::OnRep_StrengthGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, StrengthGrowthRate, OldValue);
}


void UMobaAttributeSet_Hero::OnRep_IntelligenceGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMobaAttributeSet_Hero, IntelligenceGrowthRate, OldValue);
}
