

#include "GAS/MobaTargetActor_PickGroundTarget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Components/DecalComponent.h"
#include "Engine/OverlapResult.h"
#include "LOD/MobaCustomCollisionChannels.h"

AMobaTargetActor_PickGroundTarget::AMobaTargetActor_PickGroundTarget()
{
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent")));
	
	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
	DecalComponent->SetupAttachment(GetRootComponent());
}




void AMobaTargetActor_PickGroundTarget::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!PrimaryPC || !PrimaryPC->IsLocalPlayerController()) { return; }

	SetActorLocation(GetTargetPoint());
}




void AMobaTargetActor_PickGroundTarget::ConfirmTargetingAndContinue()
{
	
	TArray<FOverlapResult> OverlapResults;
	const FVector TargetPosition = GetActorLocation();
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(GetInstigator());
	
	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(TargetingAreaRadius);
	
	GetWorld()->OverlapMultiByObjectType(OverlapResults, TargetPosition, FQuat::Identity, ObjectQueryParams, CollisionShape, CollisionQueryParams);

	TSet<AActor*> TargetedActors;

	const IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(OwningAbility->GetAvatarActorFromActorInfo());	

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* OverlappedActor = OverlapResult.GetActor();
		if (!OverlappedActor) { continue; }
		
		if (!OwningAbility) { continue; }
		
		if (!OwnerTeamInterface) { continue; }
		
		const ETeamAttitude::Type OwnerAttitudeTowardsActor = OwnerTeamInterface->GetTeamAttitudeTowards(*OverlappedActor);
		if (OwnerAttitudeTowardsActor == ETeamAttitude::Friendly && !bShouldTargetFriendly) { continue; }
		if (OwnerAttitudeTowardsActor == ETeamAttitude::Hostile && !bShouldTargetHostile) { continue; }
		
		TargetedActors.Add(OverlappedActor);
	}

	FGameplayAbilityTargetDataHandle TargetDataHandle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActorArray(TargetedActors.Array(), false);

	
	FGameplayAbilityTargetData_SingleTargetHit* HitLocation = new FGameplayAbilityTargetData_SingleTargetHit;
	HitLocation->HitResult.ImpactPoint = GetActorLocation();
	HitLocation->HitResult.bBlockingHit = true;
	HitLocation->HitResult.Location = GetActorLocation();

	TargetDataHandle.Add(HitLocation);
	
	//Trigger TargetDataTask ValidData delegate
	TargetDataReadyDelegate.Broadcast(TargetDataHandle);
}




void AMobaTargetActor_PickGroundTarget::SetTargetingAreaRadius(const float NewRadius)
{
	TargetingAreaRadius = NewRadius;
	DecalComponent->DecalSize = FVector{ NewRadius };
}



FVector AMobaTargetActor_PickGroundTarget::GetTargetPoint() const
{
	if (!PrimaryPC || !PrimaryPC->IsLocalPlayerController()) { return GetActorLocation(); }


	FHitResult HitResult;
	
	FVector PlayerViewLocation = FVector::ZeroVector;
	FRotator PlayerViewRotation = FRotator::ZeroRotator;
	PrimaryPC->GetPlayerViewPoint(PlayerViewLocation, PlayerViewRotation);
	
	const FVector TraceEnd = PlayerViewLocation + PlayerViewRotation.Vector() * TargetingMaxRange;  
	
	GetWorld()->LineTraceSingleByChannel(HitResult, PlayerViewLocation, TraceEnd, ECC_Target);

	if (!HitResult.bBlockingHit)
	{
		//In case we aim too high we take the ground vertical point of the TraceEnd
		const FVector TraceEndGroundPoint = TraceEnd + FVector::DownVector * TNumericLimits<float>::Max();
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceEnd, TraceEndGroundPoint, ECC_Target);
	}

	//Still no hit
	if (!HitResult.bBlockingHit) { return GetActorLocation(); }

	if (bShouldDrawDebug)
	{
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, TargetingAreaRadius, 24, FColor::Red);
	}
	
	return HitResult.ImpactPoint;
}
