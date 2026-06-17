#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EMRRunUpgradeDefinition.generated.h"

UCLASS(BlueprintType)
class LOD_API UEMRRunUpgradeDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    static FPrimaryAssetType GetRunUpgradeAssetType();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Upgrade")
    FName UpgradeId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Upgrade")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Upgrade")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Upgrade", meta = (Categories = "EMR.RunUpgrade"))
    FGameplayTag UpgradeTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMR|Upgrade", meta = (ClampMin = "0", UIMin = "0", ToolTip = "Maximum number of times this upgrade can be purchased. 0 means unlimited."))
    int32 MaxPurchaseCount = 1;
};
