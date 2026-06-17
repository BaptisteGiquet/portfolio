

#include "MobaTargetActor_Line.h"

#include "NiagaraComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "LOD/MobaCustomCollisionChannels.h"
#include "Net/UnrealNetwork.h"


AMobaTargetActor_Line::AMobaTargetActor_Line()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	SetRootComponent(RootComp);

	TargetActorVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFX"));
	TargetActorVFX->SetupAttachment(GetRootComponent());

	TargetEndDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SphereDetection"));
	TargetEndDetectionSphere->SetupAttachment(GetRootComponent());
	TargetEndDetectionSphere->SetCollisionResponseToChannel(ECC_SpringArm, ECR_Ignore);

	ShouldProduceTargetDataOnServer = true;
}


void AMobaTargetActor_Line::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateTargetTrace();	
}


void AMobaTargetActor_Line::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, AvatarActor, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, OwnerTeamID, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, TargetRange, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CylinderDetectionRadius, COND_None, REPNOTIFY_Always)
}


void AMobaTargetActor_Line::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	IGenericTeamAgentInterface::SetGenericTeamId(TeamID);

	OwnerTeamID = TeamID;
}


FGenericTeamId AMobaTargetActor_Line::GetGenericTeamId() const
{
	return OwnerTeamID;
}


void AMobaTargetActor_Line::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	if (!OwningAbility) { return; }

	AvatarActor = OwningAbility->GetAvatarActorFromActorInfo();
	if (!AvatarActor) { return; }

	GetWorldTimerManager().SetTimer(PeriodicalTargetingTimerHandle, this, &ThisClass::DoTargetCheckAndReport, TargetingFrequency, true);
}

void AMobaTargetActor_Line::BeginDestroy()
{
	if (GetWorld() && PeriodicalTargetingTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(PeriodicalTargetingTimerHandle);
	}

	Super::BeginDestroy();

}


void AMobaTargetActor_Line::BeginPlay()
{
	Super::BeginPlay();
	
}


void AMobaTargetActor_Line::DoTargetCheckAndReport()
{
	if (!HasAuthority()) { return; }

	TSet<AActor*> OverlappingActorSet;
	TargetEndDetectionSphere->GetOverlappingActors(OverlappingActorSet);

	TArray<TWeakObjectPtr<AActor>> ValidTargets;
	for (AActor* OverlappingActor : OverlappingActorSet)
	{
		if (ShouldReportActorAsTarget(OverlappingActor))
		{
			ValidTargets.Add(OverlappingActor);
		}
	}

	if (ValidTargets.IsEmpty()) { return; }
	
	FGameplayAbilityTargetDataHandle TargetDataHandle;
	FGameplayAbilityTargetData_ActorArray* ActorArray = new FGameplayAbilityTargetData_ActorArray;
	ActorArray->SetActors(ValidTargets);
	TargetDataHandle.Add(ActorArray);

	TargetDataReadyDelegate.Broadcast(TargetDataHandle);
}


bool AMobaTargetActor_Line::ShouldReportActorAsTarget(const AActor* ActorToCheck) const
{
	if (!ActorToCheck || ActorToCheck == AvatarActor || ActorToCheck == this) { return false; }

	
	if (GetTeamAttitudeTowards(*ActorToCheck) == ETeamAttitude::Hostile)
	{
		return true;
	}
	else
	{
		return false;
	}
}


void AMobaTargetActor_Line::UpdateTargetTrace()
{
	FVector ViewLocation = GetActorLocation();
	FRotator ViewRotation = GetActorRotation();
	if (AvatarActor)
	{
		AvatarActor->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}

	const FVector LookEndPoint = ViewLocation + ViewRotation.Vector() * 100000.f;
	const FRotator LookRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), LookEndPoint);
	SetActorRotation(LookRotation);

	FVector SweepEndLocation = GetActorLocation() + LookRotation.Vector() * TargetRange;

	TArray<FHitResult> HitResults;
	CreateSweepMulti(SweepEndLocation, HitResults);

	FVector LineEndLocation = SweepEndLocation;
	float TargetActorLength = TargetRange;

	for (FHitResult HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		
		if (!HitActor) { continue; }
		if (GetTeamAttitudeTowards(*HitActor) == ETeamAttitude::Friendly) { continue; }

		// Update TargetActor length when we hit something that is not Friendly
		LineEndLocation = HitResult.ImpactPoint;
		TargetActorLength = FVector::Distance(GetActorLocation(), LineEndLocation);
		break;
	}

	TargetEndDetectionSphere->SetWorldLocation(LineEndLocation);
	if (TargetActorVFX)
	{
		TargetActorVFX->SetVariableFloat(TargetActorFXLengthParamName, TargetActorLength / 100 /* Niagara Length is in meters */);
	}
}


void AMobaTargetActor_Line::CreateSweepMulti(FVector SweepEndLocation, TArray<FHitResult>& HitResults)
{
	FCollisionShape CollisionShape;
	CollisionShape.MakeSphere(CylinderDetectionRadius);
	
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(AvatarActor);
	CollisionQueryParams.AddIgnoredActor(this);

	FCollisionResponseParams CollisionResponseParams(ECR_Overlap);
	GetWorld()->SweepMultiByChannel(HitResults, GetActorLocation(), SweepEndLocation, FQuat::Identity, ECC_WorldDynamic, CollisionShape, CollisionQueryParams, CollisionResponseParams);
}


void AMobaTargetActor_Line::ConfigureTargetSetting(const float InTargetRange, const float InCylinderDetectionRadius, const float InTargetingFrequency, const FGenericTeamId InOwnerTeamID, const bool InbShouldDrawDebug)
{
	TargetRange = InTargetRange;
	CylinderDetectionRadius = InCylinderDetectionRadius;
	TargetingFrequency = InTargetingFrequency;
	bShouldDrawDebug = InbShouldDrawDebug;
	SetGenericTeamId(InOwnerTeamID);
}





