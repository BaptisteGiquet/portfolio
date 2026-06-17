

#include "MobaGameplayAbility_Shoot.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS/MobaAbilitySystemStatics.h"
#include "GAS/MobaGameplayTags.h"
#include "GAS/MobaProjectileActor.h"


UMobaGameplayAbility_Shoot::UMobaGameplayAbility_Shoot()
{
	ActivationOwnedTags.AddTag(MobaAbilityAimingEventTags::Ability_Aiming);
	ActivationOwnedTags.AddTag(MobaStatusTags::Status_Crosshair);
}


void UMobaGameplayAbility_Shoot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	
	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_WaitGameplayEvent* WaitStartShootingEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MobaGameplayAbilityPressedTags::Ability_BasicAttack_Pressed);
		WaitStartShootingEvent->EventReceived.AddDynamic(this, &ThisClass::StartShooting);
		WaitStartShootingEvent->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitStopShootingEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MobaGameplayAbilityReleasedTags::Ability_BasicAttack_Released);
		WaitStopShootingEvent->EventReceived.AddDynamic(this, &ThisClass::StopShooting);
		WaitStopShootingEvent->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitShootProjectileEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MobaPhaseShootTags::Ability_Phase_Shoot, nullptr, false, false);
		WaitShootProjectileEvent->EventReceived.AddDynamic(this, &ThisClass::ShootProjectile);
		WaitShootProjectileEvent->ReadyForActivation();
	}
}


void UMobaGameplayAbility_Shoot::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}


void UMobaGameplayAbility_Shoot::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AimTargetASC)
	{
		AimTargetASC->RegisterGameplayTagEvent(MobaStatusTags::Status_Dead).RemoveAll(this);
		AimTargetASC = nullptr;	
	}

	SendLocalGameplayEvent(MobaTargetTags::Target_Updated, FGameplayEventData());
	
	StopShooting(FGameplayEventData());

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UMobaGameplayAbility_Shoot::StartShooting(FGameplayEventData EventData)
{
	if (!ShootMontage) { return; }

	if (HasAuthority(&CurrentActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayShootMontageAndWait = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ShootMontage);
		PlayShootMontageAndWait->ReadyForActivation();	
	}
	else
	{
		PlayMontageLocally(ShootMontage);	
	}

	FindAimTarget();
	StartAimTargetCheckTimer();
}


void UMobaGameplayAbility_Shoot::StopShooting(FGameplayEventData EventData)
{
	if (!ShootMontage) { return; }

	StopMontageAfterCurrentSection(ShootMontage);
	StopAimTargetCheckTimer();
}


void UMobaGameplayAbility_Shoot::ShootProjectile(FGameplayEventData EventData)
{
	if (HasAuthority(&CurrentActivationInfo))
	{
		AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
		if (!OwnerAvatarActor) { return; }
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerAvatarActor;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		FVector SocketLocation = OwnerAvatarActor->GetActorLocation();
		USkeletalMeshComponent* MeshComp = GetOwningComponentFromActorInfo();
		if (!MeshComp) { return; }
		
		TArray<FName> OutNames;
		UGameplayTagsManager::Get().SplitGameplayTagFName(EventData.EventTag,OutNames);
		
		if (OutNames.Num() != 0)
		{
			FName SocketName = OutNames.Last();
			SocketLocation = MeshComp->GetSocketLocation(SocketName);
		}

		AMobaProjectileActor* Projectile = GetWorld()->SpawnActor<AMobaProjectileActor>(ProjectileClass, SocketLocation, OwnerAvatarActor->GetActorRotation(), SpawnParams);
		if (Projectile)
		{
			Projectile->ShootProjectile(ShootProjectileSpeed, ShootProjectileMaxDistance, GetAimTargetIfValid(), GetOwnerTeamID(), MakeOutgoingGameplayEffectSpec(ProjectileHitEffectClass, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo)));
		}
	}
}


AActor* UMobaGameplayAbility_Shoot::GetAimTargetIfValid() const
{
	if (HasValidTarget())
	{
		return AimTarget;
	}
	else
	{
		return nullptr;
	}
}


void UMobaGameplayAbility_Shoot::FindAimTarget()
{
	if (HasValidTarget()) { return; }

	if (AimTargetASC)
	{
		AimTargetASC->RegisterGameplayTagEvent(MobaStatusTags::Status_Dead).RemoveAll(this);
		AimTargetASC = nullptr;	
	}

	AimTarget = GetAimTarget(ShootProjectileMaxDistance, ETeamAttitude::Hostile);
	if (AimTarget)
	{
		AimTargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AimTarget);
		if (AimTargetASC)
		{
			AimTargetASC->RegisterGameplayTagEvent(MobaStatusTags::Status_Dead).AddUObject(this, &ThisClass::TargetDeadTagUpdated);
		}
	}

	FGameplayEventData EventData;
	EventData.Target = AimTarget;
	SendLocalGameplayEvent(MobaTargetTags::Target_Updated, EventData);
}


void UMobaGameplayAbility_Shoot::StartAimTargetCheckTimer()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	World->GetTimerManager().SetTimer(AimTargetCheckTimerHandle, this, &ThisClass::FindAimTarget, AimTargetCheckFrequency, true);
}


void UMobaGameplayAbility_Shoot::StopAimTargetCheckTimer()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	World->GetTimerManager().ClearTimer(AimTargetCheckTimerHandle);
}


bool UMobaGameplayAbility_Shoot::HasValidTarget() const
{
	if (!AimTarget || UMobaAbilitySystemStatics::IsActorDead(AimTarget)) { return false; }
	
	if (!IsTargetInRange()) { return false; }

	return true;
}


bool UMobaGameplayAbility_Shoot::IsTargetInRange() const
{
	if (!AimTarget) { return false; }

	const float Distance = FVector::Distance(AimTarget->GetActorLocation(), GetAvatarActorFromActorInfo()->GetActorLocation());

	return Distance <= ShootProjectileMaxDistance;
}


void UMobaGameplayAbility_Shoot::TargetDeadTagUpdated(const FGameplayTag DeadTag, int32 DeadTagCount)
{
	if (DeadTagCount > 0)
	{
		FindAimTarget();
	}
}