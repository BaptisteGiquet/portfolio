
#include "GAS/Abilities/MobaGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GAS/MobaGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"

UMobaGameplayAbility::UMobaGameplayAbility()
{
	ActivationBlockedTags.AddTag(MobaStatusTags::Status_Dead);
	ActivationBlockedTags.AddTag(MobaStatusTags::Status_Stun);
}



bool UMobaGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const FGameplayAbilitySpec* AbilitySpec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	if (AbilitySpec && AbilitySpec->Level <= 0)
	{
		return false;
	}
	
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}





UAnimInstance* UMobaGameplayAbility::GetAvatarAnimInstance() const
{
	const USkeletalMeshComponent* AvatarSkeletalMeshComponent = GetOwningComponentFromActorInfo();
	if (!AvatarSkeletalMeshComponent)
	{
		return nullptr;
	}
		
	return AvatarSkeletalMeshComponent->GetAnimInstance();	
}




TArray<FHitResult> UMobaGameplayAbility::GetHitResultFromSweepTargetData(const FGameplayAbilityTargetDataHandle& LocationTargetData, const ETeamAttitude::Type TeamAttitude, const float SweepRadius, const bool bDrawDebug, const bool bIgnoreAvatar) const
{
	TArray<FHitResult> OutResults;
	TSet<AActor*> HitActors;
	
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const IGenericTeamAgentInterface* AvatarTeamInterface = Cast<IGenericTeamAgentInterface>(AvatarActor);
	if (!AvatarTeamInterface)
	{
		OutResults;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	
	TArray<AActor*> ActorsToIgnore;
	if (bIgnoreAvatar && AvatarActor)
	{
		ActorsToIgnore.Add(AvatarActor);
	}
	
	
	// If bDrawDebug true, then we drawdebug for duration
	const EDrawDebugTrace::Type DebugDrawType = bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
	
	
	for (const TSharedPtr<FGameplayAbilityTargetData>& LocationData : LocationTargetData.Data)
	{
		// This get the SourceLocation specified in AN_SendTargetsPositions, FGameplayAbilityTargetData_LocationInfo override the GetOrigin(), passing his SourceLocation to it.
		const FVector StartLocation = LocationData->GetOrigin().GetLocation();
		const FVector EndLocation = LocationData->GetEndPoint();
		
		TArray<FHitResult> Results;
		UKismetSystemLibrary::SphereTraceMultiForObjects(this, StartLocation, EndLocation, SweepRadius, ObjectTypes, false, ActorsToIgnore, DebugDrawType, Results, false);
		
		for (const FHitResult& Hit : Results)
		{
			AActor* ActorHitBySweep = Hit.GetActor();
			if (!ActorHitBySweep || HitActors.Contains(ActorHitBySweep))
			{
				continue;
			}
			
			// We compare Avatar and HitActor TeamAttitude to see if they are enemy, if not we don't hit them
			const IGenericTeamAgentInterface* ActorHitTeamInterface = Cast<IGenericTeamAgentInterface>(ActorHitBySweep);
			if (!ActorHitTeamInterface)
			{
				continue;
			}
			
			const ETeamAttitude::Type ActorHitTeamAttitude = AvatarTeamInterface->GetTeamAttitudeTowards(*ActorHitBySweep);
			if (ActorHitTeamAttitude == ETeamAttitude::Hostile)
			{
				HitActors.Add(ActorHitBySweep);
				OutResults.Add(Hit);
			}
		}
	}
	return OutResults;
}




void UMobaGameplayAbility::ApplyGameplayEffectToHitResultActor(const FHitResult& InHitResult, const TSubclassOf<UGameplayEffect>& InGameplayEffectClass, const int32 AbilityLevel)
{
	const FGameplayEffectSpecHandle GameplayEffectSpecHandle = MakeOutgoingGameplayEffectSpec(InGameplayEffectClass, AbilityLevel);
	if (!GameplayEffectSpecHandle.IsValid()) return;

	AActor* HitActor = InHitResult.GetActor();
	if (!IsValid(HitActor)) return;
	
	FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(HitActor);
	if (TargetDataHandle.Num() == 0) return;
	
	FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext(GetCurrentAbilitySpecHandle(), CurrentActorInfo);
	EffectContextHandle.AddHitResult(InHitResult);

	GameplayEffectSpecHandle.Data->SetContext(EffectContextHandle);

	ApplyGameplayEffectSpecToTarget(GetCurrentAbilitySpecHandle(), CurrentActorInfo, CurrentActivationInfo, GameplayEffectSpecHandle, TargetDataHandle);
}




void UMobaGameplayAbility::PushSelf(const FVector& PushVelocity)
{
	ACharacter* OwningAvatarCharacter = GetAvatarCharacter();
	if (OwningAvatarCharacter)
	{
		OwningAvatarCharacter->LaunchCharacter(PushVelocity, true, true);
	}
}




void UMobaGameplayAbility::PushTarget(AActor* Target, const FGameplayTag PushEventTag, const FGameplayEventData EventData)
{
	if (!IsValid(Target)) return;
	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, PushEventTag, EventData);
}


void UMobaGameplayAbility::PlayMontageLocally(UAnimMontage* MontageToPlay)
{
	UAnimInstance* AvatarAnimInstance = GetAvatarAnimInstance();
	if (!AvatarAnimInstance && !MontageToPlay) { return; }

	if (!AvatarAnimInstance->Montage_IsPlaying(MontageToPlay))
	{
		AvatarAnimInstance->Montage_Play(MontageToPlay);
	}
}


void UMobaGameplayAbility::StopMontageAfterCurrentSection(UAnimMontage* MontageToStop)
{
	UAnimInstance* AvatarAnimInstance = GetAvatarAnimInstance();
	if (!AvatarAnimInstance && !MontageToStop) { return; }

	const FName CurrentSectionName = AvatarAnimInstance->Montage_GetCurrentSection(MontageToStop);
	AvatarAnimInstance->Montage_SetNextSection(CurrentSectionName, NAME_None, MontageToStop);
	
}


AActor* UMobaGameplayAbility::GetAimTarget(const float AimDistance, const ETeamAttitude::Type TeamAttitude) const
{
	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if (!OwnerAvatarActor) { return nullptr; }
	
	APlayerController* OwnerPlayerController = Cast<APlayerController>(OwnerAvatarActor->GetInstigatorController());
	if (!OwnerPlayerController) { return nullptr; }

	FVector OwnerViewLocation = FVector::ZeroVector;
	FRotator OwnerViewRotation = FRotator::ZeroRotator;
	//OwnerAvatarActor->GetActorEyesViewPoint(OwnerViewLocation, OwnerViewRotation);
	OwnerPlayerController->GetPlayerViewPoint(OwnerViewLocation, OwnerViewRotation);
	
	FVector AimPointEnd = OwnerViewLocation + (OwnerViewRotation.Vector() * AimDistance);
	
	FCollisionObjectQueryParams CollisionObjectParams;
	CollisionObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(OwnerAvatarActor);
	
	
	//DrawDebugLine(GetWorld(), OwnerViewLocation, AimPointEnd, FColor::Red, false, 2.f, 0, 3.f);	
	
	
	TArray<FHitResult> HitResults;
	if (GetWorld()->LineTraceMultiByObjectType(HitResults, OwnerViewLocation, AimPointEnd, CollisionObjectParams, CollisionParams))
	{
		for (FHitResult& HitResult : HitResults)
		{
			if (IsActorTeamAttitude(HitResult.GetActor(), TeamAttitude))
			{
				return HitResult.GetActor();
			}
		}
	}
	return nullptr;
}


bool UMobaGameplayAbility::IsActorTeamAttitude(const AActor* OtherActor, ETeamAttitude::Type TeamAttitude) const
{
	if (!OtherActor) { return false; }
	
	IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(GetAvatarActorFromActorInfo());
	if (OwnerTeamInterface)
	{
		return OwnerTeamInterface->GetTeamAttitudeTowards(*OtherActor) == TeamAttitude;
	}

	return false;
}


FGenericTeamId UMobaGameplayAbility::GetOwnerTeamID() const
{
	const IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(GetAvatarActorFromActorInfo());
	if (OwnerTeamInterface)
	{
		return OwnerTeamInterface->GetGenericTeamId();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MobaGameplayAbility] NoTeam"))
		return FGenericTeamId::NoTeam;
	}
}


ACharacter* UMobaGameplayAbility::GetAvatarCharacter()
{
	if (!AvatarCharacter)
	{
		AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	}

	return AvatarCharacter;
}


void UMobaGameplayAbility::SendLocalGameplayEvent(const FGameplayTag& EventTag, const FGameplayEventData& EventData)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (OwnerASC)
	{
		OwnerASC->HandleGameplayEvent(EventTag, &EventData);
	}
}
