
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MobaAbilitySystemGenerics.generated.h"


class UGameplayAbility;
class UGameplayEffect;

UCLASS()
class UMobaAbilitySystemGenerics : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	TSubclassOf<UGameplayEffect> GetDeathEffect() const { return DeathEffect; }
	TSubclassOf<UGameplayEffect> GetSpendUpgradePointEffect() const { return SpendUpgradePointEffect; }
	TSubclassOf<UGameplayEffect> GetSpendGoldEffect() const { return SpendGoldEffect; }
	TSubclassOf<UGameplayEffect> GetGrantGoldEffect() const { return GrantGoldEffect; } 
	const TArray<TSubclassOf<UGameplayEffect>>& GetInitialEffects() const { return InitialEffects; }
	const TArray<TSubclassOf<UGameplayEffect>>& GetRespawnEffects() const { return RespawnEffects; }

	const TArray<TSubclassOf<UGameplayAbility>>& GetPassiveAbilities() const { return PassiveAbilities; }

	TObjectPtr<UDataTable> GetBaseStatsDataTable() const { return BaseStatsDataTable; }
	FCurveTableRowHandle GetExperienceCurveHandle() const;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	TSubclassOf<UGameplayEffect> DeathEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	TSubclassOf<UGameplayEffect> SpendUpgradePointEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	TSubclassOf<UGameplayEffect> SpendGoldEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	TSubclassOf<UGameplayEffect> GrantGoldEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	TArray<TSubclassOf<UGameplayEffect>> InitialEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects")
	TArray<TSubclassOf<UGameplayEffect>> RespawnEffects;
	
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TArray<TSubclassOf<UGameplayAbility>> PassiveAbilities;
	

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TObjectPtr<UDataTable> BaseStatsDataTable = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	FCurveTableRowHandle ExperienceCurveHandle;
};
