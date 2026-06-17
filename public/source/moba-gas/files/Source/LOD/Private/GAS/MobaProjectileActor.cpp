

#include "MobaProjectileActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "Net/UnrealNetwork.h"


AMobaProjectileActor::AMobaProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	
	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	SetRootComponent(RootComp);

	bReplicates = true;
	
}


void AMobaProjectileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		if (Target)
		{
			MoveDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		}
	}

	SetActorLocation(GetActorLocation() + MoveDirection * ProjectileSpeed * DeltaSeconds);
}


void AMobaProjectileActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, TeamID, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MoveDirection, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, ProjectileSpeed, COND_None, REPNOTIFY_Always)
}


void AMobaProjectileActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!OtherActor || OtherActor == GetOwner() || GetTeamAttitudeTowards(*OtherActor) != ETeamAttitude::Hostile) { return; }

	UAbilitySystemComponent* OtherActorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (OtherActorASC)
	{
		if (HasAuthority() && HitEffectSpecHandle.IsValid())
		{
			OtherActorASC->ApplyGameplayEffectSpecToSelf(*HitEffectSpecHandle.Data.Get());
			GetWorldTimerManager().ClearTimer(ShootTimerHandle);
		}

		FHitResult HitResult;
		HitResult.ImpactPoint = GetActorLocation();
		HitResult.ImpactNormal = GetActorForwardVector();

		SendLocalGameplayCue(OtherActor, HitResult);
		
		Destroy();
	}
	
}


void AMobaProjectileActor::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamID = NewTeamID;
}


void AMobaProjectileActor::ShootProjectile(float InSpeed, float InMaxDistance, AActor* InTarget, FGenericTeamId InTeamID, FGameplayEffectSpecHandle InHitEffectHandle)
{
	ProjectileSpeed = InSpeed;
	Target = InTarget;

	FRotator OwnerViewRotation = FRotator::ZeroRotator;
	SetGenericTeamId(InTeamID);

	if (GetOwner())
	{
		FVector OwnerViewLocation = FVector::ZeroVector;
		GetOwner()->GetActorEyesViewPoint(OwnerViewLocation, OwnerViewRotation);
	}

	MoveDirection = OwnerViewRotation.Vector();
	HitEffectSpecHandle = InHitEffectHandle;

	const float TravelMaxTime = InMaxDistance / InSpeed;
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(ShootTimerHandle, this, &ThisClass::TravelMaxDistanceReached, TravelMaxTime);
	}
}


void AMobaProjectileActor::TravelMaxDistanceReached()
{
	Destroy();	
}


void AMobaProjectileActor::SendLocalGameplayCue(AActor* CueTargetActor, const FHitResult& HitResult)
{
	if (!CueTargetActor || !HitGameplayCueTag.IsValid()) { return; }
	
	FGameplayCueParameters GameplayCueParams;
	GameplayCueParams.Location = HitResult.ImpactPoint;
	GameplayCueParams.Normal = HitResult.ImpactNormal;

	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(CueTargetActor, HitGameplayCueTag, EGameplayCueEvent::Executed, GameplayCueParams);
}


void AMobaProjectileActor::BeginPlay()
{
	Super::BeginPlay();
	
}



