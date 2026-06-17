#include "Components/EMRPlayerViewStateComponent.h"

UEMRPlayerViewStateComponent::UEMRPlayerViewStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UEMRPlayerViewStateComponent::ShouldSendLookTargetUpdate(
    float CurrentTimeSeconds,
    const FVector& NewLocation,
    const FVector& CurrentReplicatedLocation,
    bool bForceReplication,
    float DistanceThreshold,
    float MinInterval,
    bool& bOutUseReliableRpc)
{
    const float SafeDistanceThreshold = FMath::Max(0.01f, DistanceThreshold);
    const bool bLocationChanged = !NewLocation.Equals(CurrentReplicatedLocation, SafeDistanceThreshold);
    if (!bForceReplication && !bLocationChanged)
    {
        bOutUseReliableRpc = false;
        return false;
    }

    bOutUseReliableRpc = bForceReplication;

    if (!bForceReplication && MinInterval > 0.0f)
    {
        const float Elapsed = CurrentTimeSeconds - LastLookTargetReplicationTimestamp;
        if (LastLookTargetReplicationTimestamp >= 0.0f && Elapsed < MinInterval)
        {
            return false;
        }
    }

    LastLookTargetReplicationTimestamp = CurrentTimeSeconds;
    return true;
}

void UEMRPlayerViewStateComponent::ResetLookTargetReplicationState()
{
    LastLookTargetReplicationTimestamp = -1.0f;
}

