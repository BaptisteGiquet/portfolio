

#include "Animation/MobaAnimNotify_SendTargetsPositions.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayTagsManager.h"
#include "Kismet/KismetSystemLibrary.h"


DEFINE_LOG_CATEGORY_STATIC(LogSendTargetsGroup, Log, All);


void UMobaAnimNotify_SendTargetsPositions::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		UE_LOG(LogSendTargetsGroup, Verbose, TEXT("[%s] MeshComp is nullptr"), *GetName());
		return;
	}

	const int32 NumberOfSockets = TargetSocketNames.Num();
	if (NumberOfSockets <= 1)
	{
		UE_LOG(LogSendTargetsGroup, Verbose, TEXT("[%s] Need at least 2 sockets to sweep"), *GetName());
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	const UAbilitySystemComponent* OwnerActorAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor); 
	if (!OwnerActor)
	{
		UE_LOG(LogSendTargetsGroup, Verbose, TEXT("[%s] Owner actor is nullptr"), *GetName());
		return;
	}

	if (!EventTag.IsValid())
	{
		UE_LOG(LogSendTargetsGroup, Verbose, TEXT("[%s] EventTag not valid"), *GetName());
		return;
	}

	if (!OwnerActorAbilitySystemComponent)
	{
		UE_LOG(LogSendTargetsGroup, Verbose, TEXT("[%s] Owner ASC is nullptr"), *GetName());
		return;
	}
	
	
	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = OwnerActor;
	EventData.Target = OwnerActor;

	TSet<AActor*> HitActors;
	const IGenericTeamAgentInterface* OwnerTeamAgentInterface = Cast<IGenericTeamAgentInterface>(OwnerActor);
	if (!OwnerTeamAgentInterface) { return; }
	
	
	// We create a source and target location for each socket (except first), then we add it in our EventData that will be sent to GA_Combo for sweep
	for (int32 SocketIndex = 1; SocketIndex < NumberOfSockets; ++SocketIndex)
	{
		const FName& PreviousSocket = TargetSocketNames[SocketIndex - 1];
		const FName& CurrentSocket = TargetSocketNames[SocketIndex];

		const FVector StartLocation = MeshComp->GetSocketLocation(PreviousSocket);
		const FVector EndLocation = MeshComp->GetSocketLocation(CurrentSocket);


		TArray<AActor*> ActorsToIgnore;
		if (bIgnoreOwner)
		{
			ActorsToIgnore.Add(OwnerActor);	
		}
		
		FCollisionObjectQueryParams CollisionParam;
		CollisionParam.AddObjectTypesToQuery(ECC_Pawn);

		FCollisionShape CollisionShape;
		CollisionShape.SetSphere(SweepSphereRadius);
		
		TArray<FHitResult> SphereTraceHitResults;
		
		if (!MeshComp->GetWorld()) { return; }
		
		MeshComp->GetWorld()->SweepMultiByObjectType(SphereTraceHitResults, StartLocation, EndLocation, FQuat::Identity, CollisionParam, CollisionShape);
		

		for (const FHitResult& HitResult : SphereTraceHitResults)
		{
			AActor* HitResultActor = HitResult.GetActor();
			if (!HitResultActor) { continue; }
			
			if (HitActors.Contains(HitResultActor)) { continue; }
			
			HitActors.Add(HitResultActor);
			ActorsToIgnore.Add(HitResultActor);
			
			if (OwnerTeamAgentInterface->GetTeamAttitudeTowards(*HitResultActor) == ETeamAttitude::Hostile)
			{
				FGameplayAbilityTargetData_SingleTargetHit* TargetHit = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);
				EventData.TargetData.Add(TargetHit);

				SendLocalGameplayCue(HitResult);
			}
		}
	}
	if (OwnerActor->HasAuthority())
	{
		// Send Start and End location for each sweep that will be made in Gameplay Ability
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, EventData);	
	}
}



void UMobaAnimNotify_SendTargetsPositions::SendLocalGameplayCue(const FHitResult& HitResult) const
{
	FGameplayCueParameters CueParam;
	CueParam.Location = HitResult.ImpactPoint;
	CueParam.Normal = HitResult.ImpactNormal;

	for (const FGameplayTag& GampelayCueTag : GameplayCueTagsToTrigger)
	{
		UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(HitResult.GetActor(), GampelayCueTag, EGameplayCueEvent::Executed, CueParam);
	}
}



FString UMobaAnimNotify_SendTargetsPositions::GetNotifyName_Implementation() const
{
	if (EventTag.IsValid())
	{
		TArray<FName> GameplayTagNames;
		UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, GameplayTagNames);
		return GameplayTagNames.Last().ToString();
	}
	return FString("AN_SendGameplayEvent: GameplayEventTag is invalid");
}

