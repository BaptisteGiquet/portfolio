#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"

#include "EMREconomySystemGenerics.generated.h"

class UGameplayEffect;

UCLASS()
class UEMREconomySystemGenerics : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	static FPrimaryAssetType GetEconomySystemGenericsAssetType();
	
    
    TSubclassOf<UGameplayEffect> GetSpendMoneyEffect() const { return SpendMoneyEffect; }
    TSubclassOf<UGameplayEffect> GetGrantMoneyEffect() const { return GrantMoneyEffect; }

	TSubclassOf<UGameplayEffect> GetRemoveReputationEffect() const { return RemoveReputationEffect; }
	TSubclassOf<UGameplayEffect> GetGrantReputationEffect() const { return GrantReputationEffect; }

	
private:
    UPROPERTY(EditDefaultsOnly, Category = "EMR|Economy|Currency")
    TSubclassOf<UGameplayEffect> SpendMoneyEffect;
	
    UPROPERTY(EditDefaultsOnly, Category = "EMR|Economy|Currency")
    TSubclassOf<UGameplayEffect> GrantMoneyEffect;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Economy|Currency")
	TSubclassOf<UGameplayEffect> GrantReputationEffect;

	UPROPERTY(EditDefaultsOnly, Category = "EMR|Economy|Currency")
	TSubclassOf<UGameplayEffect> RemoveReputationEffect;
};