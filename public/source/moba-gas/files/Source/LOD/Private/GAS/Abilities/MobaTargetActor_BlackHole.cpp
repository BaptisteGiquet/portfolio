

#include "MobaTargetActor_BlackHole.h"

#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "Particles/ParticleSystemComponent.h"


AMobaTargetActor_BlackHole::AMobaTargetActor_BlackHole()
{
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	SetRootComponent(RootComp);

	DetectionSphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphereComp"));
	DetectionSphereComp->SetupAttachment(GetRootComponent());
	DetectionSphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	DetectionSphereComp->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::ActorEnterBlackHoleRange);
	DetectionSphereComp->OnComponentEndOverlap.AddDynamic(this, &ThisClass::ActorLeaveBlackHoleRange);

	VFXComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("VFXComp"));
	VFXComp->SetupAttachment(GetRootComponent());

	bReplicates = true;
	ShouldProduceTargetDataOnServer = true;
	PrimaryActorTick.bCanEverTick = true;
}


void AMobaTargetActor_BlackHole::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, OwnerTeamID)
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, BlackHoleRange, COND_None, REPNOTIFY_Always)
}

void AMobaTargetActor_BlackHole::Destroyed()
{
	UE_LOG(LogTemp, Warning, TEXT("%s Destroyed"), *GetNameSafe(this))
	Super::Destroyed();
}


void AMobaTargetActor_BlackHole::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	IGenericTeamAgentInterface::SetGenericTeamId(TeamID);

	OwnerTeamID = TeamID;
}


FGenericTeamId AMobaTargetActor_BlackHole::GetGenericTeamId() const
{
	return OwnerTeamID;
}


void AMobaTargetActor_BlackHole::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(BlackHoleDurationTimerHandle, this, &ThisClass::StopBlackHole, BlackHoleDuration);
	}
}


void AMobaTargetActor_BlackHole::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (HasAuthority())
	{
		
		for (TPair<AActor*, UNiagaraComponent*>& TargetPair : ActorsInRangeMap)
		{
			AActor* Target = TargetPair.Key;
			UNiagaraComponent* NiagaraComponent = TargetPair.Value;

			FVector PullDirection = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
			Target->SetActorLocation(Target->GetActorLocation() + PullDirection * BlackHolePullSpeed * DeltaSeconds);

			if (NiagaraComponent)
			{
				NiagaraComponent->SetVariablePosition(BlackHoleVFXOriginParamName, VFXComp->GetComponentLocation());
			}
		}
	}
}


void AMobaTargetActor_BlackHole::ConfirmTargetingAndContinue()
{
	StopBlackHole();
}


void AMobaTargetActor_BlackHole::CancelTargeting()
{
	StopBlackHole();
	Super::CancelTargeting();

}


void AMobaTargetActor_BlackHole::ConfigureBlackHole(const float InBlackHoleRange, const float InBlackHolePullSpeed, const float InBlackHoleDuration, const FGenericTeamId& InTeamID)
{
	if (!HasAuthority()) { return; }
	BlackHoleRange = InBlackHoleRange;
	BlackHolePullSpeed = InBlackHolePullSpeed;
	BlackHoleDuration = InBlackHoleDuration;
	SetGenericTeamId(InTeamID);
	
	if (DetectionSphereComp)
	{
		DetectionSphereComp->SetSphereRadius(BlackHoleRange);
	}
}


void AMobaTargetActor_BlackHole::ActorEnterBlackHoleRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryAddTarget(OtherActor);
}


void AMobaTargetActor_BlackHole::ActorLeaveBlackHoleRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	RemoveTarget(OtherActor);
}


void AMobaTargetActor_BlackHole::OnRep_BlackHoleRange()
{
	DetectionSphereComp->SetSphereRadius(BlackHoleRange);
}


void AMobaTargetActor_BlackHole::TryAddTarget(AActor* TargetToAdd)
{
	if (!TargetToAdd || ActorsInRangeMap.Contains(TargetToAdd)) { return; }

	if (GetTeamAttitudeTowards(*TargetToAdd) != ETeamAttitude::Hostile) { return; }

	UNiagaraComponent* NiagaraComponent = nullptr;
	if (BlackHoleLinkVFX)
	{
		NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(BlackHoleLinkVFX, TargetToAdd->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::Type::KeepRelativeOffset, false);
		if (NiagaraComponent)
		{
			NiagaraComponent->SetVariablePosition(BlackHoleVFXOriginParamName, VFXComp->GetComponentLocation());
		}
	}

	ActorsInRangeMap.Add(TargetToAdd, NiagaraComponent);
}


void AMobaTargetActor_BlackHole::RemoveTarget(AActor* TargetToRemove)
{
	if (!TargetToRemove) { return; }

	if (ActorsInRangeMap.Contains(TargetToRemove))
	{
		UNiagaraComponent* NiagaraComponent = nullptr;
		ActorsInRangeMap.RemoveAndCopyValue(TargetToRemove, NiagaraComponent);

		if (IsValid(NiagaraComponent))
		{
			NiagaraComponent->DestroyComponent();
		}
	}
}


void AMobaTargetActor_BlackHole::StopBlackHole()
{
	TArray<TWeakObjectPtr<AActor>> FinalTargets;
	for (TPair<AActor*, UNiagaraComponent*>& TargetPair : ActorsInRangeMap)
	{
		FinalTargets.Add(TargetPair.Key);
		UNiagaraComponent* NiagaraComponent = TargetPair.Value;
		if (IsValid(NiagaraComponent))
		{
			NiagaraComponent->DestroyComponent();
		}
	}

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	FGameplayAbilityTargetData_ActorArray* TargetActorArray = new FGameplayAbilityTargetData_ActorArray;

	TargetActorArray->SetActors(FinalTargets);
	TargetDataHandle.Add(TargetActorArray);

	FGameplayAbilityTargetData_SingleTargetHit* BlowupLocation = new FGameplayAbilityTargetData_SingleTargetHit;
	BlowupLocation->HitResult.ImpactPoint = GetActorLocation();
	TargetDataHandle.Add(BlowupLocation);

	TargetDataReadyDelegate.Broadcast(TargetDataHandle);
}