

#include "Characters/BaseCharacter.h"

#include "Characters/CharacterTypes.h"
#include "Items/Weapons/Weapon.h"
#include "Components/BoxComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	
	AttributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("AttributeComponent"));
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABaseCharacter::GetHit_Implementation(const FVector& InImpactPoint, AActor* Hitter)
{
	IHitInterface::GetHit_Implementation(InImpactPoint, Hitter);
	if (IsAlive() && Hitter)
	{
		HandleHitReactDirection(Hitter->GetActorLocation());
	}
	else
	{
		Die();
	}
	PlayHitSound(InImpactPoint);
	SpawnHitParticles(InImpactPoint);
}

void ABaseCharacter::SetWeaponCollisionEnabled(ECollisionEnabled::Type InCollision)
{
	if (EquippedWeapon && EquippedWeapon->GetHitBox())
	{
		EquippedWeapon->GetHitBox()->SetCollisionEnabled(InCollision);
		EquippedWeapon->ActorsToIgnore.Empty();
	}
}

bool ABaseCharacter::CanAttack()
{
	return true;
}

void ABaseCharacter::Attack()
{
	if (CombatTarget && CombatTarget->ActorHasTag(FName("Dead")))
	{
		CombatTarget = nullptr;
	}
}

int32 ABaseCharacter::PlayAttackMontage()
{
	return PlayRandomMontageSection(AttackMontage);
}

void ABaseCharacter::HandleDamage(float DamageAmount)
{
	if (AttributeComponent)
	{
		AttributeComponent->ReceiveDamage(DamageAmount);
	}
}

void ABaseCharacter::HandleHitReactDirection(const FVector& InImpactPoint)
{
	const FVector ForwardVector = GetActorForwardVector();
	// Flatten InImpactPoint so it has the same Z as ActorLocation
	const FVector ImpactPointFlatten = FVector(InImpactPoint.X, InImpactPoint.Y, GetActorLocation().Z);
	//Get the normalize vector between actor location and hit location
	const FVector FromActorToHit = (ImpactPointFlatten - GetActorLocation()).GetSafeNormal();

	
	// ForwardVector * FromActorToHit = |ForwardVector| |FromActorToHit| * cos(theta)
	// |ForwardVector| and |FromActorToHit| = 1 so ForwardVector * FromActorToHit = cos(theta)
	const double CosTheta = FVector::DotProduct(ForwardVector, FromActorToHit);
	// Take inverse cosine (arc-cosine) of cos(theta) to get the angle theta
	double Theta = FMath::Acos(CosTheta);
	// Convert from radians to degrees
	Theta = FMath::RadiansToDegrees(Theta);

	// if CrossProduct points down it means that FromActorToHit is on the left of the ForwardVector so Theta is negative
	const FVector CrossProduct = FVector::CrossProduct(ForwardVector, FromActorToHit);
	if (CrossProduct.Z < 0)
	{
		Theta *= -1.f;
	}
		
	if ((Theta >= -45) && (Theta <= 45))
	{
		PlayHitReactMontage(FName("HitFromFront"));
	}
	else if ((Theta > 45) && (Theta <= 135))
	{
		PlayHitReactMontage(FName("HitFromRight"));		
	}
	else if ((Theta > 135) || (Theta <= -135))
	{
		PlayHitReactMontage(FName("HitFromBack"));
	}
	else if ((Theta > -135) && (Theta < -45))
	{
		PlayHitReactMontage(FName("HitFromLeft"));
	}
}

void ABaseCharacter::PlayHitReactMontage(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABaseCharacter::PlayHitSound(const FVector& InImpactPoint)
{
	if (HitReactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitReactSound,InImpactPoint,GetActorRotation());	
	}
}

void ABaseCharacter::SpawnHitParticles(const FVector& InImpactPoint)
{
	if (HitReactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, HitReactParticle, InImpactPoint);
	}
}

void ABaseCharacter::Die()
{
	Tags.Add(FName("Dead"));
	DisableMeshCollision();
	if (DeathMontage) PlayDeathMontage();
}

int32 ABaseCharacter::PlayDeathMontage()
{
	const int32 MontageSectionIndex = PlayRandomMontageSection(DeathMontage);
	const int32 EnumCount = static_cast<int32>(EDeadPose::EDP_COUNT);
	const EDeadPose Pose = static_cast<EDeadPose>(MontageSectionIndex);
	if (MontageSectionIndex < EnumCount)
	{
		DeadPose = Pose;
	}
	return MontageSectionIndex;
}

void ABaseCharacter::PlayDodgeMontage()
{
	PlayMontageSection(DodgeMontage, FName("Default"));
}

void ABaseCharacter::DisableCapsuleCollision()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ABaseCharacter::IsAlive()
{
	return AttributeComponent && AttributeComponent->IsAlive();
}

void ABaseCharacter::StopAttackMontage()
{
	if (AttackMontage)
		StopAnimMontage(AttackMontage);
}

void ABaseCharacter::DisableMeshCollision()
{
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABaseCharacter::OnAttackEnd()
{
}

void ABaseCharacter::OnDodgeEnd()
{
}

FVector ABaseCharacter::GetTranslationWarpTarget()
{
	if (CombatTarget == nullptr) return FVector();

	const FVector TargetLocation = CombatTarget->GetActorLocation();
	const FVector HitterLocation = GetActorLocation();
	// Create a normalize vector from character to target
	FVector TargetToMe = (TargetLocation - HitterLocation).GetSafeNormal();
	TargetToMe *= WarpTargetDistance;
	DrawDebugSphere(GetWorld(), (TargetLocation + TargetToMe), 20.f, 12, FColor::Red);
	return TargetLocation + TargetToMe;
}

FVector ABaseCharacter::GetRotationWarpTarget()
{
	if (CombatTarget == nullptr) return FVector();
	
	return CombatTarget->GetActorLocation();
}

void ABaseCharacter::PlayMontageSection(UAnimMontage* Montage, const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && Montage)
	{
		AnimInstance->Montage_Play(Montage);
		AnimInstance->Montage_JumpToSection(SectionName, Montage);
	}
	
}

int32 ABaseCharacter::PlayRandomMontageSection(UAnimMontage* AnimMontage)
{
	const int32 NumberOfSection = AnimMontage->GetNumSections();
	const int32 RandomSectionIndex = FMath::RandRange(1, NumberOfSection);
	const FName SectionName = AnimMontage->GetSectionName(RandomSectionIndex);
	PlayMontageSection(AnimMontage, SectionName);
	return RandomSectionIndex;
}






