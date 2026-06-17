// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/Weapon.h"

#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Interfaces/HitInterface.h"


AWeapon::AWeapon()
{
	GetHitBox()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetHitBox()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	GetHitBox()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	
	HitBoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("HitBoxTraceStart"));
	HitBoxTraceStart->SetupAttachment(GetRootComponent());
	HitBoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("HitBoxTraceEnd"));
	HitBoxTraceEnd->SetupAttachment(GetRootComponent());
}


void AWeapon::AttachMeshToSocket(USceneComponent* InParent, FName InSocketName)
{
	FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, false);
	GetItemMesh()->AttachToComponent(InParent, TransformRules, InSocketName);
}

void AWeapon::PlayEquipSound()
{
	if (EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquipSound, GetActorLocation(), GetActorRotation());
	}
}

void AWeapon::DisableSphereDetectionCollision()
{
	if (GetOverlapSphereDetection())
	{
		GetOverlapSphereDetection()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWeapon::DeactivateVisualEffect()
{
	if (HoveringVisualEffect)
	{
		HoveringVisualEffect->Deactivate();	
	}
}

void AWeapon::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	SetItemState(EItemState::EIS_Equipped);
	SetOwner(NewOwner);
	SetInstigator(NewInstigator);
	AttachMeshToSocket(InParent, InSocketName);
	DisableSphereDetectionCollision();
	DeactivateVisualEffect();
	PlayEquipSound();
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	GetOverlapSphereDetection()->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereBeginOverlap);
	GetOverlapSphereDetection()->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	GetHitBox()->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnHitBoxBeginOverlap);
}

void AWeapon::ExecuteGetHit(FHitResult BoxHit)
{
	IHitInterface* HitInterface = Cast<IHitInterface>(BoxHit.GetActor());
	if (HitInterface)
	{
		HitInterface->Execute_GetHit(BoxHit.GetActor(), BoxHit.ImpactPoint, GetOwner());
	}
}

bool AWeapon::IsActorSameType(AActor* OtherActor)
{
	return GetOwner()->ActorHasTag(TEXT("Enemy")) && OtherActor->ActorHasTag(TEXT("Enemy"));
}

void AWeapon::OnHitBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnHitBoxBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	if (IsActorSameType(OtherActor)) return;
	
	FHitResult BoxHit;
	BoxTrace(BoxHit);
	
	if (BoxHit.GetActor() && DamageType)
	{
		if (IsActorSameType(BoxHit.GetActor())) return;
		
		UGameplayStatics::ApplyDamage(BoxHit.GetActor(), BaseDamage, GetInstigatorController(),this, DamageType);
		ExecuteGetHit(BoxHit);
		CreateFields(BoxHit.ImpactPoint);
	}
}

void AWeapon::BoxTrace(FHitResult& BoxHit)
{
	ActorsToIgnore.Add(this);
	ActorsToIgnore.Add(GetOwner());
	const FVector HitBoxTraceStartLocation = HitBoxTraceStart->GetComponentLocation();
	const FVector HitBoxTraceEndLocation = HitBoxTraceEnd->GetComponentLocation();

	UKismetSystemLibrary::BoxTraceSingle(this, HitBoxTraceStartLocation, HitBoxTraceEndLocation, BoxTraceExtent, HitBoxTraceStart->GetComponentRotation(), TraceTypeQuery1, false, ActorsToIgnore, bShowBoxDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, BoxHit, true);
	ActorsToIgnore.AddUnique(BoxHit.GetActor());
}
