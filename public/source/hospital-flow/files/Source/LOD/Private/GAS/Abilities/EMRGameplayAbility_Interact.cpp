#include "GAS/Abilities/EMRGameplayAbility_Interact.h"


#include "Interfaces/EMRInteractableInterface.h"
#include "Characters/Player/EMRPlayerController.h"
#include "GAS/EMRTags.h"
#include "DrawDebugHelpers.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameFramework/Character.h"

namespace
{
    bool IsExamMachineEventTag(const FGameplayTag& EventTag)
    {
        return EventTag.IsValid() && EventTag.MatchesTag(EMRTags::Machine::Any);
    }
}

UEMRGameplayAbility_Interact::UEMRGameplayAbility_Interact()
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}


void UEMRGameplayAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !IsValid(GetAvatarActorFromActorInfo()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	
    if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
    {
        ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
        if (!Character)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        AEMRPlayerController* PlayerController = Cast<AEMRPlayerController>(GetAvatarActorFromActorInfo()->GetInstigatorController());
        if (!PlayerController)
        {
            UE_LOG(LogTemp, Warning, TEXT("[GA_Interact] No PlayerController (Owner=%s Avatar=%s)"),
                ActorInfo ? *GetNameSafe(ActorInfo->OwnerActor.Get()) : TEXT("nullptr"),
                ActorInfo ? *GetNameSafe(ActorInfo->AvatarActor.Get()) : TEXT("nullptr"));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        const FHitResult HitResult = GetHitResultFromSphereTrace(PlayerController);
        AActor* InteractableActor = HitResult.GetActor();
        IEMRInteractableInterface* Interactable = InteractableActor ? Cast<IEMRInteractableInterface>(InteractableActor) : nullptr;

        if (!Interactable)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }

        FGameplayEventData InteractionData = Interactable->Execute_GetInteractionEventData(InteractableActor, GetAvatarActorFromActorInfo());
        if (!IsExamMachineEventTag(InteractionData.EventTag))
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
            return;
        }
        if (HitResult.bBlockingHit)
        {
            InteractionData.TargetData = FGameplayAbilityTargetDataHandle(new FGameplayAbilityTargetData_SingleTargetHit(HitResult));
        }
        SendGameplayEvent(InteractionData.EventTag, InteractionData);

        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


FHitResult UEMRGameplayAbility_Interact::GetHitResultFromSphereTrace(APlayerController* InPlayerController) const
{
    if (!InPlayerController)
    {
        return FHitResult();
    }

	FVector PlayerViewLocation;
	FRotator PlayerViewRotation;
	InPlayerController->GetPlayerViewPoint(PlayerViewLocation, PlayerViewRotation);

    const FVector TraceStart  = PlayerViewLocation;
    const FVector TraceEnd    = TraceStart + (PlayerViewRotation.Vector() * TraceDistance);

    FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceRadius);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GAInteractTrace), false, GetAvatarActorFromActorInfo());
    QueryParams.AddIgnoredActor(GetAvatarActorFromActorInfo());

    TArray<FHitResult> HitResults;
    const bool bHit = GetWorld()->SweepMultiByChannel(HitResults, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility, SphereShape, QueryParams);

    if (bDrawDebug)
    {
        const FColor DebugColor = bHit ? FColor::Green : FColor::Red;
        DrawDebugLine(GetWorld(), TraceStart, TraceEnd, DebugColor, false, 2.f, 0, 2.f);
        DrawDebugSphere(GetWorld(), TraceEnd, TraceRadius, 12, DebugColor, false, 2.f, 0, 1.f);
    }

    if (!bHit)
    {
        return FHitResult();
    }

    FHitResult BestHit;
    float BestScore = -1.f;

    const FVector ViewDirection = PlayerViewRotation.Vector();

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!HitActor)
        {
            continue;
        }

        if (!HitActor->GetClass()->ImplementsInterface(UEMRInteractableInterface::StaticClass()))
        {
            continue;
        }

        if (!IEMRInteractableInterface::Execute_IsInteractableEnabled(HitActor))
        {
            continue;
        }

        const FVector DirectionToHit = (Hit.ImpactPoint - TraceStart).GetSafeNormal();
        const float Dot = FVector::DotProduct(DirectionToHit, ViewDirection);
        const float ClampedDot = FMath::Clamp(Dot, -1.f, 1.f);
        const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDot));

        if (AngleDegrees > MaxInteractAngle)
        {
            continue;
        }

        const float Distance = Hit.Distance;
        const float Score = (1.f - (AngleDegrees / MaxInteractAngle)) + (TraceDistance - Distance);

        if (Score > BestScore)
        {
            BestScore = Score;
            BestHit = Hit;
        }
    }

    return BestHit;
}


