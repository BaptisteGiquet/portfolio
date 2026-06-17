#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "EMRBTTask_SnapToTarget.generated.h"

/**
 * Snaps the controlled patient to the desired navigation target once MoveTo succeeds
 * and clears navigation-related Blackboard entries so the AI waits for the next intent.
 */
UCLASS()
class LOD_API UEMRBTTask_SnapToTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UEMRBTTask_SnapToTarget();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;

protected:
    /** Duration used to interpolate the snap for a smoother arrival. Zero keeps the previous instant teleport. */
    UPROPERTY(EditAnywhere, Category = "Snap", meta = (ClampMin = "0.0"))
    float SnapLerpDuration = 0.25f;

    /** If non-zero, prevents snapping when the target is farther than this distance (cm). */
    UPROPERTY(EditAnywhere, Category = "EMR", meta = (ClampMin = "0.0"))
    float MaxSnapDistance = 200.0f;

    struct FSnapToTargetMemory
    {
        FVector StartLocation = FVector::ZeroVector;
        FRotator StartRotation = FRotator::ZeroRotator;
        FVector TargetLocation = FVector::ZeroVector;
        FRotator TargetRotation = FRotator::ZeroRotator;
        float StartTime = 0.f;
        bool bInitialized = false;
    };

    /** Blackboard entry holding the navigation target actor (e.g., waiting seat or machine). */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetActorKey;

    /** Blackboard entry holding the target location provided by the navigation intent. */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

    /** Blackboard entry holding the rotation the patient should adopt at the destination. */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetRotationKey;
};
