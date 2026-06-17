#include "Behavior/Tasks/EMRBTTask_SnapToTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Patient/EMRAIController.h"
#include "Characters/Patient/EMRPatient.h"
#include "Environment/EMRWaitingSeatComponent.h"
#include "GameFramework/Actor.h"


DEFINE_LOG_CATEGORY_STATIC(LogEMRBTTaskSnapToTarget, Log, All);

namespace
{
    void ApplyPatientWaitingSeatArrival(AActor* TargetActor, APawn* ControlledPawn)
    {
        AEMRPatient* Patient = Cast<AEMRPatient>(ControlledPawn);
        if (!Patient || !Patient->HasAuthority() || !TargetActor)
        {
            return;
        }

        UEMRWaitingSeatComponent* Seat = TargetActor->FindComponentByClass<UEMRWaitingSeatComponent>();
        if (!Seat)
        {
            return;
        }

        const FGameplayTag SeatAnimationTag = Seat->GetWaitingSeatAnimationTag();
        if (!SeatAnimationTag.IsValid() || Seat->GetReservedActor() != Patient)
        {
            UE_LOG(
                LogEMRBTTaskSnapToTarget,
                Warning,
                TEXT("[SnapToTarget] Skipping waiting-seat arrival seat-state mutation for patient=%s target=%s (seat context unresolved/stale)."),
                *GetNameSafe(Patient),
                *GetNameSafe(TargetActor));
            return;
        }

        Seat->OccupySeat(Patient);
        Patient->SetSeatedAnimationState(true, SeatAnimationTag);
    }
}


UEMRBTTask_SnapToTarget::UEMRBTTask_SnapToTarget()
{
    NodeName = TEXT("Snap To Target");
    bNotifyTick = true;

    TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UEMRBTTask_SnapToTarget, TargetActorKey), AActor::StaticClass());
    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UEMRBTTask_SnapToTarget, TargetLocationKey));
    TargetRotationKey.AddRotatorFilter(this, GET_MEMBER_NAME_CHECKED(UEMRBTTask_SnapToTarget, TargetRotationKey));

    TargetActorKey.SelectedKeyName = TEXT("Target");
    TargetLocationKey.SelectedKeyName = TEXT("PatientWaitPointLocation");
    TargetRotationKey.SelectedKeyName = TEXT("PatientWaitPointRotation");
}


EBTNodeResult::Type UEMRBTTask_SnapToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::ExecuteTask(OwnerComp, NodeMemory);

    UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
    AAIController* AIController = OwnerComp.GetAIOwner();

    if (!BlackboardComponent || !AIController)
    {
        return EBTNodeResult::Failed;
    }

    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
    {
        return EBTNodeResult::Failed;
    }

    if (!NodeMemory)
    {
        return EBTNodeResult::Failed;
    }

    AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetActorKey.SelectedKeyName));
    FVector TargetLocation = BlackboardComponent->GetValueAsVector(TargetLocationKey.SelectedKeyName);
    FRotator TargetRotation = BlackboardComponent->GetValueAsRotator(TargetRotationKey.SelectedKeyName);

    if (TargetLocation.IsNearlyZero() && TargetActor)
    {
        TargetLocation = TargetActor->GetActorLocation();
    }

    if (TargetRotation.IsNearlyZero() && TargetActor)
    {
        TargetRotation = TargetActor->GetActorRotation();
    }

    AIController->StopMovement();

    // Only apply the target's X/Y so the patient keeps their current height (Z).
    TargetLocation.Z = ControlledPawn->GetActorLocation().Z;

    if (MaxSnapDistance > 0.0f)
    {
        const float DistSq = FVector::DistSquared2D(ControlledPawn->GetActorLocation(), TargetLocation);
        if (DistSq > FMath::Square(MaxSnapDistance))
        {
            return EBTNodeResult::Failed;
        }
    }

    FSnapToTargetMemory* Memory = reinterpret_cast<FSnapToTargetMemory*>(NodeMemory);

    if (SnapLerpDuration <= KINDA_SMALL_NUMBER)
    {
        const bool bTeleported = ControlledPawn->TeleportTo(TargetLocation, TargetRotation, false, true);
        if (!bTeleported)
        {
            return EBTNodeResult::Failed;
        }

        ApplyPatientWaitingSeatArrival(TargetActor, ControlledPawn);
    }
    else
    {
        Memory->StartLocation = ControlledPawn->GetActorLocation();
        Memory->StartRotation = ControlledPawn->GetActorRotation();
        Memory->TargetLocation = TargetLocation;
        Memory->TargetRotation = TargetRotation;
        Memory->StartTime = OwnerComp.GetWorld()->GetTimeSeconds();
        Memory->bInitialized = true;

        return EBTNodeResult::InProgress;
    }

    if (AEMRAIController* EMRAI = Cast<AEMRAIController>(AIController))
    {
        EMRAI->ResetBlackboardState();
    }
    else
    {
        BlackboardComponent->ClearValue(TargetActorKey.SelectedKeyName);
        BlackboardComponent->ClearValue(TargetLocationKey.SelectedKeyName);
        BlackboardComponent->ClearValue(TargetRotationKey.SelectedKeyName);
    }

    return EBTNodeResult::Succeeded;
}


uint16 UEMRBTTask_SnapToTarget::GetInstanceMemorySize() const
{
    return sizeof(FSnapToTargetMemory);
}


void UEMRBTTask_SnapToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
    AAIController* AIController = OwnerComp.GetAIOwner();

    if (!BlackboardComponent || !AIController || !NodeMemory)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    APawn* ControlledPawn = AIController->GetPawn();
    UWorld* World = ControlledPawn ? ControlledPawn->GetWorld() : nullptr;

    FSnapToTargetMemory* Memory = reinterpret_cast<FSnapToTargetMemory*>(NodeMemory);

    if (!ControlledPawn || !World || !Memory->bInitialized)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    const float ElapsedTime = World->GetTimeSeconds() - Memory->StartTime;
    const float Alpha = FMath::Clamp(ElapsedTime / FMath::Max(SnapLerpDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);

    const FVector NewLocation = FMath::Lerp(Memory->StartLocation, Memory->TargetLocation, Alpha);
    const FQuat NewRotation = FQuat::Slerp(Memory->StartRotation.Quaternion(), Memory->TargetRotation.Quaternion(), Alpha);

    ControlledPawn->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);

    if (Alpha >= 1.f - KINDA_SMALL_NUMBER)
    {
        const bool bTeleported = ControlledPawn->TeleportTo(Memory->TargetLocation, Memory->TargetRotation, false, true);
        if (!bTeleported)
        {
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
        }

        AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetActorKey.SelectedKeyName));
        ApplyPatientWaitingSeatArrival(TargetActor, ControlledPawn);

        if (AEMRAIController* EMRAI = Cast<AEMRAIController>(AIController))
        {
            EMRAI->ResetBlackboardState();
        }
        else
        {
            BlackboardComponent->ClearValue(TargetActorKey.SelectedKeyName);
            BlackboardComponent->ClearValue(TargetLocationKey.SelectedKeyName);
            BlackboardComponent->ClearValue(TargetRotationKey.SelectedKeyName);
        }

        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}
