
#include "GAS/MobaAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "MobaAbilitySystemStatics.h"
#include "Attributes/MobaAttributeSet.h"
#include "Abilities/MobaGameplayTypes.h"
#include "MobaGameplayTags.h"
#include "Attributes/MobaAttributeSet_Hero.h"
#include "Data/MobaAbilitySystemGenerics.h"
#include "Net/UnrealNetwork.h"


UMobaAbilitySystemComponent::UMobaAbilitySystemComponent()
{
	GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet::GetHealthAttribute()).AddUObject(this, &UMobaAbilitySystemComponent::OnHealthUpdated);
	GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet::GetManaAttribute()).AddUObject(this, &UMobaAbilitySystemComponent::OnManaUpdated);

	GetGameplayAttributeValueChangeDelegate(UMobaAttributeSet_Hero::GetExperienceAttribute()).AddUObject(this, &UMobaAbilitySystemComponent::OnExperienceUpdated);
	
	GenericConfirmInputID = static_cast<int32>(EAbilityInputID::Confirm);
	GenericCancelInputID = static_cast<int32>(EAbilityInputID::Cancel);
}



void UMobaAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMobaAbilitySystemComponent, AbilitySpecChanged, COND_OwnerOnly, REPNOTIFY_Always);
}



void UMobaAbilitySystemComponent::StoreEssentialVariables()
{
	if (!AbilitySystemGenerics) { return; }
	
	CachedDeathEffect = AbilitySystemGenerics->GetDeathEffect();
	CachedSpendUpgradePointEffect = AbilitySystemGenerics->GetSpendUpgradePointEffect();
	CachedSpendGoldEffect = AbilitySystemGenerics->GetSpendGoldEffect();
	CachedInitialEffects = AbilitySystemGenerics->GetInitialEffects();
	CachedRespawnEffects = AbilitySystemGenerics->GetRespawnEffects();
	CachedPassiveAbilities = AbilitySystemGenerics->GetPassiveAbilities();
	CachedHeroBaseStatsDataTable = AbilitySystemGenerics->GetBaseStatsDataTable();
	CachedExperienceCurveHandle = AbilitySystemGenerics->GetExperienceCurveHandle();
}
 


void UMobaAbilitySystemComponent::InitializeAbilitySystemComponent_Server()
{
	StoreEssentialVariables();
	InitializeBaseAttributes();
	ApplyStartupEffects();
	GrantStartupAbilities();
}



void UMobaAbilitySystemComponent::InitializeBaseAttributes()
{
	
	const AActor* OwnerAvatarActor = GetAvatarActor();
	if (!OwnerAvatarActor) { return; }

	const UClass* OwnerAvatarClass = OwnerAvatarActor->GetClass();
	if (!OwnerAvatarClass) { return; }

	
	const FHeroBaseStats* HeroBaseStatsFound = nullptr;
	if (CachedHeroBaseStatsDataTable)
	{
		for (const FName& HeroNameRow : CachedHeroBaseStatsDataTable->GetRowNames())
		{
			const FHeroBaseStats* RowData = CachedHeroBaseStatsDataTable->FindRow<FHeroBaseStats>(HeroNameRow, "");
			if (!RowData || !RowData->CharacterClass) { continue; }
		
			if (RowData && RowData->CharacterClass == OwnerAvatarClass)
			{
				HeroBaseStatsFound = RowData;
				break;
			}
		}
	}
	

	
	if (HeroBaseStatsFound)
	{
		SetNumericAttributeBase(UMobaAttributeSet::GetMaxHealthAttribute(), HeroBaseStatsFound->MaxHealth);
		SetNumericAttributeBase(UMobaAttributeSet::GetMaxManaAttribute(), HeroBaseStatsFound->MaxMana);
		SetNumericAttributeBase(UMobaAttributeSet_Hero::GetStrengthAttribute(), HeroBaseStatsFound->Strength);
		SetNumericAttributeBase(UMobaAttributeSet_Hero::GetIntelligenceAttribute(), HeroBaseStatsFound->Intelligence);
		SetNumericAttributeBase(UMobaAttributeSet::GetArmorAttribute(), HeroBaseStatsFound->Armor);
		SetNumericAttributeBase(UMobaAttributeSet::GetMagicResistanceAttribute(), HeroBaseStatsFound->MagicResistance);
		
		SetNumericAttributeBase(UMobaAttributeSet::GetAttackDamageAttribute(), HeroBaseStatsFound->AttackDamage);
		SetNumericAttributeBase(UMobaAttributeSet::GetAbilityPowerAttribute(), HeroBaseStatsFound->AbilityPower);
		SetNumericAttributeBase(UMobaAttributeSet::GetAttackSpeedAttribute(), HeroBaseStatsFound->AttackSpeed);
		SetNumericAttributeBase(UMobaAttributeSet::GetArmorPenetrationAttribute(), HeroBaseStatsFound->ArmorPenetration);
		SetNumericAttributeBase(UMobaAttributeSet::GetMagicResistancePenetrationAttribute(), HeroBaseStatsFound->MagicResistancePenetration);
		SetNumericAttributeBase(UMobaAttributeSet::GetMoveSpeedAttribute(), HeroBaseStatsFound->MoveSpeed);
		SetNumericAttributeBase(UMobaAttributeSet::GetCooldownReductionAttribute(), HeroBaseStatsFound->CooldownReduction);

		SetNumericAttributeBase(UMobaAttributeSet::GetAttackDamageGrowthRateAttribute(), HeroBaseStatsFound->StrengthGrowthRate);
		SetNumericAttributeBase(UMobaAttributeSet::GetAbilityPowerGrowthRateAttribute(), HeroBaseStatsFound->IntelligenceGrowthRate);
	}


	const FRealCurve* ExperienceCurve = CachedExperienceCurveHandle.GetCurve("GetExperienceCurve");
	if (!ExperienceCurve) { return; }

	const int MaxLevel = ExperienceCurve->GetNumKeys();
	SetNumericAttributeBase(UMobaAttributeSet_Hero::GetMaxLevelAttribute(), MaxLevel);
	

	FOnAttributeChangeData InitialExperienceData;
	InitialExperienceData.Attribute = UMobaAttributeSet_Hero::GetExperienceAttribute();
	InitialExperienceData.OldValue = GetNumericAttributeBase(UMobaAttributeSet_Hero::GetExperienceAttribute());
	InitialExperienceData.NewValue = GetNumericAttributeBase(UMobaAttributeSet_Hero::GetExperienceAttribute());
	OnExperienceUpdated(InitialExperienceData);
}



void UMobaAbilitySystemComponent::ApplyStartupEffects()
{
	for (const TSubclassOf<UGameplayEffect>& GameplayEffectClass : CachedInitialEffects)
	{
		AuthApplyGameplayEffectToSelf(GameplayEffectClass);
	}
}



void UMobaAbilitySystemComponent::GrantStartupAbilities()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { return; }

	// Abilities that you already have when starting a game (ex: basic attack, dodge, jump...)
	for (const TPair<EAbilityInputID, TSubclassOf<UGameplayAbility>>& BasicAbilityPair : BasicAbilities)
	{
		if (BasicAbilityPair.Value)
		{
			constexpr int32 BasicAbilityLevel = 1;
			GiveAbility(FGameplayAbilitySpec(BasicAbilityPair.Value, BasicAbilityLevel, static_cast<int32>(BasicAbilityPair.Key)));	
		}
	}

	// Abilities that you need to unlock
	for (const TPair<EAbilityInputID, TSubclassOf<UGameplayAbility>>& MainAbilityPair : MainAbilities)
	{
		if (MainAbilityPair.Value)
		{
			constexpr int32 MainAbilityLevel = 0;
			GiveAbility(FGameplayAbilitySpec(MainAbilityPair.Value, MainAbilityLevel, static_cast<int32>(MainAbilityPair.Key)));	
		}
	}

	for (const TSubclassOf<UGameplayAbility>& PassiveAbility : CachedPassiveAbilities)
	{
		if (PassiveAbility)
		{
			constexpr int32 PassiveAbilityLevel = 1;
			GiveAbility(FGameplayAbilitySpec(PassiveAbility, PassiveAbilityLevel, INDEX_NONE, nullptr));
		}
	}
}



void UMobaAbilitySystemComponent::ApplyRespawnEffects()
{
	for (const TSubclassOf<UGameplayEffect>& RespawnEffectClass : CachedRespawnEffects)
	{
		AuthApplyGameplayEffectToSelf(RespawnEffectClass);
	}
}



void UMobaAbilitySystemComponent::ApplySpendGoldEffect()
{
	if (CachedSpendGoldEffect)
	{
		AuthApplyGameplayEffectToSelf(CachedSpendGoldEffect);	
	}
}



const TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>>& UMobaAbilitySystemComponent::GetMainAbilities() const
{
	return MainAbilities;
}



const UMobaAbilitySystemGenerics* UMobaAbilitySystemComponent::GetAbilitySystemGenerics() const
{
	return AbilitySystemGenerics;
}


void UMobaAbilitySystemComponent::Server_UpgradeAbilityWithHandle_Implementation(const FGameplayAbilitySpecHandle AbilitySpecHandle)
{
	bool bFound = false;
	const float CurrentUpgradePoints = GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetUpgradePointAttribute(), bFound);
	if (!bFound || CurrentUpgradePoints <= 0) { return; }

	FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilitySpecHandle);

	if (!AbilitySpec || UMobaAbilitySystemStatics::IsAbilityAtMaxLevel(*AbilitySpec)) { return; }

	// Remove one upgrade point and add one level to Ability
	if (CachedSpendUpgradePointEffect)
	{
		AuthApplyGameplayEffectToSelf(CachedSpendUpgradePointEffect, 1);
		
		AbilitySpec->Level += 1;

		MarkAbilitySpecDirty(*AbilitySpec);

		AbilitySpecChanged.SetVariablesFromAbilitySpec(*AbilitySpec);
	}
}



void UMobaAbilitySystemComponent::AuthApplyGameplayEffectToSelf(const TSubclassOf<UGameplayEffect> GameplayEffect, const int32 Level)
{
	if (!GetOwner()) { return; }

	if (!GameplayEffect || !GetOwner()->HasAuthority()) { return; }

	
	const FGameplayEffectSpecHandle GameplayEffectSpecHandle = MakeOutgoingSpec(GameplayEffect, Level, MakeEffectContext());
	if (GameplayEffectSpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToSelf(*GameplayEffectSpecHandle.Data.Get());	
	}
}



void UMobaAbilitySystemComponent::OnHealthUpdated(const FOnAttributeChangeData& HealthData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { return; }

	bool bFound = false;
	const float MaxHealth = GetGameplayAttributeValue(UMobaAttributeSet::GetMaxHealthAttribute(), bFound);

	if (bFound && HealthData.NewValue >= MaxHealth)
	{
		if (!HasMatchingGameplayTag(MobaStatusTags::Status_Health_Full))
		{
			AddLooseGameplayTag(MobaStatusTags::Status_Health_Full);
		}
	}
	else
	{
		RemoveLooseGameplayTag(MobaStatusTags::Status_Health_Full);
	}
	
	
	
	
	if (HealthData.NewValue <= 0.f)
	{
		if (!HasMatchingGameplayTag(MobaStatusTags::Status_Health_Empty) && CachedDeathEffect && HealthData.GEModData)
		{
			AddLooseGameplayTag(MobaStatusTags::Status_Health_Empty);

			AuthApplyGameplayEffectToSelf(CachedDeathEffect);
				
			FGameplayEventData DeadAbilityEventData;
			DeadAbilityEventData.ContextHandle = HealthData.GEModData->EffectSpec.GetContext();
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), MobaStatusTags::Status_Dead, DeadAbilityEventData);
		}
	}
	else
	{
		RemoveLooseGameplayTag(MobaStatusTags::Status_Health_Empty);
	}
}



void UMobaAbilitySystemComponent::OnManaUpdated(const FOnAttributeChangeData& ManaData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { return; }

	bool bFound = false;
	const float MaxMana = GetGameplayAttributeValue(UMobaAttributeSet::GetMaxManaAttribute(), bFound);

	if (bFound && ManaData.NewValue >= MaxMana)
	{
		if (!HasMatchingGameplayTag(MobaStatusTags::Status_Mana_Full))
		{
			AddLooseGameplayTag(MobaStatusTags::Status_Mana_Full);
		}
	}
	else
	{
		RemoveLooseGameplayTag(MobaStatusTags::Status_Mana_Full);
	}
	
	
	
	if (ManaData.NewValue <= 0.f)
	{
		if (!HasMatchingGameplayTag(MobaStatusTags::Status_Mana_Empty))
		{
			AddLooseGameplayTag(MobaStatusTags::Status_Mana_Empty);
		}
	}
	else
	{
		RemoveLooseGameplayTag(MobaStatusTags::Status_Mana_Empty);
	}
}



void UMobaAbilitySystemComponent::OnExperienceUpdated(const FOnAttributeChangeData& ExperienceData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { return; }

	if (IsAtMaxLevel()) { return; }

	
	const FRealCurve* ExperienceCurve = CachedExperienceCurveHandle.GetCurve("GetExperienceCurve");
	if (!ExperienceCurve) { return; }
	

	const float CurrentExperience = ExperienceData.NewValue;
	const int32 CurrentLevel = GetNumericAttributeBase(UMobaAttributeSet_Hero::GetLevelAttribute());
	const int32 MaxLevel = static_cast<float>(GetNumericAttributeBase(UMobaAttributeSet_Hero::GetMaxLevelAttribute()));
	
	float PreviousLevelThreshold = 0.f;
	float NextLevelExperienceThreshold = 0.f;
	int32 NewLevel = CurrentLevel;

	// Search if we passed a level after this exp gain
	for (int32 NextLevel = CurrentLevel; NextLevel <= MaxLevel; ++NextLevel)
	{
		const float ExperienceToReachNextLevel = ExperienceCurve->Eval(static_cast<float>(NextLevel));
		if (CurrentExperience < ExperienceToReachNextLevel)
		{
			NextLevelExperienceThreshold = ExperienceToReachNextLevel;
			break;
		}

		// We reached NextLevel
		NewLevel = NextLevel;
		PreviousLevelThreshold = ExperienceToReachNextLevel;
	}

	
	const int32 CurrentUpgradePoints = GetNumericAttributeBase(UMobaAttributeSet_Hero::GetUpgradePointAttribute());

	const int32 NumberOfLevelsGained = NewLevel - CurrentLevel;
	const int32 NewUpgradePoints = CurrentUpgradePoints + NumberOfLevelsGained;

	SetNumericAttributeBase(UMobaAttributeSet_Hero::GetLevelAttribute(), NewLevel);
	SetNumericAttributeBase(UMobaAttributeSet_Hero::GetPreviousLevelExperienceAttribute(), PreviousLevelThreshold);
	SetNumericAttributeBase(UMobaAttributeSet_Hero::GetNextLevelExperienceAttribute(), NextLevelExperienceThreshold);
	
	SetNumericAttributeBase(UMobaAttributeSet_Hero::GetUpgradePointAttribute(), NewUpgradePoints);
}



bool UMobaAbilitySystemComponent::IsAtMaxLevel() const
{
	bool bFound = false;
	const float CurrentLevel = GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetLevelAttribute(), bFound);
	const float MaxLevel = GetGameplayAttributeValue(UMobaAttributeSet_Hero::GetMaxLevelAttribute(), bFound);
	
	return bFound && CurrentLevel >= MaxLevel;
}



void UMobaAbilitySystemComponent::OnRep_AbilitySpecChanged()
{
	FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(FGameplayAbilitySpecHandle(AbilitySpecChanged.HandleValue));
	if (AbilitySpec)
	{
		AbilitySpec->Level = AbilitySpecChanged.NewLevel;
		AbilitySpecDirtiedCallbacks.Broadcast(*AbilitySpec);
	}
}
