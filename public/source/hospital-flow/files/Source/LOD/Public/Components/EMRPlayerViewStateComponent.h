#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EMRPlayerViewStateComponent.generated.h"

UCLASS(ClassGroup = (EMR), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class LOD_API UEMRPlayerViewStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEMRPlayerViewStateComponent();

    bool ShouldSendLookTargetUpdate(
        float CurrentTimeSeconds,
        const FVector& NewLocation,
        const FVector& CurrentReplicatedLocation,
        bool bForceReplication,
        float DistanceThreshold,
        float MinInterval,
        bool& bOutUseReliableRpc);

    void ResetLookTargetReplicationState();

private:
    float LastLookTargetReplicationTimestamp = -1.0f;
};

