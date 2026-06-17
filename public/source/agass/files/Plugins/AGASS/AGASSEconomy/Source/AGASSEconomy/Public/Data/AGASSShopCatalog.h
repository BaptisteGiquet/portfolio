#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AGASSShopCatalog.generated.h"

class AAGASSPlaceableItemActor;
class UAGASSItemDefinition;

USTRUCT(BlueprintType)
struct AGASSECONOMY_API FAGASSShopCatalogEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Shop")
	FName EntryId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Shop")
	FText DisplayNameOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Shop")
	TSoftClassPtr<AAGASSPlaceableItemActor> ItemClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Shop")
	TSoftObjectPtr<UAGASSItemDefinition> ItemDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Shop")
	bool bAutoClaimIntoCarry = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Shop")
	bool bOverridePurchaseCost = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Shop", meta = (EditCondition = "bOverridePurchaseCost", ClampMin = "0"))
	int32 PurchaseCostOverride = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Shop")
	FTransform SpawnOffset = FTransform::Identity;

	int32 ResolvePurchaseCost(const UAGASSItemDefinition* ResolvedDefinition) const;
};

UCLASS(BlueprintType)
class AGASSECONOMY_API UAGASSShopCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AGASS|Shop")
	TArray<FAGASSShopCatalogEntry> Entries;

	const FAGASSShopCatalogEntry* FindEntryById(FName EntryId) const;
	const FAGASSShopCatalogEntry* GetDefaultEntry() const;
};
