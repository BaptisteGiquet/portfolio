
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/MobaGameplayTypes.h"
#include "Interfaces/MobaAbilityInterface.h"
#include "MobaAbilitySystemComponent.generated.h"

class UMobaAbilitySystemGenerics;
class UGameplayAbility;


USTRUCT(BlueprintType)
struct FAbilitySpecChanged
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FGameplayAbilitySpecHandle HandleValue = FGameplayAbilitySpecHandle();

	UPROPERTY()
	int32 NewLevel = 0;
	
	void SetVariablesFromAbilitySpec(const FGameplayAbilitySpec& AbilitySpec)
	{
		HandleValue = AbilitySpec.Handle;
		NewLevel    = AbilitySpec.Level;
	}
};


UCLASS()
class UMobaAbilitySystemComponent : public UAbilitySystemComponent, public IMobaAbilityInterface
{
	GENERATED_BODY()

public:
	UMobaAbilitySystemComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitializeAbilitySystemComponent_Server();
	void ApplyRespawnEffects();

	void ApplySpendGoldEffect();
	
	virtual const TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>>& GetMainAbilities() const override;
	virtual const UMobaAbilitySystemGenerics* GetAbilitySystemGenerics() const override;

	UFUNCTION(Server, Reliable)
	void Server_UpgradeAbilityWithHandle(const FGameplayAbilitySpecHandle AbilitySpecHandle);

	
private:
	void StoreEssentialVariables();
	void ApplyStartupEffects();
	void GrantStartupAbilities();
	void InitializeBaseAttributes();
	
	void AuthApplyGameplayEffectToSelf(const TSubclassOf<UGameplayEffect> GameplayEffect, const int32 Level = 1);
	void OnHealthUpdated(const FOnAttributeChangeData& HealthData);
	void OnManaUpdated(const FOnAttributeChangeData& ManaData);
	void OnExperienceUpdated(const FOnAttributeChangeData& ExperienceData);

	bool IsAtMaxLevel() const;

	UFUNCTION()
	virtual void OnRep_AbilitySpecChanged();
	
	UPROPERTY(ReplicatedUsing = OnRep_AbilitySpecChanged)
	FAbilitySpecChanged AbilitySpecChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>> BasicAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TMap<EAbilityInputID, TSubclassOf<UGameplayAbility>> MainAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TObjectPtr<UMobaAbilitySystemGenerics> AbilitySystemGenerics;

	
	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> CachedDeathEffect;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> CachedSpendUpgradePointEffect;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> CachedSpendGoldEffect;
	
	UPROPERTY(Transient)
	TArray<TSubclassOf<UGameplayEffect>> CachedInitialEffects;
	
	UPROPERTY(Transient)
	TArray<TSubclassOf<UGameplayEffect>> CachedRespawnEffects;
	
	UPROPERTY(Transient)
	TArray<TSubclassOf<UGameplayAbility>> CachedPassiveAbilities;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedHeroBaseStatsDataTable;
	
	FCurveTableRowHandle CachedExperienceCurveHandle;
};
