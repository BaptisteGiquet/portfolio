#pragma once

#include "CoreMinimal.h"
#include "UI/Frontend/Data/EMRListDataObjectBase.h"
#include "EMRCommonHubUpgradeListDataObject.generated.h"

class UEMRRunUpgradeDefinition;

UCLASS()
class LOD_API UEMRCommonHubUpgradeListDataObject : public UEMRListDataObjectBase
{
    GENERATED_BODY()

public:
    void Init(UEMRRunUpgradeDefinition* InDefinition, int32 InOfferIndex, int32 InVoteCount);

    UEMRRunUpgradeDefinition* GetDefinition() const { return Definition; }
    int32 GetOfferIndex() const { return OfferIndex; }
    int32 GetVoteCount() const { return VoteCount; }

protected:
    virtual void OnDataObjectInitialized() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UEMRRunUpgradeDefinition> Definition = nullptr;

    UPROPERTY(Transient)
    int32 OfferIndex = INDEX_NONE;

    UPROPERTY(Transient)
    int32 VoteCount = 0;
};

